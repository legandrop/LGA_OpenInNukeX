@echo off
chcp 65001 >nul
setlocal EnableExtensions EnableDelayedExpansion

REM ============================================================================
REM  Publica el release de Windows en GitHub: arma el asset y lo sube a la
REM  release v<version>, creandola (con su tag) si todavia no existe.
REM
REM      LGA_OpenInNukeX_v<version>_win.zip
REM
REM  Es el espejo de github_release_mac.sh. Lo que en macOS hace deploy.sh
REM  --zip (armar el asset) aca no puede vivir en deploy.bat: el zip lleva
REM  adentro el LGA_OpenInNukeX_Setup.exe, que recien existe DESPUES de que
REM  instalador.bat corra Inno Setup. Por eso el armado del zip vive aca.
REM
REM  La version SIEMPRE sale de VERSION (sincronizado con CMakeLists.txt y el
REM  ChangeLog via tools/sync_version.py). No se pasa por parametro a
REM  proposito: un numero escrito a mano en la linea de comandos es la forma
REM  mas facil de publicar un asset que no coincide con lo que dice la app.
REM
REM  Convive con _LGA_Release\_LGA_ReleaseGen-OpenInNukeX.bat, que hace el
REM  release completo (bump de version, commit, push, zip y publicacion). Este
REM  script NO bumpea ni commitea: publica lo que ya esta en main. Las dos
REM  rutas producen el mismo asset con el mismo nombre.
REM ============================================================================

for %%I in ("%~dp0.") do set "QTCLIENT_DIR=%%~fI"
for %%I in ("%QTCLIENT_DIR%\..") do set "REPO_ROOT=%%~fI"
for %%I in ("%REPO_ROOT%\..") do set "NUKE_DIR=%%~fI"

set "ARTIFACT_NAME=LGA_OpenInNukeX"
set "PLUGIN_FOLDER=LGA_OpenInNukeX"

REM Por defecto publica en el propio repo. En una app derivada con repo de
REM releases separado, exportar LGA_RELEASE_REPO o pasar --repo.
set "RELEASE_REPO=%LGA_RELEASE_REPO%"
if "%RELEASE_REPO%"=="" set "RELEASE_REPO=legandrop/LGA_OpenInNukeX"

set "DEPLOY_DIR=%QTCLIENT_DIR%\release\deploy"
set "OUTPUT_DIR=%QTCLIENT_DIR%\installer_output"
set "SETUP_EXE=%OUTPUT_DIR%\LGA_OpenInNukeX_Setup.exe"
set "INSTALLER_BAT=%QTCLIENT_DIR%\instalador.bat"
set "SYNC_VERSION_BAT=%REPO_ROOT%\sync_version.bat"
set "NUKE_INSTALLER_DIR=%NUKE_DIR%\_LGA_Release\Installers\LGA_OpenInNukeX-Nuke"
set "COMMON_INSTALLER_DIR=%NUKE_DIR%\_LGA_Release\Installers\Common"
set "SEVENZIP=%ProgramFiles%\7-Zip\7z.exe"

set "DRY_RUN=false"
set "ASSUME_YES=false"
set "INSTALLER_MODE=prompt"
set "GH_CMD="

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--dry-run" ( set "DRY_RUN=true" & shift /1 & goto parse_args )
if /I "%~1"=="--yes" ( set "ASSUME_YES=true" & shift /1 & goto parse_args )
if /I "%~1"=="-y" ( set "ASSUME_YES=true" & shift /1 & goto parse_args )
if /I "%~1"=="--use-existing-installer" ( set "INSTALLER_MODE=use-existing" & shift /1 & goto parse_args )
if /I "%~1"=="--rebuild-installer" ( set "INSTALLER_MODE=rebuild" & shift /1 & goto parse_args )
if /I "%~1"=="--repo" (
    if "%~2"=="" (
        echo ERROR: --repo requiere owner/repo.
        exit /b 1
    )
    set "RELEASE_REPO=%~2"
    shift /1
    shift /1
    goto parse_args
)
if /I "%~1"=="-h" goto show_help
if /I "%~1"=="--help" goto show_help
echo ERROR: opcion desconocida: %~1
REM Etiqueta aparte y NO `goto show_help`: esa sale con 0, o sea que una opcion
REM mal tipeada no publicaba nada y devolvia exito. El `if errorlevel 1` de
REM instalador.bat lo habria leido como publicacion correcta.
goto show_help_error
:args_done

