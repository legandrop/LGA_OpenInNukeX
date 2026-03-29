@echo off
setlocal EnableExtensions

for %%I in ("%~dp0.") do set "QTCLIENT_DIR=%%~fI"
set "RELEASE_DIR=%QTCLIENT_DIR%\release"
set "DEPLOY_DIR=%RELEASE_DIR%\deploy"
set "QT_DIR=C:\Qt\6.5.3\mingw_64"
set "MINGW_DIR=C:\Qt\Tools\mingw1120_64"

echo ===============================================
echo    LGA_OpenInNukeX
echo ===============================================
echo.

if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [ERROR] No se encontro Qt en: %QT_DIR%
    exit /b 1
)

if not exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo [ERROR] No se encontro Qt6Config.cmake en: %QT_DIR%\lib\cmake\Qt6
    exit /b 1
)

if not exist "%MINGW_DIR%\bin\mingw32-make.exe" (
    echo [ERROR] No se encontro mingw32-make.exe en: %MINGW_DIR%
    exit /b 1
)

set "PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%"

echo Cerrando instancias previas de LGA_OpenInNukeX...
taskkill /f /im LGA_OpenInNukeX.exe >nul 2>&1
echo Instancias cerradas
echo.
echo =================================
echo    COMPILANDO PARA PRODUCCION
echo =================================

if exist "%QTCLIENT_DIR%\limpiar.bat" (
    call "%QTCLIENT_DIR%\limpiar.bat" >nul 2>&1
)

echo Configurando proyecto con CMake (Release)...
cmake -S "%QTCLIENT_DIR%" -B "%RELEASE_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%\lib\cmake"
if errorlevel 1 (
    echo Error en la configuracion de CMake
    exit /b 1
)

echo Compilando...
cmake --build "%RELEASE_DIR%" -j 4
if errorlevel 1 (
    echo Error en la compilacion
    exit /b 1
)

echo.
echo =================================
echo    CREANDO PAQUETE DE DEPLOY
echo =================================

if not exist "%DEPLOY_DIR%" (
    mkdir "%DEPLOY_DIR%"
    echo Directorio deploy creado
) else (
    echo Directorio deploy ya existe
)

echo.
echo Copiando ejecutable principal...
copy /Y "%RELEASE_DIR%\LGA_OpenInNukeX.exe" "%DEPLOY_DIR%\LGA_OpenInNukeX.exe" >nul
if exist "%DEPLOY_DIR%\LGA_OpenInNukeX.exe" (
    echo LGA_OpenInNukeX.exe copiado exitosamente
) else (
    echo ERROR: No se pudo copiar LGA_OpenInNukeX.exe
    exit /b 1
)

if exist "%QTCLIENT_DIR%\nukeXpath.txt" (
    copy /Y "%QTCLIENT_DIR%\nukeXpath.txt" "%DEPLOY_DIR%\nukeXpath.txt" >nul
    echo Archivo nukeXpath.txt copiado
)

echo.
echo Copiando iconos necesarios...
copy /Y "%QTCLIENT_DIR%\resources\app_icon.ico" "%DEPLOY_DIR%\app_icon.ico" >nul 2>&1
if not exist "%DEPLOY_DIR%\app_icon.ico" (
    copy /Y "%QTCLIENT_DIR%\scripts\deploy\app_icon.ico" "%DEPLOY_DIR%\app_icon.ico" >nul 2>&1
)
if exist "%DEPLOY_DIR%\app_icon.ico" (
    echo Icono app_icon.ico copiado exitosamente
) else (
    echo Warning: No se pudo copiar app_icon.ico desde ninguna ubicacion
)

echo.
echo Copiando archivo de tema...
copy /Y "%QTCLIENT_DIR%\dark_theme.qss" "%DEPLOY_DIR%\dark_theme.qss" >nul 2>&1
if exist "%DEPLOY_DIR%\dark_theme.qss" (
    echo Archivo dark_theme.qss copiado exitosamente
) else (
    echo Warning: No se pudo copiar dark_theme.qss
)

echo.
echo Desplegando dependencias de Qt...
pushd "%DEPLOY_DIR%" >nul
windeployqt.exe LGA_OpenInNukeX.exe
if errorlevel 1 (
    popd >nul
    echo Error ejecutando windeployqt
    exit /b 1
)

echo.
echo Verificando SetUserFTA.exe (requerido para asociaciones de archivos)...
if exist "SetUserFTA.exe" (
    echo SetUserFTA.exe ya existe en deploy\
) else (
    if exist "%QTCLIENT_DIR%\scripts\deploy\SetUserFTA.exe" (
        echo SetUserFTA.exe encontrado en scripts\deploy\, copiando...
        copy /Y "%QTCLIENT_DIR%\scripts\deploy\SetUserFTA.exe" "SetUserFTA.exe" >nul
        if exist "SetUserFTA.exe" (
            echo SetUserFTA.exe copiado exitosamente
        ) else (
            echo ERROR: No se pudo copiar SetUserFTA.exe
        )
    ) else (
        echo ERROR: SetUserFTA.exe no encontrado
        echo.
        echo INSTRUCCIONES PARA OBTENER SETUSERFTA:
        echo 1. Visita: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/
        echo 2. Descarga SetUserFTA.exe
        echo 3. Copia SetUserFTA.exe a QtClient\scripts\deploy\ o directamente a QtClient\release\deploy\
        echo.
    )
)

echo.
echo Verificando archivos finales en deploy...
if exist "LGA_OpenInNukeX.exe" (
    echo LGA_OpenInNukeX.exe: OK
) else (
    echo LGA_OpenInNukeX.exe: FALTA
)

if exist "app_icon.ico" (
    echo app_icon.ico: OK
) else (
    echo app_icon.ico: FALTA (opcional)
)

if exist "SetUserFTA.exe" (
    echo SetUserFTA.exe: OK
) else (
    echo SetUserFTA.exe: FALTA (requerido para asociaciones)
)

if exist "dark_theme.qss" (
    echo dark_theme.qss: OK
) else (
    echo dark_theme.qss: FALTA (tema visual)
)

popd >nul

echo.
echo ===============================================
echo Deploy completado. Archivos listos en release\deploy\
echo ===============================================
echo.

exit /b 0

