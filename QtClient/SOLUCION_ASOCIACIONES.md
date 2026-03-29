> **Regla de documentacion**: este archivo describe el estado actual del codigo. No es un historial de cambios, changelog ni bitacora temporal.
> **Regla de documentacion**: este archivo debe incluir una seccion de referencias tecnicas con rutas completas a los archivos mas importantes relacionados, y para cada archivo nombrar las funciones, clases o metodos clave vinculados a este tema.

# Solución de Problemas — Asociaciones de Archivos .nk

---

## Windows

### Problema: Los archivos .nk no se abren con LGA_OpenInNukeX

Si después de usar el botón **APPLY** los archivos .nk siguen sin abrirse correctamente:

#### 1. Verificar SetUserFTA.exe

- Asegúrate de que `SetUserFTA.exe` esté en la misma carpeta que `LGA_OpenInNukeX.exe`
- Si no está, descárgalo desde: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/
- Colócalo en la carpeta `release/deploy/`

#### 2. Limpiar asociaciones previas

Ejecuta el script de limpieza:
```
scripts\limpiar_registro.bat
```

Este script elimina todas las asociaciones previas de archivos .nk que puedan estar causando conflictos.

#### 3. Ejecutar como administrador

1. Cierra LGA_OpenInNukeX si está abierto
2. Haz clic derecho en `LGA_OpenInNukeX.exe`
3. Selecciona "Ejecutar como administrador"
4. Usa el botón **APPLY** nuevamente

#### 4. Reiniciar el Explorador de Windows

Si aún no funciona:
1. Presiona `Ctrl + Shift + Esc` para abrir el Administrador de tareas
2. Busca "Explorador de Windows" o "Windows Explorer"
3. Haz clic derecho y selecciona "Reiniciar"

#### 5. Verificación manual

1. Haz clic derecho en un archivo .nk
2. Selecciona "Abrir con" > "Elegir otra aplicación"
3. Busca "LGA_OpenInNukeX" en la lista
4. Marca "Usar siempre esta aplicación para abrir archivos .nk"

#### 6. Solución de último recurso

1. Reinicia completamente Windows
2. Ejecuta `limpiar_registro.bat`
3. Ejecuta `LGA_OpenInNukeX.exe` como administrador
4. Usa el botón **APPLY**

### ¿Por qué es necesario SetUserFTA?

Windows 10/11 tiene protecciones que detectan cuando una aplicación intenta cambiar asociaciones de archivos sin pasar por el sistema oficial ("UserChoice Protection"). SetUserFTA genera el hash de validación correcto que Windows espera, evitando que detecte la asociación como "hijacking".

### Archivos importantes (Windows)

- `LGA_OpenInNukeX.exe` — Aplicación principal
- `SetUserFTA.exe` — Herramienta requerida para asociaciones funcionales en Windows
- `app_icon.ico` — Icono de la aplicación
- `scripts\limpiar_registro.bat` — Script de limpieza

### Logs de depuración (Windows)

```
%APPDATA%\LGA\OpenInNukeX\OpenInNukeX.log
```

---

## macOS

### Problema: Los archivos .nk no se abren con LGA_OpenInNukeX

#### 1. Instalar duti (recomendado)

`duti` es una herramienta de línea de comandos que establece handlers de archivos por defecto en macOS:

```bash
brew install duti
```

Luego vuelve a hacer clic en **APPLY** en la ventana de configuración.

#### 2. Configurar manualmente en Finder

Si no quieres usar `duti`:

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
| `src/configwindow.cpp` | `applyFileAssociation()`, `executeMacAssociation()`, `executeRegistryCommands()` |
| `src/main.cpp` | `NukeApp::event()` (QFileOpenEvent), timer 200ms macOS |
| `src/logger.cpp` | `logInfo()`, `logError()` |