echo.
echo ================================================================
echo           LGA OpenInNukeX - Release de Windows
echo ================================================================
echo.

REM ---------------------------------------------------------------- preflight

if not exist "%SEVENZIP%" (
    echo ERROR: No se encontro 7-Zip en "%SEVENZIP%".
    echo Instalalo desde https://www.7-zip.org/ antes de publicar.
    exit /b 1
)
echo OK: 7-Zip encontrado.

where git >nul 2>nul
if errorlevel 1 (
    echo ERROR: git no esta disponible en PATH.
    exit /b 1
)

REM Python se chequea aca y no cuando falla: sync_version.bat lo invoca, y sin el
REM devuelve 9009. Ese errorlevel llega al chequeo de version y sale como "la
REM version esta desincronizada", que manda a arreglar algo que no esta roto.
python --version >nul 2>nul
if errorlevel 1 (
    echo ERROR: python no esta disponible en PATH. Lo necesita sync_version.
    exit /b 1
)

where gh >nul 2>nul
if not errorlevel 1 (
    set "GH_CMD=gh"
) else (
    if exist "%ProgramFiles%\GitHub CLI\gh.exe" set "GH_CMD=%ProgramFiles%\GitHub CLI\gh.exe"
)
if "!GH_CMD!"=="" (
    echo ERROR: GitHub CLI [gh] no esta instalado.
    echo Instalalo desde https://cli.github.com/ y corre 'gh auth login'.
    exit /b 1
)

"!GH_CMD!" auth status >nul 2>nul
if errorlevel 1 (
    echo ERROR: gh no esta autenticado. Corre 'gh auth login' con tu usuario.
    exit /b 1
)
echo OK: GitHub CLI autenticado.

REM La identidad importa: GitHub atribuye el tag por email, no por nombre. Un
REM email viejo deja el commit del tag fuera del grafico de contribuciones de la
REM cuenta buena, y la historia no se reescribe. Ver la seccion "GitHub, autoria
REM y menciones" de las reglas del repo.
set "GIT_NAME="
set "GIT_EMAIL="
for /f "usebackq delims=" %%V in (`git -C "%REPO_ROOT%" config user.name 2^>nul`) do set "GIT_NAME=%%V"
for /f "usebackq delims=" %%V in (`git -C "%REPO_ROOT%" config user.email 2^>nul`) do set "GIT_EMAIL=%%V"
if "!GIT_EMAIL!"=="" (
    echo ERROR: git user.email no esta configurado.
    exit /b 1
)
echo Identidad git: !GIT_NAME! ^<!GIT_EMAIL!^>

REM El working tree limpio no es cosmetico: el tag apunta a HEAD, asi que con
REM cambios sin commitear el release queda apuntando a un commit que NO es lo
REM que se empaqueto.
set "TREE_DIRTY=false"
for /f "usebackq delims=" %%S in (`git -C "%REPO_ROOT%" status --porcelain 2^>nul`) do set "TREE_DIRTY=true"
if "!TREE_DIRTY!"=="true" if /I not "%DRY_RUN%"=="true" (
    echo ERROR: working tree con cambios. Commit/stash antes de publicar.
    git -C "%REPO_ROOT%" status --short
    exit /b 1
)

set "CURRENT_BRANCH="
for /f "usebackq delims=" %%B in (`git -C "%REPO_ROOT%" rev-parse --abbrev-ref HEAD 2^>nul`) do set "CURRENT_BRANCH=%%B"
if /I not "!CURRENT_BRANCH!"=="main" (
    echo ERROR: para publicar el release hay que estar en main. Branch actual: !CURRENT_BRANCH!
    exit /b 1
)
echo OK: branch main.

