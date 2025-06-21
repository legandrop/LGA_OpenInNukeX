# LGA_OpenInNukeX v1.51

Cliente Qt/C++ para abrir archivos .nk en NukeX, reemplazando el ejecutable Python para evitar falsos positivos de antivirus.

## Características

- **Sin ventana de consola**: Ejecutable configurado como aplicación Windows pura
- **Asociación de archivos profesional**: Botón Apply con actualización inmediata del explorador usando SHChangeNotify
- **Configuración en AppData**: Archivos de configuración y logs se guardan en `%AppData%\LGA\LGA_OpenInNukeX_Qt`
- **Conexión TCP**: Conecta a instancia de NukeX en puerto 54325 con timeout de 10 segundos
- **Fallback automático**: Si no hay conexión TCP, lanza nueva instancia de NukeX
- **Interfaz de configuración moderna**: Ventana con botones Apply y Save, fondo #161616
- **Sistema de logging**: Logs detallados guardados automáticamente en AppData
- **Instalador portable**: Scripts de instalación/desinstalación incluidos
- **Estructura organizada**: Código fuente en `src/`, recursos en `resources/`, scripts en `scripts/`
- **Actualización visual inmediata**: Cambios de iconos y asociaciones visibles al instante en el explorador

## Estructura del Proyecto

```
QtClient/
├── src/                           # Código fuente
│   ├── main.cpp                   # Punto de entrada principal
│   ├── nukeopener.h/cpp          # Lógica de conexión TCP y NukeX
│   ├── configwindow.h/cpp        # Ventana de configuración con botón Apply mejorado
│   └── logger.h/cpp              # Sistema de logging
├── resources/                     # Recursos de la aplicación
│   ├── LGA_NukeShortcuts.ico     # Icono principal de la aplicación
│   ├── LGA_NukeShortcuts.png     # Icono principal en formato PNG
│   ├── app_icon.ico              # Icono para archivos .nk asociados
│   ├── app_icon.png              # Icono para archivos .nk en formato PNG
│   └── app.rc                    # Archivo de recursos Windows
├── scripts/                       # Scripts de compilación e instalación
│   ├── compilar.bat              # Compilación para desarrollo
│   ├── deploy.bat                # Compilación y deploy para producción
│   ├── limpiar.bat               # Limpieza de archivos de compilación
│   ├── test_debug.bat            # Script de testing
│   └── instalador.bat            # Generador de instalador .exe (Inno Setup)
├── build/                         # Directorio de compilación (generado)
├── release/                       # Directorio de release (generado)
├── CMakeLists.txt                # Configuración CMake con librerías Windows
└── README_Qt.md                  # Esta documentación
```

## Uso

### Con archivo de script
```bash
LGA_OpenInNukeX.exe "ruta/al/archivo.nk"
```

### Sin argumentos (configuración)
```bash
LGA_OpenInNukeX.exe
```
Abre ventana de configuración con:
- Texto descriptivo: "Use this app to open .nk files in your preferred NukeX version"
- **Botón APPLY**: Asocia automáticamente archivos .nk con actualización inmediata del explorador
- Campo de ruta para seleccionar NukeX.exe
- **Botón SAVE**: Guarda la configuración de ruta de NukeX

## Instalación

### Generar Instalador .exe
```bash
cd QtClient/scripts
instalador.bat
```

**Características del instalador generado:**
- **Instalador .exe profesional** usando Inno Setup
- Instala en `C:\Portable\LGA\OpenInNukeX` por defecto
- **Descarga automática** de Inno Setup si no está instalado
- **Icono personalizado** (LGA_NukeShortcuts) para la aplicación
- **Asociación de archivos** .nk con icono específico (app_icon)
- **Acceso directo activado por defecto** en escritorio y menú inicio
- Opciones durante instalación:
  - Crear accesos directos (activado por defecto)
  - Asociar archivos .nk automáticamente
- **Desinstalador integrado** en Panel de Control
- **Sin falsos positivos** de antivirus
- **Aplicación totalmente portable** - no requiere PATH del sistema

### Instalador generado: `LGA_OpenInNukeX_v1.51_Setup.exe`
- Tamaño compacto con compresión LZMA
- Interfaz moderna en español/inglés
- Verificación de integridad de archivos
- Limpieza completa al desinstalar

## Compilación

### Limpieza de archivos
```bash
cd scripts
limpiar.bat
```

