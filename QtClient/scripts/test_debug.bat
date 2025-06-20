@echo off
echo =================================
echo     PROBANDO EJECUTABLE
echo =================================

echo Ejecutando sin argumentos:
.\release\LGA_OpenInNukeX_Qt.exe
echo.

echo Ejecutando con archivo de prueba:
.\release\LGA_OpenInNukeX_Qt.exe "C:\Users\leg4-pc\.nuke\LGA_OpenInNukeX\QtClient\test_file.nk"
echo.

echo Verificando log:
if exist "LGA_OpenInNukeX_Qt.log" (
    echo === CONTENIDO DEL LOG ===
    type "LGA_OpenInNukeX_Qt.log"
) else (
    echo No se creo archivo de log
)

pause 