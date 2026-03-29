@echo off
chcp 65001 >nul
setlocal EnableExtensions EnableDelayedExpansion

for %%I in ("%~dp0.") do set "QTCLIENT_DIR=%%~fI"
set "DEPLOY_DIR=%QTCLIENT_DIR%\release\deploy"
set "ISS_FILE=%QTCLIENT_DIR%\installer.iss"
set "OUTPUT_DIR=%QTCLIENT_DIR%\installer_output"
set "QT_DIR=C:\Qt\6.5.3\mingw_64"
set "MINGW_DIR=C:\Qt\Tools\mingw1120_64"

echo ================================================================
echo           LGA OpenInNukeX - Generador de Instalador
echo ================================================================
echo.

if not exist "%DEPLOY_DIR%" (
    echo [ERROR] La carpeta release\deploy no existe.
    echo Ejecuta primero deploy.bat para generar los archivos de distribucion.
    echo.
    exit /b 1
)

if not exist "%DEPLOY_DIR%\LGA_OpenInNukeX.exe" (
    echo [ERROR] No se encontro LGA_OpenInNukeX.exe en release\deploy\
    echo Ejecuta primero deploy.bat para generar los archivos de distribucion.
    echo.
    exit /b 1
)

set "INNO_PATH=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%INNO_PATH%" (
    echo [INFO] Inno Setup no encontrado. Descargando e instalando...

    if not exist "%QTCLIENT_DIR%\temp_inno" mkdir "%QTCLIENT_DIR%\temp_inno"
    pushd "%QTCLIENT_DIR%\temp_inno" >nul

    echo [INFO] Descargando Inno Setup 6...
    powershell -Command "& {Invoke-WebRequest -Uri 'https://jrsoftware.org/download.php/is.exe' -OutFile 'innosetup.exe'}"

    if not exist "innosetup.exe" (
        echo [ERROR] No se pudo descargar Inno Setup.
        echo Descarga manualmente desde: https://jrsoftware.org/isdl.php
        popd >nul
        rmdir /s /q "%QTCLIENT_DIR%\temp_inno"
        exit /b 1
    )

    echo [INFO] Instalando Inno Setup...
    start /wait innosetup.exe /SILENT /NORESTART

    popd >nul
    rmdir /s /q "%QTCLIENT_DIR%\temp_inno"

    if not exist "%INNO_PATH%" (
        echo [ERROR] Inno Setup no se instalo correctamente.
        echo Instala manualmente desde: https://jrsoftware.org/isdl.php
        exit /b 1
    )

    echo [OK] Inno Setup instalado correctamente.
    echo.
)

if not exist "%ISS_FILE%" (
    echo [ERROR] No se encontro el script installer.iss
    echo Asegurate de que el archivo installer.iss existe en el directorio QtClient.
    exit /b 1
)

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo [INFO] Compilando instalador con Inno Setup...
echo [INFO] Script: %ISS_FILE%
echo [INFO] Compilador: %INNO_PATH%
echo.

pushd "%QTCLIENT_DIR%" >nul
"%INNO_PATH%" "%ISS_FILE%"
set "INSTALL_EXIT=!errorlevel!"
popd >nul

if not "!INSTALL_EXIT!"=="0" (
    echo [ERROR] Error al compilar el instalador.
    echo Verifica el script installer.iss y los archivos de origen.
    exit /b 1
)

set "INSTALLER_FILE=%OUTPUT_DIR%\LGA_OpenInNukeX_Setup.exe"
if exist "%INSTALLER_FILE%" (
    echo.
    echo ================================================================
    echo                    INSTALADOR GENERADO EXITOSAMENTE
    echo ================================================================
    echo.
    echo Ubicacion: %INSTALLER_FILE%
    echo Tamano:
    for %%A in ("%INSTALLER_FILE%") do echo %%~zA bytes
    echo.
    echo El instalador incluye:
    echo - Aplicacion LGA OpenInNukeX
    echo - Todas las dependencias Qt necesarias
    echo - Icono LGA_NukeShortcuts para la aplicacion
    echo - Icono app_icon para archivos .nk asociados
    echo - Opcion para asociar archivos .nk
    echo - Accesos directos en escritorio y menu inicio
    echo - Aplicacion portable
    echo - Desinstalador completo incluido
    echo.

    if /i "%LGA_SKIP_INSTALLER_RUN_PROMPT%"=="1" (
        echo [INFO] Se omite la ejecucion interactiva del instalador porque el release lo invoco en modo no interactivo.
    ) else (
        set /p "TEST_INSTALLER=Queres ejecutar el instalador ahora? (y/n): "
        if /i "!TEST_INSTALLER!"=="y" (
            start "" "%INSTALLER_FILE%"
        )
    )
) else (
    echo [ERROR] El instalador no se genero correctamente.
    echo Verifica los logs de Inno Setup para mas detalles.
    exit /b 1
)

exit /b 0
