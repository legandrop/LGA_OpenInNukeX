#!/bin/bash

# Publica el release de macOS en GitHub: crea (o reusa) el tag v<version> y sube los dos
# assets a la release de ese tag.
#
#   <APP>_v<version>_mac.zip   la app + el componente de Nuke (lo consume PipeSync)
#   <APP>_v<version>_mac.dmg   primera instalacion
#
# La version SIEMPRE sale de VERSION, que a su vez esta sincronizado con el ChangeLog y con
# CMakeLists.txt (ver tools/sync_version.py). No se pasa por parametro a proposito: un
# numero escrito a mano en la linea de comandos es la forma mas facil de publicar un asset
# que no coincide con lo que dice la app adentro.
#
# Por defecto publica en el propio repo (origin). En una app derivada que tenga un repo de
# releases separado, cambiar RELEASE_REPO o exportar LGA_RELEASE_REPO.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# APP_NAME es el `.app` (con espacios); ARTIFACT_NAME es la base del nombre de ARCHIVO
# del .zip/.dmg (con guiones bajos). Estaban conflacionados en una sola variable.
APP_NAME="LGA OpenInNukeX"
ARTIFACT_NAME="LGA_OpenInNukeX"
RELEASE_REPO="${LGA_RELEASE_REPO:-legandrop/LGA_OpenInNukeX}"

# El nombre de los assets NO usa el `_Mac_v<version>` generico del template: el catalogo de
# updates de PipeSync ya declara para esta app `LGA_OpenInNukeX_v{v}_mac.zip`, y es el patron
# por el que su card busca el asset del release. Cambiarlo la dejaria sin encontrarlo.
# El asset de Windows que ya se publica usa el mismo orden, asi que ademas quedan parejos.
# Este repo es PRIVADO, asi que la release tambien lo es y solo la ve quien tenga acceso.
# El flujo es el mismo que en un repo publico: lo unico que cambia es quien puede bajarla.

PARALLEL_CORES="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
DRY_RUN=false
DEPLOY_MODE="prompt"
ASSUME_YES=false

show_help() {
    echo "Uso: $0 [--parallel N] [--dry-run] [--yes] [--repo owner/repo]"
    echo "         [--use-existing-deploy | --rebuild-deploy]"
    echo ""
    echo "Publica (o reemplaza) los assets macOS en la release v<version> de $RELEASE_REPO:"
    echo "  ${ARTIFACT_NAME}_v<version>_mac.zip  (app + componente de Nuke)"
    echo "  ${ARTIFACT_NAME}_v<version>_mac.dmg  (primera instalacion)"
    echo ""
    echo "Opciones:"
    echo "  --parallel N           Usa N nucleos para build/deploy"
    echo "  --dry-run              Recorre el flujo completo sin crear tags ni subir nada"
    echo "  --yes                  No preguntar: reusar lo que exista y reemplazar assets"
    echo "  --repo owner/repo      Publicar en otro repo (equivale a LGA_RELEASE_REPO)"
    echo "  --use-existing-deploy  Usa el deploy/ existente si esta completo"
    echo "  --rebuild-deploy       Fuerza deploy.sh (y regenerar el DMG) antes de publicar"
}

require_cmd() {
    local cmd="$1"
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "ERROR: falta el comando requerido: $cmd"
        exit 1
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --parallel)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --parallel requiere un valor numerico."
                exit 1
            fi
            PARALLEL_CORES="$2"; shift 2 ;;
        --repo)
            if [[ $# -lt 2 ]]; then
                echo "ERROR: --repo requiere owner/repo."
                exit 1
            fi
            RELEASE_REPO="$2"; shift 2 ;;
        --dry-run) DRY_RUN=true; shift ;;
        --yes|-y) ASSUME_YES=true; shift ;;
        --use-existing-deploy) DEPLOY_MODE="use-existing"; shift ;;
        --rebuild-deploy) DEPLOY_MODE="rebuild"; shift ;;
        -h|--help) show_help; exit 0 ;;
        *) echo "ERROR: opcion desconocida: $1"; show_help; exit 1 ;;
    esac
