> **Regla de documentacion**: este archivo describe el estado actual del codigo. No es un historial de cambios, changelog ni bitacora temporal.
> **Regla de documentacion**: este archivo debe incluir una seccion de referencias tecnicas con rutas completas a los archivos mas importantes relacionados, y para cada archivo nombrar las funciones, clases o metodos clave vinculados a este tema.

# LGA_OpenInNukeX — Qt Client

Cliente Qt/C++ multiplataforma para abrir archivos .nk en NukeX. Disponible para **Windows 10/11** y **macOS 12+**.

## Características

- **Multiplataforma**: mismo codebase para Windows y macOS con `#ifdef Q_OS_WIN` guards
- **Sin ventana de consola**: aplicación GUI pura en ambas plataformas
- **Apertura inteligente**: si NukeX está corriendo envía el archivo vía TCP; si no, lanza nueva instancia
- **Asociación de archivos**: Windows usa `LGA_WinSetFTA.exe` (helper .NET 9) para escribir `UserChoiceLatest` en Windows 11; macOS la resuelve con Launch Services, sin herramientas externas
- **Configuración en AppData/Application Support**: archivos de configuración y logs en ubicación estándar del sistema
- **Conexión TCP async**: conecta al servidor NukeX en puerto 54325 con timeout; sin bloqueos (`readyRead` signal)
- **Fallback automático**: si no hay instancia activa de NukeX, lanza el ejecutable configurado con `--nukex`
- **Interfaz moderna oscura**: tema QSS con fondo #161616
- **Barra de titulo oscura en Windows**: DWM fuerza caption oscura independientemente del tema del sistema
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
│   ├── icons/
│   │   ├── Alta/OpenInNukeX.icon # Fuente del icono macOS (Icon Composer)
│   │   ├── AppIcon.icns         # Icono app macOS, fallback (CFBundleIconFile)
│   │   └── Assets.car           # Icono app macOS moderno (CFBundleIconName)
│   ├── app_icon.ico             # Icono para archivos .nk (Windows)
│   ├── app_icon.png             # Icono fuente
│   ├── ../LGA_OpenInNukeX.rc    # Recursos Windows usados por CMake (icono, versión)
│   └── dark_theme.qss           # Tema oscuro
├── cmake/
│   └── Info.plist.in            # Bundle macOS (CFBundleDocumentTypes, UTI .nk)
├── scripts/                     # Utilidades auxiliares Windows (asociacion .nk, no se mueven)
└── CMakeLists.txt               # Configuración CMake multiplataforma

<repo>/                          # Los entry-points de build viven en la RAIZ, no en QtClient/
├── compilar.bat                 # Build Windows incremental + deploy + run (motor de desarrollo)
├── compilar_release.bat         # Wrapper de dos lineas hacia compilar.bat --release (uso manual)
├── deploy.bat                   # Release portable Windows
├── limpiar.bat                  # Limpieza manual
├── instalador.bat               # Instalador Windows
├── github_release_win.bat       # Publicacion del release de Windows en GitHub
├── compilar.sh                  # Build dev macOS (Debug, rápido)
├── compilar_release.sh          # Wrapper de dos lineas hacia compilar.sh --release (uso manual)
├── deploy.sh                    # Release macOS
├── limpiar.sh                   # Limpieza de build
├── create_dmg.sh                # DMG de primera instalacion (macOS)
└── github_release_mac.sh        # Publicacion del release de macOS en GitHub
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

Requiere Qt 6.5.3, MinGW 13.1, Ninja, LLVM/lld y CMake.

Los scripts se corren desde la RAIZ del repo (no desde `QtClient/`): se paran
solos en `QtClient/` internamente para configurar CMake y copiar dependencias.

```bat
compilar.bat            # Debug incremental, copia dependencias y lanza la app
compilar_release.bat    # Release manual, en el arbol build-release\ (forwardea a compilar.bat --release)
deploy.bat              # Release + deploy portable en QtClient\release\deploy
instalador.bat          # Regenera Release y crea el instalador con Inno Setup
```

