"""
______________________________________________________________________________________

  LGA_OpenInNukeX v1.67 | Lega
  Initializes a server in NukeX to handle external commands via port 54325
______________________________________________________________________________________

"""

import nuke
import socket
import threading
import time
import logging
import queue
from logging.handlers import QueueHandler, QueueListener
import os
import sys
import traceback

# ---------------------------------------------------------------------------
# Logging flags  (alineados al patron de LGA_ToolPack / Python-Startup)
# ---------------------------------------------------------------------------
DEBUG = True
DEBUG_CONSOLE = False
DEBUG_LOG = True

# ---------------------------------------------------------------------------
# Logging internals
# ---------------------------------------------------------------------------
current_conn = None
script_start_time = None
debug_log_listener = None
_logging_lock = threading.Lock()

TOOL_NAME = "OpenInNukeX"
LOG_FILENAME = f"debugPy_{TOOL_NAME}.log"


class RelativeTimeFormatter(logging.Formatter):
    def format(self, record):
        global script_start_time
        if script_start_time is None:
            script_start_time = record.created
        relative_time = record.created - script_start_time
        record.relative_time = f"{relative_time:.3f}s"
        return super().format(record)


def _get_log_file_path():
    return os.path.join(os.path.dirname(__file__), "logs", LOG_FILENAME)


