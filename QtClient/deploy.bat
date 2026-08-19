@echo off
setlocal EnableExtensions

REM Deploy de Windows. Arma release\deploy\ con el .exe, las DLLs de Qt, los recursos y el
REM payload del Nuke Bridge; eso es lo que despues empaqueta instalador.bat con Inno Setup.
REM
REM Dos cosas cambiaron respecto de la version anterior, y las dos eran bugs reales:
REM
REM 1. Compilaba en un TERCER arbol propio (build_deploy\) con su copia entera de la logica
REM    de configuracion de CMake. La convencion LGA son DOS arboles: build\ para Debug y
REM    build-release\ para Release. Con tres, el deploy nunca reusaba nada y ademas la
REM    logica duplicada se desincronizaba del script que si se mantiene.
REM 2. No copiaba bridge\, asi que en el .exe distribuido el boton INSTALL del Nuke Bridge
REM    no tenia que instalar. El chequeo del final ahora corta si falta.

REM Parseo real y no un solo `if` sobre %~1: antes cualquier opcion desconocida —o un
REM --no-run en segunda posicion— se ignoraba en silencio.
REM
REM Los shift van SIEMPRE con /1. Sin el, `shift` desplaza tambien %0, asi que despues de
REM consumir la primera opcion `%~dp0` deja de ser la carpeta del script y pasa a ser la del
REM que llamo. Con eso QTCLIENT_DIR apuntaba al repo de release y el deploy moria diciendo
REM que faltaba resources\SetUserFTA.exe, teniendolo al lado. Como instalador.bat siempre
REM pasa --no-run, el deploy de Windows estaba roto en todos los casos.
set "NO_RUN=false"
set "PARALLEL_ARGS="

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--no-run" ( set "NO_RUN=true" & shift /1 & goto parse_args )
if /I "%~1"=="--parallel" (
    if "%~2"=="" (
        echo ERROR: --parallel requiere una cantidad de nucleos.
        exit /b 1
    )
    set "PARALLEL_ARGS=--parallel %~2"
    shift /1
    shift /1
    goto parse_args
)
echo ERROR: Opcion desconocida: %~1
echo Uso: %~nx0 [--no-run] [--parallel N]
exit /b 1
:args_done

for %%I in ("%~dp0.") do set "QTCLIENT_DIR=%%~fI"
set "BUILD_DIR=%QTCLIENT_DIR%\build-release"
set "RELEASE_DIR=%QTCLIENT_DIR%\release"
set "DEPLOY_DIR=%RELEASE_DIR%\deploy"
set "QT_DIR=C:\Qt\6.5.3\mingw_64"
set "MINGW_DIR=C:\Qt\Tools\mingw1310_64"

REM --release y NO el build de desarrollo: lo que se publica tiene que ir optimizado y sin
REM asserts. Y SIN --no-deploy: el bundle de deploy tiene que pasar por windeployqt.
call "%~dp0compilar_dev.bat" --release --no-run %PARALLEL_ARGS%
if errorlevel 1 exit /b 1

if not exist "%BUILD_DIR%\LGA_OpenInNukeX.exe" (
    echo ERROR: no se encontro "%BUILD_DIR%\LGA_OpenInNukeX.exe".
    exit /b 1
)

REM Que lo que se va a empaquetar sea REALMENTE Release. El chequeo es barato y ataja el
REM caso en que alguien toque los scripts y vuelva a empaquetar el arbol de desarrollo.
findstr /C:"CMAKE_BUILD_TYPE:STRING=Release" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
if errorlevel 1 (
    echo ERROR: "%BUILD_DIR%" no esta configurado en Release.
    exit /b 1
)

if exist "%DEPLOY_DIR%" rmdir /S /Q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

xcopy /E /I /Y "%BUILD_DIR%" "%DEPLOY_DIR%" >nul
if errorlevel 1 exit /b 1

REM Refrescar los recursos propios por si se tocaron despues del build.
copy /Y "%QTCLIENT_DIR%\dark_theme.qss" "%DEPLOY_DIR%\" >nul
copy /Y "%QTCLIENT_DIR%\resources\app_icon.ico" "%DEPLOY_DIR%\" >nul
if exist "%QTCLIENT_DIR%\resources\SetUserFTA.exe" (
    copy /Y "%QTCLIENT_DIR%\resources\SetUserFTA.exe" "%DEPLOY_DIR%\" >nul
)

REM Sacar del deploy lo que es del arbol de build y no del producto. La lista tiene que
REM cubrir TODO lo que genera Ninja/CMake: lo que quede aca se lo lleva despues Inno Setup
REM adentro del instalador que baja el usuario. El que mas pesa es el arbol _autogen\, con
REM los moc y sus objetos.
for %%D in (CMakeFiles LGA_OpenInNukeX_autogen .qt) do (
    if exist "%DEPLOY_DIR%\%%D" rmdir /S /Q "%DEPLOY_DIR%\%%D" >nul 2>&1
)
for %%F in (
    CMakeCache.txt
    cmake_install.cmake
    build.ninja
    .ninja_deps
    .ninja_log
    compile_commands.json
    install_manifest.txt
) do (
    if exist "%DEPLOY_DIR%\%%F" del /Q "%DEPLOY_DIR%\%%F" >nul 2>&1
)

echo Verificando el deploy...
for %%F in (
    LGA_OpenInNukeX.exe
    Qt6Core.dll
    Qt6Widgets.dll
    Qt6Network.dll
    dark_theme.qss
    app_icon.ico
) do (
    if not exist "%DEPLOY_DIR%\%%F" (
        echo ERROR: Falta %%F en el deploy.
        exit /b 1
    )
)

if not exist "%DEPLOY_DIR%\platforms\qwindows.dll" (
    echo ERROR: Falta platforms\qwindows.dll en el deploy.
    exit /b 1
)

REM El payload del bridge: sin el, el boton INSTALL falla en la maquina del usuario.
for %%F in (init.py LGA_QtAdapter_OpenInNukeX.py) do (
    if not exist "%DEPLOY_DIR%\bridge\%%F" (
        echo ERROR: Falta bridge\%%F en el deploy.
        echo        Sin eso el boton INSTALL del Nuke Bridge no tiene que instalar.
        exit /b 1
    )
)

echo.
echo Deploy completado en "%DEPLOY_DIR%".
if /I "%NO_RUN%"=="true" exit /b 0

REM Suelta y no en primer plano: si no, la consola queda retenida hasta que alguien cierre
REM la ventana de la app a mano.
start "" "%DEPLOY_DIR%\LGA_OpenInNukeX.exe"
exit /b 0