### Desarrollo (Debug)
```bash
cd scripts
compilar.bat
```

### Producción (Release + Deploy)
```bash
cd scripts
deploy.bat
```

### Instalación completa
```bash
cd QtClient/scripts
deploy.bat
instalador.bat
```

## Asociación de Archivos

### Método automático (recomendado)
1. Ejecuta `LGA_OpenInNukeX.exe` sin argumentos
2. Haz clic en el botón **APPLY**
3. Confirma la asociación en el diálogo
4. **Los cambios son inmediatos** - verás el "flash" visual en el explorador
5. Los archivos .nk ahora muestran el icono personalizado y se abren automáticamente

### Implementación técnica mejorada
La aplicación ahora usa las mejores prácticas de Windows para asociaciones:
- **Registros múltiples**: ProgID, extensión, OpenWithProgids, Applications
- **SHChangeNotify**: Actualización inmediata del explorador sin reiniciar
- **Iconos personalizados**: Integración completa con el sistema de archivos
- **UserChoice**: Configuración como aplicación por defecto para .nk

## Requisitos

- Qt 6.8.2 con MinGW 13.1.0
- CMake 3.16+
- Windows 10/11
- Permisos de administrador (solo para instalación y asociaciones de archivos)

## Archivos Principales

- `src/main.cpp`: Punto de entrada, configuración QStandardPaths
- `src/nukeopener.h/cpp`: Lógica de conexión TCP y lanzamiento de NukeX
- `src/configwindow.h/cpp`: Ventana con botones Apply/Save, asociación mejorada
- `src/logger.h/cpp`: Sistema de logging en AppData
- `scripts/instalador.bat`: Generador de instalador .exe con Inno Setup
- `installer.iss`: Script de Inno Setup para el instalador profesional (sin PATH)
- `resources/app.rc`: Configuración de iconos y versión de la aplicación

## Configuración y Datos

### Ubicación de archivos
- **Instalación**: `C:\Portable\LGA\OpenInNukeX\`
- **Configuración**: `%AppData%\LGA\LGA_OpenInNukeX_Qt\nukeXpath.txt`
- **Logs**: `%AppData%\LGA\LGA_OpenInNukeX_Qt\LGA_OpenInNukeX_Qt.log`
- **Acceso directo**: `%USERPROFILE%\Desktop\LGA OpenInNukeX.lnk` (por defecto)

### Registro de Windows (asociaciones mejoradas)
- **Extensión**: `HKEY_CLASSES_ROOT\.nk`
- **Programa**: `HKEY_CLASSES_ROOT\LGA.NukeScript`
- **OpenWithProgids**: Para integración "Abrir con..."
- **Applications**: Para registro de aplicación
- **UserChoice**: Para configuración por defecto
- **Comando**: `shell/open/command`
- **Icono**: `DefaultIcon` (si existe app_icon.ico)

### Estructura de Despliegue

```
C:\Portable\LGA\OpenInNukeX\
├── LGA_OpenInNukeX.exe           # Ejecutable principal
├── Qt6*.dll                      # Librerías Qt necesarias
├── platforms/                    # Plugins de plataforma Qt
├── styles/                       # Plugins de estilos Qt
├── app_icon.ico                  # Icono para asociaciones
└── ...                           # Otras dependencias Qt
```

## Configuración de Iconos

### Iconos de la aplicación
- **LGA_NukeShortcuts.ico/.png**: Icono principal que aparece en:
  - Ejecutable de la aplicación
  - Ventana de configuración
  - Accesos directos
  - Lista de programas del sistema

### Iconos de archivos asociados
- **app_icon.ico/.png**: Icono que aparece en:
  - Archivos .nk en el explorador de Windows
  - Lista "Abrir con..." del menú contextual
  - Asociaciones de archivos del sistema

## Características de Seguridad

- **Sin falsos positivos**: Cliente nativo Qt en lugar de Python
- **Instalador profesional**: Uso de Inno Setup, herramienta estándar reconocida
- **Permisos mínimos**: Solo requiere administrador para instalación/asociaciones
- **Limpieza completa**: Desinstalador integrado remueve todas las trazas del sistema
- **Verificación de integridad**: El instalador valida archivos antes de proceder
- **Distribución segura**: Un solo archivo .exe firmado digitalmente (opcional)
- **Aplicación portable**: No modifica PATH del sistema ni variables de entorno 