REM Que VERSION, CMakeLists.txt y el ChangeLog digan lo mismo. Si no coinciden,
REM el asset se llamaria distinto de lo que la app muestra en su Help.
if not exist "%SYNC_VERSION_BAT%" (
    echo ERROR: No existe %SYNC_VERSION_BAT%
    exit /b 1
)
call "%SYNC_VERSION_BAT%" --check-only
if errorlevel 1 (
    echo ERROR: la version esta desincronizada. Corre sync_version y commitea antes de publicar.
    exit /b 1
)
echo OK: superficies de version sincronizadas.

set "APP_VERSION="
if not exist "%REPO_ROOT%\VERSION" (
    echo ERROR: No existe %REPO_ROOT%\VERSION
    exit /b 1
)
REM `for /f` sobre el archivo y no `set /p`: los dos sacan el CR de un VERSION en
REM CRLF, pero el `for /f` ademas no depende del codepage. Con `chcp 65001` puesto
REM arriba, `set /p` con la entrada redirigida devuelve vacio, asi que leer el
REM archivo con la forma que no toca stdin saca del medio esa clase de sorpresa.
for /f "usebackq delims=" %%V in ("%REPO_ROOT%\VERSION") do set "APP_VERSION=%%V"
echo !APP_VERSION!| findstr /R /C:"^[0-9][0-9]*\.[0-9][0-9.]*$" >nul
if errorlevel 1 (
    echo ERROR: VERSION invalida: !APP_VERSION!
    exit /b 1
)

set "TAG=v!APP_VERSION!"
set "ZIP_NAME=%ARTIFACT_NAME%_v!APP_VERSION!_win.zip"
set "ZIP_PATH=%OUTPUT_DIR%\!ZIP_NAME!"

echo Version detectada: !APP_VERSION!
echo Tag objetivo:      !TAG!
echo Repo de releases:  %RELEASE_REPO%
if /I "%DRY_RUN%"=="true" echo Modo:              DRY RUN ^(sin cambios remotos^)
echo.

REM Los tres scripts del instalador de Nuke salen del deposito de release y NO
REM de este repo: una copia propia es una segunda implementacion del mismo
REM instalador que se desincroniza. Los nombres son fijos, son los que busca el
REM card de LGA Updates de PipeSync.
for %%F in ("%NUKE_INSTALLER_DIR%\installer_win.bat" "%NUKE_INSTALLER_DIR%\i_win_engine.ps1" "%COMMON_INSTALLER_DIR%\i_win_plugin_engine.ps1") do (
    if not exist "%%~F" (
        echo ERROR: falta %%~F
        echo        El zip lo necesita para que PipeSync pueda instalar el componente
        echo        de Nuke desde su card. Clona/actualiza el repo _LGA_Release al lado.
        exit /b 1
    )
)
echo OK: instalador del plugin Nuke encontrado en el deposito de release.

REM Los instructivos van adentro del zip, igual que en el de macOS.
for %%L in (es en) do (
    if not exist "%REPO_ROOT%\install_%%L.pdf" (
        echo ERROR: falta %REPO_ROOT%\install_%%L.pdf
        echo        Se genera con: node "%NUKE_DIR%\_LGA_Release\Installers\docs\build_install_pdfs.mjs" --install
        exit /b 1
    )
)
echo OK: instructivos install_es.pdf / install_en.pdf encontrados.
echo.

REM ------------------------------------------------------- instalador / setup

set "SETUP_EXISTS=false"
if exist "%SETUP_EXE%" set "SETUP_EXISTS=true"

REM Cadena de decision con etiquetas y no con `if/else if` anidados: en cmd un
REM bloque entre parentesis se expande entero al parsearlo, y con las variables
REM demoradas de adentro es donde se rompe primero.
if /I "%INSTALLER_MODE%"=="rebuild" goto :installer_rebuild
if /I "%INSTALLER_MODE%"=="use-existing" goto :installer_use_existing
if "!SETUP_EXISTS!"=="true" goto :installer_ask
echo No hay instalador para la version !APP_VERSION!. Generando uno nuevo...
call :run_installer
if errorlevel 1 exit /b 1
goto :installer_ready

