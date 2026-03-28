# LGA_OpenInNukeX

LGA_OpenInNukeX is a two-part application designed to open .nk files directly in NukeX from the file explorer — available for **Windows and macOS**.

## 1. The Server (NukeX Integration)

The server component runs within NukeX and listens for commands from external applications. It is **platform-independent** — the same `init.py` works on both Windows and macOS.

**Setup:**
Copy the entire `OpenInNukeX` folder (which contains the `init.py` file) into your `.nuke` directory. Then add the following lines to your `.nuke/init.py` file:

```python
# Import LGA_OpenInNukeX
nuke.pluginAddPath("./LGA_OpenInNukeX")
```

**Functionality:**
The server listens on port 54325 for incoming commands. It supports:
- **"ping"**: To check if NukeX is running and responsive.
- **"run_script"**: To open a specified .nk file in the running NukeX instance.

## 2. The Client (Qt/C++ Application)

The client is a portable application built with Qt/C++ that provides a user-friendly interface for configuring and managing .nk file associations.

**Platforms:** Windows 10/11 and macOS 12+

### How it Works (Intelligent Opening Logic)

When you double-click a `.nk` file:

1. **If NukeX is already open**: The file is sent to the running NukeX instance via TCP (port 54325), regardless of the preferred version configured. No new instance is launched.
2. **If no NukeX is open**: The application launches the NukeX executable configured in settings, then opens the file.

### Windows

**Installation:**
Run `LGA_OpenInNukeX_Installer.exe` (default install to `C:/Program Files/LGA/LGA_OpenInNukeX`) or use the portable version.

**File Association:**
Launch the app, click **APPLY** to associate `.nk` files. Uses Windows registry (SetUserFTA optional for better robustness).

### macOS

**Installation:**
Copy `LGA_OpenInNukeX.app` to `/Applications` or any preferred location.

**File Association:**
Launch the app, click **APPLY**. The app registers with Launch Services and attempts to set the default handler via `duti` (if installed via Homebrew). If `duti` is not available, the app shows Finder instructions (right-click any .nk → Get Info → Open with → Change All).

**Nuke Path:**
Browse to the Nuke binary inside the `.app` bundle, e.g.:
```
/Applications/Nuke16.0v8/Nuke16.0v8.app/Contents/MacOS/Nuke16.0
```

**Important Notes:**
- LGA_OpenInNukeX is designed to work with **NukeX** versions (launched with `--nukex` flag).
- On macOS, the client receives files via Apple Events (QFileOpenEvent), not command-line arguments.

---

## Technical References

| File | Key functions |
|------|--------------|
| `init.py` | TCP server, `run_script` handler, port 54325 |
| `QtClient/src/main.cpp` | `main()`, `NukeApp::event()` (QFileOpenEvent handler) |
| `QtClient/src/nukeopener.cpp` | `sendToNuke()`, `onConnected()`, `onResponseReceived()`, `openNukeWithFile()` |
| `QtClient/src/configwindow.cpp` | `applyFileAssociation()`, `executeMacAssociation()`, `executeRegistryCommands()`, `browseNukePath()` |
| `QtClient/src/nukescanner.cpp` | `getCommonNukePaths()`, `scanDirectory()`, `isValidNukeExecutable()` |
| `QtClient/CMakeLists.txt` | macOS bundle config, Info.plist, cross-platform targets |
| `QtClient/cmake/Info.plist.in` | CFBundleDocumentTypes (.nk), bundle identifier |
