# LGA_OpenInNukeX_Qt

Cliente Qt/C++ para abrir archivos .nk en NukeX, reemplazando el ejecutable Python para evitar falsos positivos de antivirus.

## Características

- **Sin ventana de consola**: Ejecutable configurado como aplicación Windows pura
- **Configuración en AppData**: Archivos de configuración y logs se guardan en `%AppData%\LGA\LGA_OpenInNukeX_Qt`
- **Conexión TCP**: Conecta a instancia de NukeX en puerto 54325 con timeout de 10 segundos
- **Fallback automático**: Si no hay conexión TCP, lanza nueva instancia de NukeX
- **Interfaz de configuración**: Cuando se ejecuta sin argumentos, muestra ventana para configurar ruta de NukeX
- **Diseño moderno**: Interfaz con fondo #161616 siguiendo estándares de diseño
- **Sistema de logging**: Logs detallados guardados automáticamente en AppData
- **Limpieza automática**: Scripts de compilación incluyen limpieza previa de archivos

## Uso

### Con archivo de script
```bash
LGA_OpenInNukeX_Qt.exe "ruta/al/archivo.nk"
```

### Sin argumentos (configuración)
```bash
LGA_OpenInNukeX_Qt.exe
```
Abre ventana de configuración para seleccionar ruta de NukeX.exe

## Compilación

### Limpieza de archivos
```bash
limpiar.bat
```
Elimina archivos de compilación anterior (Makefiles, *.o, *.exe, directorios release/build/debug)

### Desarrollo (Debug)
```bash
compilar.bat
```

### Producción (Release + Deploy)
```bash
deploy.bat
```

### Alternativa con qmake
```bash
compilar_qmake.bat
```

Todos los scripts de compilación ejecutan automáticamente `limpiar.bat` antes de compilar.

## Requisitos

- Qt 6.8.2 con MinGW 13.1.0
- CMake 3.16+ (para compilación con CMake)
- Windows 10/11

## Archivos Principales

- `main.cpp`: Punto de entrada, configuración QStandardPaths, logging de directorio AppData
- `nukeopener.h/cpp`: Lógica de conexión TCP, lanzamiento de NukeX, lectura de configuración desde AppData
- `configwindow.h/cpp`: Ventana de configuración, guardado en AppData con creación automática de directorios
- `logger.h/cpp`: Sistema de logging con archivos en AppData
- `limpiar.bat`: Script de limpieza completa de archivos de compilación

## Configuración y Datos

### Ubicación de archivos
- **Configuración**: `C:\Users\[usuario]\AppData\Roaming\LGA\LGA_OpenInNukeX_Qt\nukeXpath.txt`
- **Logs**: `C:\Users\[usuario]\AppData\Roaming\LGA\LGA_OpenInNukeX_Qt\LGA_OpenInNukeX_Qt.log`

### Estructura de Despliegue

```
release/deploy/
├── LGA_OpenInNukeX_Qt.exe     # Ejecutable principal (sin consola)
├── Qt6*.dll                   # Librerías Qt necesarias
├── platforms/                 # Plugins de plataforma Qt
├── styles/                    # Plugins de estilos Qt
└── ...                        # Otras dependencias Qt
```

La configuración ya no se guarda junto al ejecutable, sino en la ubicación estándar de Windows para datos de aplicación. 