:installer_rebuild
echo Regenerando el instalador por --rebuild-installer...
call :run_installer
if errorlevel 1 exit /b 1
goto :installer_ready

:installer_use_existing
if not "!SETUP_EXISTS!"=="true" (
    echo ERROR: --use-existing-installer requiere %SETUP_EXE%
    exit /b 1
)
echo Usando el instalador existente.
goto :installer_ready

:installer_ask
echo Instalador existente detectado: %SETUP_EXE%
REM La respuesta por defecto es regenerar: el unico criterio para reusarlo seria
REM que el archivo este ahi, y nada lo compara contra el HEAD que se acaba de
REM verificar limpio. Reusar sin mirar puede publicar un artefacto construido
REM desde otro commit de la misma version.
call :confirm "Usar este instalador existente?" no
if errorlevel 1 (
    call :run_installer
    if errorlevel 1 exit /b 1
) else (
    echo Usando el instalador existente.
)

:installer_ready

if not exist "%SETUP_EXE%" (
    echo ERROR: no se encontro el instalador esperado: %SETUP_EXE%
    exit /b 1
)

REM Que el Setup.exe sea de ESTA version. En macOS el nombre del artefacto lleva
REM la version, asi que un artefacto viejo no puede colarse; el Setup.exe de
REM Windows tiene nombre fijo y installer_output\ no se limpia nunca. Sin este
REM chequeo, reusar el instalador -con --use-existing-installer, o con --yes que
REM contesta que si a la pregunta de reusarlo- publica el
REM LGA_OpenInNukeX_v<nueva>_win.zip con el setup de la version anterior adentro,
REM y todos los preflight pasan igual. La ProgramVersion sale de MyAppVersion del
REM installer.iss, que sync_version mantiene atado a VERSION.
set "SETUP_VERSION="
for /f "usebackq delims=" %%V in (`powershell -NoProfile -Command "(Get-Item '%SETUP_EXE%').VersionInfo.ProductVersion.Trim()"`) do set "SETUP_VERSION=%%V"
if not "!SETUP_VERSION!"=="!APP_VERSION!" (
    echo ERROR: el instalador dice ser v!SETUP_VERSION! y el repo esta en v!APP_VERSION!.
    echo        Es un Setup.exe de otra version. Regeneralo con --rebuild-installer.
    exit /b 1
)
echo OK: el instalador es de la version !APP_VERSION!.

REM El payload del zip se toma del DEPLOY y no del repo: asi lo que se publica
REM es exactamente lo que la app instala, y no dos copias que puedan divergir.
for %%F in (init.py LGA_QtAdapter_OpenInNukeX.py) do (
    if not exist "%DEPLOY_DIR%\bridge\%%F" (
        echo ERROR: falta %DEPLOY_DIR%\bridge\%%F
        echo        Corre deploy.bat: sin el payload del bridge el zip publicado
        echo        no puede instalar el componente de Nuke.
        exit /b 1
    )
)

REM --------------------------------------------------------------- armar zip

echo.
echo Armando !ZIP_NAME!...

set "STAGE_DIR=%TEMP%\%ARTIFACT_NAME%_win_release"
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
mkdir "%STAGE_DIR%" || goto :stage_error
mkdir "%STAGE_DIR%\%PLUGIN_FOLDER%" || goto :stage_error

copy /Y "%REPO_ROOT%\install_es.pdf" "%STAGE_DIR%\install_es.pdf" >nul || goto :stage_error
copy /Y "%REPO_ROOT%\install_en.pdf" "%STAGE_DIR%\install_en.pdf" >nul || goto :stage_error
copy /Y "%SETUP_EXE%" "%STAGE_DIR%\LGA_OpenInNukeX_Setup.exe" >nul || goto :stage_error
copy /Y "%DEPLOY_DIR%\bridge\init.py" "%STAGE_DIR%\%PLUGIN_FOLDER%\init.py" >nul || goto :stage_error
copy /Y "%DEPLOY_DIR%\bridge\LGA_QtAdapter_OpenInNukeX.py" "%STAGE_DIR%\%PLUGIN_FOLDER%\LGA_QtAdapter_OpenInNukeX.py" >nul || goto :stage_error
copy /Y "%NUKE_INSTALLER_DIR%\installer_win.bat" "%STAGE_DIR%\installer_win.bat" >nul || goto :stage_error
copy /Y "%NUKE_INSTALLER_DIR%\i_win_engine.ps1" "%STAGE_DIR%\i_win_engine.ps1" >nul || goto :stage_error
copy /Y "%COMMON_INSTALLER_DIR%\i_win_plugin_engine.ps1" "%STAGE_DIR%\i_win_plugin_engine.ps1" >nul || goto :stage_error

