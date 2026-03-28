#!/bin/bash

# Workaround para Qt 6.5.3 en macOS 15+ / M1
# Si estamos en arm64, relanzar bajo Rosetta (x86_64)
if [ "$(uname -m)" = "arm64" ]; then
    echo "Detectado ARM64. Relanzando bajo Rosetta para compatibilidad con Qt..."
    exec arch -x86_64 "$0" "$@"
fi

echo "🔨 Compilación completa LGA_OpenInNukeX"

QT_PATH="$HOME/Qt/6.5.3/macos"
if [ ! -d "$QT_PATH" ]; then
    echo "❌ Qt 6.5.3 no encontrado en $QT_PATH"
    exit 1
fi

export CMAKE_PREFIX_PATH="$QT_PATH"
export PATH="$QT_PATH/bin:$PATH"
export QTDIR="$QT_PATH"

PARALLEL_CORES=$(sysctl -n hw.ncpu)

# Crear directorio de build
mkdir -p build
cd build

# AGL framework dummy (necesario en macOS 12+ con Qt 6)
if [ -f "/System/Library/Frameworks/AGL.framework/Versions/A/AGL" ] || \
   [ -f "/Library/Frameworks/AGL.framework/Versions/A/AGL" ]; then
    echo "AGL framework funcional encontrado."
    AGL_NEEDED=false
else
    AGL_NEEDED=true
    echo "Creando dummy AGL framework..."
    if [ ! -f "AGL.framework/Versions/A/AGL" ]; then
        mkdir -p AGL.framework/Versions/A
        echo "void _aglDummy(){}" > agl_dummy.c
        clang -dynamiclib agl_dummy.c -o AGL.framework/Versions/A/AGL \
            -install_name /System/Library/Frameworks/AGL.framework/Versions/A/AGL \
            -arch x86_64 -arch arm64
        ln -sf A AGL.framework/Versions/Current
        ln -sf Versions/Current/AGL AGL.framework/AGL
        rm agl_dummy.c
        echo "Dummy AGL creado."
    fi
fi

# Detectar SDK
SDK_PATH=$(xcrun --show-sdk-path 2>/dev/null || xcrun --sdk macosx --show-sdk-path 2>/dev/null)
if [ -n "$SDK_PATH" ]; then export SDKROOT="$SDK_PATH"; fi

CMAKE_FLAGS=(
    -G "Unix Makefiles"
    -DCMAKE_PREFIX_PATH="$QT_PATH"
    -DQt6_DIR="$QT_PATH/lib/cmake/Qt6"
    -DCMAKE_BUILD_TYPE=Debug
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,-no_warn_duplicate_libraries"
)
if [ "$AGL_NEEDED" = "true" ]; then
    CMAKE_FLAGS+=(-DCMAKE_EXE_LINKER_FLAGS="-F $PWD -Wl,-no_warn_duplicate_libraries")
fi

echo "⚙️  Configurando CMake..."
cmake .. "${CMAKE_FLAGS[@]}"
if [ $? -ne 0 ]; then echo "❌ Error en cmake configure"; exit 1; fi

echo "🔨 Compilando con $PARALLEL_CORES núcleos..."
cmake --build . --parallel $PARALLEL_CORES
if [ $? -ne 0 ]; then echo "❌ Error en compilación"; exit 1; fi

cd ..

APP_BUNDLE="build/LGA_OpenInNukeX.app"

echo "📦 Ejecutando macdeployqt..."
"$QT_PATH/bin/macdeployqt" "$APP_BUNDLE" 2>/dev/null || echo "Advertencia: macdeployqt falló, copiando manualmente..."

# Copia manual de fallback para plugins críticos
echo "🔌 Verificando plugins críticos..."

PLUGINS_DIR="$APP_BUNDLE/Contents/PlugIns"
mkdir -p "$PLUGINS_DIR/platforms"
mkdir -p "$PLUGINS_DIR/styles"
mkdir -p "$PLUGINS_DIR/imageformats"

if [ ! -f "$PLUGINS_DIR/platforms/libqcocoa.dylib" ]; then
    echo "Copiando libqcocoa.dylib..."
    cp "$QT_PATH/plugins/platforms/libqcocoa.dylib" "$PLUGINS_DIR/platforms/" 2>/dev/null || true
fi

if [ ! -f "$PLUGINS_DIR/styles/libqmacstyle.dylib" ]; then
    cp "$QT_PATH/plugins/styles/libqmacstyle.dylib" "$PLUGINS_DIR/styles/" 2>/dev/null || true
fi

# Formatos de imagen útiles
for PLUGIN in libqsvg.dylib libqjpeg.dylib libqpng.dylib; do
    if [ ! -f "$PLUGINS_DIR/imageformats/$PLUGIN" ]; then
        cp "$QT_PATH/plugins/imageformats/$PLUGIN" "$PLUGINS_DIR/imageformats/" 2>/dev/null || true
    fi
done

# Verificación final
if [ ! -f "$PLUGINS_DIR/platforms/libqcocoa.dylib" ]; then
    echo "❌ Plugin Cocoa faltante. La app no puede ejecutarse sin él."
    exit 1
fi

echo "✅ Compilación completada en $(date)"
echo "🚀 Ejecutando LGA_OpenInNukeX..."

arch -x86_64 "$APP_BUNDLE/Contents/MacOS/LGA_OpenInNukeX"
