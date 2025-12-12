"""
______________________________________________________________________________________

  LGA_OpenInNukeX v1.21 | Lega
  Initializes a server in NukeX to handle external commands via port 54325
______________________________________________________________________________________

"""

import nuke
import socket
import threading
import time

# Variable global para almacenar la conexion del socket
current_conn = None


if nuke.env["nukex"] and not nuke.env["studio"]:

    def handle_client(conn):
        with conn:
            try:
                data = conn.recv(1024).decode()
                if data:
                    if data == "ping":  # Agregar manejo de ping
                        if not nuke.env["studio"]:
                            conn.sendall("pong".encode())
                        else:
                            print("Received ping from NukeStudio, not responding.")
                    else:
                        command, filepath = data.split(
                            "||"
                        )  # Separa el comando de la ruta del archivo
                        if command == "run_script":
                            nuke.executeInMainThreadWithResult(
                                lambda: run_script(filepath)
                            )
                            response = "Script executed successfully\n"
                        else:
                            response = f"Received command: {data}\n"
                        conn.sendall(response.encode())
            except Exception as e:
                response = f"Error: {str(e)}\n"
                conn.sendall(response.encode())
            finally:
                conn.close()

    def run_script(filepath):
        import nuke
        from qt_compat_openinnukex import QApplication

        # Cierra el proyecto actual si esta modificado y lo verifica
        proyecto_modificado = nuke.root().modified()
        nuke.scriptClose()
        if nuke.root().modified() == proyecto_modificado and proyecto_modificado:
            # nuke.message('Cierre cancelado por el usuario.')
            pass
        else:
            # nuke.message('El proyecto fue cerrado.')
            nuke.scriptOpen(filepath)  # Abre el nuevo archivo .nk
            
            # Intentar traer Nuke al frente con verificaciones de seguridad
            try:
                app_instance = QApplication.instance()
                if app_instance:
                    active_window = app_instance.activeWindow()
                    if active_window:
                        active_window.raise_()
                        active_window.activateWindow()
                    else:
                        # Si no hay ventana activa, intentar usar la ventana principal de Nuke
                        main_window = None
                        for widget in app_instance.topLevelWidgets():
                            if widget.isWindow() and widget.isVisible():
                                main_window = widget
                                break
                        if main_window:
                            main_window.raise_()
                            main_window.activateWindow()
                        else:
                            print("No se pudo encontrar una ventana de Nuke para activar")
                else:
                    print("No se pudo obtener la instancia de QApplication")
            except Exception as e:
                print(f"Error al intentar activar la ventana de Nuke: {e}")
                # El script se abrirá de todas formas, solo no se activará la ventana

    # Configuracion y lanzamiento del servidor
    def nuke_server(port=54325):
        host = "localhost"
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            s.bind((host, port))
        except socket.error as e:
            print(f"Error al vincular al puerto {port}: {e}")
            s.close()
            return
        s.listen(1)
        print(f"OpenInNukeX active on port {port}")

        while True:
            conn, addr = s.accept()
            print(f"Connected by {addr}")
            client_thread = threading.Thread(target=handle_client, args=(conn,))
            client_thread.start()

    # print("Este es NukeX, ejecutando el script.")
    thread = threading.Thread(target=nuke_server, args=(54325,))
    thread.daemon = True
    thread.start()
else:
    print("OpenInNukeX inactive. This is not NukeX.")
    pass
