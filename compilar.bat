@echo off
setlocal EnableExtensions

set "FORCE_CLEAN=false"
set "NO_DEPLOY=false"
set "NO_RUN=false"
REM CONVENCION LGA - el build de desarrollo (Debug) y el de release viven en arboles
REM SEPARADOS. Con un arbol solo, alternar entre trabajar y empaquetar invalida la cache de
REM CMake y obliga a recompilar todo cada vez.
set "BUILD_TYPE=Debug"
set "BUILD_SUBDIR=build"
set "WAIT_FOR_APP=false"
set "SIM_SLOW=false"
set "PARALLEL_CORES=%NUMBER_OF_PROCESSORS%"

REM Los shift van SIEMPRE con /1. Sin el, `shift` desplaza tambien %0, y `%~dp0` —que se usa
REM mas abajo para resolver REPO_ROOT (y de ahi QTCLIENT_DIR)— deja de ser la carpeta del
REM script para pasar a ser la del que llamo. Aca entra siempre con --release como minimo,
REM asi que sin /1 la ruta salia mal en todas las corridas.
:parse_args
if "%~1"=="" goto main
if /I "%~1"=="--force-clean" ( set "FORCE_CLEAN=true" & shift /1 & goto parse_args )
if /I "%~1"=="--no-deploy" ( set "NO_DEPLOY=true" & shift /1 & goto parse_args )
if /I "%~1"=="--no-run" ( set "NO_RUN=true" & shift /1 & goto parse_args )
if /I "%~1"=="--release" ( set "BUILD_TYPE=Release" & set "BUILD_SUBDIR=build-release" & shift /1 & goto parse_args )
if /I "%~1"=="--wait" ( set "WAIT_FOR_APP=true" & shift /1 & goto parse_args )
if /I "%~1"=="--sim-slow" ( set "SIM_SLOW=true" & shift /1 & goto parse_args )
if /I "%~1"=="--parallel" (
    if "%~2"=="" (
        echo ERROR: --parallel requiere una cantidad de nucleos.
        exit /b 1
    )
    set "PARALLEL_CORES=%~2"
    shift /1
    shift /1
    goto parse_args
)
if /I "%~1"=="--help" goto show_help
if /I "%~1"=="-h" goto show_help
echo ERROR: Opcion desconocida: %~1
goto show_help_error

:show_help
echo Uso: %~nx0 [opciones]
echo.
echo   --release       Compila en Release, en el arbol build-release\ (lo usa deploy.bat).
echo                   Por defecto compila en Debug, en build\. Los dos arboles son
echo                   SEPARADOS para que pedir un release no invalide la cache de dev.
echo   --force-clean   Elimina el arbol de build antes de configurar.
echo   --no-deploy     Omite windeployqt y copia de runtimes.
echo   --no-run        Compila pero no ejecuta la aplicacion.
echo   --wait          Deja la app en primer plano: la consola queda retenida hasta
echo                   cerrarla y se ve su salida. Por defecto se lanza suelta.
echo   --sim-slow      Lanza la app SIMULANDO UNA MAQUINA LENTA: prioridad idle y solo
echo                   2 nucleos. Los procesos hijos heredan la politica. Windows no
echo                   permite throttlear el I/O de disco, asi que sus numeros NO son
echo                   comparables con los de macOS.
echo   --parallel N    Usa N procesos de compilacion.
exit /b 0

:show_help_error
echo Use %~nx0 --help para ver las opciones.
exit /b 1

