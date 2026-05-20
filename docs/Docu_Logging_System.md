> **Regla de documentacion**: este archivo describe el estado actual del codigo. No es un historial de cambios, changelog ni bitacora temporal.
> **Regla de documentacion**: este archivo debe incluir una seccion de referencias tecnicas con rutas completas a los archivos mas importantes relacionados, y para cada archivo nombrar las funciones, clases o metodos clave vinculados a este tema.

# Guia del Sistema de Logging de LGA_OpenInNukeX

## Descripcion General

Esta guia documenta el sistema de logging implementado en `LGA_OpenInNukeX` para registrar toda la actividad del servidor TCP que recibe comandos externos para abrir scripts en NukeX o pegar contenido del clipboard en el proyecto actual. El objetivo principal es capturar la maxima cantidad de informacion posible antes, durante y despues de operaciones criticas (como `scriptClose`, `scriptOpen` y `nodePaste`) para poder diagnosticar crashes de Nuke.

## Objetivos del Sistema

- Escribir logs solo a archivo por defecto.
- Guardar el log en `logs/debugPy_OpenInNukeX.log` dentro del proyecto.
- Limpiar el archivo anterior al iniciar y escribir encabezado con fecha/hora.
- Mantener timestamps relativos desde el inicio.
- Incluir `module::funcName` en cada linea para saber exactamente donde ocurrio cada evento.
- Ser seguro para hilos usando `QueueHandler`, `QueueListener` y `threading.Lock`.
- Capturar snapshots completos del entorno de Nuke antes de operaciones peligrosas.
- Hacer flush del log antes de `scriptClose` y `scriptOpen` para que la informacion quede escrita incluso si Nuke crashea.
- Registrar el comando `paste_clipboard` sin pasar por el flujo de cierre/apertura de scripts.

## Flags Base

```python
DEBUG = True
DEBUG_CONSOLE = False
DEBUG_LOG = True
```

Comportamiento por defecto:

- `DEBUG = True`: activa el sistema de debug.
- `DEBUG_CONSOLE = False`: no imprime en Script Editor o consola.
- `DEBUG_LOG = True`: escribe al archivo `.log`.

## Ubicacion y Nombre del Log

```text
LGA_OpenInNukeX/logs/debugPy_OpenInNukeX.log
```

Estructura:

