#!/bin/bash
set -euo pipefail

# Deploy de macOS de LGA OpenInNukeX. Produce un bundle AUTOCONTENIDO y, opcionalmente, los
# dos artefactos de release. Sigue el esquema del template; ver
# ../../../Desktop/Codin/LGA_Base_QT_C_Py/docs/Doc_Deploy_macOS.md.
#
# Lo que hace distinto de las otras apps LGA, y por que
# ----------------------------------------------------
# El .zip NO lleva solo el .app. Esta app publica DOS cosas en el mismo release: la
# aplicacion y el componente Python que corre adentro de NukeX. El card de LGA Updates de
# PipeSync descarga ese mismo zip, lo extrae y corre el `installer_mac.sh` de adentro para
# instalar el componente de Nuke — por eso el zip tiene la misma forma que el de Windows:
#
#     LGA_OpenInNukeX/         init.py + LGA_QtAdapter_OpenInNukeX.py + VERSION
#     installer_mac.sh         entry point que invoca PipeSync (del repo de release)
#     i_mac_plugin_engine.sh   el motor comun al que forwardea
#     LGA OpenInNukeX.app      la aplicacion
#     install_es.pdf           el instructivo en castellano
#     install_en.pdf           el instructivo en ingles
#
# Y por eso el NOMBRE del archivo es `LGA_OpenInNukeX_v<version>_mac.zip` y no
# `..._Mac_v<version>.zip` como dice la convencion generica: es el patron que el catalogo de
# PipeSync ya tiene declarado para esta app.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

APP_NAME="LGA OpenInNukeX"
ARTIFACT_NAME="LGA_OpenInNukeX"
PLUGIN_FOLDER="LGA_OpenInNukeX"
REPO_ROOT="$(cd .. && pwd)"

show_help() {
    echo "Uso: $0 [--zip] [--dmg] [--no-open-finder] [--parallel N]"
    echo ""
    echo "  --zip             Crear deploy/${ARTIFACT_NAME}_v<version>_mac.zip firmado"
    echo "  --dmg             Crear deploy/${ARTIFACT_NAME}_v<version>_mac.dmg (primera instalacion)"
    echo "  --no-open-finder  No preguntar ni abrir Finder al finalizar"
    echo "  --parallel N      Usar N nucleos para compilar"
    echo ""
    echo "Sin --zip/--dmg y con terminal interactiva, el script pregunta por cada artefacto."
    echo "En modo NO interactivo (pipe, CI, agente) no pregunta nada: hay que pasar los flags."
}

CREATE_ZIP_MODE="prompt"
CREATE_DMG_MODE="prompt"
NO_OPEN_FINDER=false
PARALLEL_CORES="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --zip) CREATE_ZIP_MODE="always"; shift ;;
        --dmg) CREATE_DMG_MODE="always"; shift ;;
        --no-open-finder) NO_OPEN_FINDER=true; shift ;;
        --parallel)
            if [[ $# -lt 2 ]]; then echo "ERROR: --parallel requiere un valor numerico."; exit 1; fi
            PARALLEL_CORES="$2"; shift 2 ;;
        -h|--help) show_help; exit 0 ;;
        *) echo "Opcion desconocida: $1"; show_help; exit 1 ;;
    esac
done

if ! [[ "$PARALLEL_CORES" =~ ^[0-9]+$ ]] || [[ "$PARALLEL_CORES" -lt 1 ]]; then
    echo "ERROR: --parallel debe ser un entero mayor o igual a 1."
    exit 1
fi

# La VERSION vive en la RAIZ del repo, que es tambien el plugin de Nuke que se publica.
if [ ! -f "$REPO_ROOT/VERSION" ]; then
    echo "ERROR: falta $REPO_ROOT/VERSION."
    exit 1
fi
APP_VERSION="$(tr -d '\r\n' < "$REPO_ROOT/VERSION")"
if ! [[ "$APP_VERSION" =~ ^[0-9]+(\.[0-9]+)+$ ]]; then
    echo "ERROR: VERSION tiene formato invalido: $APP_VERSION"
    exit 1
