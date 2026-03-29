> **Regla de documentacion**: este archivo describe el estado actual del codigo. No es un historial de cambios, changelog ni bitacora temporal.
> **Regla de documentacion**: este archivo debe incluir una seccion de referencias tecnicas con rutas completas a los archivos mas importantes relacionados, y para cada archivo nombrar las funciones, clases o metodos clave vinculados a este tema.

# LGA_OpenInNukeX — Qt Client

Cliente Qt/C++ multiplataforma para abrir archivos .nk en NukeX. Disponible para **Windows 10/11** y **macOS 12+**.

## Características

- **Multiplataforma**: mismo codebase para Windows y macOS con `#ifdef Q_OS_WIN` guards
- **Sin ventana de consola**: aplicación GUI pura en ambas plataformas
- **Apertura inteligente**: si NukeX está corriendo envía el archivo vía TCP; si no, lanza nueva instancia
- **Asociación de archivos**: Windows usa registro (`HKCU`) + SetUserFTA opcional; macOS usa `lsregister` + `duti` opcional
- **Configuración en AppData/Application Support**: archivos de configuración y logs en ubicación estándar del sistema
- **Conexión TCP async**: conecta al servidor NukeX en puerto 54325 con timeout; sin bloqueos (`readyRead` signal)
- **Fallback automático**: si no hay instancia activa de NukeX, lanza el ejecutable configurado con `--nukex`
- **Interfaz moderna oscura**: tema QSS con fondo #161616
- **Sistema de logging**: logs detallados guardados en AppData/Application Support

## Estructura del Proyecto

```
QtClient/
├── src/
│   ├── main.cpp                 # NukeApp (captura QFileOpenEvent en macOS), main()
│   ├── nukeopener.h/cpp         # TCP client, lanzamiento de NukeX
│   ├── configwindow.h/cpp       # Ventana de configuración (Win + Mac)
│   ├── nukescanner.h/cpp        # Detección automática de versiones Nuke
│   └── logger.h/cpp             # Sistema de logging
├── resources/
│   ├── LGA_NukeShortcuts.ico    # Icono app (Windows)
│   ├── LGA_NukeShortcuts.png    # Icono fuente
│   ├── LGA_OpenInNukeX.icns     # Icono app (macOS)
│   ├── app_icon.ico             # Icono para archivos .nk (Windows)
│   ├── app_icon.png             # Icono fuente
│   ├── ../LGA_OpenInNukeX.rc    # Recursos Windows usados por CMake (icono, versión)
│   └── dark_theme.qss           # Tema oscuro
├── cmake/
│   └── Info.plist.in            # Bundle macOS (CFBundleDocumentTypes, UTI .nk)
├── scripts/                     # Scripts Windows (.bat)
│   ├── compilar.bat
│   ├── deploy.bat
│   ├── limpiar.bat
│   └── instalador.bat
├── compilar.sh                  # Build macOS (macdeployqt)
├── compilar_dev.sh              # Build dev macOS (Debug, rápido)
├── deploy.sh                    # Release macOS
├── limpiar.sh                   # Limpieza de build
└── CMakeLists.txt               # Configuración CMake multiplataforma
```

## Uso

### Con archivo (modo cliente — abre el .nk)
```bash
# Windows
LGA_OpenInNukeX.exe "ruta/al/archivo.nk"

# macOS (terminal)
LGA_OpenInNukeX.app/Contents/MacOS/LGA_OpenInNukeX "ruta/al/archivo.nk"

# macOS (Finder) — automático vía QFileOpenEvent / Apple Events
```

### Sin argumentos (modo configuración)
Abre la ventana de configuración con:
- Botón **APPLY**: asocia archivos .nk con esta app
- Campo de ruta + **BROWSE** + **SAVE**: configura el ejecutable de NukeX

## Compilación

### Windows

Requiere Qt 6.x con MinGW y CMake.