def setup_debug_logging():
    """Configura logging asincrono a archivo con encabezado de fecha/hora."""
    global debug_log_listener

    with _logging_lock:
        log_file_path = _get_log_file_path()
        os.makedirs(os.path.dirname(log_file_path), exist_ok=True)

        try:
            with open(log_file_path, "w", encoding="utf-8") as f:
                f.write(f"Fecha: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        except Exception as e:
            print(f"Warning: No se pudo inicializar el archivo de log: {e}")

        logger = logging.getLogger(f"{TOOL_NAME.lower()}_logger")
        logger.setLevel(logging.DEBUG)
        logger.propagate = False

        if logger.handlers:
            logger.handlers.clear()

        file_handler = logging.FileHandler(log_file_path, encoding="utf-8")
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(
            RelativeTimeFormatter(
                "[%(relative_time)s] [%(module)s::%(funcName)s] %(message)s"
            )
        )

        log_queue = queue.Queue()
        queue_handler = QueueHandler(log_queue)
        queue_handler.setLevel(logging.DEBUG)
        logger.addHandler(queue_handler)

        if debug_log_listener:
            try:
                debug_log_listener.stop()
            except Exception:
                pass

        debug_log_listener = QueueListener(
            log_queue, file_handler, respect_handler_level=True
        )
        debug_log_listener.daemon = True
        debug_log_listener.start()

        return logger


debug_logger = setup_debug_logging()


def debug_print(*message, level="info"):
    """Logging con soporte de niveles, archivo y consola opcionales."""
    global script_start_time

    if not DEBUG:
        return

    msg = " ".join(str(arg) for arg in message)

    if DEBUG_LOG:
        if script_start_time is None:
            script_start_time = time.time()
        log_method = getattr(debug_logger, level, debug_logger.info)
        try:
            log_method(msg, stacklevel=2)
        except TypeError:
            log_method(msg)

    if DEBUG_CONSOLE:
        if script_start_time is None:
            script_start_time = time.time()
        relative_time = time.time() - script_start_time
        print(f"[{relative_time:.3f}s] {msg}")


def _collect_nuke_environment():
    """Captura un snapshot del entorno de Nuke para diagnosticar crashes."""
    info = []
    try:
        info.append(f"Nuke version: {nuke.env.get('NukeVersionString', '?')}")
        info.append(
            f"NukeX: {nuke.env.get('nukex', '?')}, Studio: {nuke.env.get('studio', '?')}"
        )
        info.append(f"Python: {sys.version}")
        info.append(f"Platform: {sys.platform}")
        info.append(f"PID: {os.getpid()}")
        info.append(f"Threads activos: {threading.active_count()}")
        info.append(f"Thread actual: {threading.current_thread().name}")
    except Exception as e:
        info.append(f"Error capturando entorno base: {e}")

    try:
        root = nuke.root()
        info.append(f"Script actual: {root.name()}")
        info.append(f"Frame range: {root.firstFrame()}-{root.lastFrame()}")
        info.append(f"Proyecto modificado: {root.modified()}")
    except Exception as e:
        info.append(f"Error capturando info de root: {e}")

    try:
        all_nodes = nuke.allNodes()
        info.append(f"Total nodos: {len(all_nodes)}")
        node_types = {}
        for n in all_nodes:
            cls = n.Class()
            node_types[cls] = node_types.get(cls, 0) + 1
        top5 = sorted(node_types.items(), key=lambda x: -x[1])[:5]
        info.append(f"Top nodos: {top5}")
    except Exception as e:
        info.append(f"Error capturando nodos: {e}")

    try:
        import psutil

        proc = psutil.Process(os.getpid())
        mem = proc.memory_info()
        info.append(f"Memoria RSS: {mem.rss / (1024*1024):.1f} MB")
        info.append(f"Memoria VMS: {mem.vms / (1024*1024):.1f} MB")
        info.append(f"CPU%: {proc.cpu_percent(interval=0)}")
    except ImportError:
        pass
    except Exception as e:
        info.append(f"Error capturando memoria: {e}")

    return info


def _log_file_info(filepath):
    """Loguea metadata del archivo que se va a abrir."""
    info = []
    try:
        if os.path.exists(filepath):
            stat = os.stat(filepath)
            info.append(f"Archivo existe: True")
            info.append(f"Tamaño: {stat.st_size / 1024:.1f} KB")
            info.append(
                f"Última modificación: {time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(stat.st_mtime))}"
            )
            info.append(f"Path absoluto: {os.path.abspath(filepath)}")
            info.append(f"Path normalizado: {os.path.normpath(filepath)}")
            info.append(f"Es symlink: {os.path.islink(filepath)}")
        else:
            info.append(f"Archivo existe: FALSE - {filepath}")
    except Exception as e:
        info.append(f"Error capturando info del archivo: {e}")
    return info


if nuke.env["nukex"] and not nuke.env["studio"]:
    debug_print("=== OpenInNukeX: DETECTADO NUKEX - INICIANDO SERVIDOR ===")
    debug_print(f"Versión de Nuke: {nuke.env['NukeVersionString']}")
    debug_print(f"NukeX: {nuke.env['nukex']}, Studio: {nuke.env['studio']}")
    debug_print(f"Python: {sys.version}")
    debug_print(f"PID: {os.getpid()}")
    debug_print(f"Log file: {_get_log_file_path()}")
    print("Este es NukeX, ejecutando el script.")

    def handle_client(conn):
        debug_print("=== NUEVA CONEXIÓN RECIBIDA ===")
        debug_print(f"Thread: {threading.current_thread().name}")
        with conn:
            try:
                debug_print("Recibiendo datos del cliente...")
                data = conn.recv(1024).decode()
                debug_print(f"Datos recibidos: '{data}'")

                if data:
                    if data == "ping":
                        debug_print("Comando PING recibido")
                        if not nuke.env["studio"]:
                            debug_print("Respondiendo PONG (NukeX)")
                            conn.sendall("pong".encode())
                        else:
                            debug_print("PING desde NukeStudio - no respondiendo")
                            print("Received ping from NukeStudio, not responding.")
                    else:
                        debug_print(f"Procesando comando: '{data}'")
                        try:
                            command, filepath = data.split("||")
                            debug_print(
                                f"Comando parseado: '{command}', Archivo: '{filepath}'"
                            )

                            if command == "run_script":
                                debug_print(
                                    f"Ejecutando script en main thread: {filepath}"
                                )
                                result = nuke.executeInMainThreadWithResult(
                                    lambda: run_script_with_logging(filepath)
                                )
                                debug_print(
                                    f"Resultado ejecución en main thread: {result}"
                                )
                                response = "Script executed successfully\n"
                            else:
                                debug_print(
                                    f"Comando desconocido: '{command}'", level="warning"
                                )
                                response = f"Received command: {data}\n"
                        except ValueError as parse_error:
                            debug_print(
                                f"ERROR parseando comando: {parse_error}", level="error"
                            )
                            response = f"Error parsing command: {data}\n"

                        debug_print("Enviando respuesta al cliente...")
                        conn.sendall(response.encode())
                        debug_print("Respuesta enviada exitosamente")
            except Exception as e:
                debug_print(f"ERROR CRÍTICO en handle_client: {e}", level="error")
                debug_print(
                    f"Traceback completo: {traceback.format_exc()}", level="error"
                )
                try:
                    response = f"Error: {str(e)}\n"
                    conn.sendall(response.encode())
                except Exception as send_error:
                    debug_print(
                        f"ERROR enviando respuesta de error: {send_error}",
                        level="error",
                    )
            finally:
                debug_print("Cerrando conexión...")
                conn.close()
                debug_print("Conexión cerrada")

    def _flush_log():
        """Fuerza el flush del log para que quede escrito antes de operaciones peligrosas."""
        try:
            logger = logging.getLogger(f"{TOOL_NAME.lower()}_logger")
            for h in logger.handlers:
                h.flush()
        except Exception:
            pass

    def run_script_with_logging(filepath):
        debug_print(f"=== INICIANDO APERTURA DE SCRIPT ===")
        debug_print(f"Filepath solicitado: {filepath}")

        # --- Snapshot del entorno ANTES de tocar nada ---
        debug_print("--- SNAPSHOT ENTORNO PRE-OPERACIÓN ---")
        for line in _collect_nuke_environment():
            debug_print(f"  {line}")

        debug_print("--- INFO ARCHIVO A ABRIR ---")
        for line in _log_file_info(filepath):
            debug_print(f"  {line}")

        try:
            debug_print("--- FASE 1: VERIFICAR ESTADO INICIAL ---")
            initial_modified = nuke.root().modified()
            initial_script = nuke.root().name()
            debug_print(f"Script actual: {initial_script}")
            debug_print(f"Proyecto modificado: {initial_modified}")

            debug_print("--- FASE 2: CERRANDO SCRIPT ACTUAL ---")
            _flush_log()
            nuke.scriptClose()

            after_close_modified = nuke.root().modified()
            after_close_script = nuke.root().name()
            debug_print(
                f"Estado post-cierre: modified={after_close_modified}, script={after_close_script}"
            )

            if nuke.root().modified() == initial_modified and initial_modified:
                debug_print(
                    "CIERRE CANCELADO POR EL USUARIO - proyecto sigue modificado",
                    level="warning",
                )
                return False
            else:
                debug_print("Proyecto cerrado exitosamente")

            debug_print("--- FASE 3: ABRIENDO NUEVO SCRIPT ---")
            debug_print(f"Llamando nuke.scriptOpen('{filepath}')...")
            _flush_log()
            nuke.scriptOpen(filepath)
            debug_print("nuke.scriptOpen() completado sin excepción")

            debug_print("--- SNAPSHOT ENTORNO POST-APERTURA ---")
            for line in _collect_nuke_environment():
                debug_print(f"  {line}")

            debug_print("--- FASE 4: ACTIVANDO VENTANA ---")
            activate_nuke_window_with_logging()

            debug_print("=== SCRIPT ABIERTO EXITOSAMENTE ===")
            return True

        except Exception as e:
            debug_print(f"ERROR CRÍTICO en run_script_with_logging: {e}", level="error")
            debug_print(f"Traceback completo: {traceback.format_exc()}", level="error")
            debug_print("--- SNAPSHOT ENTORNO POST-ERROR ---", level="error")
            try:
                for line in _collect_nuke_environment():
                    debug_print(f"  {line}", level="error")
            except Exception:
                debug_print("No se pudo capturar entorno post-error", level="error")
            raise

    def activate_nuke_window_with_logging():
        try:
            debug_print("Obteniendo instancia de QApplication...")
            from LGA_QtAdapter_OpenInNukeX import QApplication

            app_instance = QApplication.instance()

            if app_instance:
                debug_print("QApplication encontrada, buscando ventana activa...")
                active_window = app_instance.activeWindow()

                if active_window:
                    debug_print("Activando ventana activa...")
                    active_window.raise_()
                    active_window.activateWindow()
                    debug_print("Ventana activada exitosamente")
                else:
                    debug_print("No hay ventana activa, buscando ventana principal...")
                    main_window = None
                    for widget in app_instance.topLevelWidgets():
                        if widget.isWindow() and widget.isVisible():
                            main_window = widget
                            debug_print(
                                f"Ventana encontrada: {widget.objectName() if hasattr(widget, 'objectName') else 'Unknown'}"
                            )
                            break

                    if main_window:
                        debug_print("Activando ventana principal...")
                        main_window.raise_()
                        main_window.activateWindow()
                        debug_print("Ventana principal activada exitosamente")
                    else:
                        debug_print(
                            "No se pudo encontrar ventana de Nuke para activar",
                            level="warning",
                        )
            else:
                debug_print(
                    "No se pudo obtener la instancia de QApplication", level="warning"
                )

        except Exception as e:
            debug_print(f"ERROR en activación de ventana: {e}", level="error")
            debug_print(
                f"Traceback activación ventana: {traceback.format_exc()}", level="error"
            )

    def nuke_server(port=54325):
        debug_print(f"=== INICIANDO SERVIDOR OpenInNukeX EN PUERTO {port} ===")
        host = "localhost"
        try:
            debug_print("Creando socket TCP...")
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            debug_print("Socket creado exitosamente")

            debug_print(f"Intentando bind al puerto {port}...")
            s.bind((host, port))
            debug_print(f"Bind exitoso al puerto {port}")

            debug_print("Poniendo socket en modo escucha...")
            s.listen(1)
            debug_print(f"OpenInNukeX activo y escuchando en puerto {port}")
            print(f"OpenInNukeX active on port {port}")

            while True:
                debug_print("Esperando conexiones entrantes...")
                conn, addr = s.accept()
                debug_print(f"Conexión aceptada desde {addr}")

                debug_print("Creando thread para manejar cliente...")
                client_thread = threading.Thread(target=handle_client, args=(conn,))
                client_thread.daemon = True
                client_thread.start()
                debug_print(f"Thread cliente iniciado: {client_thread.name}")

        except socket.error as e:
            debug_print(f"ERROR de socket en puerto {port}: {e}", level="error")
            print(f"Error al vincular al puerto {port}: {e}")
            s.close()
            return
        except Exception as e:
            debug_print(f"ERROR CRÍTICO al iniciar servidor: {e}", level="error")
            debug_print(f"Traceback servidor: {traceback.format_exc()}", level="error")
            try:
                s.close()
            except Exception:
                pass
            return

    print("Este es NukeX, ejecutando el script.")
    debug_print("Creando thread principal del servidor...")
    thread = threading.Thread(target=nuke_server, args=(54325,))
    thread.daemon = True
    thread.name = "OpenInNukeX_Server"
    debug_print("Iniciando thread del servidor...")
    thread.start()
    debug_print("Thread del servidor iniciado exitosamente")
else:
    debug_print("=== OpenInNukeX: NO ES NUKEX - SERVIDOR INACTIVO ===")
    print("OpenInNukeX inactive. This is not NukeX.")


def cleanup_logging():
    global debug_log_listener
    if not debug_log_listener:
        return
    try:
        debug_log_listener.stop()
    except Exception:
        pass
    finally:
        debug_log_listener = None


try:
    import atexit

    atexit.register(cleanup_logging)
    debug_print("Cleanup de logging registrado con atexit")
except Exception as e:
    debug_print(f"No se pudo registrar cleanup: {e}", level="warning")
