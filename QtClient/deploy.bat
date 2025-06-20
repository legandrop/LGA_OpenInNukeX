@echo off
echo =================================
echo    COMPILANDO PARA PRODUCCION
echo =================================

REM Configurar el entorno de Qt
set QT_DIR=C:\Qt\6.8.2\mingw_64
set MINGW_DIR=C:\Qt\Tools\mingw1310_64
set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%

REM Crear directorio de release si no existe
if not exist "release" mkdir release
cd release

REM Configurar con CMake para Release
echo Configurando proyecto con CMake (Release)...
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_DIR% ..

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
echo    CREANDO PAQUETE DE DESPLIEGUE
echo =================================

REM Crear directorio de despliegue
if not exist "deploy" mkdir deploy
copy "LGA_OpenInNukeX_Qt.exe" "deploy\"

REM Copiar nukeXpath.txt si existe en el directorio padre
if exist "..\nukeXpath.txt" (
    copy "..\nukeXpath.txt" "deploy\nukeXpath.txt"
    echo Archivo nukeXpath.txt copiado.
)

REM Usar windeployqt para incluir todas las dependencias de Qt
echo Desplegando dependencias de Qt...
cd deploy
windeployqt.exe LGA_OpenInNukeX_Qt.exe

echo.
echo =================================
echo    DESPLIEGUE COMPLETADO
echo =================================
echo Paquete completo creado en: release\deploy\
echo Ejecutable principal: LGA_OpenInNukeX_Qt.exe

echo.
echo Presiona cualquier tecla para salir...
pause > nul 