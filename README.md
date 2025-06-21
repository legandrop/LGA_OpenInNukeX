# LGA_OpenInNukeX

LGA_OpenInNukeX is a two-part application designed to open .nk files directly in NukeX from the Windows file explorer.

## 1. The Server (NukeX Integration)

The server component runs within NukeX and listens for commands from external applications.

**Setup:**
To enable the server, copy the entire `OpenInNukeX` folder (which contains the `init.py` file) into your `.nuke` directory. Then, add the following lines to your `.nuke/init.py` file:

```python
# Import LGA_OpenInNukeX
nuke.pluginAddPath("./LGA_OpenInNukeX")
```

**Functionality:**
The server listens on a specific port (default: 54325) for incoming commands. It supports:
*   **"ping"**: To check if NukeX is running and responsive.
*   **"run_script"**: To open a specified .nk file in NukeX. This allows other applications (like the client part of LGA_OpenInNukeX) to send a command to open a file.

## 2. The Client (Windows Application)

The client is a portable Windows-only application (built with Qt/C++) that provides a user-friendly interface for managing .nk file associations.

**Installation:**
The client can be installed by running the `LGA_OpenInNukeX_Installer.exe`. By default, it installs to `C:/Program Files/LGA/LGA_OpenInNukeX` (or `C:/Users/<YourUser>/AppData/Local/LGA/LGA_OpenInNukeX` for portable use). The installer will guide you through the process.

**Key Features:**
*   **File Association**: After installation, launch the application. It provides a button to associate `.nk` files with `LGA_OpenInNukeX`. This makes `LGA_OpenInNukeX.exe` the default application for opening `.nk` files.
*   **NukeX Version Selection**: The application allows you to specify a preferred NukeX executable path from your installed Nuke versions. This setting determines which NukeX instance will be launched when no NukeX is currently running.

**How it Works (Intelligent Opening Logic):**
`LGA_OpenInNukeX` prioritizes opening `.nk` files in an already running NukeX instance. When you double-click a `.nk` file:
1.  **If NukeX is already open**: The file will be sent to the *currently running* NukeX instance via the server component, regardless of the preferred NukeX version selected in the client's settings. This ensures a seamless workflow and avoids launching multiple NukeX instances unnecessarily.
2.  **If no NukeX is open**: The application will launch the NukeX executable specified in your client settings and then open the `.nk` file with that version.

**Important Notes:**
*   `LGA_OpenInNukeX` is designed to work exclusively with **NukeX** versions, not other Nuke variants (e.g., Nuke Non-commercial, Nuke Studio).
*   This application is only compatible with **Windows** operating systems.

---
