@echo off
echo ========================================
echo  ASOCIACION COMPLETA CON SETUSERFTA
echo ========================================
echo.
echo Este script:
echo 1. Limpia COMPLETAMENTE el registro
echo 2. Registra el ProgID 
echo 3. Usa SetUserFTA para asociar
echo.
pause

REM ============================================
REM PASO 1: LIMPIEZA TOTAL DEL REGISTRO
REM ============================================
echo.
echo ========================================
echo  PASO 1: LIMPIANDO REGISTRO COMPLETAMENTE
echo ========================================

REM Limpiar UserChoice (lo mas importante)
echo Limpiando UserChoice...
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nk\UserChoice" /f >nul 2>&1
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nk" /f >nul 2>&1

REM Limpiar TODAS las asociaciones directas
echo Limpiando asociaciones directas...
reg delete "HKEY_CURRENT_USER\Software\Classes\.nk" /f >nul 2>&1
reg delete "HKEY_LOCAL_MACHINE\Software\Classes\.nk" /f >nul 2>&1

REM Limpiar TODOS los ProgIDs posibles
echo Limpiando ProgIDs antiguos...
reg delete "HKEY_CURRENT_USER\Software\Classes\NukeScript" /f >nul 2>&1
reg delete "HKEY_CURRENT_USER\Software\Classes\nuke_auto_file" /f >nul 2>&1
reg delete "HKEY_CURRENT_USER\Software\Classes\Foundry.Nuke.Script" /f >nul 2>&1
reg delete "HKEY_CURRENT_USER\Software\Classes\LGA.NukeScript" /f >nul 2>&1
reg delete "HKEY_CURRENT_USER\Software\Classes\LGA.NukeScript.1" /f >nul 2>&1

REM Limpiar Applications
echo Limpiando Applications...
reg delete "HKEY_CURRENT_USER\Software\Classes\Applications\LGA_OpenInNukeX.exe" /f >nul 2>&1
reg delete "HKEY_CURRENT_USER\Software\Classes\Applications\Nuke15.0.exe" /f >nul 2>&1

echo ✓ Limpieza total completada

REM Esperar un poco para que Windows procese
echo Esperando que Windows procese la limpieza...
timeout /t 2 >nul

REM ============================================
REM PASO 2: REGISTRAR PROGID LIMPIO
REM ============================================
echo.
echo ========================================
echo  PASO 2: REGISTRANDO PROGID LIMPIO
echo ========================================

REM Definir variables
set "PROGID=LGA.NukeScript.1"
set "EXEPATH=%~dp0..\release\deploy\LGA_OpenInNukeX.exe"
set "ICONPATH=%~dp0..\release\deploy\app_icon.ico"

REM Verificar que el ejecutable existe
if not exist "%EXEPATH%" (
    echo ❌ ERROR: No se encuentra el ejecutable en:
    echo %EXEPATH%
    echo.
    echo Asegurate de haber compilado el proyecto primero:
    echo 1. Ejecuta deploy.bat
    echo 2. Verifica que LGA_OpenInNukeX.exe este en release\deploy\
    pause
    exit /b 1
)

echo Registrando ProgID: %PROGID%
echo Ejecutable: %EXEPATH%

REM Registrar ProgID principal
reg add "HKEY_CURRENT_USER\Software\Classes\%PROGID%" /ve /t REG_SZ /d "Nuke Script File" /f >nul
reg add "HKEY_CURRENT_USER\Software\Classes\%PROGID%\shell\open\command" /ve /t REG_SZ /d "\"%EXEPATH%\" \"%%1\"" /f >nul

REM Registrar icono si existe
if exist "%ICONPATH%" (
    reg add "HKEY_CURRENT_USER\Software\Classes\%PROGID%\DefaultIcon" /ve /t REG_SZ /d "\"%ICONPATH%\",0" /f >nul
    echo ✓ ProgID registrado con icono
) else (
    echo ✓ ProgID registrado sin icono
)

REM Verificar que se registro correctamente
reg query "HKEY_CURRENT_USER\Software\Classes\%PROGID%\shell\open\command" /ve >nul 2>&1
if %errorlevel% == 0 (
    echo ✓ ProgID verificado correctamente
) else (
    echo ❌ ERROR: ProgID no se registro correctamente
    pause
    exit /b 1
)

REM ============================================
REM PASO 3: USAR SETUSERFTA
REM ============================================
echo.
echo ========================================
echo  PASO 3: USANDO SETUSERFTA
echo ========================================

set "SETUSERFTA=%~dp0..\release\deploy\SetUserFTA.exe"

REM Verificar que SetUserFTA existe
if not exist "%SETUSERFTA%" (
    echo ❌ ERROR: SetUserFTA.exe no encontrado en:
    echo %SETUSERFTA%
    echo.
    echo SOLUCION:
    echo 1. Descarga SetUserFTA.exe desde:
    echo    https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/
    echo 2. Copia SetUserFTA.exe a: release\deploy\
    pause
    exit /b 1
)

echo Ejecutando SetUserFTA...
echo Comando: "%SETUSERFTA%" .nk %PROGID%

"%SETUSERFTA%" .nk %PROGID%

if %errorlevel% == 0 (
    echo ✓ SetUserFTA ejecutado exitosamente
) else (
    echo ❌ ERROR: SetUserFTA fallo con codigo %errorlevel%
    echo.
    echo Posibles soluciones:
    echo 1. Ejecuta este script como administrador
    echo 2. Verifica que el antivirus no bloquee SetUserFTA
    echo 3. Reinicia Windows y vuelve a intentar
    pause
    exit /b 1
)

REM ============================================
REM PASO 4: VERIFICACION Y FINALIZACION
REM ============================================
echo.
echo ========================================
echo  PASO 4: VERIFICACION FINAL
echo ========================================

REM Verificar UserChoice
echo Verificando UserChoice...
reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nk\UserChoice" /v ProgId >nul 2>&1
if %errorlevel% == 0 (
    for /f "tokens=3" %%a in ('reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nk\UserChoice" /v ProgId 2^>nul ^| find "ProgId"') do (
        echo ✓ UserChoice ProgId: %%a
    )
) else (
    echo ❌ UserChoice no configurado
)

REM Notificar cambios al explorador
echo Notificando cambios al Explorador...
rundll32.exe shell32.dll,SHChangeNotify 0x08000000,0x0000,0,0

echo.
echo ========================================
echo  PROCESO COMPLETADO
echo ========================================
echo.
echo La asociacion deberia estar funcionando ahora.
echo.
echo PASOS SIGUIENTES:
echo 1. Cierra TODAS las ventanas del Explorador
echo 2. Abre un nuevo Explorador
echo 3. Busca un archivo .nk y haz doble click
echo.
echo Si todavia no funciona:
echo 1. Reinicia el Explorador: Ctrl+Shift+Esc -> Explorador -> Reiniciar
echo 2. Reinicia Windows completamente
echo.
pause 