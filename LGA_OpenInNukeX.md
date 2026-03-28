> **Regla de documentacion**: este archivo describe el estado actual del codigo. No es un historial de cambios, changelog ni bitacora temporal.
> **Regla de documentacion**: este archivo debe incluir una seccion de referencias tecnicas con rutas completas a los archivos mas importantes relacionados, y para cada archivo nombrar las funciones, clases o metodos clave vinculados a este tema.

# LGA_OpenInNukeX

**Aplicación Qt/C++ multiplataforma para abrir archivos .nk directamente en NukeX desde el explorador de archivos — Windows y macOS.**

## Características Principales

- **Aplicación multiplataforma**: Windows 10/11 y macOS 12+
- **Asociación de archivos**: Establece LGA_OpenInNukeX como aplicación predeterminada para archivos .nk
- **Apertura directa**: Doble clic en archivos .nk los abre en NukeX automáticamente
- **Inteligente**: Si NukeX ya está abierto, envía el archivo vía TCP. Si no, lanza una nueva instancia
- **Configuración visual**: Interfaz gráfica para configurar la ruta de NukeX
- **Logging detallado**: Registro completo de operaciones para debugging
- **Sin falsos positivos**: Ejecutable nativo Qt en lugar de Python

## Lógica de Apertura

Cuando se hace doble clic en un archivo `.nk`:

1. **Si NukeX está abierto**: El archivo se envía a la instancia activa vía TCP (puerto 54325)
2. **Si NukeX no está abierto**: Se lanza el ejecutable configurado con `--nukex` y el archivo como argumento

## Requisitos del Sistema

| | Windows | macOS |
|---|---|---|
| OS | Windows 10/11 (x64) | macOS 12+ (Intel o Apple Silicon via Rosetta) |
| NukeX | Cualquier versión instalada | Cualquier versión instalada |
| Permisos | Usuario estándar (no requiere admin) | Usuario estándar |
| Dependencias | SetUserFTA.exe (opcional) | duti via Homebrew (opcional) |

## Instalación y Uso

### Windows

#### Opción 1: Instalador
1. Ejecutar `LGA_OpenInNukeX_Installer.exe` (instala en `C:/Program Files/LGA/LGA_OpenInNukeX`)
2. Configurar la ruta de NukeX en la primera ejecución

#### Opción 2: Portable
1. Extraer en cualquier ubicación
2. Ejecutar `LGA_OpenInNukeX.exe`
3. Configurar ruta de NukeX y aplicar asociación

### macOS

1. Copiar `LGA_OpenInNukeX.app` a `/Applications` o cualquier ubicación
2. Abrir la app (sin argumentos muestra la ventana de configuración)
3. Configurar la ruta del binario de Nuke, por ejemplo:
   ```
   /Applications/Nuke16.0v8/Nuke16.0v8.app/Contents/MacOS/Nuke16.0
   ```
4. Hacer clic en **APPLY** para registrar la asociación de archivos .nk

## Configuración

### Ventana de Configuración

Se muestra al ejecutar la app sin argumentos:

- **Sección File Association**: Botón **APPLY** para establecer .nk como asociación predeterminada
- **Sección Preferred Nuke Version**: Campo de ruta + botón **BROWSE** + botón **SAVE**

En macOS, **BROWSE** navega a `/Applications` y permite seleccionar el binario dentro del `.app` bundle. Si se selecciona el `.app` directamente, la app resuelve automáticamente el binario dentro de `Contents/MacOS/`.

### Archivos de Configuración

| Plataforma | Ruta |
|---|---|
| Windows | `%AppData%\LGA\OpenInNukeX\nukeXpath.txt` |
| macOS | `~/Library/Application Support/LGA/OpenInNukeX/nukeXpath.txt` |

### Logs

| Plataforma | Ruta |
|---|---|
| Windows | `%AppData%\LGA\OpenInNukeX\OpenInNukeX.log` |
| macOS | `~/Library/Application Support/LGA/OpenInNukeX/OpenInNukeX.log` |

Los logs se borran al iniciar la aplicación. Niveles: `INFO`, `WARNING`, `ERROR`.

## Asociación de Archivos

### Windows
Usa el registro de Windows (`HKEY_CURRENT_USER`):
- Registra el ProgID `LGA.NukeScript.1`
- Usa SetUserFTA si está disponible (más robusto), fallback a PowerShell/reg
- Notifica al explorador vía `SHChangeNotify()`