`compilar.bat` conserva el cache de `build` y solo recompila los archivos
modificados. `--force-clean` queda reservado para una limpieza explicita o para
la migracion automatica desde un cache creado con otro generador/toolchain.
`instalador.bat` ejecuta primero `deploy.bat --no-run`, por lo que nunca
empaqueta accidentalmente un ejecutable Release anterior. En las dos
plataformas `compilar.*` ES el motor de desarrollo (Debug, `build/`); el de
Release es explicito (`compilar_release.*`), y ni `deploy.bat` ni `deploy.sh`
pasan por ese wrapper: llaman directo al motor con `--release --no-run`.

### macOS

Requiere Qt 6.5.3, que se instala universal (x86_64 + arm64). El bundle se compila
universal y en Apple Silicon se ejecuta **nativo arm64**. `compilar.sh --rosetta`
fuerza el lanzamiento traducido, util solo para reproducir un bug de la rebanada Intel.

Igual que en Windows, se corren desde la raíz del repo: se paran solos en
`QtClient/` internamente.

```bash
./limpiar.sh             # Limpia build/
./compilar.sh            # Debug rápido, copia plugins Qt mínimos, lanza la app
./compilar_release.sh    # Release manual, en el arbol build-release/ (forwardea a compilar.sh --release)
./deploy.sh              # Release, genera .app distribuible
```

En macOS 12+ AGL ya no existe, y Qt lo arrastra igual a traves de `WrapOpenGL::WrapOpenGL`.
No se crea ningun framework dummy: `CMakeLists.txt` lo neutraliza antes de `find_package` y
despues lo purga del target.

## Asociación de Archivos

### Windows
- Registra ProgID `LGA.NukeScript.1` en `HKCU\Software\Classes`
- Requiere `LGA_WinSetFTA.exe` (y `LookUpLut4.bin` al lado) para aplicar la asociación `.nk` con hash `UserChoiceLatest` en Windows 11
- Si el helper falta, APPLY puede mostrar éxito parcial pero el doble clic no cambiará en sistemas con `HashVersion=1`
- Llama `SHChangeNotify()` para actualizar el explorador inmediatamente

### macOS
- Registra el .app bundle con Launch Services: `lsregister -f LGA_OpenInNukeX.app`
- Se pone como handler predeterminado de los `.nk` llamando a `NSWorkspace setDefaultApplicationAtURL:toOpenContentType:` desde `src/macintegration.mm`. Es la misma API que usaba `duti` por dentro, así que ya no hace falta instalarlo por Homebrew
- macOS muestra SU PROPIO cartel de confirmación antes de cambiar la asociación; no se puede saltear. Si el usuario lo rechaza (o la llamada falla), la app muestra las instrucciones para hacerlo a mano con Get Info + Change All

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
| Qt | 6.5.3 + MinGW 13.1 | 6.5.3 (universal x86_64 + arm64) |
| CMake | 3.16+ | 3.16+ |
| OS | Windows 10/11 | macOS 12+ |
| Extras | Ninja, LLVM/lld, Inno Setup (opcional) | — |

## Referencias Técnicas

| Archivo | Funciones / Clases clave |
|---------|--------------------------|
| `src/main.cpp` | `NukeApp` (subclase QApplication), `NukeApp::event()` (QFileOpenEvent), `main()` |
| `src/nukeopener.h/cpp` | `sendToNuke()`, `onConnected()`, `onResponseReceived()`, `openNukeWithFile()`, `onSocketTimeout()`, `showAutoCloseMessage()` |
| `src/configwindow.h/cpp` | `applyFileAssociation()`, `executeMacAssociation()`, `executeRegistryCommands()`, `browseNukePath()`, `resolveNukeBinaryFromBundle()`, `getAppBundlePath()`, `loadStyleSheet()` |
| `src/nukescanner.h/cpp` | `getCommonNukePaths()`, `scanDirectory()`, `isValidNukeExecutable()`, `isValidNukeAppBundle()`, `parseNukeExecutable()` |
| `CMakeLists.txt` | Targets Win/Mac, `MACOSX_BUNDLE`, deployment target 12.0, Info.plist, icns, purga de AGL, build universal |
| `cmake/Info.plist.in` | `CFBundleDocumentTypes` (.nk), `UTExportedTypeDeclarations` (com.foundry.nuke.script), bundle ID |