fi

# La version del CMake es la FUENTE DE VERDAD (convencion LGA); el archivo VERSION es un
# espejo. Si divergen, el asset se llamaria distinto de lo que la app muestra adentro.
# El patron es TOLERANTE a propósito: sin anclar el final ni exigir `LANGUAGES CXX` exacto,
# igual que el de tools/sync_version.py. Con el patron estricto anterior, agregar un lenguaje
# o un comentario al final de esa linea hacia abortar el deploy con "CMakeLists.txt dice ''",
# culpando a la version en vez de al parser.
CMAKE_VERSION="$(sed -nE 's/^[[:space:]]*project\([[:space:]]*LGA_OpenInNukeX[[:space:]]+VERSION[[:space:]]+([0-9][0-9.]*).*/\1/p' CMakeLists.txt | head -1)"
if [ -z "$CMAKE_VERSION" ]; then
    echo "ERROR: no se pudo leer la version del CMakeLists.txt."
    exit 1
fi
CMAKE_VERSION_SHORT="${CMAKE_VERSION%.*}"
if [ "$CMAKE_VERSION_SHORT" != "$APP_VERSION" ] && [ "$CMAKE_VERSION" != "$APP_VERSION" ]; then
    echo "ERROR: VERSION dice '$APP_VERSION' y CMakeLists.txt dice '$CMAKE_VERSION'."
    echo "       Corre tools/sync_version.py: la fuente de verdad es el CMakeLists."
    exit 1
fi

echo "Deploy de $APP_NAME v$APP_VERSION"
echo ""

# --release y NO el build de desarrollo: lo que se publica tiene que ir optimizado y sin
# asserts. Compila en build-release/, un arbol SEPARADO del de dev, asi deployar no invalida
# la cache incremental con la que se venia trabajando (ni al reves).
BUILD_DIR="build-release"
./compilar_dev.sh --release --no-run --parallel "$PARALLEL_CORES"

if [ ! -d "${BUILD_DIR}/${APP_NAME}.app" ]; then
    echo "ERROR: no se encontro ${BUILD_DIR}/${APP_NAME}.app"
    exit 1
fi

# Que el bundle que se va a empaquetar sea REALMENTE Release. El chequeo es barato y ataja el
# caso en que alguien toque el script y vuelva a empaquetar el arbol de desarrollo: un
# binario Debug se ve igual, pesa parecido y anda, solo que lento y con asserts vivos.
# El `|| true` no es decorativo: bajo `pipefail`, si faltara el CMakeCache.txt el `sed` no-cero
# mataria el script en la propia asignacion, ANTES de llegar al mensaje de error de abajo.
DEPLOY_BUILD_TYPE="$(sed -n 's/^CMAKE_BUILD_TYPE:[^=]*=//p' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | head -1 || true)"
if [ "$DEPLOY_BUILD_TYPE" != "Release" ]; then
    echo "ERROR: ${BUILD_DIR} quedo configurado en '${DEPLOY_BUILD_TYPE:-?}', no en Release."
    exit 1
fi

# Se borra SOLO el bundle, no la carpeta entera: con `rm -rf deploy`, correr `--zip` destruia
# el `.dmg` de la misma version y `--dmg` destruia el `.zip`, asi que en dos pasadas nunca se
# podian tener los dos artefactos juntos.
mkdir -p deploy
rm -rf "deploy/${APP_NAME}.app"

# ditto y no `cp -R`: conserva symlinks, permisos y metadata del bundle tal cual.
ditto "${BUILD_DIR}/${APP_NAME}.app" "deploy/${APP_NAME}.app"

QT_PATH="$HOME/Qt/6.5.3/macos"
if [ -x "$QT_PATH/bin/macdeployqt" ]; then
    echo "Ejecutando macdeployqt..."
    "$QT_PATH/bin/macdeployqt" "deploy/${APP_NAME}.app" 2>/dev/null || \
        echo "Advertencia: macdeployqt fallo; se verifica abajo si el bundle quedo completo."
