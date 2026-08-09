#!/bin/bash

# Compilacion rapida para desarrollo - solo recompila archivos modificados
set -uo pipefail

show_help() {
    echo "Uso: $0 [--release] [--force-clean] [--parallel N] [--no-deploy] [--no-run] [--wait] [--sim-slow] [--no-rosetta]"
    echo ""
    echo "Opciones:"
    echo "  --release        Compilar en Release, en el arbol build-release/ (lo usa deploy.sh)."
    echo "                   Por defecto se compila en Debug, en build/. Los dos arboles son"
    echo "                   SEPARADOS para que pedir un release no invalide la cache de dev."
    echo "  --force-clean    Limpiar el arbol de build completamente antes de compilar"
    echo "  --parallel N     Usar N nucleos (default: todos los del sistema)"
    echo "  --no-deploy      Saltar copia de plugins Qt (mas rapido, puede fallar si no estan)"
    echo "  --no-run         Compilar y deployar sin lanzar la app"
    echo "  --wait           Dejar la app en foreground: la terminal queda retenida hasta"
    echo "                   cerrarla y se ven su stdout/stderr y su exit code."
    echo "                   Por defecto la app se lanza en background y el script termina."
    echo "  --no-rosetta     Correr nativo arm64 en vez de bajo Rosetta (default en Apple"
    echo "                   Silicon por un workaround historico de Qt 6.5.3). Es lo que hay"
    echo "                   que usar para medir performance: bajo Rosetta se mide una"
    echo "                   maquina que no existe."
    echo "  --sim-slow       Lanzar la app SIMULANDO UNA MAQUINA LENTA: QoS background (en Apple"
    echo "                   Silicon la restringe a los E-cores) + I/O de disco throttled. Los"
    echo "                   procesos hijos heredan la politica. Sirve para reproducir en una"
    echo "                   maquina rapida los freezes que sufren los usuarios en las lentas."
    echo ""
    echo "Ejemplo: $0 --parallel 4"
}

FORCE_CLEAN=false
NO_DEPLOY=false
# CONVENCION LGA — el build de desarrollo (Debug) y el de release viven en arboles
# SEPARADOS. Con un arbol solo, alternar entre trabajar y empaquetar invalida la cache de
# CMake y obliga a recompilar todo cada vez.
BUILD_TYPE=Debug
BUILD_DIR=build
PARALLEL_CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
NO_RUN=false
# CONVENCION LGA — por defecto la app se lanza en BACKGROUND y el script termina enseguida.
# Dejarla en foreground retiene la terminal hasta que alguien cierre la app a mano, lo que
# cuelga al que compila (y a cualquier agente) por tiempo indefinido.
WAIT_FOR_APP=false
# Ver la seccion "Simular una maquina lenta" de las reglas y docs/Doc_SimSlow.md del template.
SIM_SLOW=false
# En ARM64 la app se lanza bajo Rosetta por un workaround historico de Qt 6.5.3. Con esto se
# corre nativo, que es lo que hay que usar para medir performance de verdad.
NO_ROSETTA=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --release) BUILD_TYPE=Release; BUILD_DIR=build-release; shift ;;
        --force-clean) FORCE_CLEAN=true; shift ;;
        --parallel)
            # Sin el guard, `--parallel` a secas muere con "$2: unbound variable" por el
            # `set -u`, y `--parallel abc` se le pasaba crudo a cmake.
            if [[ $# -lt 2 ]] || ! [[ "$2" =~ ^[0-9]+$ ]] || [ "$2" -lt 1 ]; then
                echo "ERROR: --parallel requiere un entero mayor o igual a 1."
                exit 1
            fi
            PARALLEL_CORES="$2"; shift 2 ;;
        --no-deploy) NO_DEPLOY=true; shift ;;
        --no-run) NO_RUN=true; shift ;;
        --wait) WAIT_FOR_APP=true; shift ;;
        --sim-slow) SIM_SLOW=true; shift ;;
        --no-rosetta) NO_ROSETTA=true; shift ;;
        --help) show_help; exit 0 ;;
        *) echo "Opcion desconocida: $1"; show_help; exit 1 ;;
    esac
done

# Tiene que coincidir con APP_NAME del CMakeLists.txt.
APP_NAME="LGA OpenInNukeX"

echo "🚀 Compilando en $BUILD_TYPE (arbol $BUILD_DIR/, $PARALLEL_CORES nucleos)"

