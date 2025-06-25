@echo off
echo ========================================
echo  LIMPIEZA DE ASOCIACIONES .NK
echo ========================================
echo.
echo Este script limpia completamente las asociaciones
echo de archivos .nk del registro de Windows.
echo.
echo ADVERTENCIA: Esto eliminara TODAS las asociaciones
echo existentes para archivos .nk
echo.
pause

echo.
echo Limpiando registro...

REM Limpiar UserChoice (lo mas importante)
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nk\UserChoice" /f >nul 2>&1
if %errorlevel% == 0 (
    echo ✓ UserChoice limpiado
) else (
    echo - UserChoice no existia o ya estaba limpio
)

REM Limpiar OpenWithList
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nk\OpenWithList" /f >nul 2>&1
if %errorlevel% == 0 (
    echo ✓ OpenWithList limpiado
) else (
    echo - OpenWithList no existia o ya estaba limpio
)

REM Limpiar OpenWithProgids
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.nk\OpenWithProgids" /f >nul 2>&1
if %errorlevel% == 0 (
    echo ✓ OpenWithProgids limpiado
) else (
    echo - OpenWithProgids no existia o ya estaba limpio
)

REM Limpiar asociacion directa en Classes
reg delete "HKEY_CURRENT_USER\Software\Classes\.nk" /ve /f >nul 2>&1
if %errorlevel% == 0 (
    echo ✓ Classes\.nk limpiado
) else (
    echo - Classes\.nk no existia o ya estaba limpio
)

REM Limpiar ProgIDs antiguos comunes
reg delete "HKEY_CURRENT_USER\Software\Classes\NukeScript" /f >nul 2>&1
reg delete "HKEY_CURRENT_USER\Software\Classes\nuke_auto_file" /f >nul 2>&1
reg delete "HKEY_CURRENT_USER\Software\Classes\Foundry.Nuke.Script" /f >nul 2>&1
echo ✓ ProgIDs antiguos limpiados

echo.
echo ========================================
echo  LIMPIEZA COMPLETADA
echo ========================================
echo.
echo El registro ha sido limpiado.
echo Ahora puedes ejecutar LGA_OpenInNukeX.exe
echo y usar el boton APPLY para establecer
echo la asociacion correctamente.
echo.
echo Si el problema persiste:
echo 1. Reinicia el Explorador de Windows
echo 2. Reinicia el ordenador
echo.
pause 