fi

# Fallback de plugins: si macdeployqt fallo, sin esto el bundle sale sin plataforma y no
# arranca en ninguna maquina que no tenga Qt instalado.
PDEPLOY="deploy/${APP_NAME}.app/Contents/PlugIns"
mkdir -p "$PDEPLOY/platforms" "$PDEPLOY/styles" "$PDEPLOY/imageformats"
for DYLIB in platforms/libqcocoa.dylib styles/libqmacstyle.dylib \
             imageformats/libqsvg.dylib imageformats/libqjpeg.dylib imageformats/libqpng.dylib; do
    if [ ! -f "$PDEPLOY/$DYLIB" ]; then
        cp "$QT_PATH/plugins/$DYLIB" "$PDEPLOY/$DYLIB" 2>/dev/null || true
    fi
done
if [ ! -f "$PDEPLOY/platforms/libqcocoa.dylib" ]; then
    echo "ERROR: falta el plugin Cocoa. El bundle no arrancaria en otra maquina."
    exit 1
fi

# El payload del bridge tiene que estar DENTRO del .app: es lo que copia el boton INSTALL.
BUNDLE_PAYLOAD="deploy/${APP_NAME}.app/Contents/Resources/bridge"
for required in init.py LGA_QtAdapter_OpenInNukeX.py; do
    if [ ! -f "$BUNDLE_PAYLOAD/$required" ]; then
        echo "ERROR: al bundle le falta Contents/Resources/bridge/$required."
        echo "       Sin eso la app instala nada y el boton INSTALL falla en la maquina del usuario."
        exit 1
    fi
done

# Sin `|| true`: es el UNICO chequeo de arquitectura de todo el pipeline, y neutralizado no
# sirve para nada. Un Mach-O solo-arm64 —por ejemplo un plugin de Qt copiado por el fallback de
# arriba desde una instalacion no universal— se publicaba igual, y el "No universal2" quedaba
# enterrado entre la salida de macdeployqt.
if [ -f "./tools/macos/validate_universal_macho.sh" ]; then
    if ! bash "./tools/macos/validate_universal_macho.sh" "deploy/${APP_NAME}.app"; then
        echo "ERROR: el bundle tiene binarios que no son universal2. No se publica asi."
        exit 1
    fi
fi

# Firma ad-hoc del bundle YA armado: la firma cubre el contenido, asi que va DESPUES de
# copiar todo adentro y ANTES de empaquetar. No es notarizacion ni confianza de Gatekeeper
# —sigue haciendo falta el `xattr -cr`—: sirve para poder verificar despues que el bundle
# llego entero y sin alterar.
echo "Firmando el bundle (ad-hoc)..."
codesign --force --deep --sign - "deploy/${APP_NAME}.app"

echo "Deploy generado en deploy/${APP_NAME}.app"

ask_yes_no() {
    local prompt="$1" answer
    read -r -p "$prompt (Y/N): " answer
    [[ "$answer" =~ ^[Yy]$ ]]
}

CREATE_ZIP=false
if [ "$CREATE_ZIP_MODE" = "always" ]; then
    CREATE_ZIP=true
elif [ "$CREATE_ZIP_MODE" = "prompt" ] && [ -t 0 ]; then
    ask_yes_no "Crear el ZIP del release?" && CREATE_ZIP=true
fi