REM VERSION viaja dentro del pack para que quede en la instalacion: es la fuente
REM de la version instalada y sentinela obligatoria del instalador. Se escribe y
REM no se copia, asi el archivo del zip queda sin BOM y con una sola linea.
REM
REM El fin de linea es LF explicito, no [Environment]::NewLine: los assets que
REM se vienen publicando -y el zip de macOS, que usa printf- llevan LF, y este
REM archivo se lee tal cual del otro lado. Que el de Windows salga con CRLF es
REM una diferencia gratuita entre dos artefactos que deberian ser identicos.
powershell -NoProfile -Command "[System.IO.File]::WriteAllText('%STAGE_DIR%\%PLUGIN_FOLDER%\VERSION', '!APP_VERSION!' + [char]10, (New-Object System.Text.UTF8Encoding($false)))"
if errorlevel 1 goto :stage_error

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if exist "!ZIP_PATH!" del /q "!ZIP_PATH!"

"%SEVENZIP%" a -tzip "!ZIP_PATH!" "%STAGE_DIR%\*" >nul
if errorlevel 1 (
    echo ERROR: 7-Zip fallo al crear el zip.
    rmdir /s /q "%STAGE_DIR%" 2>nul
    exit /b 1
)
rmdir /s /q "%STAGE_DIR%" 2>nul
echo OK: zip creado en !ZIP_PATH!

REM ------------------------------------------------- verificacion del artefacto
REM
REM Sobre el ARTEFACTO y no sobre el staging: el error clasico es que falte una
REM pieza que solo se nota del otro lado del empaquetado, cuando el usuario
REM aprieta INSTALL y no pasa nada.
echo.
echo Verificando el zip antes de publicar...
set "ZIP_LIST=%TEMP%\%ARTIFACT_NAME%_win_ziplist.txt"
"%SEVENZIP%" l -ba "!ZIP_PATH!" > "!ZIP_LIST!" 2>nul
if errorlevel 1 (
    echo ERROR: no se pudo listar el contenido del zip.
    del /q "!ZIP_LIST!" 2>nul
    exit /b 1
)
set "ZIP_OK=true"
for %%E in (LGA_OpenInNukeX_Setup.exe installer_win.bat i_win_engine.ps1 i_win_plugin_engine.ps1 install_es.pdf install_en.pdf) do (
    findstr /I /C:"%%E" "!ZIP_LIST!" >nul || (
        echo ERROR: al zip le falta %%E
        set "ZIP_OK=false"
    )
)
for %%E in (init.py LGA_QtAdapter_OpenInNukeX.py VERSION) do (
    findstr /I /C:"%PLUGIN_FOLDER%\%%E" "!ZIP_LIST!" >nul || (
        echo ERROR: al zip le falta %PLUGIN_FOLDER%\%%E
        set "ZIP_OK=false"
    )
)
del /q "!ZIP_LIST!" 2>nul
if not "!ZIP_OK!"=="true" (
    echo El zip se descarta por fallar la verificacion.
    del /q "!ZIP_PATH!" 2>nul
    exit /b 1
)
echo OK: zip verificado ^(setup, instalador de Nuke, payload e instructivos presentes^).
echo.

REM ------------------------------------------------------------- publicacion

set "REMOTE_RELEASE_EXISTS=false"
"!GH_CMD!" release view "!TAG!" --repo "%RELEASE_REPO%" >nul 2>nul
if not errorlevel 1 set "REMOTE_RELEASE_EXISTS=true"

if "!REMOTE_RELEASE_EXISTS!"=="false" goto :create_release

