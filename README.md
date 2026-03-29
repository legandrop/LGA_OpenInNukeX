<p>
  <img src="Doc_Media/image1.png" alt="LGA OpenInNukeX logo" width="56" height="56" align="left" style="margin-right:8px;">
  <span style="font-size:1.6em;font-weight:700;line-height:1;">LGA OPENINNUKEX</span><br>
  <span style="font-style:italic;line-height:1;">Lega | v1.65</span><br>
</p>
<br clear="left">


Al instalar Nuke, hacer doble click en un archivo `.nk` siempre abre una nueva instancia de Nuke (no NukeX). 
Aunque se puede redirigir para usar NukeX, igual se crea una nueva instancia cada vez, acumulando varias ventanas abiertas.

`LGA_OpenInNukeX` soluciona esto.
Permite abrir archivos `.nk` directamente en **NukeX** desde el explorador de archivos:<br>
&nbsp;&nbsp;&nbsp;•	Si ya hay una instancia de **NukeX** abierta → abre el script ahí<br>
&nbsp;&nbsp;&nbsp;•	Si no hay ninguna → abre una nueva instancia de **NukeX** <br>

<br>

Es una aplicación de dos partes:<br>
&nbsp;&nbsp;&nbsp;• **Servidor en NukeX**: corre desde [init.py](/Users/leg4/.nuke/LGA_OpenInNukeX/init.py), escucha por TCP en el puerto `54325` y recibe comandos externos.<br>
&nbsp;&nbsp;&nbsp;• **Cliente Qt/C++**: corre fuera de Nuke, administra la asociación de archivos `.nk`, detecta la ruta configurada de NukeX y decide si enviar el archivo a una instancia ya abierta o lanzar una nueva.<br><br>

La lógica de apertura es esta:<br>
&nbsp;&nbsp;&nbsp;1. Si ya hay una instancia de **NukeX** abierta, el cliente envía el `.nk` vía TCP a esa sesión activa.<br>
&nbsp;&nbsp;&nbsp;2. Si no hay una instancia abierta, el cliente lanza el ejecutable de **NukeX** configurado en la sección **Preferred Nuke Version**.
<br>

Solo funciona con **NukeX** y no con otras variantes de Nuke. El servidor en `init.py` verifica esto antes de iniciar.
![](Doc_Media/image2.png)

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
- **macOS**: copiar `LGA_OpenInNukeX.app` a `/Applications`. Después abrir la aplicación para entrar a la configuración.

<br><br>


## Configuración

- El botón **APPLY** asocia los archivos `.nk` con OpenInNukeX. Después de aplicarlo, hacer doble clic en cualquier `.nk` desde el Explorer o Finder lo abrirá a través de esta app.
  - En **Windows** se usa la app `SetUserFTA.exe` que viene incluida en el paquete.
  - En **macOS** se registra con Launch Services y usa `duti` si está instalado (`brew install duti`). Si no, la app indica el flujo manual en Finder: clic derecho en un `.nk` → Obtener información → Abrir con → Cambiar todo.

- La sección **Preferred Nuke Version** escanea automáticamente las rutas comunes del sistema al abrirse y muestra un botón por cada versión de Nuke detectada. Al hacer clic en uno de esos botones la ruta se carga en el campo de texto. Si la instalación de Nuke está en una ubicación no estándar, se puede usar **BROWSE** para localizarla manualmente.
  - En **Windows** escanea `C:\Program Files` y `C:\Program Files (x86)`.
  - En **macOS** escanea `/Applications`.
  - El botón **BROWSE** en macOS acepta tanto el `.app` como el binario directamente y resuelve la ruta automáticamente.

<br><br>

## Compilación

**Windows**

```bat
cd QtClient/scripts
deploy.bat       # Release + paquete portable
instalador.bat   # Genera el instalador .exe con Inno Setup
```

**macOS**

```bash
cd QtClient
./compilar_dev.sh   # Build rápido (Debug) para desarrollo y pruebas
./deploy.sh         # Build Release, genera el .app distribuible
```

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
