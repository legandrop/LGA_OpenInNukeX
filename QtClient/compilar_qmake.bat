@echo off
echo =================================
echo   COMPILANDO CON QMAKE (ALT)
echo =================================

REM Configurar el entorno de Qt
set QT_DIR=C:\Qt\6.8.2\mingw_64
set MINGW_DIR=C:\Qt\Tools\mingw1310_64
set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%

REM Limpiar archivos anteriores
if exist "Makefile" del Makefile
if exist "*.o" del *.o
if exist "LGA_OpenInNukeX_Qt.exe" del LGA_OpenInNukeX_Qt.exe

REM Generar Makefile
echo Generando Makefile con qmake...
qmake LGA_OpenInNukeX_Qt.pro

if %ERRORLEVEL% neq 0 (
    echo Error generando Makefile con qmake
    pause
    exit /b 1
)

REM Compilar
echo Compilando...
mingw32-make

if %ERRORLEVEL% neq 0 (
    echo Error en la compilacion
    pause
    exit /b 1
)

REM Copiar nukeXpath.txt si existe en el directorio padre
if exist "..\nukeXpath.txt" (
    copy "..\nukeXpath.txt" "nukeXpath.txt"
    echo Archivo nukeXpath.txt copiado.
)

echo.
echo =================================
echo    COMPILACION COMPLETADA
echo =================================
echo Ejecutable: LGA_OpenInNukeX_Qt.exe

echo.
echo Presiona cualquier tecla para salir...
pause > nul 