done

if ! [[ "$PARALLEL_CORES" =~ ^[0-9]+$ ]] || [[ "$PARALLEL_CORES" -lt 1 ]]; then
    echo "ERROR: --parallel debe ser un entero mayor o igual a 1."
    exit 1
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "ERROR: github_release_mac.sh esta pensado para ejecutarse en macOS."
    exit 1
fi

require_cmd git
require_cmd gh
require_cmd python3
require_cmd hdiutil
require_cmd ditto

if ! gh auth status >/dev/null 2>&1; then
    echo "ERROR: gh no esta autenticado. Ejecuta 'gh auth login' con tu usuario."
    exit 1
fi

# La identidad importa: GitHub atribuye el tag por email, no por nombre. Un email viejo deja
# el commit del tag fuera del grafico de contribuciones de la cuenta buena, y la historia no
# se reescribe. Ver la seccion "GitHub, autoria y menciones" de las reglas del repo.
GIT_EMAIL="$(git config user.email || true)"
if [[ -z "$GIT_EMAIL" ]]; then
    echo "ERROR: git user.email no esta configurado."
    exit 1
fi
echo "Identidad git: $(git config user.name || echo '?') <$GIT_EMAIL>"

# El working tree limpio no es cosmetico: el tag apunta a HEAD, asi que con cambios sin
# commitear el release queda apuntando a un commit que NO es lo que se empaqueto.
if [[ -n "$(git status --porcelain)" ]] && [[ "$DRY_RUN" != "true" ]]; then
    echo "ERROR: working tree con cambios. Commit/stash antes de publicar."
    git status --short
    exit 1
fi

CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD)"
if [[ "$CURRENT_BRANCH" != "main" ]]; then
    echo "ERROR: para publicar el release de macOS hay que estar en main. Branch actual: $CURRENT_BRANCH"
    exit 1
fi

# Que VERSION, CMakeLists.txt y el ChangeLog digan lo mismo. Si no coinciden, el asset se
# llamaria distinto de lo que la app muestra en su Help.
# En esta app el ChangeLog, el VERSION y sync_version.py viven en la RAIZ del repo: QtClient/
# es solo el cliente Qt y la raiz es tambien el plugin de Nuke que se publica.
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
if ! python3 "$REPO_ROOT/tools/sync_version.py" --check-only; then
    echo "ERROR: la version esta desincronizada. Corre sync_version y commitea antes de publicar."
    exit 1
fi

APP_VERSION="$(tr -d '\r\n' < "$REPO_ROOT/VERSION")"
if ! [[ "$APP_VERSION" =~ ^[0-9]+(\.[0-9]+)+$ ]]; then
    echo "ERROR: VERSION invalida: $APP_VERSION"
    exit 1
fi

TAG="v${APP_VERSION}"
ZIP_NAME="${ARTIFACT_NAME}_v${APP_VERSION}_mac.zip"
DMG_NAME="${ARTIFACT_NAME}_v${APP_VERSION}_mac.dmg"
ZIP_PATH="${SCRIPT_DIR}/deploy/${ZIP_NAME}"
DMG_PATH="${SCRIPT_DIR}/deploy/${DMG_NAME}"
APP_PATH="${SCRIPT_DIR}/deploy/${APP_NAME}.app"

echo "Version detectada: ${APP_VERSION}"
echo "Tag objetivo:      ${TAG}"
echo "Repo de releases:  ${RELEASE_REPO}"
if [[ "$DRY_RUN" == "true" ]]; then
    echo "Modo:              DRY RUN (sin cambios remotos)"
fi
echo ""

