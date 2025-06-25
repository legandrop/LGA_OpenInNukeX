# Cambios para Eliminar Permisos de Administrador

## 📋 Resumen de Cambios

Este documento detalla todos los cambios realizados para que **LGA_OpenInNukeX** funcione completamente sin requerir permisos de administrador.

## 🔧 Cambios Técnicos Realizados

### 1. **Manifiesto de Aplicación** (`LGA_OpenInNukeX.exe.manifest`)
- **ANTES**: `level="requireAdministrator"`
- **DESPUÉS**: `level="asInvoker"`
- **EFECTO**: La aplicación ya no solicita UAC al ejecutarse

### 2. **Instalador** (`installer.iss`)
- **ANTES**: `PrivilegesRequired=admin` + `DefaultDirName={pf64}`
- **DESPUÉS**: `PrivilegesRequired=lowest` + `DefaultDirName={autopf}`
- **EFECTO**: El instalador se ejecuta con permisos mínimos e instala en AppData

### 3. **Sistema de Logging** (`src/logger.cpp`)
- **ANTES**: Logs en `logs/` dentro del directorio de instalación
- **DESPUÉS**: Logs en `%AppData%\LGA\OpenInNukeX\` con rotación por fecha
- **EFECTO**: No necesita permisos para escribir en Program Files

### 4. **Archivo de Recursos** (`LGA_OpenInNukeX.rc`)
- **ANTES**: Comentario "Manifest para solicitar permisos de administrador"
- **DESPUÉS**: "Manifest de la aplicación (sin permisos de administrador)"
- **EFECTO**: Clarifica que no se requieren permisos

## 📁 Nuevas Ubicaciones de Archivos

### Configuración y Logs (AppData)
```
%AppData%\LGA\OpenInNukeX\
├── nukeXpath.txt                           # Configuración de ruta de NukeX
└── OpenInNukeX.log                         # Logs (se borra al iniciar)
```

### Instalación (Sin permisos admin)
```
%LocalAppData%\Programs\LGA\OpenInNukeX\   # O ubicación personalizada
├── LGA_OpenInNukeX.exe
├── SetUserFTA.exe                          # Opcional
├── app_icon.ico
├── Qt6*.dll
└── ...
```

## 🔐 Registro de Windows

### Asociaciones de Archivos (Solo HKEY_CURRENT_USER)
- **Limpieza**: Elimina asociaciones conflictivas previas
- **ProgID**: Registra `LGA.NukeScript.1` en `HKCU\Software\Classes`
- **Asociación**: Usa SetUserFTA (sin admin) o PowerShell como fallback
- **Notificación**: Llama `SHChangeNotify()` para actualizar el Explorador

## ✅ Beneficios Obtenidos

1. **✓ Sin UAC**: La aplicación se ejecuta directamente sin solicitar permisos
2. **✓ Sin Admin en Instalación**: El instalador funciona con permisos de usuario estándar
3. **✓ Logs Seguros**: Se escriben en AppData, siempre accesible
4. **✓ Asociaciones Robustas**: SetUserFTA maneja asociaciones sin elevar permisos
5. **✓ Compatibilidad Total**: Funciona en cuentas de usuario limitadas
6. **✓ Sin Antivirus**: Reduce falsos positivos al no solicitar permisos elevados

## 🧪 Cómo Probar

1. **Ejecutar `scripts/test_sin_admin.bat`** (NO como administrador)
2. **Verificar** que la aplicación se abre sin UAC
3. **Comprobar logs** en `%AppData%\LGA\OpenInNukeX\OpenInNukeX.log`
4. **Probar asociación** de archivos .nk
5. **Confirmar** que todo funciona sin permisos elevados

## 📝 Documentación Actualizada

### Archivos Modificados:
- `LGA_OpenInNukeX.md` - Referencias a logs y permisos
- `QtClient/README_Qt.md` - Ubicaciones y características de seguridad
- `QtClient/SOLUCION_ASOCIACIONES.md` - Ubicación de logs
- `QtClient/scripts/limpiar_registro.bat` - Eliminada sugerencia de admin
- `QtClient/scripts/asociar_con_fta.bat` - Eliminada sugerencia de admin

### Archivos Nuevos:
- `QtClient/scripts/test_sin_admin.bat` - Script de prueba
- `QtClient/CAMBIOS_SIN_ADMIN.md` - Este documento

## 🚀 Resultado Final

**LGA_OpenInNukeX v1.52** ahora funciona completamente **sin permisos de administrador**:

- ✅ Aplicación ejecuta con permisos de usuario estándar
- ✅ Instalador no requiere UAC
- ✅ Logs se escriben en ubicación segura (AppData)
- ✅ Asociaciones de archivos funcionan con SetUserFTA
- ✅ Compatible con entornos corporativos restringidos
- ✅ Menos falsos positivos de antivirus

---

**Nota**: SetUserFTA es una herramienta de terceros que permite establecer asociaciones de archivos por usuario sin requerir permisos de administrador. Es la solución recomendada por Microsoft para este tipo de operaciones. 