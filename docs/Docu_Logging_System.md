> **Regla de documentacion**: este archivo describe el estado actual del codigo. No es un historial de cambios, changelog ni bitacora temporal.
> **Regla de documentacion**: este archivo debe incluir una seccion de referencias tecnicas con rutas completas a los archivos mas importantes relacionados, y para cada archivo nombrar las funciones, clases o metodos clave vinculados a este tema.

# Guia del Sistema de Logging de LGA_OpenInNukeX

## Descripcion General

Esta guia documenta el sistema de logging implementado en `LGA_OpenInNukeX` para registrar toda la actividad del servidor TCP que recibe comandos externos para abrir scripts en NukeX. El objetivo principal es capturar la maxima cantidad de informacion posible antes, durante y despues de operaciones criticas (como `scriptClose` y `scriptOpen`) para poder diagnosticar crashes de Nuke.

## Objetivos del Sistema

- Escribir logs solo a archivo por defecto.
- Guardar el log en `logs/debugPy_OpenInNukeX.log` dentro del proyecto.
- Limpiar el archivo anterior al iniciar y escribir encabezado con fecha/hora.
- Mantener timestamps relativos desde el inicio.
- Incluir `module::funcName` en cada linea para saber exactamente donde ocurrio cada evento.
- Ser seguro para hilos usando `QueueHandler`, `QueueListener` y `threading.Lock`.
- Capturar snapshots completos del entorno de Nuke antes de operaciones peligrosas.
- Hacer flush del log antes de `scriptClose` y `scriptOpen` para que la informacion quede escrita incluso si Nuke crashea.

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
- `setup_debug_logging()`: configura el logger asincrono con encabezado de fecha/hora.
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

- `_log_file_info(filepath)`: captura metadata del archivo `.nk` que se va a abrir:
  - Existencia, tamano, fecha de modificacion
  - Path absoluto, normalizado, si es symlink

### Reglas del logger

- `logger.propagate = False`
- Handler asincrono con cola (`QueueHandler` + `QueueListener`)
- Formatter: `[%(relative_time)s] [%(module)s::%(funcName)s] %(message)s`
- `threading.Lock` para proteger el setup
- Limpieza del archivo al iniciar con encabezado de fecha/hora
- Flush explicito antes de `scriptClose()` y `scriptOpen()`

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

1. **SNAPSHOT PRE-OPERACIÓN**: captura entorno completo + info del archivo antes de tocar nada.
2. **FASE 1 - VERIFICAR ESTADO INICIAL**: lee estado de `nuke.root()`.
3. **FASE 2 - CERRANDO SCRIPT ACTUAL**: flush + `scriptClose()`.
4. **FASE 3 - ABRIENDO NUEVO SCRIPT**: flush + `scriptOpen()` + snapshot post-apertura.
5. **FASE 4 - ACTIVANDO VENTANA**: `raise_()` + `activateWindow()`.
6. **SNAPSHOT POST-ERROR** (si hay excepcion): captura entorno despues del error.

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
  - `setup_debug_logging()`
  - `debug_print()`
  - `_flush_log()`
  - `cleanup_logging()`
  - `_collect_nuke_environment()`
  - `_log_file_info()`
  - `run_script_with_logging()`
  - `handle_client()`
  - `nuke_server()`
  - `activate_nuke_window_with_logging()`
