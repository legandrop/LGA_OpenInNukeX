@echo off
echo ========================================
echo  PRUEBA SIN PERMISOS DE ADMINISTRADOR
echo ========================================
echo.
echo Este script verifica que LGA_OpenInNukeX
echo funciona correctamente sin permisos elevados.
echo.
echo IMPORTANTE: NO ejecutes este script como administrador
echo.

REM Verificar que NO somos administrador
net session >nul 2>&1
if %errorlevel% == 0 (
    echo ❌ ERROR: Este script está ejecutándose como administrador
    echo.
    echo Para probar correctamente:
    echo 1. Cierra esta ventana
    echo 2. Ejecuta el script SIN "Ejecutar como administrador"
    echo.
    pause
    exit /b 1
)

echo ✓ Ejecutándose con permisos de usuario estándar (correcto)
echo.

REM Verificar que el ejecutable existe
set "EXEPATH=%~dp0..\release\deploy\LGA_OpenInNukeX.exe"
if not exist "%EXEPATH%" (
    echo ❌ ERROR: No se encuentra el ejecutable
    echo Esperado en: %EXEPATH%
    echo.
    echo Compila primero con deploy.bat
    pause
    exit /b 1
)

echo ✓ Ejecutable encontrado: %EXEPATH%
echo.

REM Mostrar ubicación donde se guardarán los logs
echo Ubicación de logs (AppData):
echo %APPDATA%\LGA\OpenInNukeX\
echo.

REM Verificar que podemos crear archivos en AppData
set "TESTFILE=%APPDATA%\LGA\OpenInNukeX\test_write.tmp"
mkdir "%APPDATA%\LGA\OpenInNukeX" >nul 2>&1
echo test > "%TESTFILE%" 2>nul
if exist "%TESTFILE%" (
    echo ✓ Permisos de escritura en AppData: OK
    del "%TESTFILE%" >nul 2>&1
) else (
    echo ❌ ERROR: No se puede escribir en AppData
    pause
    exit /b 1
)

echo.
echo ========================================
echo  EJECUTANDO APLICACIÓN
echo ========================================
echo.
echo Abriendo LGA_OpenInNukeX...
echo Los logs aparecerán en: %APPDATA%\LGA\OpenInNukeX\OpenInNukeX.log
echo.

REM Ejecutar la aplicación
start "" "%EXEPATH%"

echo ✓ Aplicación iniciada correctamente
echo.
echo Verifica que:
echo 1. La aplicación se abre sin solicitar permisos
echo 2. Se pueden hacer asociaciones de archivos
echo 3. Los logs se crean en AppData (no en Program Files)
echo.
pause 