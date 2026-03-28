<p>
  <img src="Doc_Media/image1.png" alt="LGA OpenInNukeX logo" width="56" height="56" align="left" style="margin-right:8px;">
  <span style="font-size:1.6em;font-weight:700;line-height:1;">LGA OPENINNUKEX</span><br>
  <span style="font-style:italic;line-height:1;">Lega | v1.65</span><br>
</p>
<br clear="left">

## Instalación

- Copiar la carpeta **LGA_OpenInNukeX** a **%USERPROFILE%/.nuke** en Windows o a **~/.nuke** en macOS.<br> Debería quedar así:
  ```
  .nuke/
  └─ LGA_OpenInNukeX/
     ├─ init.py
     └─ LGA_QtAdapter_OpenInNukeX.py
  ```

- Con un editor de texto, agregar esta línea al archivo **init.py** que está dentro de la carpeta **.nuke**:

  ```python
  nuke.pluginAddPath('./LGA_OpenInNukeX')
  ```

- **Windows**: ejecutar `LGA_OpenInNukeX_Setup.exe`. Al terminar la instalación, abrir la aplicación para entrar a la configuración.
- **macOS**: copiar `LGA_OpenInNukeX.app` a una ubicación portable cualquiera. Puede estar en `/Applications`, pero no es obligatorio; la app se registra desde el bundle que se está ejecutando. Después abrir la aplicación para entrar a la configuración.

<br><br>

## ¿Qué hace?

`LGA_OpenInNukeX` es una aplicación de dos partes para abrir archivos `.nk` directamente en **NukeX** desde el explorador de archivos.

- **Servidor en NukeX**: corre desde [init.py](/Users/leg4/.nuke/LGA_OpenInNukeX/init.py), escucha por TCP en el puerto `54325` y recibe comandos externos.
- **Cliente Qt/C++**: corre fuera de Nuke, administra la asociación de archivos `.nk`, detecta la ruta configurada de NukeX y decide si enviar el archivo a una instancia ya abierta o lanzar una nueva.

La lógica de apertura es esta:

1. Si ya hay una instancia de **NukeX** abierta, el cliente envía el `.nk` vía TCP a esa sesión activa.
2. Si no hay una instancia abierta, el cliente lanza el ejecutable configurado de **NukeX** con el archivo como argumento.

Está pensado para trabajar con **NukeX** y no con otras variantes de Nuke.

<br><br>

![](Doc_Media/image2.png)

<br><br>

## Configuración

- El botón **APPLY** asocia a los archivos `.nk` con OpenInNukeX. Esto permite que al abrir cualquier `.nk` desde el Explorer o Finder, sea OpenInNukeX quien gestione cómo se abren los archivos.
- En **Windows** se usa una herramienta externa para esto: `SetUserFTA`.
- En **macOS** la app se registra con Launch Services y, si `duti` está instalado, intenta dejarse como handler por defecto de `.nk`. Si `duti` no está disponible, la app muestra el flujo manual de Finder para hacer `Open with` y `Change All`.
- Si hay alguna ventana de **Nuke** abierta, los archivos se abrirán siempre en esa ventana abierta.
- Si no hay una ventana de **Nuke** abierta, se usará la versión de Nuke seleccionada en el menú de abajo, en **NukeX**.

**Ruta de NukeX**

- En Windows hay que elegir el ejecutable de NukeX.
- En macOS se puede elegir el `.app` o directamente el binario dentro del bundle. Ejemplo:

  ```bash
  /Applications/Nuke16.0v8/Nuke16.0v8.app/Contents/MacOS/Nuke16.0
  ```

<br><br>

## Notas para developers

El cliente funciona en **Windows 10/11** y **macOS 12+**.

- Sin argumentos abre la ventana de configuración.
- Con un `.nk` recibido desde Finder, Explorer o línea de comandos intenta abrirlo en NukeX.
- Guarda configuración y logs en la ubicación estándar del sistema.

**Configuración**

- **APPLY**: registra la asociación de archivos `.nk`.
- **BROWSE**: permite elegir el ejecutable o bundle de NukeX.
- **SAVE**: guarda la ruta preferida de NukeX.

**Rutas de configuración**

| Plataforma | Configuración | Logs |
|---|---|---|
| Windows | `%AppData%\\LGA\\OpenInNukeX\\nukeXpath.txt` | `%AppData%\\LGA\\OpenInNukeX\\OpenInNukeX.log` |
| macOS | `~/Library/Application Support/LGA/OpenInNukeX/nukeXpath.txt` | `~/Library/Application Support/LGA/OpenInNukeX/OpenInNukeX.log` |

<br><br>

## Compilación

**Windows**

```bat
cd QtClient/scripts
compilar.bat
deploy.bat
instalador.bat
```

**macOS**

```bash
cd QtClient
./limpiar.sh
./compilar_dev.sh
./compilar.sh
./deploy.sh
```

Los scripts de macOS contemplan el empaquetado del `.app` y los pasos de deploy del cliente Qt.

<br><br>

## Referencias Técnicas

| Archivo | Funciones / clases clave |
|---|---|
| [init.py](/Users/leg4/.nuke/LGA_OpenInNukeX/init.py) | `setup_debug_logging()`, `handle_client()`, `run_script_with_logging()`, servidor TCP en puerto `54325` |
| [main.cpp](/Users/leg4/.nuke/LGA_OpenInNukeX/QtClient/src/main.cpp) | `NukeApp::event()`, `main()` |
| [nukeopener.cpp](/Users/leg4/.nuke/LGA_OpenInNukeX/QtClient/src/nukeopener.cpp) | `sendToNuke()`, `onConnected()`, `onResponseReceived()`, `openNukeWithFile()` |
| [configwindow.cpp](/Users/leg4/.nuke/LGA_OpenInNukeX/QtClient/src/configwindow.cpp) | `applyFileAssociation()`, `executeMacAssociation()`, `executeRegistryCommands()`, `browseNukePath()` |
| [nukescanner.cpp](/Users/leg4/.nuke/LGA_OpenInNukeX/QtClient/src/nukescanner.cpp) | `getCommonNukePaths()`, `scanDirectory()`, `isValidNukeExecutable()`, `isValidNukeAppBundle()` |
| [CMakeLists.txt](/Users/leg4/.nuke/LGA_OpenInNukeX/QtClient/CMakeLists.txt) | definición de targets Win/Mac, bundle macOS, recursos, deploy |
| [Info.plist.in](/Users/leg4/.nuke/LGA_OpenInNukeX/QtClient/cmake/Info.plist.in) | `CFBundleDocumentTypes`, UTI `.nk`, bundle identifier |
