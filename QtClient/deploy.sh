#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PROJECT_VERSION_FULL="$(sed -nE 's/^project\(LGA_OpenInNukeX VERSION ([0-9]+\.[0-9]+\.[0-9]+) LANGUAGES CXX\)$/\1/p' CMakeLists.txt)"
if [ -z "$PROJECT_VERSION_FULL" ]; then
    echo "❌ No se pudo obtener la version desde CMakeLists.txt"
    exit 1
fi

PROJECT_VERSION_SHORT="${PROJECT_VERSION_FULL%.*}"

# Workaround para Qt 6.5.3 en macOS 15+ / M1
if [ "$(uname -m)" = "arm64" ]; then
    echo "Detectado ARM64. Relanzando bajo Rosetta..."
    exec arch -x86_64 "$0" "$@"
fi

echo "🚀 Deploy LGA_OpenInNukeX v$PROJECT_VERSION_SHORT para distribución"

# Borrar deploy anterior
if [ -d "deploy" ]; then
    echo "Eliminando deploy anterior..."
    rm -rf deploy
fi

QT_PATH="$HOME/Qt/6.5.3/macos"
if [ ! -d "$QT_PATH" ]; then
    echo "❌ Qt 6.5.3 no encontrado en $QT_PATH"
    exit 1
fi

export CMAKE_PREFIX_PATH="$QT_PATH"
export PATH="$QT_PATH/bin:$PATH"

PARALLEL_CORES=$(sysctl -n hw.ncpu)

mkdir -p build
cd build

# AGL dummy
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

SDK_PATH=$(xcrun --show-sdk-path 2>/dev/null || xcrun --sdk macosx --show-sdk-path 2>/dev/null)
if [ -n "$SDK_PATH" ]; then export SDKROOT="$SDK_PATH"; fi

CMAKE_FLAGS=(
    -DCMAKE_PREFIX_PATH="$QT_PATH"
    -DQt6_DIR="$QT_PATH/lib/cmake/Qt6"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
)
if [ "$AGL_NEEDED" = "true" ]; then
    CMAKE_FLAGS+=(-DCMAKE_EXE_LINKER_FLAGS="-F $PWD")
fi

echo "⚙️  Configurando CMake (Release)..."
cmake .. "${CMAKE_FLAGS[@]}"
if [ $? -ne 0 ]; then echo "❌ Error en cmake configure"; exit 1; fi

echo "🔨 Compilando Release con $PARALLEL_CORES núcleos..."
cmake --build . --config Release --parallel $PARALLEL_CORES
if [ $? -ne 0 ]; then echo "❌ Error en compilación"; exit 1; fi

cd ..

APP_BUNDLE_BUILD="build/LGA_OpenInNukeX.app"
APP_BUNDLE_DEPLOY="deploy/LGA_OpenInNukeX.app"

if [ ! -d "$APP_BUNDLE_BUILD" ]; then
    echo "❌ No se encontro la app compilada en $APP_BUNDLE_BUILD"
    exit 1
fi

mkdir -p deploy

echo "📦 Copiando bundle base desde build/..."
cp -R "$APP_BUNDLE_BUILD" "$APP_BUNDLE_DEPLOY"

# Refrescar recursos propios por si fueron modificados despues del build
cp resources/LGA_OpenInNukeX.icns \
   "$APP_BUNDLE_DEPLOY/Contents/Resources/"

cp dark_theme.qss \
   "$APP_BUNDLE_DEPLOY/Contents/Resources/"

# macdeployqt para incluir Qt frameworks
echo "📦 Ejecutando macdeployqt en deploy bundle..."
"$QT_PATH/bin/macdeployqt" "$APP_BUNDLE_DEPLOY" 2>/dev/null || \
    echo "Advertencia: macdeployqt falló, copiando plugins manualmente..."

# Fallback plugins
PDEPLOY="$APP_BUNDLE_DEPLOY/Contents/PlugIns"
mkdir -p "$PDEPLOY/platforms" "$PDEPLOY/styles" "$PDEPLOY/imageformats"

for DYLIB in \
    "platforms/libqcocoa.dylib" \
    "styles/libqmacstyle.dylib" \
    "imageformats/libqsvg.dylib" \
    "imageformats/libqjpeg.dylib" \
    "imageformats/libqpng.dylib"; do
    if [ ! -f "$PDEPLOY/$DYLIB" ]; then
        cp "$QT_PATH/plugins/$DYLIB" "$PDEPLOY/$DYLIB" 2>/dev/null || true
    fi
done

if [ ! -f "$PDEPLOY/platforms/libqcocoa.dylib" ]; then
    echo "❌ Plugin Cocoa faltante. Deploy incompleto."
    exit 1
fi

echo ""
echo "✅ Deploy completado en: deploy/LGA_OpenInNukeX.app"
echo ""

# Ofrecer crear ZIP
read -p "¿Crear ZIP para distribución? (s/N): " CREATE_ZIP
if [[ "$CREATE_ZIP" =~ ^[sS]$ ]]; then
    ZIP_NAME="LGA_OpenInNukeX_v${PROJECT_VERSION_SHORT}_mac.zip"
    cd deploy
    zip -r "../$ZIP_NAME" LGA_OpenInNukeX.app
    cd ..
    echo "📦 ZIP creado: $ZIP_NAME"
fi

# Abrir Finder en deploy/
open deploy/
