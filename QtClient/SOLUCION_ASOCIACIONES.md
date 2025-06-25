# Solución de Problemas - Asociaciones de Archivos .nk

## Problema: Los archivos .nk no se abren con LGA_OpenInNukeX

Si después de usar el botón **APPLY** los archivos .nk siguen sin abrirse correctamente, sigue estos pasos:

### 1. Verificar SetUserFTA.exe

- Asegúrate de que `SetUserFTA.exe` esté en la misma carpeta que `LGA_OpenInNukeX.exe`
- Si no está, descárgalo desde: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/
- Colócalo en la carpeta `release/deploy/`

### 2. Limpiar asociaciones previas

Ejecuta el script de limpieza:
```
scripts\limpiar_registro.bat
```

Este script elimina todas las asociaciones previas de archivos .nk que puedan estar causando conflictos.

### 3. Ejecutar como administrador

1. Cierra LGA_OpenInNukeX si está abierto
2. Haz clic derecho en `LGA_OpenInNukeX.exe`
3. Selecciona "Ejecutar como administrador"
4. Usa el botón **APPLY** nuevamente

### 4. Reiniciar el Explorador de Windows

Si aún no funciona:
1. Presiona `Ctrl + Shift + Esc` para abrir el Administrador de tareas
2. Busca "Explorador de Windows" o "Windows Explorer"
3. Haz clic derecho y selecciona "Reiniciar"

### 5. Verificación manual

Para verificar si la asociación está funcionando:
1. Haz clic derecho en un archivo .nk
2. Selecciona "Abrir con" > "Elegir otra aplicación"
3. Busca "LGA_OpenInNukeX" en la lista
4. Marca "Usar siempre esta aplicación para abrir archivos .nk"

### 6. Solución de último recurso

Si nada funciona:
1. Reinicia completamente Windows
2. Ejecuta `limpiar_registro.bat`
3. Ejecuta `LGA_OpenInNukeX.exe` como administrador
4. Usa el botón **APPLY**

## ¿Por qué es necesario SetUserFTA?

Windows 10/11 tiene protecciones que detectan cuando una aplicación intenta cambiar asociaciones de archivos sin pasar por el sistema oficial. Esto se llama "UserChoice Protection".

SetUserFTA es la única herramienta que puede establecer asociaciones correctamente generando el hash de validación que Windows espera, evitando que detecte la asociación como "hijacking".

## Archivos importantes

- `LGA_OpenInNukeX.exe` - Aplicación principal
- `SetUserFTA.exe` - Herramienta para asociaciones (REQUERIDA)
- `app_icon.ico` - Icono de la aplicación
- `scripts\limpiar_registro.bat` - Script de limpieza

## Logs de depuración

Los logs se guardan en:
```
%APPDATA%\LGA\OpenInNukeX\OpenInNukeX.log
```

Revisa estos archivos si necesitas más información sobre errores. 