### macOS
- Registra la app con Launch Services vía `lsregister`
- Usa `duti` (Homebrew) para establecer la app como handler predeterminado de `.nk`
- Si `duti` no está instalado, muestra instrucciones para configurar manualmente: clic derecho en un `.nk` → Obtener información → Abrir con → Cambiar todo

## Estructura del Proyecto

```
LGA_OpenInNukeX/
├── QtClient/
│   ├── src/
│   │   ├── main.cpp              # NukeApp (QFileOpenEvent), lógica de inicio
│   │   ├── nukeopener.h/cpp      # TCP client, lanzamiento de NukeX
│   │   ├── configwindow.h/cpp    # Ventana de configuración (Win + Mac)
│   │   ├── nukescanner.h/cpp     # Detección automática de versiones Nuke
│   │   └── logger.h/cpp          # Sistema de logging
│   ├── resources/
│   │   ├── LGA_NukeShortcuts.ico/.png   # Icono de la app
│   │   ├── LGA_OpenInNukeX.icns         # Icono macOS
│   │   ├── app_icon.ico/.png            # Icono para archivos .nk (Windows)
│   │   ├── app.rc                       # Recursos Windows
│   │   └── dark_theme.qss               # Tema oscuro
│   ├── cmake/
│   │   └── Info.plist.in         # Bundle macOS (CFBundleDocumentTypes, UTI)
│   ├── scripts/                  # Scripts Windows (.bat)
│   ├── compilar.sh               # Build macOS (con macdeployqt)
│   ├── compilar_dev.sh           # Build dev macOS (rápido, Debug)
│   ├── deploy.sh                 # Release macOS
│   ├── limpiar.sh                # Limpieza de build
│   └── CMakeLists.txt            # Configuración CMake multiplataforma
├── init.py                       # Servidor TCP para NukeX (.nuke/)
└── LGA_OpenInNukeX.md            # Esta documentación
```

## Compilación

### Windows

Requiere Qt 6.x con MinGW y CMake.

```bat
cd QtClient/scripts
deploy.bat
```

### macOS

Requiere Qt 6.5.3 instalado (probado con Qt 6.5.3 para x86_64).

```bash
cd QtClient
./compilar_dev.sh   # Build rápido para desarrollo
./compilar.sh       # Build con macdeployqt
./deploy.sh         # Release (genera .app distribuible)
```

## Resolución de Problemas

### "Asociación no funciona" (Windows)
1. Usar botón **APPLY** (limpia conflictos automáticamente)
2. Verificar logs en `%AppData%\LGA\OpenInNukeX\OpenInNukeX.log`
3. Asegurarse de que la ruta de NukeX sea correcta

### "Asociación no funciona" (macOS)
1. Instalar `duti`: `brew install duti`
2. Volver a hacer clic en **APPLY**
3. Alternativa manual: clic derecho en `.nk` → Obtener información → Abrir con → Cambiar todo

### "Error al abrir NukeX"
Verificar que la ruta configurada apunta al binario correcto:
- **Windows**: `C:\Program Files\Nuke16.0v8\Nuke16.0.exe`
- **macOS**: `/Applications/Nuke16.0v8/Nuke16.0v8.app/Contents/MacOS/Nuke16.0`

### "SetUserFTA.exe no encontrado" (Windows — mensaje informativo)
No es un error. La app funciona con el método fallback. Para mayor robustez, descargar SetUserFTA y colocarlo junto al ejecutable.

## Referencias Técnicas

| Archivo | Funciones / Clases clave |
|---------|--------------------------|
| `QtClient/src/main.cpp` | `NukeApp::event()` (QFileOpenEvent), `main()`, timer 200ms macOS |
| `QtClient/src/nukeopener.cpp` | `sendToNuke()`, `onConnected()`, `onResponseReceived()`, `openNukeWithFile()`, `onSocketTimeout()` |
| `QtClient/src/configwindow.cpp` | `applyFileAssociation()`, `executeMacAssociation()`, `executeRegistryCommands()`, `browseNukePath()`, `resolveNukeBinaryFromBundle()`, `getAppBundlePath()` |
| `QtClient/src/nukescanner.cpp` | `getCommonNukePaths()`, `scanDirectory()`, `isValidNukeExecutable()`, `isValidNukeAppBundle()` |
| `QtClient/CMakeLists.txt` | Targets Win/Mac, MACOSX_BUNDLE, Info.plist, icns, AGL dummy |
| `QtClient/cmake/Info.plist.in` | CFBundleDocumentTypes (.nk), UTExportedTypeDeclarations, bundle ID |
| `init.py` | Servidor TCP puerto 54325, handler `run_script`, handler `ping` |
