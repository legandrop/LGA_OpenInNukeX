@echo off
setlocal

REM Compila en Release, en el arbol build-release\.
REM
REM Antes forwardeaba a compilar_dev.bat SIN --release, o sea que el script "de release"
REM compilaba Debug: sin optimizar y con los asserts vivos. Nada lo delataba, porque el
REM ejecutable se ve igual, pesa parecido y anda — solo que lento, en la maquina del usuario.
call "%~dp0compilar_dev.bat" --release %*
set "EXIT_CODE=%ERRORLEVEL%"
endlocal & exit /b %EXIT_CODE%
