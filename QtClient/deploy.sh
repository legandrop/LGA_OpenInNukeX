#!/bin/bash

# Workaround para Qt 6.5.3 en macOS 15+ / M1
if [ "$(uname -m)" = "arm64" ]; then
    echo "Detectado ARM64. Relanzando bajo Rosetta..."
    exec arch -x86_64 "$0" "$@"
fi

echo "🚀 Deploy LGA_OpenInNukeX para distribución"

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

# Crear estructura del bundle de distribución
mkdir -p deploy/LGA_OpenInNukeX.app/Contents/{MacOS,Resources,Frameworks,PlugIns}

cp build/LGA_OpenInNukeX.app/Contents/MacOS/LGA_OpenInNukeX \
   deploy/LGA_OpenInNukeX.app/Contents/MacOS/

# Icono
cp resources/LGA_OpenInNukeX.icns \
   deploy/LGA_OpenInNukeX.app/Contents/Resources/

# QSS (necesario para la UI)
cp dark_theme.qss \
   deploy/LGA_OpenInNukeX.app/Contents/Resources/

# Info.plist con metadatos completos
cat > deploy/LGA_OpenInNukeX.app/Contents/Info.plist << 'EOL'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>LGA_OpenInNukeX</string>
    <key>CFBundleDisplayName</key>
    <string>LGA OpenInNukeX</string>
    <key>CFBundleIdentifier</key>
    <string>com.lga.openinnukex</string>
    <key>CFBundleVersion</key>
    <string>1.54.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.54</string>
    <key>CFBundleExecutable</key>
    <string>LGA_OpenInNukeX</string>
    <key>CFBundleIconFile</key>
    <string>LGA_OpenInNukeX</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>12.0.0</string>
    <key>LSArchitecturePriority</key>
    <array>
        <string>arm64</string>
        <string>x86_64</string>
    </array>
    <key>LSRequiresNativeExecution</key>
    <false/>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
    <key>CFBundleDocumentTypes</key>
    <array>
        <dict>
            <key>CFBundleTypeExtensions</key>
            <array><string>nk</string></array>
            <key>CFBundleTypeName</key>
            <string>Nuke Script</string>
            <key>CFBundleTypeRole</key>
            <string>Viewer</string>
            <key>LSHandlerRank</key>
            <string>Owner</string>
            <key>LSItemContentTypes</key>
            <array>
                <string>com.foundry.nuke.script</string>
                <string>public.data</string>
            </array>
        </dict>
    </array>
    <key>UTExportedTypeDeclarations</key>
    <array>
        <dict>
            <key>UTTypeIdentifier</key>
            <string>com.foundry.nuke.script</string>
            <key>UTTypeDescription</key>
            <string>Nuke Script</string>
            <key>UTTypeConformsTo</key>
            <array><string>public.data</string></array>
            <key>UTTypeTagSpecification</key>
            <dict>
                <key>public.filename-extension</key>
                <array><string>nk</string></array>
            </dict>
        </dict>
    </array>
    <key>NSAppTransportSecurity</key>
    <dict>
        <key>NSAllowsArbitraryLoads</key>
        <true/>
    </dict>
</dict>
</plist>
EOL

# macdeployqt para incluir Qt frameworks
echo "📦 Ejecutando macdeployqt en deploy bundle..."
"$QT_PATH/bin/macdeployqt" deploy/LGA_OpenInNukeX.app 2>/dev/null || \
    echo "Advertencia: macdeployqt falló, copiando plugins manualmente..."

# Fallback plugins
PDEPLOY="deploy/LGA_OpenInNukeX.app/Contents/PlugIns"
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
    ZIP_NAME="LGA_OpenInNukeX_v1.65_mac.zip"
    cd deploy
    zip -r "../$ZIP_NAME" LGA_OpenInNukeX.app
    cd ..
    echo "📦 ZIP creado: $ZIP_NAME"
fi

# Abrir Finder en deploy/
open deploy/
