@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

echo ================================================================
echo           LGA OpenInNukeX v1.51 - Generador de Instalador
echo ================================================================
echo.

REM Verificar si ya existe la carpeta deploy
if not exist "..\release\deploy" (
    echo [ERROR] La carpeta release\deploy no existe. 
    echo Ejecuta primero deploy.bat para generar los archivos de distribución.
    echo.
    exit /b 1
)

REM Verificar si el ejecutable principal existe
if not exist "..\release\deploy\LGA_OpenInNukeX.exe" (
    echo [ERROR] No se encontró LGA_OpenInNukeX.exe en release\deploy\
    echo Ejecuta primero deploy.bat para generar los archivos de distribución.
    echo.
    exit /b 1
)

REM Verificar si Inno Setup está instalado
set "INNO_PATH=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%INNO_PATH%" (
    echo [INFO] Inno Setup no encontrado. Descargando e instalando...

    REM Crear directorio temporal
    if not exist temp_inno mkdir temp_inno
    cd temp_inno

    REM Descargar Inno Setup
    echo [INFO] Descargando Inno Setup 6...
    powershell -Command "& {Invoke-WebRequest -Uri 'https://jrsoftware.org/download.php/is.exe' -OutFile 'innosetup.exe'}"

    if not exist "innosetup.exe" (
        echo [ERROR] No se pudo descargar Inno Setup.
        echo Descarga manualmente desde: https://jrsoftware.org/isdl.php
        cd ..
        rmdir /s /q temp_inno
        exit /b 1
    )

    echo [INFO] Instalando Inno Setup...
    start /wait innosetup.exe /SILENT /NORESTART

    REM Limpiar archivos temporales
    cd ..
    rmdir /s /q temp_inno

    REM Verificar instalación
    if not exist "%INNO_PATH%" (
        echo [ERROR] Inno Setup no se instaló correctamente.
        echo Instala manualmente desde: https://jrsoftware.org/isdl.php
        exit /b 1
    )

    echo [OK] Inno Setup instalado correctamente.
    echo.
)

REM Verificar que existe el script .iss
if not exist "..\installer.iss" (
    echo [ERROR] No se encontró el script installer.iss
    echo Asegúrate de que el archivo installer.iss existe en el directorio QtClient.
    exit /b 1
)

REM Crear directorio de salida si no existe
if not exist "..\installer_output" mkdir "..\installer_output"

echo [INFO] Compilando instalador con Inno Setup...
echo [INFO] Script: installer.iss
echo [INFO] Compilador: %INNO_PATH%
echo.

REM Compilar el instalador
"%INNO_PATH%" "..\installer.iss"

if !errorLevel! neq 0 (
    echo [ERROR] Error al compilar el instalador.
    echo Verifica el script installer.iss y los archivos de origen.
    exit /b 1
)

REM Verificar que se generó el instalador
set "INSTALLER_FILE=..\installer_output\LGA_OpenInNukeX_Setup.exe"
if exist "%INSTALLER_FILE%" (
    echo.
    echo ================================================================
    echo                    INSTALADOR GENERADO EXITOSAMENTE
    echo ================================================================
    echo.
    echo Ubicación: %INSTALLER_FILE%
    echo Tamaño: 
    for %%A in ("%INSTALLER_FILE%") do echo %%~zA bytes
    echo.
    echo El instalador incluye:
    echo ✓ Aplicación LGA OpenInNukeX v1.51
    echo ✓ Todas las dependencias Qt necesarias
    echo ✓ Icono LGA_NukeShortcuts para la aplicación
    echo ✓ Icono app_icon para archivos .nk asociados
    echo ✓ Opción para asociar archivos .nk
    echo ✓ Accesos directos en escritorio y menú inicio (activado por defecto)
    echo ✓ Aplicación completamente portable
    echo ✓ Desinstalador completo incluido
    echo.
    
    set /p "TEST_INSTALLER=¿Queres ejecutar el instalador ahora? (y/n): "
    if /i "!TEST_INSTALLER!"=="y" (
        start "" "%INSTALLER_FILE%"
    )
) else (
    echo [ERROR] El instalador no se generó correctamente.
    echo Verifica los logs de Inno Setup para más detalles.
)


exit /b 0 