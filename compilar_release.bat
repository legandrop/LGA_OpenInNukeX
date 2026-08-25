@echo off
setlocal

REM Compila en Release, en el arbol build-release\. Wrapper de dos lineas: no
REM configura nada por su cuenta, solo le pasa --release al motor de desarrollo.
REM Uso normal: correrlo a mano para probar un Release sin pasar por deploy.bat,
REM que compila directo contra compilar.bat --release --no-run.
cd /d "%~dp0"
call ".\compilar.bat" --release %*
set "EXIT_CODE=%ERRORLEVEL%"
endlocal & exit /b %EXIT_CODE%