if [ "$CREATE_ZIP" = "true" ]; then
    ZIP_NAME="${ARTIFACT_NAME}_v${APP_VERSION}_mac.zip"
    STAGE="deploy/.stage"
    rm -rf "$STAGE"
    mkdir -p "$STAGE/$PLUGIN_FOLDER"

    # El payload del zip se toma del BUNDLE ya firmado y no del repo: asi lo que se publica
    # es exactamente lo que la app instala, y no dos copias que puedan divergir.
    cp "$BUNDLE_PAYLOAD/init.py" "$STAGE/$PLUGIN_FOLDER/"
    cp "$BUNDLE_PAYLOAD/LGA_QtAdapter_OpenInNukeX.py" "$STAGE/$PLUGIN_FOLDER/"
    printf '%s\n' "$APP_VERSION" > "$STAGE/$PLUGIN_FOLDER/VERSION"

    # El instalador NO es de esta app: es el wrapper por producto del repo de release, que
    # forwardea al motor comun `i_mac_plugin_engine.sh` —transaccional, con backup, rollback y
    # validacion del init.py—. Los nombres de los tres archivos son fijos: son los que busca el
    # card de LGA Updates de PipeSync (`UpdateRunner::locateEngine`).
    #
    # Se toman del deposito y NO se copian a este repo: una copia propia es una segunda
    # implementacion del mismo instalador que se desincroniza, y la primera version de esto fue
    # exactamente eso — sin backup ni rollback.
    INSTALLER_SRC="$REPO_ROOT/../LGA_Release/Installers/LGA_OpenInNukeX-Nuke/installer_mac.sh"
    ENGINE_SRC="$REPO_ROOT/../LGA_Release/Installers/Common/i_mac_plugin_engine.sh"
    for required in "$INSTALLER_SRC" "$ENGINE_SRC"; do
        if [ ! -f "$required" ]; then
            echo "ERROR: falta $required"
            echo "       El zip lo necesita para que PipeSync pueda instalar el componente de"
            echo "       Nuke desde su card. Clona/actualiza el repo LGA_Release al lado."
            exit 1
        fi
    done
    cp "$INSTALLER_SRC" "$STAGE/installer_mac.sh"
    cp "$ENGINE_SRC" "$STAGE/i_mac_plugin_engine.sh"
    chmod +x "$STAGE/installer_mac.sh" "$STAGE/i_mac_plugin_engine.sh"

    # Los instructivos van adentro del zip, igual que en el de Windows. Se cortan si
    # faltan: con un `if -f` silencioso el zip salia sin hoja de instalacion y nadie
    # se enteraba hasta abrirlo.
    for lang in es en; do
        if [ ! -f "$REPO_ROOT/install_${lang}.pdf" ]; then
            echo "ERROR: falta $REPO_ROOT/install_${lang}.pdf"
            echo "       Se genera con: node ../LGA_Release/Installers/docs/build_install_pdfs.mjs --install"
            exit 1
        fi
        cp "$REPO_ROOT/install_${lang}.pdf" "$STAGE/install_${lang}.pdf"
    done

    ditto "deploy/${APP_NAME}.app" "$STAGE/${APP_NAME}.app"

    # ditto y NO zip: `zip -r` RESUELVE los symlinks en vez de guardarlos, y un .app de Qt
    # esta lleno (Versions/Current, el binario de cada framework). Con zip el bundle llega al
    # usuario mucho mas pesado, con cada framework duplicado, y con la firma invalida.
    rm -f "deploy/${ZIP_NAME}"
    (cd "$STAGE" && ditto -c -k --sequesterRsrc . "../${ZIP_NAME}")
    rm -rf "$STAGE"
    echo "ZIP creado: deploy/${ZIP_NAME}"

    # Verificacion sobre el ARTEFACTO, no sobre deploy/: los dos errores clasicos
    # —empaquetar con zip, o copiar algo adentro despues de firmar— aparecen recien del otro
    # lado del empaquetado.
    CHECK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/${ARTIFACT_NAME}_check_XXXXXX")"
    ditto -x -k "deploy/${ZIP_NAME}" "$CHECK_DIR"
    ZIP_OK=true
    SYMLINKS="$(find "$CHECK_DIR/${APP_NAME}.app" -type l 2>/dev/null | wc -l | tr -d ' ')"
    if [ "$SYMLINKS" -eq 0 ]; then
        echo "ERROR: el .app del zip no tiene ningun symlink — se empaqueto con 'zip' en vez de 'ditto'."
        ZIP_OK=false
    fi
    if ! codesign --verify --deep --strict "$CHECK_DIR/${APP_NAME}.app" 2>/dev/null; then
        echo "ERROR: la firma del .app dentro del zip no verifica."
        ZIP_OK=false
    fi
    if [ ! -f "$CHECK_DIR/installer_mac.sh" ] || [ ! -f "$CHECK_DIR/i_mac_plugin_engine.sh" ] \
       || [ ! -f "$CHECK_DIR/$PLUGIN_FOLDER/VERSION" ]; then
        echo "ERROR: al zip le falta installer_mac.sh o el payload del bridge —"
        echo "       PipeSync no podria instalar el componente de Nuke desde este release."
        ZIP_OK=false
    fi
    rm -rf "$CHECK_DIR"
    if [ "$ZIP_OK" != "true" ]; then
        rm -f "deploy/${ZIP_NAME}"
        echo "ZIP descartado por fallar la verificacion."
        exit 1
    fi
    echo "OK: zip verificado (symlinks=$SYMLINKS, firma valida, payload y motor presentes)."
