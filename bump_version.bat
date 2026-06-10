@echo off
setlocal

echo AVISO: bump_version.bat esta deprecated. Usar sync_version.bat.
call "%~dp0sync_version.bat" %*
set "EXIT_CODE=%ERRORLEVEL%"
endlocal & exit /b %EXIT_CODE%
