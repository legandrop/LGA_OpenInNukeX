@echo off
echo ===============================================
echo    LGA_OpenInNukeX - Script de Deploy v0.14
echo ===============================================

REM Cerrar cualquier instancia de la aplicación que pueda estar ejecutándose
echo Cerrando instancias previas de LGA_OpenInNukeX...
taskkill /f /im LGA_OpenInNukeX.exe 2>nul
echo ✓ Instancias cerradas

REM Configurar el entorno de Qt
set QT_DIR=C:\Qt\6.8.2\mingw_64
set MINGW_DIR=C:\Qt\Tools\mingw1310_64
set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%

echo.
echo =================================
echo    COMPILANDO PARA PRODUCCION
echo =================================

REM Limpiar archivos anteriores
echo Limpiando archivos anteriores...
call limpiar.bat >nul 2>&1

REM Cambiar al directorio padre (QtClient)
cd ..

REM Crear directorio de release si no existe
if not exist "release" mkdir release
cd release

REM Configurar con CMake para Release
echo Configurando proyecto con CMake (Release)...
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QT_DIR% ..

if %ERRORLEVEL% neq 0 (
    echo ❌ Error en la configuracion de CMake
    
    exit /b 1
)

REM Compilar
echo Compilando...
mingw32-make -j4

if %ERRORLEVEL% neq 0 (
    echo ❌ Error en la compilacion
    
    exit /b 1
)

echo.
echo =================================
echo    CREANDO PAQUETE DE DEPLOY
echo =================================

REM Crear directorio de despliegue
if not exist "deploy" (
    mkdir deploy
    echo ✓ Directorio deploy creado
) else (
    echo ✓ Directorio deploy ya existe
)

echo.
echo Copiando ejecutable principal...
copy "LGA_OpenInNukeX.exe" "deploy\LGA_OpenInNukeX.exe" >nul
if exist "deploy\LGA_OpenInNukeX.exe" (
    echo ✓ LGA_OpenInNukeX.exe copiado exitosamente
) else (
    echo ❌ ERROR: No se pudo copiar LGA_OpenInNukeX.exe
    
    exit /b 1
)

REM Copiar nukeXpath.txt si existe en el directorio padre
if exist "..\nukeXpath.txt" (
    copy "..\nukeXpath.txt" "deploy\nukeXpath.txt" >nul
    echo ✓ Archivo nukeXpath.txt copiado
)

echo.
echo Copiando iconos necesarios...
REM Primero intentar desde resources
copy "..\resources\app_icon.ico" "deploy\app_icon.ico" >nul 2>&1
if not exist "deploy\app_icon.ico" (
    REM Si no existe en resources, copiar desde scripts/deploy
    copy "..\..\scripts\deploy\app_icon.ico" "deploy\app_icon.ico" >nul 2>&1
)
if exist "deploy\app_icon.ico" (
    echo ✓ Icono app_icon.ico copiado exitosamente
) else (
    echo ⚠ Warning: No se pudo copiar app_icon.ico desde ninguna ubicación
)

echo.
echo Copiando archivo de tema...
copy "..\dark_theme.qss" "deploy\dark_theme.qss" >nul 2>&1
if exist "deploy\dark_theme.qss" (
    echo ✓ Archivo dark_theme.qss copiado exitosamente
) else (
    echo ⚠ Warning: No se pudo copiar dark_theme.qss
)

echo.
echo Desplegando dependencias de Qt...
cd deploy
windeployqt.exe LGA_OpenInNukeX.exe

echo.
echo Verificando SetUserFTA.exe (requerido para asociaciones de archivos)...

REM Verificar si ya existe en deploy
if exist "SetUserFTA.exe" (
    echo ✓ SetUserFTA.exe ya existe en deploy\
) else (
    REM Verificar si existe en scripts/deploy/
    if exist "..\..\scripts\deploy\SetUserFTA.exe" (
        echo ✓ SetUserFTA.exe encontrado en scripts\deploy\, copiando...
        copy "..\..\scripts\deploy\SetUserFTA.exe" "SetUserFTA.exe" >nul
        if exist "SetUserFTA.exe" (
            echo ✓ SetUserFTA.exe copiado exitosamente
        ) else (
            echo ❌ ERROR: No se pudo copiar SetUserFTA.exe
        )
    ) else (
        echo ❌ ERROR: SetUserFTA.exe no encontrado
        echo.
        echo INSTRUCCIONES PARA OBTENER SETUSERFTA:
        echo 1. Visita: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/
        echo 2. Descarga SetUserFTA.exe
        echo 3. Copia SetUserFTA.exe a QtClient\scripts\deploy\ O directamente a QtClient\release\deploy\
        echo.
        echo NOTA: SetUserFTA es esencial para que las asociaciones
        echo de archivos funcionen correctamente en Windows 10/11.
        echo Sin él, Windows detectará las asociaciones como
        echo "hijacking" y las reseteará automáticamente.
        echo.
    )
)

echo.
echo Verificando archivos finales en deploy...
if exist "LGA_OpenInNukeX.exe" (
    echo ✓ LGA_OpenInNukeX.exe: OK
) else (
    echo ❌ LGA_OpenInNukeX.exe: FALTA
)

if exist "app_icon.ico" (
    echo ✓ app_icon.ico: OK
) else (
    echo ⚠ app_icon.ico: FALTA (opcional)
)

if exist "SetUserFTA.exe" (
    echo ✓ SetUserFTA.exe: OK
) else (
    echo ❌ SetUserFTA.exe: FALTA (requerido para asociaciones)
)

if exist "dark_theme.qss" (
    echo ✓ dark_theme.qss: OK
) else (
    echo ⚠ dark_theme.qss: FALTA (tema visual)
)

echo.
echo ===============================================
echo Deploy completado. Archivos listos en release\deploy\
echo ===============================================
echo.
echo NOTA IMPORTANTE:
echo SetUserFTA.exe es esencial para que las asociaciones
echo de archivos funcionen correctamente en Windows 10/11.
echo Sin él, Windows detectará las asociaciones como
echo "hijacking" y las reseteará automáticamente.
echo.

REM Volver al directorio de scripts
cd ..\..\scripts