fi

CREATE_DMG=false
if [ "$CREATE_DMG_MODE" = "always" ]; then
    CREATE_DMG=true
elif [ "$CREATE_DMG_MODE" = "prompt" ] && [ -t 0 ]; then
    ask_yes_no "Crear el DMG de instalacion?" && CREATE_DMG=true
fi
if [ "$CREATE_DMG" = "true" ]; then
    bash ./create_dmg.sh --no-open
fi

# Publicacion en GitHub. Es el equivalente de la pregunta que hace instalador.bat en Windows:
# en macOS no hay instalador, el artefacto distribuible sale de aca.
#
# Se ofrece SOLO con los dos assets presentes, porque la release los lleva juntos y
# github_release_mac.sh aborta si le falta alguno. Y solo con terminal: el generador de
# release del repo privado corre `deploy.sh </dev/null` y publica por su cuenta, asi que sin
# tty no hay que preguntar nada.
#
# LGA_SKIP_RELEASE_PROMPT corta el bucle: github_release_mac.sh puede volver a llamar a
# deploy.sh (sus modos que regeneran el deploy), y ahi esta pregunta no tiene que reaparecer.
RELEASE_ZIP="deploy/${ARTIFACT_NAME}_v${APP_VERSION}_mac.zip"
RELEASE_DMG="deploy/${ARTIFACT_NAME}_v${APP_VERSION}_mac.dmg"
if [ -t 0 ] && [ "${LGA_SKIP_RELEASE_PROMPT:-}" != "1" ] \
   && [ -f "$RELEASE_ZIP" ] && [ -f "$RELEASE_DMG" ]; then
    echo ""
    if ask_yes_no "Publicar el release v${APP_VERSION} en GitHub?"; then
        # --use-existing-deploy: lo que se acaba de construir y verificar es exactamente lo
        # que se quiere publicar. Sin el flag el publicador pregunta si reusarlo y, por
        # defecto, lo reconstruye entero.
        #
        # El `||` no es opcional: bajo `set -e` un fallo de la publicacion mataria el script
        # y reportaria como fallido un deploy que salio bien. La publicacion es un extra.
        if ! LGA_SKIP_RELEASE_PROMPT=1 bash ./github_release_mac.sh --use-existing-deploy; then
            echo ""
            echo "AVISO: la publicacion en GitHub fallo. Los artefactos locales quedaron en deploy/."
        fi
    fi
fi

if [ "$NO_OPEN_FINDER" = "false" ] && [ -t 0 ]; then
    # El `if` y no un `&&` suelto: siendo el ultimo comando del script, un `&&` que no se
    # ejecuta deja exit code 1, o sea que contestar "N" hacia que el deploy —exitoso— se
    # reportara como fallido a cualquier `./deploy.sh && ...` o wrapper de CI.
    if ask_yes_no "Mostrar ${APP_NAME}.app en Finder?"; then
        open -R "deploy/${APP_NAME}.app"
    fi
fi

exit 0
