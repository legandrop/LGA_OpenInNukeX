> **Regla de documentacion**: este archivo describe el estado actual del codigo. No es un historial de cambios, changelog ni bitacora temporal.
> **Regla de documentacion**: este archivo debe incluir una seccion de referencias tecnicas con rutas completas a los archivos mas importantes relacionados, y para cada archivo nombrar las funciones, clases o metodos clave vinculados a este tema.

# Solución de Problemas — Asociaciones de Archivos .nk

---

## Windows

### Problema: Los archivos .nk no se abren con LGA_OpenInNukeX

Si después de usar el botón **APPLY** los archivos .nk siguen sin abrirse correctamente:

#### 1. Verificar LGA_WinSetFTA.exe

- Asegúrate de que `LGA_WinSetFTA.exe`, `LGA_WinSetFTA.dll`, `LGA_WinSetFTA.runtimeconfig.json` y `LookUpLut4.bin` estén en la misma carpeta que `LGA_OpenInNukeX.exe`
- El paquete de build/deploy los genera automáticamente desde `tools/win_file_assoc/`
- Requiere **.NET 9 runtime** instalado en el sistema (build framework-dependent)

#### 2. Limpiar asociaciones previas

Ejecuta el script de limpieza:
```
scripts\limpiar_registro.bat
```

Este script elimina todas las asociaciones previas de archivos .nk que puedan estar causando conflictos.

#### 3. Reiniciar el Explorador de Windows

Si APPLY muestra éxito pero el doble clic no cambia:
1. Presiona `Ctrl + Shift + Esc` para abrir el Administrador de tareas
2. Busca "Explorador de Windows" o "Windows Explorer"
3. Haz clic derecho y selecciona "Reiniciar"

#### 4. Verificación manual

1. Haz clic derecho en un archivo .nk
2. Selecciona "Abrir con" > "Elegir otra aplicación"
3. Busca "LGA_OpenInNukeX" en la lista
4. Marca "Usar siempre esta aplicación para abrir archivos .nk"

#### 5. Solución de último recurso

1. Reinicia completamente Windows
2. Ejecuta `limpiar_registro.bat`
3. Abre `LGA_OpenInNukeX.exe` y usa el botón **APPLY**

### ¿Por qué es necesario LGA_WinSetFTA?

En Windows 11 con `HashVersion=1`, el sistema usa **`UserChoiceLatest`** e ignora el `UserChoice` legacy. Escribir solo el hash antiguo hace que la UI muestre la app como predeterminada pero el doble clic siga abriendo otra aplicación. `LGA_WinSetFTA` calcula el hash correcto para `UserChoiceLatest` y registra `Software\Classes\.nk` con el ProgID de OpenInNukeX.

### Archivos importantes (Windows)

- `LGA_OpenInNukeX.exe` — Aplicación principal
- `LGA_WinSetFTA.exe` (+ `.dll`, `.runtimeconfig.json`, `LookUpLut4.bin`) — Helper para asociaciones en Windows 11
- `app_icon.ico` — Icono de la aplicación
- `scripts\limpiar_registro.bat` — Script de limpieza

### Logs de depuración (Windows)

```
%APPDATA%\LGA\OpenInNukeX\OpenInNukeX.log
```

---

## macOS

### Problema: Los archivos .nk no se abren con LGA_OpenInNukeX

#### 1. Contestar que sí al cartel del sistema

Al tocar **APPLY**, macOS muestra su propio cartel preguntando si querés cambiar la aplicación
con la que se abren los `.nk`. Ese cartel lo pone el sistema y no se puede saltear: si se
contesta que no, la asociación no cambia. Volvé a tocar **APPLY** y aceptá.

La app ya no necesita `duti`: hace la misma llamada a Launch Services que hacía esa herramienta.

#### 2. Configurar manualmente en Finder

Si el cartel no aparece o la asociación sigue sin tomar:

1. Haz clic derecho en cualquier archivo `.nk` en el Finder
2. Selecciona **"Obtener información"** (o `Cmd + I`)
3. Expande la sección **"Abrir con:"**
4. Selecciona `LGA_OpenInNukeX` en el menú desplegable
5. Haz clic en **"Cambiar todo..."**
6. Confirma en el diálogo

#### 3. Verificar que la app está registrada con Launch Services

Ejecuta en Terminal:
```bash
/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f /Applications/LGA_OpenInNukeX.app
```

Luego reinicia el Finder:
```bash
killall Finder
```

#### 4. Verificar la ruta de NukeX configurada

La ruta debe apuntar al **binario** dentro del `.app` bundle, no al bundle en sí:

```
/Applications/Nuke16.0v8/Nuke16.0v8.app/Contents/MacOS/Nuke16.0
```

Verifica que el archivo existe y tiene permisos de ejecución:
```bash
ls -la "/Applications/Nuke16.0v8/Nuke16.0v8.app/Contents/MacOS/Nuke16.0"
```

#### 5. Revisar logs

```
~/Library/Application Support/LGA/OpenInNukeX/OpenInNukeX.log
```

#### 6. Problema: la app muestra ConfigWindow en lugar de abrir el archivo

Esto puede ocurrir si la app no recibe el `QFileOpenEvent` de macOS a tiempo. Verifica que:
- La app esté correctamente registrada con Launch Services (paso 3)
- El archivo `.nk` esté asociado a `LGA_OpenInNukeX` (paso 2)

---

## Referencias Técnicas

| Archivo | Funciones relevantes |
|---------|---------------------|
| `QtClient/src/winfileassociation.cpp` | `apply()`, `invokeSetFtaHelper()`, `registerExtensionClass()`, `currentNkProgId()`, `isUserChoiceLatestActive()` |
| `tools/win_file_assoc/Program.cs` | CLI del helper `LGA_WinSetFTA` |
| `QtClient/src/configwindow.cpp` | `applyFileAssociation()`, `executeMacAssociation()` |
| `QtClient/src/main.cpp` | `NukeApp::event()` (QFileOpenEvent), timer 200ms macOS |
| `QtClient/src/logger.cpp` | `logInfo()`, `logError()` |