#
# `$2` es la respuesta cuando NO hay a quien preguntarle (sin terminal y sin --yes); por
# defecto "yes". Con --yes siempre es que si, que es el modo documentado para correrlo sin
# terminal.
#
# El parametro existe por un caso concreto: reusar un `deploy/` que ya estaba ahi. El unico
# criterio para reusarlo es que el NOMBRE del archivo coincida con la version — nada lo
# compara contra el HEAD que se acaba de verificar limpio. Asumiendo que si, una corrida sin
# terminal publicaba, en silencio, un artefacto construido desde otro commit de la misma
# version. Ahi la respuesta sin tty pasa a ser "no" (recompilar), que es la conservadora.
confirm() {
    local prompt="$1" default_no_tty="${2:-yes}" answer
    if [[ "$ASSUME_YES" == "true" ]]; then
        return 0
    fi
    if [[ ! -t 0 ]]; then
        [[ "$default_no_tty" == "yes" ]] && return 0 || return 1
    fi
    read -r -p "$prompt [Y/n] " answer
    case "$answer" in
        [nN]|[nN][oO]) return 1 ;;
        *) return 0 ;;
    esac
}

run_deploy() {
    bash ./deploy.sh --zip --no-open-finder --parallel "$PARALLEL_CORES"
}

DEPLOY_EXISTS=false
if [[ -d "$APP_PATH" && -f "$ZIP_PATH" ]]; then
    DEPLOY_EXISTS=true
fi

if [[ "$DEPLOY_MODE" == "rebuild" ]]; then
    echo "Regenerando deploy macOS por --rebuild-deploy..."
    run_deploy
elif [[ "$DEPLOY_MODE" == "use-existing" ]]; then
    if [[ "$DEPLOY_EXISTS" != "true" ]]; then
        echo "ERROR: --use-existing-deploy requiere el .app y el .zip de v${APP_VERSION} en deploy/."
        exit 1
    fi
    echo "Usando el deploy existente."
elif [[ "$DEPLOY_EXISTS" == "true" ]]; then
    echo "Deploy existente detectado para la version ${APP_VERSION}."
    if confirm "Usar este deploy existente?" "no"; then
        echo "Usando el deploy existente."
    else
        echo "Regenerando deploy macOS..."
        run_deploy
    fi
else
    echo "No hay deploy completo para la version ${APP_VERSION}. Generando uno nuevo..."
    run_deploy
fi

if [[ ! -d "$APP_PATH" ]]; then
    echo "ERROR: no se encontro el bundle esperado: $APP_PATH"
    exit 1
fi
if [[ ! -f "$ZIP_PATH" ]]; then
    echo "ERROR: no se encontro el asset esperado: $ZIP_PATH"
    exit 1
fi

if [[ "$DEPLOY_MODE" == "rebuild" || ! -f "$DMG_PATH" ]]; then
    bash ./create_dmg.sh --no-open
elif [[ "$DEPLOY_MODE" == "use-existing" ]]; then
    # Con --use-existing-deploy no se vuelve a preguntar: quien pasa ese flag ya decidio
    # reusar lo que hay. Preguntarlo igual era una pregunta de mas cuando deploy.sh encadena
    # aca, porque el DMG lo acaba de generar dos lineas antes.
    echo "Usando el DMG existente: $(basename "$DMG_PATH")"
else
    echo "DMG existente detectado: $(basename "$DMG_PATH")"
    if ! confirm "Usar este DMG existente?"; then
        bash ./create_dmg.sh --no-open
    fi
fi
if [[ ! -f "$DMG_PATH" ]]; then
    echo "ERROR: no se encontro el asset esperado: $DMG_PATH"
    exit 1
fi

# Verificacion final sobre lo que REALMENTE se va a subir. Los tres chequeos juntos detectan
# los tres errores clasicos del empaquetado de macOS (ver docs/Doc_Deploy_macOS.md): 0
# symlinks delata un `zip -r`, y codesign delata la firma faltante o un bundle armado
# despues de firmar.
echo ""
echo "Verificando el ZIP antes de publicar..."
VERIFY_DIR="$(mktemp -d "${TMPDIR:-/tmp}/${ARTIFACT_NAME}_verify_XXXXXX")"
trap 'rm -rf "$VERIFY_DIR"' EXIT
ditto -x -k "$ZIP_PATH" "$VERIFY_DIR"
SYMLINK_COUNT="$(find "$VERIFY_DIR/${APP_NAME}.app" -type l | wc -l | tr -d ' ')"
if [[ "$SYMLINK_COUNT" -eq 0 ]]; then
    echo "ERROR: el ZIP no conserva symlinks. Se empaqueto con 'zip' en vez de 'ditto'."
    exit 1
