@echo off
echo =================================
echo     LIMPIANDO ARCHIVOS
echo =================================

echo Eliminando archivos de compilacion...

REM Limpiar archivos de qmake
if exist "Makefile*" (
    del /Q Makefile*
    echo - Makefiles eliminados
)

if exist "*.o" (
    del /Q *.o
    echo - Archivos objeto eliminados
)

REM Limpiar ejecutables locales
if exist "*.exe" (
    del /Q *.exe
    echo - Ejecutables eliminados
)

REM Limpiar directorios de build
if exist "release" (
    rmdir /S /Q release
    echo - Directorio release eliminado
)

if exist "build" (
    rmdir /S /Q build
    echo - Directorio build eliminado
)

if exist "debug" (
    rmdir /S /Q debug
    echo - Directorio debug eliminado
)

REM Limpiar archivos de log
if exist "*.log" (
    del /Q *.log
    echo - Archivos de log eliminados
)

REM Limpiar archivos temporales de Qt
if exist "moc_*" (
    del /Q moc_*
    echo - Archivos MOC eliminados
)

if exist "ui_*" (
    del /Q ui_*
    echo - Archivos UI eliminados
)

if exist "*_resource.rc" (
    del /Q *_resource.rc
    echo - Archivos de recursos eliminados
)

echo.
echo =================================
echo     LIMPIEZA COMPLETADA
echo =================================
echo.
pause 