:main
REM Parado siempre en la raiz del repo: el script vive ahi, pero CMakeLists.txt,
REM los arboles build*/ y todos los recursos del cliente Qt siguen en QtClient\.
cd /d "%~dp0"
for %%I in ("%~dp0.") do set "REPO_ROOT=%%~fI"
set "QTCLIENT_DIR=%REPO_ROOT%\QtClient"
set "BUILD_DIR=%QTCLIENT_DIR%\%BUILD_SUBDIR%"
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
    REM Reconfigurar tambien si la cache quedo con OTRO build type. Chequear solo la
    REM existencia del CMakeCache alcanzaba con un arbol unico; con dos, un arbol heredado
    REM puede tener Debug adentro y compilaria Debug EN SILENCIO al pedirle un release. Nada
    REM lo delata: el binario se ve igual, pesa parecido y anda, solo que lento y con asserts.
    findstr /C:"CMAKE_BUILD_TYPE:STRING=%BUILD_TYPE%" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
    if errorlevel 1 (
        echo La cache no esta en %BUILD_TYPE%: se reconfigura el arbol.
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
        -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
        -DCMAKE_CXX_FLAGS_DEBUG="-g -O0 -Wno-unused-parameter" ^
        -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" ^
        -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld"
    if errorlevel 1 exit /b 1
) else (
    echo Reutilizando configuracion incremental existente.
)

echo Compilando %BUILD_TYPE% con Ninja (%PARALLEL_CORES% procesos) en %BUILD_SUBDIR%\...
cmake --build "%BUILD_DIR%" --parallel "%PARALLEL_CORES%"
if errorlevel 1 exit /b 1

copy /Y "%QTCLIENT_DIR%\dark_theme.qss" "%BUILD_DIR%\dark_theme.qss" >nul
copy /Y "%QTCLIENT_DIR%\resources\app_icon.ico" "%BUILD_DIR%\app_icon.ico" >nul
call "%REPO_ROOT%\tools\win_file_assoc\build_win_setfta.bat" "%BUILD_DIR%"
if errorlevel 1 exit /b 1

REM ============================================================
REM  DEPENDENCIAS DE RUNTIME: verificar -> reparar -> verificar
REM
REM  El arbol de build es incremental y nadie lo limpia, asi que en la corrida
REM  normal ya esta todo puesto y esto son N chequeos "if not exist": no cuesta
REM  nada. windeployqt se llama SOLO cuando falta algo, porque es el unico que
REM  conoce la clausura transitiva de Qt.
REM
REM  Antes esto gateaba TODO detras de un solo archivo [Qt6Core.dll] y llamaba a
REM  windeployqt PRIMERO en vez de como ultimo recurso: un centinela que miente
REM  igual que el que tenian FileManagerS3/PipeSync antes de migrar.
REM ============================================================

REM Lista canonica, relativa a %BUILD_DIR%\. Derivada del find_package(Qt6
REM REQUIRED COMPONENTS Core Widgets Network) de CMakeLists.txt Y verificada con
REM `objdump -p` contra el binario real. La app no usa QIcon sobre SVG ni
REM QSsl/HTTPS [verificado en src/], asi que NO lleva iconengines\,
REM imageformats\ ni plugins\tls\: serian dependencias que la app nunca carga.
REM
REM Qt6Gui.dll: find_package solo pide Core/Widgets/Network porque Gui es
REM dependencia TRANSITIVA de Widgets, no un componente propio, y por eso no
REM aparecia en la lista original -razonada solo a partir del CMakeLists-. Pero
REM tanto LGA_OpenInNukeX.exe como platforms\qwindows.dll la IMPORTAN DIRECTO
REM [confirmado con objdump -p sobre los binarios de build\]: sin el archivo el
REM build pasaba la verificacion igual -no es una dependencia del find_package,
REM pero si del binario- y la app no arrancaba en un arbol limpio.
set "DEP_LIST=libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Network.dll platforms\qwindows.dll"

if /I "%NO_DEPLOY%"=="true" goto verify

echo Verificando dependencias de runtime...
set DEPS_MISSING=false
for %%D in (%DEP_LIST%) do call :check_dep "%%D"

