# LGA_OpenInNukeX: Integración NukeX y Hiero

Sistema para optimizar el flujo de trabajo entre The Foundry NukeX y Hiero, permitiendo abrir scripts de Nuke (.nk) directamente desde Hiero o mediante aplicaciones cliente independientes desarrolladas en Python y Qt/C++.

## Propósito General

Facilita la apertura eficiente de archivos .nk en NukeX mediante comunicación TCP con instancias en ejecución o lanzando nuevas instancias. Incluye funcionalidad de Hiero para localizar, verificar versiones y abrir scripts de Nuke asociados a clips seleccionados en la línea de tiempo.

## Componentes Principales

### 1. Servidor NukeX
**Archivo**: `LGA_OpenInNukeX/init.py`  
Servidor TCP que se ejecuta al inicio de NukeX (no Nuke Studio) escuchando en puerto `54325`. Recibe rutas de archivos `.nk` y las abre en la instancia actual.

**Funciones principales**:
- `nuke_server()`: Inicializa y gestiona el servidor TCP
- `handle_client()`: Procesa comandos "ping" y "run_script" 
- `run_script()`: Cierra proyecto actual (si modificado) y abre archivo .nk especificado

### 2. Cliente Python (Legacy)
**Archivo**: `LGA_OpenInNukeX/Developement/LGA_OpenInNukeX.py`  
**Ejecutable compilado**: `LGA_OpenInNukeX.exe`  
Cliente original en Python compilado con PyInstaller. Recibe ruta de archivo .nk como argumento, intenta conexión TCP con servidor NukeX, o lanza nueva instancia si falla.

**Funciones principales**:
- `send_to_nuke()`: Gestiona conexión TCP o lanzamiento directo de NukeX
- `get_nuke_path_from_file()`: Lee ruta de ejecutable desde `nukeXpath.txt`
- `open_nuke_with_file()`: Abre nueva instancia de NukeX con archivo .nk

### 3. Cliente Qt/C++ (Actual)
**Directorio**: `LGA_OpenInNukeX/QtClient/`  
**Archivos principales**:
- `LGA_OpenInNukeX/QtClient/main.cpp`: Punto de entrada, manejo de argumentos, configuración AppData
- `LGA_OpenInNukeX/QtClient/nukeopener.h`: Definición de clase NukeOpener 
- `LGA_OpenInNukeX/QtClient/nukeopener.cpp`: Lógica de conexión TCP y lanzamiento de NukeX
- `LGA_OpenInNukeX/QtClient/configwindow.h`: Definición de ventana de configuración
- `LGA_OpenInNukeX/QtClient/configwindow.cpp`: Interfaz de configuración con fondo #161616
- `LGA_OpenInNukeX/QtClient/logger.h`: Sistema de logging a AppData
- `LGA_OpenInNukeX/QtClient/logger.cpp`: Implementación de logs detallados
- `LGA_OpenInNukeX/QtClient/CMakeLists.txt`: Configuración CMake sin ventana de consola
- `LGA_OpenInNukeX/QtClient/deploy.bat`: Script despliegue producción con limpieza automática
- `LGA_OpenInNukeX/QtClient/limpiar.bat`: Script limpieza de archivos de compilación

Cliente desarrollado en Qt/C++ para evitar falsos positivos de antivirus del ejecutable Python. Ejecuta sin ventana de consola y guarda configuración en AppData. Con argumentos procesa archivos .nk, sin argumentos muestra interfaz de configuración moderna.

**Funciones principales**:
- `main()`: Configuración de organización/aplicación para QStandardPaths, logging de directorio AppData
- `NukeOpener::sendToNuke()`: Conexión TCP con timeout de 10 segundos
- `NukeOpener::getNukePathFromFile()`: Lee configuración desde AppData con mensajes informativos
- `NukeOpener::onConnected()`: Envío de comando "run_script||ruta_archivo"
- `NukeOpener::openNukeWithFile()`: Lanzamiento de NukeX con argumentos --nukex
- `ConfigWindow::saveNukePath()`: Guarda configuración en AppData con creación automática de directorios
- `Logger::getLogFilePath()`: Retorna ruta de log en AppData con creación automática de directorios

**Configuración y archivos**:
- **Configuración**: `C:\Users\[usuario]\AppData\Roaming\LGA\LGA_OpenInNukeX_Qt\nukeXpath.txt`
- **Logs**: `C:\Users\[usuario]\AppData\Roaming\LGA\LGA_OpenInNukeX_Qt\LGA_OpenInNukeX_Qt.log`
- **Ejecutable**: `LGA_OpenInNukeX_Qt.exe` (sin ventana de consola)

### 4. Integración con Hiero
**Archivo**: `Python/Startup/LGA_NKS/LGA_NKS_OpenInNukeX.py`  
Integración que permite abrir scripts de Nuke desde línea de tiempo de Hiero. Identifica archivos fuente de clips, construye rutas a scripts .nk asociados (estructura `.../Comp/1_projects/`) y verifica versiones más recientes.

**Funciones principales**:
- `main()`: Orquesta selección de clips y determinación de rutas
- `open_nuke_script()`: Comunicación con servidor NukeX
- `find_latest_version()`: Busca versión más reciente basada en patrón `_v01`
- `get_project_path()`: Extrae ruta de carpeta de proyecto desde clip
- `show_version_dialog()`: Diálogo para selección de versión

## Flujo de Trabajo

1. **Servidor NukeX**: `init.py` lanza servidor TCP al iniciar NukeX
2. **Invocación**: Cliente recibe ruta .nk como argumento o muestra interfaz de configuración
3. **Conexión TCP**: Intenta conectar a `localhost:54325` con timeout 10 segundos
4. **Comando**: Envía `run_script||ruta_normalizada` si conecta exitosamente
5. **Fallback**: Si falla conexión, lanza nueva instancia NukeX con `--nukex archivo.nk`
