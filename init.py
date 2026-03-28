"""
______________________________________________________________________________________

  LGA_OpenInNukeX v1.65 | Lega
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
from pathlib import Path
import os

# Variable global para almacenar la conexion del socket
current_conn = None

# Variable global para trackear tiempo de inicio del script
script_start_time = None

# Listener global para logging asincrono
debug_log_listener = None


# Formatter personalizado para incluir tiempo relativo
class RelativeTimeFormatter(logging.Formatter):
    def format(self, record):
        global script_start_time
        if script_start_time is None:
            script_start_time = record.created

        # Calcular tiempo relativo en segundos con 3 decimales (milisegundos)
        relative_time = record.created - script_start_time
        record.relative_time = f"{relative_time:.3f}s"
        return super().format(record)


# Configurar logging para escribir en tiempo real a un archivo
def setup_debug_logging():
    """Configura el logging para debug que escribe en tiempo real a un archivo."""
    global debug_log_listener
    log_file_path = os.path.join(
        os.path.dirname(__file__), "..", "logs", "debugPy_OpenInNukeX.log"
    )

    # Asegurar que el directorio de logs existe
    os.makedirs(os.path.dirname(log_file_path), exist_ok=True)

    # Limpiar el archivo de log al iniciar el script
    if os.path.exists(log_file_path):
        try:
            with open(log_file_path, "w", encoding="utf-8") as f:
                f.write("")  # Limpiar el contenido del archivo
        except Exception as e:
            print(f"Warning: No se pudo limpiar el archivo de log: {e}")

    # Configurar el logger - SOLO PARA ARCHIVO, NO CONSOLA
    logger = logging.getLogger("openinnukex_logger")
    logger.setLevel(logging.DEBUG)

    # IMPORTANTE: Desactivar la propagación al logger root (que va a consola)
    logger.propagate = False

    # Limpiar handlers existentes para evitar duplicados
    if logger.handlers:
        logger.handlers.clear()

    # Crear handler para archivo con encoding UTF-8
    file_handler = logging.FileHandler(log_file_path, encoding="utf-8")
    file_handler.setLevel(logging.DEBUG)

    # Usar formatter personalizado con tiempo relativo
    formatter = RelativeTimeFormatter("[%(relative_time)s] %(message)s")
    file_handler.setFormatter(formatter)

    # Usar logging asincrono para reducir bloqueos
    log_queue = queue.Queue()
    queue_handler = QueueHandler(log_queue)
    queue_handler.setLevel(logging.DEBUG)
    logger.addHandler(queue_handler)

    # Detener listener anterior si existe
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


# Inicializar el logger de debug
debug_logger = setup_debug_logging()


# Variable global para activar o desactivar los prints
DEBUG = True
DEBUG_PRINT_CONSOLE = os.getenv("LGA_DEBUG_CONSOLE", "0") == "1"


def debug_print(*message):
    global script_start_time
    if DEBUG:
        # Inicializar tiempo de inicio si no está establecido
        if script_start_time is None:
            script_start_time = time.time()

        # Crear el mensaje uniendo todos los argumentos
        msg = " ".join(str(arg) for arg in message)

        # Calcular tiempo relativo para el print en consola
        relative_time = time.time() - script_start_time
        timestamped_msg = f"[{relative_time:.3f}s] {msg}"

        if DEBUG_PRINT_CONSOLE:
            print(timestamped_msg)  # Print con timestamp
        debug_logger.info(msg)  # El logger ya incluye el timestamp en el archivo


if nuke.env["nukex"] and not nuke.env["studio"]:
    debug_print("=== OpenInNukeX: DETECTADO NUKEX - INICIANDO SERVIDOR ===")
    debug_print(f"Versión de Nuke: {nuke.env['NukeVersionString']}")
    debug_print(f"NukeX: {nuke.env['nukex']}, Studio: {nuke.env['studio']}")
    print("Este es NukeX, ejecutando el script.")

    def handle_client(conn):
        debug_print("=== NUEVA CONEXIÓN RECIBIDA ===")
        with conn:
            try:
                debug_print("Recibiendo datos del cliente...")
                data = conn.recv(1024).decode()
                debug_print(f"Datos recibidos: '{data}'")

                if data:
                    if data == "ping":  # Agregar manejo de ping
                        debug_print("Comando PING recibido")
                        if not nuke.env["studio"]:
                            debug_print("Respondiendo PONG (NukeX)")
                            conn.sendall("pong".encode())
                        else:
                            debug_print("PING desde NukeStudio - no respondiendo")
                            print("Received ping from NukeStudio, not responding.")
                    else:
                        debug_print(f"Procesando comando complejo: '{data}'")
                        try:
                            command, filepath = data.split(
                                "||"
                            )  # Separa el comando de la ruta del archivo
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
                                debug_print(f"Comando desconocido: '{command}'")
                                response = f"Received command: {data}\n"
                        except ValueError as parse_error:
                            debug_print(f"ERROR parseando comando: {parse_error}")
                            response = f"Error parsing command: {data}\n"

                        debug_print("Enviando respuesta al cliente...")
                        conn.sendall(response.encode())
                        debug_print("Respuesta enviada exitosamente")
            except Exception as e:
                debug_print(f"ERROR CRÍTICO en handle_client: {str(e)}")
                import traceback

                debug_print(f"Traceback completo: {traceback.format_exc()}")
                try:
                    response = f"Error: {str(e)}\n"
                    conn.sendall(response.encode())
                except Exception as send_error:
                    debug_print(f"ERROR enviando respuesta de error: {send_error}")
            finally:
                debug_print("Cerrando conexión...")
                conn.close()
                debug_print("Conexión cerrada")

    def run_script_with_logging(filepath):
        debug_print(f"=== INICIANDO APERTURA DE SCRIPT: {filepath} ===")

        try:
            # Verificar estado inicial
            debug_print("Verificando estado inicial de Nuke...")
            initial_modified = nuke.root().modified()
            debug_print(f"Proyecto modificado inicialmente: {initial_modified}")

            # Intentar cerrar script
            debug_print("Cerrando script actual...")
            nuke.scriptClose()

            # Verificar cierre
            after_close_modified = nuke.root().modified()
            debug_print(f"Estado después del cierre: {after_close_modified}")

            # Verificar si el cierre fue exitoso
            if nuke.root().modified() == initial_modified and initial_modified:
                debug_print(
                    "CIERRE CANCELADO POR EL USUARIO - proyecto sigue modificado"
                )
                return False
            else:
                debug_print("Proyecto cerrado exitosamente")

            # Abrir nuevo script
            debug_print(f"Abriendo script: {filepath}")
            nuke.scriptOpen(filepath)  # Abre el nuevo archivo .nk
            debug_print("Script abierto exitosamente")

            # Activar ventana con logging detallado
            debug_print("Activando ventana de Nuke...")
            activate_nuke_window_with_logging()

            debug_print("=== SCRIPT ABIERTO EXITOSAMENTE ===")
            return True

        except Exception as e:
            debug_print(f"ERROR CRÍTICO en run_script_with_logging: {e}")
            import traceback

            debug_print(f"Traceback completo: {traceback.format_exc()}")
            raise  # Re-lanzar para que se capture arriba

    def activate_nuke_window_with_logging():
        """Activación de ventana con logging detallado"""
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
                                f"Posible ventana principal encontrada: {widget.objectName() if hasattr(widget, 'objectName') else 'Unknown'}"
                            )
                            break

                    if main_window:
                        debug_print("Activando ventana principal...")
                        main_window.raise_()
                        main_window.activateWindow()
                        debug_print("Ventana principal activada exitosamente")
                    else:
                        debug_print(
                            "WARNING: No se pudo encontrar una ventana de Nuke para activar"
                        )
            else:
                debug_print("WARNING: No se pudo obtener la instancia de QApplication")

        except Exception as e:
            debug_print(f"ERROR en activación de ventana: {e}")
            import traceback

            debug_print(f"Traceback activación ventana: {traceback.format_exc()}")
            # No relanzar - permitir que el script se abra aunque falle la activación

    # Configuracion y lanzamiento del servidor
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
            debug_print(f"ERROR de socket al iniciar servidor en puerto {port}: {e}")
            print(f"Error al vincular al puerto {port}: {e}")
            s.close()
            return
        except Exception as e:
            debug_print(f"ERROR CRÍTICO al iniciar servidor: {e}")
            import traceback

            debug_print(f"Traceback servidor: {traceback.format_exc()}")
            try:
                s.close()
            except:
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
    pass


# Función para cleanup del logging al salir (si es necesario)
def cleanup_logging():
    """Limpia el listener de logging al terminar"""
    global debug_log_listener
    if debug_log_listener:
        try:
            debug_print("Deteniendo listener de logging...")
            debug_log_listener.stop()
            debug_print("Listener de logging detenido")
        except Exception as e:
            debug_print(f"Error deteniendo listener: {e}")


# Registrar cleanup si es posible
try:
    import atexit

    atexit.register(cleanup_logging)
    debug_print("Cleanup de logging registrado con atexit")
except Exception as e:
    debug_print(f"No se pudo registrar cleanup: {e}")
