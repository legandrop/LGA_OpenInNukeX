@echo off
setlocal EnableExtensions

set "FORCE_CLEAN=false"
set "NO_DEPLOY=false"
set "NO_RUN=false"
set "PARALLEL_CORES=%NUMBER_OF_PROCESSORS%"

:parse_args
if "%~1"=="" goto main
if /I "%~1"=="--force-clean" ( set "FORCE_CLEAN=true" & shift & goto parse_args )
if /I "%~1"=="--no-deploy" ( set "NO_DEPLOY=true" & shift & goto parse_args )
if /I "%~1"=="--no-run" ( set "NO_RUN=true" & shift & goto parse_args )
if /I "%~1"=="--parallel" (
    if "%~2"=="" (
        echo ERROR: --parallel requiere una cantidad de nucleos.
        exit /b 1
    )
    set "PARALLEL_CORES=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--help" goto show_help
if /I "%~1"=="-h" goto show_help
echo ERROR: Opcion desconocida: %~1
goto show_help_error

:show_help
echo Uso: %~nx0 [opciones]
echo.
echo   --force-clean   Elimina build antes de configurar.
echo   --no-deploy     Omite windeployqt y copia de runtimes.
echo   --no-run        Compila pero no ejecuta la aplicacion.
echo   --parallel N    Usa N procesos de compilacion.
exit /b 0

:show_help_error
echo Use %~nx0 --help para ver las opciones.
exit /b 1

:main
for %%I in ("%~dp0.") do set "QTCLIENT_DIR=%%~fI"
set "BUILD_DIR=%QTCLIENT_DIR%\build"
set "QT_DIR=C:\Qt\6.5.3\mingw_64"
set "MINGW_DIR=C:\Qt\Tools\mingw1310_64"
set "NINJA_DIR=C:\Qt\Tools\Ninja"
set "LLVM_DIR=C:\Program Files\LLVM\bin"
set "APP_EXE=%BUILD_DIR%\LGA_OpenInNukeX.exe"

if not exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo ERROR: Qt 6.5.3 no encontrado en "%QT_DIR%".
    exit /b 1
)
if not exist "%MINGW_DIR%\bin\g++.exe" (
    echo ERROR: MinGW 13.1 no encontrado en "%MINGW_DIR%".
    exit /b 1
)
if not exist "%NINJA_DIR%\ninja.exe" (
    echo ERROR: Ninja no encontrado en "%NINJA_DIR%".
    exit /b 1
)
if not exist "%LLVM_DIR%\ld.lld.exe" (
    echo ERROR: lld no encontrado en "%LLVM_DIR%".
    exit /b 1
)

set "PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%NINJA_DIR%;%LLVM_DIR%;%PATH%"

taskkill /F /IM LGA_OpenInNukeX.exe >nul 2>&1
if not errorlevel 1 (
    echo Instancia anterior cerrada.
    ping 127.0.0.1 -n 2 >nul
)

if exist "%BUILD_DIR%\CMakeCache.txt" (
    findstr /C:"CMAKE_GENERATOR:INTERNAL=Ninja" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
    if errorlevel 1 (
        echo Cache incompatible detectado: el build sera migrado a Ninja.
        set "FORCE_CLEAN=true"
    )
    findstr /I /C:"mingw1310_64/bin/c++.exe" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
    if errorlevel 1 (
        echo Toolchain anterior detectado: el build sera migrado a MinGW 13.1.
        set "FORCE_CLEAN=true"
    )
)

if /I "%FORCE_CLEAN%"=="true" (
    echo Limpiando build incompatible o solicitado...
    if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%"
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo Configurando CMake con Ninja, MinGW 13.1 y lld...
    cmake -S "%QTCLIENT_DIR%" -B "%BUILD_DIR%" -G "Ninja" ^
        -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
        -DCMAKE_BUILD_TYPE=Debug ^
        -DCMAKE_CXX_FLAGS_DEBUG="-g -O0 -Wno-unused-parameter" ^
        -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" ^
        -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld"
    if errorlevel 1 exit /b 1
) else (
    echo Reutilizando configuracion incremental existente.
)

echo Compilando con Ninja (%PARALLEL_CORES% procesos)...
cmake --build "%BUILD_DIR%" --parallel "%PARALLEL_CORES%"
if errorlevel 1 exit /b 1

copy /Y "%QTCLIENT_DIR%\dark_theme.qss" "%BUILD_DIR%\dark_theme.qss" >nul
copy /Y "%QTCLIENT_DIR%\resources\app_icon.ico" "%BUILD_DIR%\app_icon.ico" >nul
copy /Y "%QTCLIENT_DIR%\resources\SetUserFTA.exe" "%BUILD_DIR%\SetUserFTA.exe" >nul

if /I "%NO_DEPLOY%"=="true" goto verify

if not exist "%BUILD_DIR%\Qt6Core.dll" (
    echo Desplegando dependencias Qt y runtime del compilador...
    "%QT_DIR%\bin\windeployqt.exe" --compiler-runtime --dir "%BUILD_DIR%" "%APP_EXE%"
    if errorlevel 1 exit /b 1
)

if not exist "%BUILD_DIR%\libgcc_s_seh-1.dll" copy /Y "%MINGW_DIR%\bin\libgcc_s_seh-1.dll" "%BUILD_DIR%\" >nul
if not exist "%BUILD_DIR%\libstdc++-6.dll" copy /Y "%MINGW_DIR%\bin\libstdc++-6.dll" "%BUILD_DIR%\" >nul
if not exist "%BUILD_DIR%\libwinpthread-1.dll" copy /Y "%MINGW_DIR%\bin\libwinpthread-1.dll" "%BUILD_DIR%\" >nul

:verify
if not exist "%APP_EXE%" (
    echo ERROR: No se genero "%APP_EXE%".
    exit /b 1
)
if not exist "%BUILD_DIR%\dark_theme.qss" (
    echo ERROR: Falta dark_theme.qss en build.
    exit /b 1
)
if not exist "%BUILD_DIR%\app_icon.ico" (
    echo ERROR: Falta app_icon.ico en build.
    exit /b 1
)
if not exist "%BUILD_DIR%\SetUserFTA.exe" (
    echo ERROR: Falta SetUserFTA.exe en build.
    exit /b 1
)

echo.
echo Compilacion lista: %APP_EXE%
if /I "%NO_RUN%"=="true" (
    echo Ejecucion omitida por --no-run.
    exit /b 0
)

echo Ejecutando LGA_OpenInNukeX...
pushd "%BUILD_DIR%" >nul
start "" "LGA_OpenInNukeX.exe"
popd >nul
exit /b 0
