#!/bin/bash

# Compilación rápida para desarrollo - solo recompila archivos modificados

show_help() {
    echo "Uso: $0 [--force-clean] [--parallel N] [--no-deploy] [--help]"
    echo ""
    echo "Opciones:"
    echo "  --force-clean    Limpiar build completamente antes de compilar"
    echo "  --parallel N     Usar N núcleos (default: todos los del sistema)"
    echo "  --no-deploy      Saltar copia de plugins Qt (más rápido, puede fallar si no están)"
    echo ""
    echo "Ejemplo: $0 --parallel 4"
}

FORCE_CLEAN=false
NO_DEPLOY=false
PARALLEL_CORES=$(sysctl -n hw.ncpu)

while [[ $# -gt 0 ]]; do
    case $1 in
        --force-clean) FORCE_CLEAN=true; shift ;;
        --parallel) PARALLEL_CORES="$2"; shift 2 ;;
        --no-deploy) NO_DEPLOY=true; shift ;;
        --help) show_help; exit 0 ;;
        *) echo "Opción desconocida: $1"; show_help; exit 1 ;;
    esac
done

echo "🚀 Compilación rápida para desarrollo (usando $PARALLEL_CORES núcleos)"

QT_PATH="$HOME/Qt/6.5.3/macos"
if [ ! -d "$QT_PATH" ]; then
    echo "❌ Qt 6.5.3 no encontrado en $QT_PATH"
    exit 1
fi

if [ "$FORCE_CLEAN" = "true" ]; then
    echo "🧹 Limpiando build anterior..."
    rm -rf build
fi

mkdir -p build
cd build

# AGL dummy (necesario en macOS 12+ con Qt)
if [ -f "/System/Library/Frameworks/AGL.framework/Versions/A/AGL" ] || \
   [ -f "/Library/Frameworks/AGL.framework/Versions/A/AGL" ]; then
    AGL_NEEDED=false
else
    AGL_NEEDED=true
    if [ ! -f "AGL.framework/Versions/A/AGL" ]; then
        mkdir -p AGL.framework/Versions/A
        echo "void _aglDummy(){}" > agl_dummy.c
        clang -dynamiclib agl_dummy.c -o AGL.framework/Versions/A/AGL \
            -install_name /System/Library/Frameworks/AGL.framework/Versions/A/AGL \
            -arch x86_64 -arch arm64
        ln -sf A AGL.framework/Versions/Current
        ln -sf Versions/Current/AGL AGL.framework/AGL
        rm agl_dummy.c
    fi
fi

CMAKE_FLAGS=(
    -G "Unix Makefiles"
    -DCMAKE_PREFIX_PATH="$QT_PATH"
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
    -DQt6_DIR="$QT_PATH/lib/cmake/Qt6"
    -DCMAKE_BUILD_TYPE=Debug
    -DCMAKE_CXX_FLAGS_DEBUG="-g -O0 -Wno-unused-parameter"
)
if [ "$AGL_NEEDED" = "true" ]; then
    CMAKE_FLAGS+=(-DCMAKE_EXE_LINKER_FLAGS="-F $PWD -Wl,-no_warn_duplicate_libraries")
else
    CMAKE_FLAGS+=(-DCMAKE_EXE_LINKER_FLAGS="-Wl,-no_warn_duplicate_libraries")
fi

if [ ! -f "CMakeCache.txt" ] || [ "$FORCE_CLEAN" = "true" ]; then
    echo "⚙️  Configurando CMake..."
    cmake .. "${CMAKE_FLAGS[@]}"
    if [ $? -ne 0 ]; then echo "❌ Error en cmake configure"; exit 1; fi
fi

echo "🔨 Compilando con $PARALLEL_CORES núcleos..."
cmake --build . --parallel $PARALLEL_CORES

if [ $? -ne 0 ]; then
    echo "❌ Error en compilación"
    exit 1
fi

cd ..

# Copiar dependencias Qt mínimas si no existen
if [ "$NO_DEPLOY" = "false" ]; then
    if [ ! -f "build/LGA_OpenInNukeX.app/Contents/PlugIns/platforms/libqcocoa.dylib" ]; then
        echo "📦 Copiando plugins Qt mínimos..."
        export PATH="$QT_PATH/bin:$PATH"

        mkdir -p build/LGA_OpenInNukeX.app/Contents/PlugIns/platforms
        cp "$QT_PATH/plugins/platforms/libqcocoa.dylib" \
           build/LGA_OpenInNukeX.app/Contents/PlugIns/platforms/ 2>/dev/null || true

        mkdir -p build/LGA_OpenInNukeX.app/Contents/PlugIns/styles
        cp "$QT_PATH/plugins/styles/libqmacstyle.dylib" \
           build/LGA_OpenInNukeX.app/Contents/PlugIns/styles/ 2>/dev/null || true
    fi
else
    echo "⏭️  Saltando plugins Qt (--no-deploy)"
fi

# Verificar dependencia crítica
if [ ! -f "build/LGA_OpenInNukeX.app/Contents/PlugIns/platforms/libqcocoa.dylib" ]; then
    echo "❌ Plugin Cocoa faltante. Ejecuta sin --no-deploy o ejecuta compilar.sh completo."
    exit 1
fi

echo "✅ Compilación completada en $(date)"
echo "🚀 Ejecutando LGA_OpenInNukeX..."

# Rosetta workaround para Qt 6.5.3 en ARM64
if [ "$(uname -m)" = "arm64" ]; then
    arch -x86_64 ./build/LGA_OpenInNukeX.app/Contents/MacOS/LGA_OpenInNukeX
else
    ./build/LGA_OpenInNukeX.app/Contents/MacOS/LGA_OpenInNukeX
fi