echo Release existente: !TAG!

REM La lista de assets se vuelca a un archivo y el `for /f` lee el ARCHIVO. No es
REM un rodeo: `for /f` corre el comando con `cmd /c <string>`, y cuando el string
REM empieza con comilla y tiene mas de dos, `cmd /c` come la primera y la ultima.
REM Con GH_CMD entrecomillado -obligatorio, la ruta de instalacion tiene un
REM espacio- el ejecutable quedaba como `gh"` y el comando no corria nunca. Con
REM el `2>nul` tragandose el error, el bucle salia vacio en silencio y
REM ASSET_EXISTS quedaba SIEMPRE en false: el asset ya publicado se pisaba con
REM --clobber sin preguntar, y la pregunta de reemplazo era codigo muerto.
set "ASSET_LIST=%TEMP%\%ARTIFACT_NAME%_win_assets.txt"
"!GH_CMD!" release view "!TAG!" --repo "%RELEASE_REPO%" --json assets -q ".assets[].name" > "!ASSET_LIST!" 2>nul
set "ASSET_EXISTS=false"
if exist "!ASSET_LIST!" (
    for /f "usebackq delims=" %%A in ("!ASSET_LIST!") do (
        if /I "%%A"=="!ZIP_NAME!" set "ASSET_EXISTS=true"
    )
    del /q "!ASSET_LIST!" 2>nul
)
if "!ASSET_EXISTS!"=="false" (
    echo La release no tiene '!ZIP_NAME!'. Se sube.
    goto :upload_asset
)

echo AVISO: el asset '!ZIP_NAME!' ya existe en la release !TAG!.
if /I "%DRY_RUN%"=="true" (
    echo [DRY RUN] Se preguntaria si reemplazar '!ZIP_NAME!'.
    goto :nothing_published
)
call :confirm "Reemplazar '!ZIP_NAME!'?" yes
if errorlevel 1 (
    REM Etiqueta propia y no `:done`: ahi se anuncia "Release Windows lista" y se
    REM dispara el refresco del manifiesto. Decir eso despues de no haber subido
    REM nada es contar lo que no paso.
    echo Saltando '!ZIP_NAME!'. No se publico nada.
    goto :nothing_published
)

:upload_asset
if /I "%DRY_RUN%"=="true" (
    echo [DRY RUN] gh release upload "!TAG!" "!ZIP_PATH!" --repo "%RELEASE_REPO%" --clobber
    goto :done
)
echo Subiendo '!ZIP_NAME!'...
"!GH_CMD!" release upload "!TAG!" "!ZIP_PATH!" --repo "%RELEASE_REPO%" --clobber
if errorlevel 1 (
    echo ERROR: no se pudo subir el asset a la release existente.
    exit /b 1
)
goto :done

:create_release
call :ensure_tag
if errorlevel 1 exit /b 1
echo Creando release !TAG! en %RELEASE_REPO%...
if /I "%DRY_RUN%"=="true" (
    echo [DRY RUN] gh release create "!TAG!" "!ZIP_PATH!" --repo "%RELEASE_REPO%"
    goto :done
)
"!GH_CMD!" release create "!TAG!" "!ZIP_PATH!" --repo "%RELEASE_REPO%" --target "main" --title "!TAG!" --notes "Release !TAG!"
if errorlevel 1 (
    echo ERROR: no se pudo crear la release en GitHub.
    exit /b 1
)

:done
echo.
echo Release Windows lista:
echo https://github.com/%RELEASE_REPO%/releases/tag/!TAG!
echo.

REM Refresco del manifiesto de versiones que consume el card de LGA Updates.
REM Falla en SILENCIO a proposito: la release ya esta publicada y el cron lo
REM levanta solo. El `if exist` tampoco es decorativo: un `call` a un .bat que
REM no esta deja ERRORLEVEL=1, o sea que una publicacion exitosa terminaria
REM devolviendo error.
if /I not "%DRY_RUN%"=="true" (
    if exist "%COMMON_INSTALLER_DIR%\refresh_manifest.bat" (
        call "%COMMON_INSTALLER_DIR%\refresh_manifest.bat" "!GH_CMD!"
    )
)