```bat
cd QtClient
compilar.bat    # Debug
deploy.bat      # Release + deploy portable
instalador.bat  # Genera instalador .exe con Inno Setup
```

### macOS

Requiere Qt 6.5.3 (x86_64). En Apple Silicon se ejecuta via Rosetta (los scripts lo manejan automáticamente).

```bash
./limpiar.sh        # Limpia build/
./compilar_dev.sh   # Debug rápido, copia plugins Qt mínimos, lanza la app
./compilar.sh       # Debug con macdeployqt completo
./deploy.sh         # Release, genera .app distribuible
```

Los scripts crean un AGL.framework dummy (necesario para linkar Qt 6.x en macOS 12+ donde AGL fue removido).

## Asociación de Archivos

### Windows
- Registra ProgID `LGA.NukeScript.1` en `HKCU\Software\Classes`
- Usa SetUserFTA si está disponible (evita UserChoice Protection de Windows 10/11)
- Fallback a PowerShell/reg si SetUserFTA no está presente
- Llama `SHChangeNotify()` para actualizar el explorador inmediatamente

### macOS
- Registra el .app bundle con Launch Services: `lsregister -f LGA_OpenInNukeX.app`
- Usa `duti` para establecer handler predeterminado: `duti -s com.lga.openinnukex .nk all`
- Si `duti` no está instalado (Homebrew), muestra instrucciones para configurar manualmente en Finder

## Detección de Versiones Nuke (NukeScanner)

### Windows
Escanea `C:/Program Files` y `C:/Program Files (x86)` buscando directorios `*Nuke*` con ejecutables `.exe`.

### macOS
Escanea `/Applications` buscando bundles `Nuke*.app` (directos o en subdirectorio). Extrae el binario de `Contents/MacOS/`. Filtra `.dylib`, `.so`, `.framework` y herramientas auxiliares.

## Configuración y Datos

| Plataforma | Configuración | Logs |
|---|---|---|
| Windows | `%AppData%\LGA\OpenInNukeX\nukeXpath.txt` | `%AppData%\LGA\OpenInNukeX\OpenInNukeX.log` |
| macOS | `~/Library/Application Support/LGA/OpenInNukeX/nukeXpath.txt` | `~/Library/Application Support/LGA/OpenInNukeX/OpenInNukeX.log` |

Los logs se borran al iniciar la app.

## Requisitos

| | Windows | macOS |
|---|---|---|
| Qt | 6.x + MinGW | 6.5.3 (x86_64) |
| CMake | 3.16+ | 3.16+ |
| OS | Windows 10/11 | macOS 12+ |
| Extras | Inno Setup (instalador, opcional) | Homebrew duti (opcional) |

## Referencias Técnicas

| Archivo | Funciones / Clases clave |
|---------|--------------------------|
| `src/main.cpp` | `NukeApp` (subclase QApplication), `NukeApp::event()` (QFileOpenEvent), `main()` |
| `src/nukeopener.h/cpp` | `sendToNuke()`, `onConnected()`, `onResponseReceived()`, `openNukeWithFile()`, `onSocketTimeout()`, `showAutoCloseMessage()` |
| `src/configwindow.h/cpp` | `applyFileAssociation()`, `executeMacAssociation()`, `executeRegistryCommands()`, `browseNukePath()`, `resolveNukeBinaryFromBundle()`, `getAppBundlePath()`, `loadStyleSheet()` |
| `src/nukescanner.h/cpp` | `getCommonNukePaths()`, `scanDirectory()`, `isValidNukeExecutable()`, `isValidNukeAppBundle()`, `parseNukeExecutable()` |
| `CMakeLists.txt` | Targets Win/Mac, `MACOSX_BUNDLE`, deployment target 12.0, Info.plist, icns, AGL dummy |
| `cmake/Info.plist.in` | `CFBundleDocumentTypes` (.nk), `UTExportedTypeDeclarations` (com.foundry.nuke.script), bundle ID |
