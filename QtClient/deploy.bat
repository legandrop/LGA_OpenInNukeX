@echo off
setlocal EnableExtensions

set "NO_RUN=false"
if /I "%~1"=="--no-run" set "NO_RUN=true"

for %%I in ("%~dp0.") do set "QTCLIENT_DIR=%%~fI"
set "BUILD_DIR=%QTCLIENT_DIR%\build_deploy"
set "RELEASE_DIR=%QTCLIENT_DIR%\release"
set "DEPLOY_DIR=%RELEASE_DIR%\deploy"
set "QT_DIR=C:\Qt\6.5.3\mingw_64"
set "MINGW_DIR=C:\Qt\Tools\mingw1310_64"
set "NINJA_DIR=C:\Qt\Tools\Ninja"
set "LLVM_DIR=C:\Program Files\LLVM\bin"
set "PARALLEL_CORES=%NUMBER_OF_PROCESSORS%"

if not exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo ERROR: Qt 6.5.3 no encontrado.
    exit /b 1
)
if not exist "%MINGW_DIR%\bin\g++.exe" (
    echo ERROR: MinGW 13.1 no encontrado.
    exit /b 1
)
if not exist "%NINJA_DIR%\ninja.exe" (
    echo ERROR: Ninja no encontrado.
    exit /b 1
)
if not exist "%LLVM_DIR%\ld.lld.exe" (
    echo ERROR: lld no encontrado.
    exit /b 1
)
if not exist "%QTCLIENT_DIR%\resources\SetUserFTA.exe" (
    echo ERROR: Falta resources\SetUserFTA.exe.
    exit /b 1
)

set "PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%NINJA_DIR%;%LLVM_DIR%;%PATH%"

taskkill /F /IM LGA_OpenInNukeX.exe >nul 2>&1

if exist "%BUILD_DIR%\CMakeCache.txt" (
    findstr /C:"CMAKE_GENERATOR:INTERNAL=Ninja" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
    if errorlevel 1 rmdir /S /Q "%BUILD_DIR%"
)

echo Configurando Release...
cmake -S "%QTCLIENT_DIR%" -B "%BUILD_DIR%" -G "Ninja" ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" ^
    -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld"
if errorlevel 1 exit /b 1

echo Compilando Release...
cmake --build "%BUILD_DIR%" --parallel "%PARALLEL_CORES%"
if errorlevel 1 exit /b 1

if exist "%DEPLOY_DIR%" rmdir /S /Q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

copy /Y "%BUILD_DIR%\LGA_OpenInNukeX.exe" "%DEPLOY_DIR%\" >nul
copy /Y "%QTCLIENT_DIR%\dark_theme.qss" "%DEPLOY_DIR%\" >nul
copy /Y "%QTCLIENT_DIR%\resources\app_icon.ico" "%DEPLOY_DIR%\" >nul
copy /Y "%QTCLIENT_DIR%\resources\SetUserFTA.exe" "%DEPLOY_DIR%\" >nul

echo Desplegando Qt y runtime MinGW...
"%QT_DIR%\bin\windeployqt.exe" --compiler-runtime --dir "%DEPLOY_DIR%" "%DEPLOY_DIR%\LGA_OpenInNukeX.exe"
if errorlevel 1 exit /b 1

if not exist "%DEPLOY_DIR%\libgcc_s_seh-1.dll" copy /Y "%MINGW_DIR%\bin\libgcc_s_seh-1.dll" "%DEPLOY_DIR%\" >nul
if not exist "%DEPLOY_DIR%\libstdc++-6.dll" copy /Y "%MINGW_DIR%\bin\libstdc++-6.dll" "%DEPLOY_DIR%\" >nul
if not exist "%DEPLOY_DIR%\libwinpthread-1.dll" copy /Y "%MINGW_DIR%\bin\libwinpthread-1.dll" "%DEPLOY_DIR%\" >nul

for %%F in (
    LGA_OpenInNukeX.exe
    Qt6Core.dll
    Qt6Widgets.dll
    Qt6Network.dll
    dark_theme.qss
    app_icon.ico
    SetUserFTA.exe
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

echo.
echo Deploy completado en "%DEPLOY_DIR%".
if /I "%NO_RUN%"=="true" exit /b 0

start "" "%DEPLOY_DIR%\LGA_OpenInNukeX.exe"
exit /b 0