QT_PATH="$HOME/Qt/6.5.3/macos"
if [ ! -d "$QT_PATH" ]; then
    echo "❌ Qt 6.5.3 no encontrado en $QT_PATH"
    exit 1
fi

# Matar SOLO el ejecutable de ESTE bundle. Un `pkill -f "$APP_NAME"` a secas matchea por
# subcadena y se lleva puestos procesos ajenos que tengan el nombre en su linea de comando.
pkill -f "${APP_NAME}.app/Contents/MacOS/${APP_NAME}" 2>/dev/null || true

if [ "$FORCE_CLEAN" = "true" ]; then
    echo "🧹 Limpiando $BUILD_DIR/ anterior..."
    rm -rf "$BUILD_DIR"
fi

# Guard anti cache-viejo de SDK: si el CMAKE_OSX_SYSROOT cacheado ya no existe (tras un update
# de Xcode que cambia MacOSXNN.sdk), limpiamos el build para reconfigurar y evitar el error
# "'type_traits' file not found".
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    CACHED_SDK="$(awk -F= '/^CMAKE_OSX_SYSROOT/{print $2}' "$BUILD_DIR/CMakeCache.txt")"
    if [ -n "$CACHED_SDK" ] && [ ! -d "$CACHED_SDK" ]; then
        echo "🧹 SDK cacheado no existe ($CACHED_SDK); limpiando para reconfigurar..."
        rm -rf "$BUILD_DIR"
    fi
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# El requerimiento de -framework AGL de Qt6 (via WrapOpenGL::WrapOpenGL) se
# neutraliza directamente en CMakeLists.txt, asi que no hace falta dummy AGL.
CMAKE_FLAGS=(
    -G "Unix Makefiles"
    -DCMAKE_PREFIX_PATH="$QT_PATH"
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
    -DQt6_DIR="$QT_PATH/lib/cmake/Qt6"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,-no_warn_duplicate_libraries"
)
if [ "$BUILD_TYPE" = "Debug" ]; then
    CMAKE_FLAGS+=(-DCMAKE_CXX_FLAGS_DEBUG="-g -O0 -Wno-unused-parameter")
fi

# Reconfigurar tambien si la cache existente quedo con OTRO build type. Chequear solo la
# existencia del CMakeCache alcanzaba con un arbol unico; con dos, un arbol heredado puede
# tener Debug adentro y compilaria Debug EN SILENCIO al pedirle un release — que es el bug
# que esta separacion viene a evitar: nada delata un release sin optimizar y con asserts.
CACHED_BUILD_TYPE=""
if [ -f "CMakeCache.txt" ]; then
    CACHED_BUILD_TYPE="$(sed -n 's/^CMAKE_BUILD_TYPE:[^=]*=//p' CMakeCache.txt | head -1)"
fi

NEEDS_RECONFIGURE=false
if [ ! -f "CMakeCache.txt" ] || [ "$FORCE_CLEAN" = "true" ]; then
    NEEDS_RECONFIGURE=true
elif [ "$CACHED_BUILD_TYPE" != "$BUILD_TYPE" ]; then
    echo "🧭 La cache esta en '$CACHED_BUILD_TYPE' y se pidio '$BUILD_TYPE'; reconfigurando..."
    NEEDS_RECONFIGURE=true
else
    CACHED_FLAGS=$(awk -F= '/^CMAKE_EXE_LINKER_FLAGS:STRING=/{print $2}' CMakeCache.txt 2>/dev/null || true)
    if [[ "$CACHED_FLAGS" == *"-F "* ]]; then
        echo "🧭 Limpiando flag -F obsoleto del dummy AGL, reconfigurando..."
        NEEDS_RECONFIGURE=true
    fi
fi

if [ "$NEEDS_RECONFIGURE" = "true" ]; then
    echo "⚙️  Configurando CMake ($BUILD_TYPE)..."
    if ! cmake .. "${CMAKE_FLAGS[@]}"; then echo "❌ Error en cmake configure"; exit 1; fi
fi

echo "🔨 Compilando con $PARALLEL_CORES nucleos..."
if ! cmake --build . --parallel "$PARALLEL_CORES"; then
    echo "❌ Error en compilacion"
    exit 1
fi

cd ..

APP_BUNDLE="$BUILD_DIR/${APP_NAME}.app"

