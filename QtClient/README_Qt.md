# LGA_OpenInNukeX_Qt

Cliente Qt/C++ para abrir archivos .nk en NukeX, reemplazando el ejecutable Python para evitar falsos positivos de antivirus.

## Características

- **Conexión TCP**: Conecta a instancia de NukeX en puerto 54325 con timeout de 10 segundos
- **Fallback automático**: Si no hay conexión TCP, lanza nueva instancia de NukeX
- **Interfaz de configuración**: Cuando se ejecuta sin argumentos, muestra ventana para configurar ruta de NukeX
- **Diseño moderno**: Interfaz con fondo #161616 siguiendo estándares de diseño

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

## Requisitos

- Qt 6.8.2 con MinGW 13.1.0
- CMake 3.16+ (para compilación con CMake)
- Windows 10/11

## Archivos Principales

- `main.cpp`: Punto de entrada y manejo de argumentos
- `nukeopener.h/cpp`: Lógica de conexión TCP y lanzamiento de NukeX  
- `configwindow.h/cpp`: Ventana de configuración de ruta de NukeX
- `nukeXpath.txt`: Archivo que almacena la ruta configurada de NukeX

## Estructura de Despliegue

```
release/deploy/
├── LGA_OpenInNukeX_Qt.exe     # Ejecutable principal
├── nukeXpath.txt              # Configuración de ruta NukeX
├── Qt6*.dll                   # Librerías Qt necesarias
├── platforms/                 # Plugins de plataforma Qt
├── styles/                    # Plugins de estilos Qt
└── ...                        # Otras dependencias Qt
``` 