@echo off
setlocal EnableExtensions

for %%I in ("%~dp0.") do set "QTCLIENT_DIR=%%~fI"

echo =================================
echo     LIMPIANDO ARCHIVOS
echo =================================
echo.
echo Eliminando archivos de compilacion...

if exist "%QTCLIENT_DIR%\Makefile*" (
    del /Q "%QTCLIENT_DIR%\Makefile*"
    echo - Makefiles eliminados
)

if exist "%QTCLIENT_DIR%\*.o" (
    del /Q "%QTCLIENT_DIR%\*.o"
    echo - Archivos objeto eliminados
)

if exist "%QTCLIENT_DIR%\*.exe" (
    del /Q "%QTCLIENT_DIR%\*.exe"
    echo - Ejecutables eliminados
)

if exist "%QTCLIENT_DIR%\release" (
    rmdir /S /Q "%QTCLIENT_DIR%\release"
    echo - Directorio release eliminado
)

if exist "%QTCLIENT_DIR%\build" (
    rmdir /S /Q "%QTCLIENT_DIR%\build"
    echo - Directorio build eliminado
)

if exist "%QTCLIENT_DIR%\debug" (
    rmdir /S /Q "%QTCLIENT_DIR%\debug"
    echo - Directorio debug eliminado
)

if exist "%QTCLIENT_DIR%\*.log" (
    del /Q "%QTCLIENT_DIR%\*.log"
    echo - Archivos de log eliminados
)

if exist "%QTCLIENT_DIR%\moc_*" (
    del /Q "%QTCLIENT_DIR%\moc_*"
    echo - Archivos MOC eliminados
)

if exist "%QTCLIENT_DIR%\ui_*" (
    del /Q "%QTCLIENT_DIR%\ui_*"
    echo - Archivos UI eliminados
)

if exist "%QTCLIENT_DIR%\*_resource.rc" (
    del /Q "%QTCLIENT_DIR%\*_resource.rc"
    echo - Archivos de recursos eliminados
)

echo.
echo =================================
echo     LIMPIEZA COMPLETADA
echo =================================
echo.

exit /b 0