```text
LGA_OpenInNukeX/
|-- docs/
|   `-- Docu_Logging_System.md
|-- logs/
|   `-- debugPy_OpenInNukeX.log
|-- init.py
`-- LGA_QtAdapter_OpenInNukeX.py
```

## Implementacion

### Componentes del sistema de logging (todos en `init.py`)

- `RelativeTimeFormatter`: formatter con tiempo relativo desde el primer record.
- `_get_log_file_path()`: retorna la ruta al archivo de log.
- `_reset_log()`: vacia el archivo de log, escribe encabezado con fecha/hora y resetea `script_start_time` a `None` para que los timestamps relativos empiecen de cero. Se llama al inicio de cada `run_script_with_logging`.
- `setup_debug_logging()`: configura el logger asincrono con encabezado de fecha/hora (se ejecuta una sola vez al importar el modulo).
- `debug_print(*message, level="info")`: funcion principal de logging con soporte de niveles.
- `_flush_log()`: fuerza flush antes de operaciones peligrosas.
- `cleanup_logging()`: detiene el listener al terminar.

### Funciones de diagnostico de crashes

- `_collect_nuke_environment()`: captura un snapshot completo del entorno de Nuke incluyendo:
  - Version de Nuke, Python, plataforma, PID
  - Threads activos
  - Script actual, frame range, estado de modificacion
  - Total de nodos y top 5 tipos de nodos
  - Memoria RSS/VMS y CPU (si `psutil` esta disponible)

- `_collect_nuke_callbacks()`: enumera todos los callbacks registrados en Nuke que podrian ejecutarse post-apertura:
  - Tipos: `onScriptLoad`, `onScriptSave`, `onScriptClose`, `onCreate`, `onDestroy`, `onUserCreate`, `knobChanged`, `updateUI`, `autolabel`
  - Para cada callback reporta: modulo, nombre de funcion, nodeClass filtrado
  - Usa fallback a atributos directos de `nuke` si `nuke.callbacks` no esta disponible

- `_log_file_info(filepath)`: captura metadata del archivo `.nk` que se va a abrir:
  - Existencia, tamano, fecha de modificacion
  - Path absoluto, normalizado, si es symlink

### Comandos TCP registrados

- `ping`: confirma que el servidor esta activo y responde `pong`.
- `run_script||<path>`: ejecuta `run_script_with_logging(filepath)`, que cierra el proyecto actual y abre el `.nk` indicado.
- `paste_clipboard`: ejecuta `paste_clipboard_with_logging()`, que llama a `nuke.nodePaste("%clipboard%")` en el proyecto actual. No llama a `scriptClose()` ni a `scriptOpen()`.

### Reglas del logger

- `logger.propagate = False`
- Handler asincrono con cola (`QueueHandler` + `QueueListener`)
- Formatter: `[%(relative_time)s] [%(module)s::%(funcName)s] %(message)s`
- `threading.Lock` para proteger el setup
- Limpieza del archivo al iniciar con encabezado de fecha/hora
- Referencia global `_debug_file_handler` al `FileHandler` real (no al `QueueHandler`)
- Flush explicito antes de `scriptClose()`, `scriptOpen()`, `nodePaste("%clipboard%")`, y antes/despues de cada operacion Qt en la activacion de ventana
- `_flush_log()` drena la cola del `QueueListener`, hace `flush()` del `FileHandler` y `os.fsync()` para garantizar escritura a disco

### Formato del log

```text
Fecha: 2026-04-14 23:45:12
[0.001s] [init::debug_print] === OpenInNukeX: DETECTADO NUKEX - INICIANDO SERVIDOR ===
[0.002s] [init::debug_print] Versión de Nuke: 15.1v2
[1.234s] [init::debug_print] === NUEVA CONEXIÓN RECIBIDA ===
[1.235s] [init::debug_print] Datos recibidos: 'run_script||C:/proyecto/shot.nk'
[1.236s] [init::debug_print] --- SNAPSHOT ENTORNO PRE-OPERACIÓN ---
[1.237s] [init::debug_print]   Nuke version: 15.1v2
[1.238s] [init::debug_print]   Total nodos: 45
[1.239s] [init::debug_print]   Memoria RSS: 2048.3 MB
[1.240s] [init::debug_print] --- FASE 2: CERRANDO SCRIPT ACTUAL ---
[1.241s] [init::debug_print] --- FASE 3: ABRIENDO NUEVO SCRIPT ---
[2.100s] [init::debug_print] === SCRIPT ABIERTO EXITOSAMENTE ===
```

### Niveles de log soportados

```python
debug_print("Mensaje normal")                    # level="info" (default)
debug_print("Detalle tecnico", level="debug")
debug_print("Situacion no critica", level="warning")
debug_print("Error grave", level="error")
```

### Fases del proceso de apertura de script

1. **RESET LOG**: `_reset_log()` vacia el archivo, escribe encabezado de fecha/hora y resetea timestamps a cero.
2. **SNAPSHOT PRE-OPERACIÓN**: captura entorno completo + info del archivo antes de tocar nada.
3. **FASE 1 - VERIFICAR ESTADO INICIAL**: lee estado de `nuke.root()`.
4. **FASE 2 - CERRANDO SCRIPT ACTUAL**: flush + `scriptClose()`.
5. **FASE 3 - ABRIENDO NUEVO SCRIPT**: registra un callback temporal `nuke.addOnCreate` que loguea cada nodo mientras se carga (`[LOAD #N] Class 'name'`), con flush cada 50 nodos y flush inmediato para nodos riesgosos (Group, Gizmo, LiveGroup, Precomp, Expression). El callback se remueve con `nuke.removeOnCreate` en un bloque `finally` (se ejecuta incluso si hay crash). Luego: snapshot post-apertura + enumeracion de callbacks registrados + flush.
6. **FASE 4 - ACTIVANDO VENTANA**: flush antes de cada operacion Qt (`raise_()`, `activateWindow()`). Enumera todos los `topLevelWidgets` con nombre, clase, visibilidad. Flush despues de cada paso.
7. **SNAPSHOT POST-ERROR** (si hay excepcion): captura entorno despues del error.

### Fases del proceso de paste desde clipboard

1. **SNAPSHOT PRE-PASTE**: captura entorno completo antes de pegar nodos.
2. **CONTEO PREVIO**: registra la cantidad de nodos antes de `nodePaste`.
3. **PASTE**: ejecuta `nuke.nodePaste("%clipboard%")` en el hilo principal de Nuke.
4. **CONTEO POSTERIOR**: registra la cantidad de nodos despues del paste y una estimacion de nodos agregados.
5. **ACTIVACION DE VENTANA**: llama a `activate_nuke_window_with_logging()`.

## Reglas obligatorias

1. El archivo de log vive en `logs/` dentro del proyecto de la tool.
2. El nombre del archivo es `debugPy_OpenInNukeX.log`.
3. El logger no propaga al root logger.
4. Los mensajes por defecto no salen a consola.
5. Se hace flush antes de cada operacion que puede crashear Nuke.
6. Se captura snapshot del entorno antes y despues de operaciones criticas.

## Referencias tecnicas

- `C:\Users\leg4-pc\.nuke\LGA_OpenInNukeX\init.py`
  - `RelativeTimeFormatter`
  - `_get_log_file_path()`
  - `_reset_log()` — vacia log, escribe encabezado, resetea timestamps
  - `setup_debug_logging()` — configura logger, `_debug_file_handler`, `QueueListener`
  - `debug_print()`
  - `_flush_log()` — drena cola del listener, flush + fsync del `FileHandler` real
  - `cleanup_logging()`
  - `_collect_nuke_environment()`
  - `_collect_nuke_callbacks()` — enumera callbacks registrados en Nuke
  - `_log_file_info()`
  - `run_script_with_logging()`
  - `paste_clipboard_with_logging()`
  - `handle_client()`
  - `nuke_server()`
  - `activate_nuke_window_with_logging()` — flush antes/despues de cada operacion Qt