# Copiar dependencias Qt minimas si no existen
if [ "$NO_DEPLOY" = "false" ]; then
    if [ ! -f "$APP_BUNDLE/Contents/PlugIns/platforms/libqcocoa.dylib" ]; then
        echo "📦 Copiando plugins Qt minimos..."
        mkdir -p "$APP_BUNDLE/Contents/PlugIns/platforms"
        cp "$QT_PATH/plugins/platforms/libqcocoa.dylib" \
           "$APP_BUNDLE/Contents/PlugIns/platforms/" 2>/dev/null || true

        mkdir -p "$APP_BUNDLE/Contents/PlugIns/styles"
        cp "$QT_PATH/plugins/styles/libqmacstyle.dylib" \
           "$APP_BUNDLE/Contents/PlugIns/styles/" 2>/dev/null || true
    fi
else
    echo "⏭️  Saltando plugins Qt (--no-deploy)"
fi

# Verificar dependencia critica
if [ ! -f "$APP_BUNDLE/Contents/PlugIns/platforms/libqcocoa.dylib" ]; then
    echo "❌ Plugin Cocoa faltante. Ejecuta sin --no-deploy o ejecuta compilar.sh completo."
    exit 1
fi

echo "✅ Compilacion completada en $(date)"
# Refrescar el cache de iconos del bundle: tras cambiar el .icns, el Dock/Finder pueden seguir
# mostrando el icono viejo por cache (iconservices). touch + lsregister -f fuerzan a re-leerlo.
LSREG="/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister"
if [ -d "$APP_BUNDLE" ]; then
    touch "$APP_BUNDLE"
    [ -x "$LSREG" ] && "$LSREG" -f "$APP_BUNDLE" >/dev/null 2>&1 || true
fi

if [ "$NO_RUN" = "true" ]; then
    echo "⏭️  Ejecucion omitida (--no-run)"
    exit 0
fi

APP_BIN="$APP_BUNDLE/Contents/MacOS/${APP_NAME}"
if [ ! -f "$APP_BIN" ]; then
    echo "❌ No se encontro el ejecutable en $APP_BIN"
    exit 1
fi

echo "🚀 Ejecutando ${APP_NAME}..."

# El lanzamiento se arma como ARRAY de prefijos para que Rosetta y --sim-slow se compongan
# en vez de pisarse: cada uno envuelve al anterior.
LAUNCH_CMD=("$APP_BIN")

# Rosetta workaround historico para Qt 6.5.3 en ARM64. Se lanza TRADUCIDO por defecto, y eso
# tiene dos consecuencias que conviene tener presentes:
#   - la rebanada arm64 —la que corre la mayoria de los usuarios y la que se publica— no se
#     ejecuta nunca en desarrollo;
#   - `--sim-slow` termina midiendo Rosetta sobre E-cores, o sea una maquina que no existe.
# Con `--no-rosetta` se corre nativo, que es lo que hay que usar para medir performance.
if [ "$(uname -m)" = "arm64" ] && [ "$NO_ROSETTA" = "false" ]; then
    LAUNCH_CMD=(arch -x86_64 "${LAUNCH_CMD[@]}")
    echo "   (bajo Rosetta; usa --no-rosetta para correr nativo arm64)"
fi

# --sim-slow: `taskpolicy -c background` clampea el QoS (en Apple Silicon deja al proceso
# SOLO en los E-cores) y `-d throttle` throttlea su I/O de disco. Los hijos HEREDAN la
# politica. Ver docs/Doc_SimSlow.md del template.
if [ "$SIM_SLOW" = "true" ]; then
    LAUNCH_CMD=(taskpolicy -c background -d throttle "${LAUNCH_CMD[@]}")
    echo "🐌 --sim-slow: QoS background + I/O throttled (simulacion de maquina lenta)"
fi

# CONVENCION LGA — NO lanzar la app en foreground por defecto. Un `"$APP_BIN"` suelto
# retiene la terminal hasta que alguien cierre la ventana a mano: para una persona es
# molesto, para un agente que compila por CLI es un cuelgue indefinido. El stdout/stderr se
# descarta a proposito, que el log completo ya va al archivo de log de la app.
if [ "$WAIT_FOR_APP" = "true" ]; then
    "${LAUNCH_CMD[@]}"
else
    "${LAUNCH_CMD[@]}" >/dev/null 2>&1 &
    disown
    echo "   PID $! (background)."
    echo "   Usa --wait si necesitas ver su salida o su exit code en la terminal."
fi
