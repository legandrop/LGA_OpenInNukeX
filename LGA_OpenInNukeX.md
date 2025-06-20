# LGA_OpenInNukeX v0.14

**Una aplicación portable en Qt/C++ para abrir archivos .nk directamente en NukeX desde el explorador de archivos de Windows.**

## ✨ Características Principales

- **🚀 Aplicación portable**: No requiere instalación, funciona desde cualquier ubicación
- **🔗 Asociación de archivos**: Establece LGA_OpenInNukeX como aplicación predeterminada para archivos .nk
- **⚡ Apertura directa**: Doble clic en archivos .nk los abre automáticamente en NukeX
- **🛠️ Configuración visual**: Interfaz gráfica para configurar la ruta de NukeX
- **📝 Logging detallado**: Registro completo de operaciones para debugging
- **🔒 Seguro**: Evita falsos positivos de antivirus (reemplaza ejecutable Python)

## 🆕 Nuevo en v0.14: Solución Definitiva para Asociaciones

### ⚠️ Problemas Resueltos: Windows 10/11 "App Default Reset" y "Comando Vacío"

**Problemas anteriores**: 
- Windows 10/11 detectaba automáticamente cuando las aplicaciones modificaban las asociaciones de archivos directamente en el registro y las **reseteaba como "hijacking"**, mostrando el mensaje "An app default was reset"
- Las asociaciones aparecían con **"comando vacío"** debido a conflictos en el registro UserChoice

**✅ Soluciones implementadas**: 
- **Integración completa con SetUserFTA** - la única herramienta que puede establecer asociaciones de archivos sin que Windows las detecte como modificaciones no autorizadas
- **Separación del registro**: Solo se registra el ProgID básico, SetUserFTA maneja completamente UserChoice con el hash correcto
- **Copia automática de iconos**: El script deploy busca iconos en múltiples ubicaciones para asegurar que se incluyan correctamente

### 🔧 SetUserFTA Integration

SetUserFTA es una herramienta desarrollada por Christoph Kolbicz que:
- **Implementa el algoritmo hash correcto** para UserChoice en Windows 10/11
- **Evita la detección de "hijacking"** por parte de Windows
- **Funciona sin permisos de administrador**
- **Es la solución recomendada por Microsoft IT Pros**

**Integración automática**: El script `deploy.bat` busca SetUserFTA.exe en `scripts/deploy/` y lo incluye automáticamente en el paquete final.

## 📋 Requisitos del Sistema

- **Sistema Operativo**: Windows 10/11 (x64)
- **NukeX**: Cualquier versión instalada
- **Permisos**: Usuario estándar (no requiere administrador)
- **Dependencias**: SetUserFTA.exe (descarga automática)

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

1. La aplicación abrirá automáticamente la ventana de configuración
2. **Configurar ruta de NukeX**:
   - Clic en "Examinar"
   - Seleccionar `Nuke15.1.exe` (o tu versión)
   - Clic en "Guardar Configuración"

3. **Establecer asociación de archivos**:
   - Clic en "Aplicar Asociación de Archivos"
   - La aplicación usará SetUserFTA para establecer la asociación correctamente
   - Confirmar en el mensaje de éxito

### Verificación

Después de la configuración:
- Los archivos `.nk` mostrarán el icono de LGA_OpenInNukeX
- Doble clic en un `.nk` abrirá NukeX automáticamente
- No aparecerán mensajes de "App default reset"

## 🔧 Desarrollo y Compilación

### Estructura del Proyecto

```
LGA_OpenInNukeX/
├── QtClient/                 # Aplicación Qt/C++
│   ├── src/                 # Código fuente
│   │   ├── main.cpp         # Punto de entrada
│   │   ├── nukeopener.cpp   # Lógica principal
│   │   ├── configwindow.cpp # Ventana de configuración
│   │   └── logger.cpp       # Sistema de logging
│   ├── resources/           # Recursos (iconos)
│   ├── scripts/            # Scripts de build
│   │   ├── compilar.bat    # Compilación
│   │   ├── deploy.bat      # Deploy + SetUserFTA
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

# 2. Descargar SetUserFTA.exe
# Visita: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/
# Coloca SetUserFTA.exe en QtClient/scripts/deploy/

# 3. Compilar y crear paquete deploy (incluye SetUserFTA)
cd scripts
deploy.bat

# 4. Crear instalador (opcional)
instalador.bat
```

### Scripts de Build

- **`deploy.bat`**: Compila la aplicación en modo Release + crea paquete portable con SetUserFTA
- **`instalador.bat`**: Genera instalador con Inno Setup
- **`limpiar.bat`**: Limpia archivos de compilación

## 📁 Archivos Importantes

### Ejecutables
- **`LGA_OpenInNukeX.exe`**: Aplicación principal
- **`SetUserFTA.exe`**: Herramienta para asociaciones (descarga automática)

### Configuración
- **`nukeXpath.txt`**: Ruta de NukeX configurada
- **`app_icon.ico`**: Icono para asociaciones de archivos

### Logs
- **`logs/LGA_OpenInNukeX_YYYY-MM-DD.log`**: Logs detallados de operaciones

## 🔍 Resolución de Problemas

### Problema: "SetUserFTA.exe no encontrado"

**Causa**: SetUserFTA.exe no está en el directorio de la aplicación.

**Solución**:
1. Descargar SetUserFTA.exe desde: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/
2. Colocar `SetUserFTA.exe` en `QtClient/scripts/deploy/`
3. Ejecutar `deploy.bat` para compilar y crear paquete completo

### Problema: "Asociación no funciona" o "Comando vacío"

**Causas posibles**: 
- SetUserFTA.exe no está presente
- Conflicto en el registro UserChoice
- Windows detectó la asociación como "hijacking"

**Solución**:
1. Verificar que SetUserFTA.exe esté en el mismo directorio que LGA_OpenInNukeX.exe
2. **IMPORTANTE**: Re-aplicar la asociación desde la ventana de configuración (esto limpia conflictos)
3. Verificar logs para errores específicos
4. Si persiste el problema, reiniciar Windows Explorer: `taskkill /f /im explorer.exe ; start explorer.exe`

### Problema: "Error al abrir NukeX"

**Causa**: Ruta de NukeX incorrecta o archivo no encontrado.

**Solución**:
1. Abrir ventana de configuración
2. Verificar/actualizar ruta de NukeX
3. Guardar configuración

## 📚 Documentación Técnica

### Algoritmo de Asociación de Archivos

1. **Registro básico**: Crea ProgID y comando en `HKEY_CURRENT_USER\Software\Classes`
2. **SetUserFTA**: Ejecuta `SetUserFTA.exe .nk LGA.NukeScript` para hash correcto
3. **Notificación**: Llama `SHChangeNotify()` para actualizar explorador
4. **Verificación**: Confirma que la asociación se estableció correctamente

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

- **Christoph Kolbicz** por SetUserFTA - la solución definitiva para asociaciones en Windows 10/11
- **Microsoft** por la documentación oficial sobre file associations
- **Qt Framework** por las herramientas de desarrollo multiplataforma

## 📞 Soporte

Para reportar bugs o solicitar funcionalidades:
1. Crear issue en GitHub
2. Incluir logs relevantes
3. Describir pasos para reproducir el problema

---

**LGA_OpenInNukeX v0.14** - Solución definitiva para asociaciones de archivos en Windows 10/11
