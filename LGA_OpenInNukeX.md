# LGA_OpenInNukeX v1.52

**Una aplicación portable en Qt/C++ para abrir archivos .nk directamente en NukeX desde el explorador de archivos de Windows.**

## ✨ Características Principales

- **🚀 Aplicación portable**: No requiere instalación, funciona desde cualquier ubicación
- **🔗 Asociación de archivos**: Establece LGA_OpenInNukeX como aplicación predeterminada para archivos .nk
- **⚡ Apertura directa**: Doble clic en archivos .nk los abre automáticamente en NukeX
- **🛠️ Configuración visual**: Interfaz gráfica para configurar la ruta de NukeX
- **📝 Logging detallado**: Registro completo de operaciones para debugging
- **🔒 Seguro**: Evita falsos positivos de antivirus (reemplaza ejecutable Python)

## 🆕 Asociación de Archivos Robusta

### ✅ Método Híbrido Implementado

La aplicación utiliza un **método híbrido robusto** que combina múltiples técnicas para garantizar que las asociaciones de archivos .nk funcionen correctamente:

**Implementación técnica**:
- **Limpieza inteligente del registro**: Elimina asociaciones conflictivas previas usando comandos `reg delete`
- **Registro de ProgID**: Crea el identificador `LGA.NukeScript.1` con comando de apertura y icono
- **SetUserFTA como preferencia**: Si está disponible, utiliza SetUserFTA para asociaciones más robustas
- **Fallback a PowerShell/reg**: Si SetUserFTA no está disponible, usa comandos directos del registro
- **Notificación al sistema**: Actualiza el explorador de archivos automáticamente

**Archivos clave**:
- `QtClient/src/configwindow.cpp`: Funciones `applyFileAssociation()`, `cleanRegistry()`, `registerProgId()`, `setFileAssociation()`
- `QtClient/src/logger.cpp`: Sistema de logging detallado para debugging

### 🔧 SetUserFTA Integration (Opcional)

SetUserFTA es una herramienta opcional desarrollada por Christoph Kolbicz que mejora la robustez de las asociaciones:
- **Implementa el algoritmo hash correcto** para UserChoice en Windows 10/11
- **Evita la detección de "hijacking"** por parte de Windows
- **Funciona sin permisos de administrador**

**Integración automática**: Si `SetUserFTA.exe` está presente en el directorio de la aplicación, se usa automáticamente. Si no está disponible, la aplicación funciona perfectamente con el método de fallback.

## 📋 Requisitos del Sistema

- **Sistema Operativo**: Windows 10/11 (x64)
- **NukeX**: Cualquier versión instalada
- **Permisos**: Usuario estándar (no requiere administrador)
- **Dependencias**: Ninguna (SetUserFTA.exe es opcional)

## 🚀 Instalación y Uso

### Opción 1: Usar el Instalador (Recomendado)

1. **Descargar** el instalador desde las releases
2. **Ejecutar** `LGA_OpenInNukeX_Installer.exe`
3. **Seguir** el asistente de instalación
4. **Configurar** la ruta de NukeX en la primera ejecución

### Opción 2: Uso Portable

1. **Descargar** el archivo portable desde las releases
2. **Extraer** en cualquier ubicación
3. **Ejecutar** `LGA_OpenInNukeX.exe`
4. **Configurar** la ruta de NukeX
5. **Aplicar** asociación de archivos

## ⚙️ Configuración

### Primera Ejecución

1. La aplicación abrirá automáticamente la ventana de configuración con una **interfaz moderna y oscura**
2. **Sección File Association**:
   - Texto descriptivo sobre la funcionalidad
   - Botón "APPLY" para establecer la asociación de archivos .nk
   
3. **Sección Preferred Nuke Version**:
   - Campo de entrada para la ruta de NukeX
   - Botón "BROWSE" para seleccionar el ejecutable
   - Botón "SAVE" para guardar la configuración

### Interfaz de Usuario

La aplicación ahora cuenta con:
- **Tema oscuro moderno** con tipografía Inter
- **Diseño responsive** con scroll automático
- **Dos secciones principales** organizadas en tarjetas
- **Colores consistentes** con el ecosistema de aplicaciones LGA

### Verificación

Después de la configuración:
- Los archivos `.nk` mostrarán el icono de LGA_OpenInNukeX
- Doble clic en un `.nk` abrirá NukeX automáticamente
- La asociación funciona de forma confiable en Windows 10/11

## 🔧 Desarrollo y Compilación

### Estructura del Proyecto

```
LGA_OpenInNukeX/
├── QtClient/                 # Aplicación Qt/C++
│   ├── src/                 # Código fuente
│   │   ├── main.cpp         # Punto de entrada
│   │   ├── nukeopener.cpp   # Lógica principal
│   │   ├── configwindow.cpp # Ventana de configuración moderna
│   │   └── logger.cpp       # Sistema de logging
│   ├── resources/           # Recursos (iconos)
│   ├── dark_theme.qss       # Tema oscuro moderno
│   ├── scripts/            # Scripts de build
│   │   ├── compilar.bat    # Compilación
│   │   ├── deploy.bat      # Deploy portable (incluye QSS)
│   │   └── instalador.bat  # Crear instalador
│   └── CMakeLists.txt      # Configuración CMake
├── Developement/           # Versión Python original
└── init.py                # Configuración inicial
```