exit /b 0

:nothing_published
echo.
echo El zip quedo armado en !ZIP_PATH!
exit /b 0

REM ============================================================== subrutinas

:run_installer
REM LGA_SKIP_INSTALLER_RUN_PROMPT corta las dos preguntas finales de
REM instalador.bat: la de ejecutar el setup y la de publicar. Sin eso el
REM instalador volveria a llamar a este script y quedaria un bucle.
set "LGA_SKIP_INSTALLER_RUN_PROMPT=1"
call "%INSTALLER_BAT%"
set "INSTALLER_EXIT=%ERRORLEVEL%"
set "LGA_SKIP_INSTALLER_RUN_PROMPT="
if not "%INSTALLER_EXIT%"=="0" (
    echo ERROR: instalador.bat fallo.
    exit /b 1
)
exit /b 0

:confirm
REM %1 = pregunta, %2 = respuesta por defecto al apretar Enter. Devuelve 0 = si,
REM 1 = no.
REM
REM El texto del prompt sigue al default. Estaba fijo en [Y/n] con el default en
REM "no", asi que en la unica pregunta que lo usa -reusar el instalador- apretar
REM Enter esperando "si" arrancaba una regeneracion completa de Inno Setup.
if /I "%ASSUME_YES%"=="true" exit /b 0
set "HINT=[Y/n]"
if /I "%~2"=="no" set "HINT=[y/N]"
set "ANSWER="
set /p "ANSWER=%~1 !HINT!: "
if "!ANSWER!"=="" (
    if /I "%~2"=="no" exit /b 1
    exit /b 0
)
if /I "!ANSWER!"=="n" exit /b 1
if /I "!ANSWER!"=="no" exit /b 1
exit /b 0

:ensure_tag
REM El tag va en el repo de codigo (origin). Cuando el repo de releases es OTRO,
REM el tag no existe ahi: por eso la release se crea con --target main.
git -C "%REPO_ROOT%" ls-remote --exit-code --tags origin "refs/tags/!TAG!" >nul 2>nul
if not errorlevel 1 (
    echo Tag !TAG! ya existe en origin.
    exit /b 0
)
git -C "%REPO_ROOT%" rev-parse -q --verify "refs/tags/!TAG!" >nul 2>nul
if errorlevel 1 (
    echo Creando tag !TAG!...
    if /I "%DRY_RUN%"=="true" (
        echo [DRY RUN] git tag -a "!TAG!" -m "Release !TAG!"
    ) else (
        git -C "%REPO_ROOT%" tag -a "!TAG!" -m "Release !TAG!"
        if errorlevel 1 (
            echo ERROR: no se pudo crear el tag !TAG!.
            exit /b 1
        )
    )
) else (
    echo Tag local !TAG! ya existe. Haciendo push...
)
if /I "%DRY_RUN%"=="true" (
    echo [DRY RUN] git push origin "!TAG!"
) else (
    git -C "%REPO_ROOT%" push origin "!TAG!"
    if errorlevel 1 (
        echo ERROR: no se pudo hacer push del tag !TAG!.
        exit /b 1
    )
)
exit /b 0

:stage_error
echo ERROR: no se pudieron preparar los archivos para el zip.
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%" 2>nul
exit /b 1

:show_help_error
call :print_help
exit /b 1

:show_help
call :print_help
exit /b 0

:print_help
echo Uso: %~nx0 [--dry-run] [--yes] [--repo owner/repo]
echo          [--use-existing-installer ^| --rebuild-installer]
echo.
echo Publica (o reemplaza) el asset de Windows en la release v^<version^>:
echo   %ARTIFACT_NAME%_v^<version^>_win.zip
echo.
echo Opciones:
echo   --dry-run                  Recorre el flujo sin crear tags ni subir nada
echo   --yes                      No preguntar: reusar y reemplazar
echo   --repo owner/repo          Publicar en otro repo (equivale a LGA_RELEASE_REPO)
echo   --use-existing-installer   Usa el Setup.exe existente
echo   --rebuild-installer        Fuerza instalador.bat antes de publicar
exit /b 0
