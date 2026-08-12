@echo off
chcp 65001 >nul
setlocal EnableExtensions EnableDelayedExpansion

for %%I in ("%~dp0.") do set "QTCLIENT_DIR=%%~fI"
set "DEPLOY_DIR=%QTCLIENT_DIR%\release\deploy"
set "ISS_FILE=%QTCLIENT_DIR%\installer.iss"
set "OUTPUT_DIR=%QTCLIENT_DIR%\installer_output"

echo ================================================================
echo           LGA OpenInNukeX - Generador de Instalador
echo ================================================================
echo.

echo [INFO] Generando un deploy Release actualizado...
call "%QTCLIENT_DIR%\deploy.bat" --no-run
if errorlevel 1 (
    echo [ERROR] No se pudo generar el deploy Release.
    exit /b 1
)
echo [OK] Deploy Release actualizado.
echo.

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

    REM LGA_SKIP_INSTALLER_RUN_PROMPT lo setean los dos flujos que invocan este
    REM .bat como un paso intermedio: el generador de release del repo privado
    REM _LGA_ReleaseGen-OpenInNukeX.bat y github_release_win.bat. Los dos publican
    REM por su cuenta, asi que las dos preguntas de abajo sobran; y en el caso del
    REM segundo, preguntar seria ademas un bucle.
    REM
    REM Estos comentarios van sin parentesis a proposito: un REM adentro de un
    REM bloque sigue siendo texto que cmd parsea, y un parentesis de cierre
    REM suelto le cierra el bloque antes de tiempo.
    if /i "%LGA_SKIP_INSTALLER_RUN_PROMPT%"=="1" (
        echo [INFO] Se omiten las preguntas finales: el instalador fue invocado como paso de un release.
    ) else (
        set /p "TEST_INSTALLER=Queres ejecutar el instalador ahora? (y/n): "
        if /i "!TEST_INSTALLER!"=="y" (
            start "" "%INSTALLER_FILE%"
        )

        echo.
        REM --use-existing-installer: el Setup.exe que se acaba de generar es
        REM exactamente el que se quiere publicar. Sin el flag el publicador
        REM volveria a preguntar si reusarlo y, por defecto, lo regeneraria.
        set /p "PUBLISH_RELEASE=Queres subir el release a GitHub? (y/n): "
        if /i "!PUBLISH_RELEASE!"=="y" (
            call "%QTCLIENT_DIR%\github_release_win.bat" --use-existing-installer
            if errorlevel 1 (
                echo.
                echo [ERROR] La publicacion en GitHub fallo. El instalador local quedo generado igual.
            )
        )
    )
) else (
    echo [ERROR] El instalador no se genero correctamente.
    echo Verifica los logs de Inno Setup para mas detalles.
    exit /b 1
)

exit /b 0