fi
if ! codesign --verify --deep --strict "$VERIFY_DIR/${APP_NAME}.app" 2>/dev/null; then
    echo "ERROR: la firma del bundle empaquetado no verifica."
    exit 1
fi
echo "OK: ${SYMLINK_COUNT} symlinks conservados y firma valida."
echo ""

release_asset_exists() {
    local asset_name="$1"
    gh release view "$TAG" --repo "$RELEASE_REPO" --json assets -q '.assets[].name' 2>/dev/null \
        | grep -Fxq "$asset_name"
}

upload_asset_if_needed() {
    local asset_name="$1"
    local asset_path="$2"
    if release_asset_exists "$asset_name"; then
        echo "AVISO: el asset '$asset_name' ya existe en la release $TAG."
        if [[ "$DRY_RUN" == "true" ]]; then
            echo "[DRY RUN] Se preguntaria si reemplazar '$asset_name'."
            return 0
        fi
        if ! confirm "Reemplazar '$asset_name'?"; then
            echo "Saltando '$asset_name'."
            return 0
        fi
    else
        echo "La release no tiene '$asset_name'. Se sube."
    fi
    if [[ "$DRY_RUN" == "true" ]]; then
        echo "[DRY RUN] gh release upload \"$TAG\" \"$asset_path\" --repo \"$RELEASE_REPO\" --clobber"
    else
        echo "Subiendo '$asset_name'..."
        gh release upload "$TAG" "$asset_path" --repo "$RELEASE_REPO" --clobber
    fi
}

ensure_tag() {
    # El tag va en el repo de codigo (origin). Cuando el repo de releases es OTRO, el tag no
    # existe ahi: por eso la release se crea con --target main y el tag propio de ese repo.
    if git ls-remote --exit-code --tags origin "refs/tags/${TAG}" >/dev/null 2>&1; then
        echo "Tag ${TAG} ya existe en origin."
        return 0
    fi
    if ! git rev-parse -q --verify "refs/tags/${TAG}" >/dev/null 2>&1; then
        echo "Creando tag ${TAG}..."
        if [[ "$DRY_RUN" == "true" ]]; then
            echo "[DRY RUN] git tag -a \"$TAG\" -m \"Release $TAG\""
        else
            git tag -a "$TAG" -m "Release $TAG"
        fi
    else
        echo "Tag local ${TAG} ya existe. Haciendo push..."
    fi
    if [[ "$DRY_RUN" == "true" ]]; then
        echo "[DRY RUN] git push origin \"$TAG\""
    else
        git push origin "$TAG"
    fi
}

if gh release view "$TAG" --repo "$RELEASE_REPO" >/dev/null 2>&1; then
    echo "Release existente: $TAG"
    upload_asset_if_needed "$ZIP_NAME" "$ZIP_PATH"
    upload_asset_if_needed "$DMG_NAME" "$DMG_PATH"
else
    ensure_tag
    echo "Creando release ${TAG} en ${RELEASE_REPO}..."
    if [[ "$DRY_RUN" == "true" ]]; then
        echo "[DRY RUN] gh release create \"$TAG\" \"$ZIP_PATH\" \"$DMG_PATH\" --repo \"$RELEASE_REPO\""
    else
        gh release create "$TAG" "$ZIP_PATH" "$DMG_PATH" \
            --repo "$RELEASE_REPO" \
            --target "main" \
            --title "$TAG" \
            --notes "Release $TAG"
    fi
fi

echo ""
echo "Release macOS lista:"
echo "https://github.com/${RELEASE_REPO}/releases/tag/${TAG}"
