@echo off
setlocal
set ROOT=%~dp0
set OUT=%ROOT%..\..\build
if not "%~1"=="" set OUT=%~1

where dotnet >nul 2>&1
if errorlevel 1 (
  echo ERROR: dotnet SDK no encontrado. Instalar .NET SDK para compilar LGA_WinSetFTA.
  exit /b 1
)

dotnet build "%ROOT%LGA_WinSetFTA.csproj" -c Release -o "%OUT%" -v q
if errorlevel 1 exit /b 1

copy /Y "%ROOT%Resources\LookUpLut4.bin" "%OUT%\LookUpLut4.bin" >nul
echo LGA_WinSetFTA.exe listo en %OUT%
exit /b 0
