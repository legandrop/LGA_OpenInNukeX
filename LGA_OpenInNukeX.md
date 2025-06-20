# LGA_OpenInNukeX: Integración NukeX y Hiero

Esta documentación describe el sistema **LGA_OpenInNukeX**, diseñado para optimizar el flujo de trabajo entre The Foundry NukeX y Hiero, permitiendo abrir scripts de Nuke (.nk) directamente desde Hiero o mediante una aplicación cliente independiente.

## Propósito General

El sistema facilita la apertura eficiente de archivos .nk en NukeX, ya sea comunicándose con una instancia de NukeX ya en ejecución (a través de un servidor TCP) o lanzando una nueva instancia de NukeX si no hay una disponible o no responde. Incluye una funcionalidad de Hiero para localizar, verificar versiones y abrir los scripts de Nuke asociados a los clips seleccionados en la línea de tiempo.

## Componentes Principales

El sistema se compone de los siguientes elementos clave:

### 1. Servidor NukeX

*   **Archivo**: `LGA_OpenInNukeX/init.py`
*   **Descripción**: Este script se ejecuta dentro de una instancia de NukeX (no Nuke Studio) al inicio. Actúa como un servidor TCP, escuchando comandos entrantes en el puerto `54325`. Su función principal es recibir rutas de archivos `.nk` y abrirlas en la instancia actual de NukeX.
*   **Funciones Principales**:
    *   `nuke_server()`: Inicializa y gestiona el servidor TCP.
    *   `handle_client()`: Procesa los comandos recibidos del cliente, incluyendo "ping" y "run_script".
    *   `run_script()`: Cierra el proyecto actual de Nuke (si está modificado) y abre el archivo .nk especificado.

### 2. Cliente de NukeX (Ejecutable)

*   **Archivo Fuente**: `LGA_OpenInNukeX/Developement/LGA_OpenInNukeX.py`
*   **Archivo de Compilación**: `LGA_OpenInNukeX/Developement/Compilar.txt` (contiene el comando `pyinstaller` para generar el ejecutable)
*   **Descripción**: Este script está diseñado para ser compilado en un ejecutable (por ejemplo, `LGA_OpenInNukeX.exe`). Cuando se invoca con la ruta de un archivo `.nk` como argumento, intenta conectarse al servidor NukeX. Si la conexión es exitosa, envía el comando para abrir el archivo. En caso de fallo de conexión (servidor no activo o no responde), lanza una nueva instancia de NukeX abriendo directamente el archivo .nk.
*   **Funciones Principales**:
    *   `send_to_nuke()`: Intenta la conexión con el servidor NukeX y envía el comando, o gestiona el lanzamiento directo de NukeX.
    *   `get_nuke_path_from_file()`: Lee la ruta del ejecutable de Nuke desde un archivo `nukeXpath.txt` ubicado en el mismo directorio del ejecutable.
    *   `open_nuke_with_file()`: Abre una nueva instancia de NukeX con el archivo .nk.

### 3. Integración con Hiero

*   **Archivo**: `Python/Startup/LGA_NKS/LGA_NKS_OpenInNukeX.py`
*   **Descripción**: Este script se integra con Hiero para permitir a los usuarios abrir scripts de Nuke directamente desde la línea de tiempo. Identifica el archivo fuente de los clips seleccionados, construye la ruta al script .nk asociado (esperando una estructura de proyecto específica como `.../Comp/1_projects/`), y verifica si existe una versión más reciente del script. Si es así, notifica al usuario y ofrece la opción de abrir la última versión. Finalmente, utiliza la lógica del "Cliente de NukeX" para abrir el archivo en NukeX.
*   **Funciones Principales**:
    *   `main()`: Punto de entrada principal que orquesta la selección de clips, la determinación de la ruta del script y la verificación de versiones.
    *   `open_nuke_script()`: Gestiona la comunicación con el servidor NukeX (o el lanzamiento directo de NukeX) para abrir el archivo .nk.
    *   `find_latest_version()`: Busca y determina la versión más reciente de un script Nuke basándose en el patrón de nombrado (ej. `_v01`).
    *   `get_project_path()`: Extrae la ruta de la carpeta del proyecto Nuke a partir de la ruta del clip de Hiero.
    *   `get_script_name()`: Normaliza el nombre del script de Nuke a partir del nombre del archivo del clip.
    *   `show_version_dialog()`: Muestra un diálogo para que el usuario elija qué versión abrir (actual o más reciente).

## Flujo de Trabajo Típico

1.  **NukeX al iniciar**: El script `LGA_OpenInNukeX/init.py` lanza un servidor en NukeX, haciéndolo receptivo a comandos externos.
2.  **Desde Hiero**: Un usuario selecciona un clip en la línea de tiempo y ejecuta la herramienta `LGA_NKS_OpenInNukeX.py`.
3.  **Hiero procesa**: El script de Hiero determina la ruta del script de Nuke asociado, verifica versiones y, si es necesario, pregunta al usuario qué versión abrir.
4.  **Cliente de NukeX (ejecutable) se comunica**: `LGA_NKS_OpenInNukeX.py` invoca (internamente o a través de un mecanismo similar) la lógica de conexión que intentaría usar el ejecutable `LGA_OpenInNukeX.exe`.
5.  **Apertura del script**: Si el servidor NukeX está activo, el comando y la ruta del archivo se envían. El servidor NukeX ejecuta `run_script` para abrir el `.nk`. Si el servidor no está activo, el ejecutable lanza una nueva instancia de NukeX directamente con el archivo.
