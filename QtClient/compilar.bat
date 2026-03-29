@echo off
setlocal EnableExtensions

for %%I in ("%~dp0.") do set "QTCLIENT_DIR=%%~fI"
set "BUILD_DIR=%QTCLIENT_DIR%\build"
set "QT_DIR=C:\Qt\6.5.3\mingw_64"
set "MINGW_DIR=C:\Qt\Tools\mingw1120_64"

echo =================================
echo    COMPILANDO PARA DESARROLLO
echo =================================

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

echo Usando Qt: %QT_DIR%
echo Usando MinGW: %MINGW_DIR%
echo.

echo Limpiando archivos anteriores...
if exist "%QTCLIENT_DIR%\limpiar.bat" (
    call "%QTCLIENT_DIR%\limpiar.bat" >nul 2>&1
)

echo Configurando proyecto con CMake...
cmake -S "%QTCLIENT_DIR%" -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="%QT_DIR%\lib\cmake"
if errorlevel 1 (
    echo Error en la configuracion de CMake
    exit /b 1
)

echo Compilando...
cmake --build "%BUILD_DIR%" -j 4
if errorlevel 1 (
    echo Error en la compilacion
    exit /b 1
)

echo.
echo =================================
echo    COMPILACION COMPLETADA
echo =================================
echo Ejecutable creado en: %BUILD_DIR%\LGA_OpenInNukeX.exe

if exist "%QTCLIENT_DIR%\nukeXpath.txt" (
    copy /Y "%QTCLIENT_DIR%\nukeXpath.txt" "%BUILD_DIR%\nukeXpath.txt" >nul
    echo Archivo nukeXpath.txt copiado.
)

exit /b 0