REM Primero las copias directas: la lista de arriba es explicita y esta probada.
if "%DEPS_MISSING%"=="true" call :copy_missing_deps
if "%DEPS_MISSING%"=="true" (
    set DEPS_MISSING=false
    for %%D in (%DEP_LIST%) do call :check_dep "%%D"
)

REM Ultimo recurso: windeployqt, el unico que conoce la clausura transitiva de
REM Qt y podria traer algo que la lista no contemple.
if "%DEPS_MISSING%"=="true" call :run_windeployqt
if "%DEPS_MISSING%"=="true" (
    set DEPS_MISSING=false
    for %%D in (%DEP_LIST%) do call :check_dep "%%D"
)

if "%DEPS_MISSING%"=="true" (
    echo.
    echo ERROR: faltan dependencias de runtime y no se pudieron reparar.
    echo        Verifica que Qt 6.5.3 mingw_64 este instalado en "%QT_DIR%".
    exit /b 1
)
echo Dependencias de runtime verificadas.

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

echo.
echo Compilacion lista: %APP_EXE%
if /I "%NO_RUN%"=="true" (
    echo Ejecucion omitida por --no-run.
    exit /b 0
)

echo Ejecutando LGA_OpenInNukeX...
REM CONVENCION LGA - por defecto la app se lanza SUELTA y el script termina enseguida.
REM Dejarla en primer plano retiene la consola hasta que alguien cierre la ventana a mano,
REM lo que cuelga al que compila (y a cualquier agente) por tiempo indefinido.
REM
REM --sim-slow: /LOW baja la prioridad a idle y /AFFINITY 3 la deja en 2 nucleos. Los
REM procesos hijos heredan las dos cosas. Windows NO permite throttlear el I/O de disco por
REM linea de comandos, asi que la degradacion es mas suave que la de macOS y los numeros de
REM las dos plataformas no se comparan entre si. Ver docs/Doc_SimSlow.md del template.
cd /d "%BUILD_DIR%"
if /I "%WAIT_FOR_APP%"=="true" (
    echo === INICIO DE EJECUCION [--wait] ===
    if /I "%SIM_SLOW%"=="true" (
        echo --sim-slow: prioridad idle + 2 nucleos ^(simulacion de maquina lenta^)
        REM Con /WAIT, `start` propaga el exit code del proceso hijo a su propio
        REM ERRORLEVEL: es la unica forma de aplicar prioridad/afinidad Y esperar.
        start "" /LOW /AFFINITY 3 /WAIT ".\LGA_OpenInNukeX.exe"
    ) else (
        REM ".\" no es cosmetico: con NoDefaultCurrentDirectoryInExePath=1 -que
        REM aparece en el entorno de algunas sesiones no interactivas, no en el
        REM registro de la maquina- cmd.exe NO busca ejecutables en el directorio
        REM actual y el nombre pelado devuelve 9009.
        .\LGA_OpenInNukeX.exe
    )
) else (
    if /I "%SIM_SLOW%"=="true" (
        echo --sim-slow: prioridad idle + 2 nucleos ^(simulacion de maquina lenta^)
        start "" /LOW /AFFINITY 3 "LGA_OpenInNukeX.exe"
    ) else (
        start "" "LGA_OpenInNukeX.exe"
    )
)
REM El codigo de salida se lee ACA, fuera del bloque: adentro, cmd.exe lo habria
REM expandido al parsear, o sea antes de que la app corriera.
call :report_exit %%ERRORLEVEL%%
cd /d "%QTCLIENT_DIR%"

REM Con --wait el script propaga el codigo de la app: sin esto un
REM "compilar.bat --wait && echo ok" imprimia ok con la app crasheada. En
REM background se sale con 0, que es el resultado del build.
if /I "%WAIT_FOR_APP%"=="true" exit /b %APP_EXIT_CODE%
exit /b 0

