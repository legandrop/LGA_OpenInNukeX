@echo off
setlocal

echo compilar.bat usa ahora el flujo incremental moderno.
call "%~dp0compilar_dev.bat" %*
set "EXIT_CODE=%ERRORLEVEL%"
endlocal & exit /b %EXIT_CODE%
