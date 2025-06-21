@echo off
echo =================================
echo     PROBANDO EJECUTABLE
echo =================================

echo Ejecutando sin argumentos:
.\release\OpenInNukeX.exe
echo.

echo Ejecutando con archivo de prueba:
.\release\OpenInNukeX.exe "C:\Users\leg4-pc\.nuke\LGA_OpenInNukeX\QtClient\test_file.nk"
echo.

echo Verificando log:
if exist "OpenInNukeX.log" (
    echo === CONTENIDO DEL LOG ===
    type "OpenInNukeX.log"
) else (
    echo No se creo archivo de log
)

pause 