REM ============================================================
REM  Subrutinas
REM
REM  Van en subrutinas y no inline a proposito: cmd.exe expande los %VAR% de un
REM  bloque if(...)/for(...) al PARSEARLO, no al ejecutarlo, asi que una
REM  variable que se escribe y se lee dentro del mismo bloque lee siempre el
REM  valor viejo. Un `call` reparsea el cuerpo en cada invocacion y esquiva el
REM  problema sin necesidad de `setlocal EnableDelayedExpansion`.
REM ============================================================

:report_exit
set "APP_EXIT_CODE=%~1"
if /I not "%WAIT_FOR_APP%"=="true" goto :eof
echo.
echo === FIN DE EJECUCION - codigo de salida: %~1 ===
if "%~1"=="-1073741819" echo DIAGNOSTICO: acceso a memoria invalido [equivale a un segfault]
if "%~1"=="-1073741571" echo DIAGNOSTICO: desbordamiento del stack [tipicamente recursion infinita]
if "%~1"=="-1073740791" echo DIAGNOSTICO: desbordamiento de buffer en el stack
goto :eof

:check_dep
if not exist "%BUILD_DIR%\%~1" (
    echo    [falta] %BUILD_DIR%\%~1
    set DEPS_MISSING=true
)
goto :eof

:copy_missing_deps
echo.
echo Faltan dependencias de runtime. Copiandolas...

REM Sin 2>nul: una copia que falla tiene que verse.
call :copy_dep "%MINGW_DIR%\bin" "" libgcc_s_seh-1.dll
call :copy_dep "%MINGW_DIR%\bin" "" libstdc++-6.dll
call :copy_dep "%MINGW_DIR%\bin" "" libwinpthread-1.dll
call :copy_dep "%QT_DIR%\bin" "" Qt6Core.dll
call :copy_dep "%QT_DIR%\bin" "" Qt6Gui.dll
call :copy_dep "%QT_DIR%\bin" "" Qt6Widgets.dll
call :copy_dep "%QT_DIR%\bin" "" Qt6Network.dll
call :copy_dep "%QT_DIR%\plugins\platforms" "platforms" qwindows.dll
goto :eof

:run_windeployqt
set "WINDEPLOYQT=%QT_DIR%\bin\windeployqt.exe"
if not exist "%WINDEPLOYQT%" (
    for /f "delims=" %%W in ('where windeployqt.exe 2^>nul') do set "WINDEPLOYQT=%%W"
)

if not exist "%WINDEPLOYQT%" (
    echo ADVERTENCIA: no se encontro windeployqt.exe.
    goto :eof
)

echo Todavia faltan dependencias: probando con windeployqt...
REM Sin --no-translations deja .qm que la app no usa (la UI es solo en ingles),
REM y sin los otros dos, OpenGL por software mas el compilador de D3D.
set "DEPLOY_FLAG=--debug"
if /I "%BUILD_TYPE%"=="Release" set "DEPLOY_FLAG=--release"
"%WINDEPLOYQT%" %DEPLOY_FLAG% --compiler-runtime --no-translations --no-opengl-sw --no-system-d3d-compiler --dir "%BUILD_DIR%" "%APP_EXE%"
if errorlevel 1 echo ADVERTENCIA: windeployqt devolvio error.
goto :eof

:copy_dep
REM %1 = directorio origen, %2 = subdirectorio dentro de build [puede ir vacio],
REM %3 = nombre del archivo.
set "DEP_DEST=%BUILD_DIR%"
if not "%~2"=="" set "DEP_DEST=%BUILD_DIR%\%~2"
if exist "%DEP_DEST%\%~3" goto :eof
if not exist "%DEP_DEST%" mkdir "%DEP_DEST%"
if not exist "%~1\%~3" (
    echo    ERROR: no existe el origen "%~1\%~3"
    goto :eof
)
copy /Y "%~1\%~3" "%DEP_DEST%\" >nul
if errorlevel 1 (
    echo    ERROR: fallo la copia de "%~1\%~3"
) else (
    echo    [ok] %~3
)
goto :eof
