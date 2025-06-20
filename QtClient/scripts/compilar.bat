@echo off
echo =================================
echo    COMPILANDO PARA DESARROLLO
echo =================================

REM Configurar el entorno de Qt
set QT_DIR=C:\Qt\6.8.2\mingw_64
set MINGW_DIR=C:\Qt\Tools\mingw1310_64
set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%

REM Limpiar archivos anteriores
echo Limpiando archivos anteriores...
call limpiar.bat >nul 2>&1

REM Cambiar al directorio padre (QtClient)
cd ..

REM Crear directorio de build si no existe
if not exist "build" mkdir build
cd build

REM Configurar con CMake
echo Configurando proyecto con CMake...
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=%QT_DIR% ..

if %ERRORLEVEL% neq 0 (
    echo Error en la configuracion de CMake
    pause
    exit /b 1
)

REM Compilar
echo Compilando...
mingw32-make -j4

if %ERRORLEVEL% neq 0 (
    echo Error en la compilacion
    pause
    exit /b 1
)

echo.
echo =================================
echo    COMPILACION COMPLETADA
echo =================================
echo Ejecutable creado en: build\LGA_OpenInNukeX.exe

REM Copiar nukeXpath.txt si existe en el directorio padre
if exist "..\nukeXpath.txt" (
    copy "..\nukeXpath.txt" "nukeXpath.txt"
    echo Archivo nukeXpath.txt copiado.
)