### Compilar desde Código Fuente

#### Requisitos de Desarrollo

- **Qt 6.8.2** con MinGW 13.1.0
- **CMake 3.25+**
- **Git** para clonar el repositorio

#### Pasos de Compilación

```bash
# 1. Clonar repositorio
git clone <repository-url>
cd LGA_OpenInNukeX/QtClient

# 2. Compilar aplicación
cd scripts
deploy.bat

# 3. Crear instalador (opcional)
instalador.bat
```

### Scripts de Build

- **`deploy.bat`**: Compila la aplicación en modo Release + crea paquete portable
- **`instalador.bat`**: Genera instalador con Inno Setup
- **`limpiar.bat`**: Limpia archivos de compilación

## 📁 Archivos Importantes

### Ejecutables
- **`LGA_OpenInNukeX.exe`**: Aplicación principal
- **`SetUserFTA.exe`**: Herramienta opcional para asociaciones más robustas

### Configuración
- **`nukeXpath.txt`**: Ruta de NukeX configurada
- **`app_icon.ico`**: Icono para asociaciones de archivos
- **`dark_theme.qss`**: Archivo de estilo para la interfaz moderna

### Logs
- **`logs/LGA_OpenInNukeX_YYYY-MM-DD.log`**: Logs detallados de operaciones

## 🔍 Resolución de Problemas

### Problema: "Asociación no funciona"

**Causas posibles**: 
- Conflictos en el registro de asociaciones previas
- Permisos insuficientes para modificar el registro
- Ruta de NukeX incorrecta

**Solución**:
1. **Re-aplicar la asociación**: Usar el botón "APPLY" en la ventana de configuración (esto limpia conflictos automáticamente)
2. **Verificar logs**: Revisar `logs/LGA_OpenInNukeX_YYYY-MM-DD.log` para errores específicos
3. **Verificar ruta de NukeX**: Asegurar que la ruta configurada sea correcta y el archivo exista
4. **Reiniciar Explorer**: Si persiste, ejecutar `taskkill /f /im explorer.exe ; start explorer.exe`

### Problema: "SetUserFTA.exe no encontrado" (Mensaje informativo)

**Explicación**: Este es un mensaje informativo, no un error. La aplicación funciona perfectamente sin SetUserFTA.

**Para usar SetUserFTA (opcional)**:
1. Descargar SetUserFTA.exe desde: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/
2. Colocar `SetUserFTA.exe` en el mismo directorio que `LGA_OpenInNukeX.exe`
3. Re-aplicar la asociación usando el botón "APPLY"

### Problema: "Error al abrir NukeX"

**Causa**: Ruta de NukeX incorrecta o archivo no encontrado.

**Solución**:
1. Abrir ventana de configuración
2. Verificar/actualizar ruta de NukeX
3. Guardar configuración

## 📚 Documentación Técnica

### Algoritmo de Asociación de Archivos

La función `applyFileAssociation()` en `QtClient/src/configwindow.cpp` ejecuta el siguiente proceso:

1. **Limpieza del registro** (`cleanRegistry()`): Elimina asociaciones conflictivas previas
2. **Registro de ProgID** (`registerProgId()`): Crea `LGA.NukeScript.1` con comando de apertura e icono
3. **Configuración de asociación** (`setFileAssociation()`): 
   - Prioridad 1: SetUserFTA (si está disponible)
   - Fallback: PowerShell o comandos `reg` directos
4. **Notificación al sistema**: Llama `SHChangeNotify()` para actualizar explorador
5. **Logging detallado**: Registra cada paso para debugging en `logger.cpp`

### Logging System

Los logs se guardan en `logs/` con formato:
```
[YYYY-MM-DD HH:MM:SS] LEVEL: Mensaje
```

Niveles: `INFO`, `WARNING`, `ERROR`

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Por favor:

1. Fork el repositorio
2. Crear feature branch (`git checkout -b feature/nueva-funcionalidad`)
3. Commit cambios (`git commit -am 'Agregar nueva funcionalidad'`)
4. Push branch (`git push origin feature/nueva-funcionalidad`)
5. Crear Pull Request

## 📄 Licencia

Este proyecto está bajo licencia MIT. Ver archivo `LICENSE` para detalles.

## 🙏 Agradecimientos

- **Christoph Kolbicz** por SetUserFTA - herramienta opcional que mejora la robustez de las asociaciones
- **Microsoft** por la documentación oficial sobre file associations
- **Qt Framework** por las herramientas de desarrollo multiplataforma

## 📞 Soporte

Para reportar bugs o solicitar funcionalidades:
1. Crear issue en GitHub
2. Incluir logs relevantes
3. Describir pasos para reproducir el problema

---
