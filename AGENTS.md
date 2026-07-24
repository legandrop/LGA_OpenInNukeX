# Instrucciones para LGA_OpenInNukeX

- Todas las reglas de este repo (`AGENTS.md`, `.cursor/rules/`) deben estar escritas en castellano.
- Este archivo es **espejo** de `.cursor/rules/instructions.mdc`. Al modificar acá, sincronizar tambien el `.mdc` — y viceversa.

## Objetivo del repo

- App LGA de dos partes:
  - **Servidor Python** en NukeX: `init.py` + `LGA_QtAdapter_OpenInNukeX.py`. Escucha TCP en `localhost:54325`, recibe comandos externos (`ping`, `run_script||<path>`, `paste_clipboard`).
  - **Cliente Qt/C++** en `QtClient/`: administra la asociacion de archivos `.nk`, detecta la ruta de NukeX y decide si mandar el `.nk` a una instancia abierta o lanzar una nueva.
- El repo tiene su propio `.git` aunque viva dentro de `~/.nuke/` (que tambien es su propio repo). Trabajar SIEMPRE con `~/.nuke/LGA_OpenInNukeX/` como raiz.
- Es la unica subrepo con Qt/C++ dentro de `~/.nuke/`; el resto de los subdirs (LGA_ToolPack, LGA_NodePack, etc.) son plugins Python puros.

## Layout del workspace

Esta app NO vive paralela al Base — vive en `~/.nuke/LGA_OpenInNukeX/` por requisito de Nuke (asi lo lee como plugin). El repo Base sigue la convencion cross-platform estandar:

| Plataforma | Ubicacion del Base |
|---|---|
| macOS | `~/Desktop/Codin/LGA_Base_QT_C_Py` |
| Windows | `C:/portable/LGA_Base_QT_C_Py` |

Las referencias al Base en las reglas de abajo usan path absoluto para esa ubicacion estandar (asumida en cualquier maquina LGA).

## Hub de aprendizajes Qt/C++ (fuente de verdad compartida)

- `~/Desktop/Codin/LGA_Base_QT_C_Py/docs/_Doc_Aprendizaje_QT_C.md` (mac) / `C:/portable/LGA_Base_QT_C_Py/docs/_Doc_Aprendizaje_QT_C.md` (win) es el hub central de aprendizajes Qt/C++ para todas las apps LGA. Tiene un INDICE al inicio, agrupado por topico (Painting, macOS especifico, Rich Text, Shortcuts, Threading, Layouts, Tooltips).
- **Antes de tocar codigo** en cualquiera de las areas listadas en el indice: **leer el INDICE del hub** (~30 segundos). Si el topico esta listado, ir a la seccion; si no esta, seguir con la tarea.
- **Escribir una entrada nueva** en el hub cuando un problema Qt/C++ cumple LAS TRES condiciones:
  1. Requirio 3+ iteraciones para resolver (o la causa raiz no era obvia).
  2. La solucion no aparece en la doc oficial de Qt, o es una trampa facil de caer.
  3. Es probable que reaparezca en otra app LGA.
- Fixes triviales NO van al hub. Al agregar una entrada, agregar TAMBIEN su linea al INDICE en la misma pasada.
- **Ejemplo concreto**: el `CMakeLists.txt` de esta app ya usa el bloque de purga de AGL para macOS 12+; ese aprendizaje sale del hub y esta bien tenerlo replicado aqui.

## Docs unificadas del Base

Fuente de verdad para las convenciones LGA:

- `~/Desktop/Codin/LGA_Base_QT_C_Py/docs/_Doc_Aprendizaje_QT_C.md` — hub Qt/C++.
- `~/Desktop/Codin/LGA_Base_QT_C_Py/docs/Doc_CustomTooltip.md` — sistema de tooltips (no usado aun en OpenInNukeX; consultar cuando se porten).
- `~/Desktop/Codin/LGA_Base_QT_C_Py/readme.md` — arquitectura del template, `AppSettings` vs `SecureConfig`, layout del workspace cross-platform.

## Build

- **No compilar por defecto.** Compilar SOLO cuando:
  - El usuario lo pide explicitamente ("compila", "probalo", "buildealo", "corre la app").
  - Es fundamental para validar un cambio no trivial (refactor grande, cambio de header publico que afecta otras clases, cambio que puede romper build en otra plataforma).
- Los scripts de build viven en `QtClient/` (no en la raiz del repo — la raiz es el servidor Python que no requiere compilar). Al compilar, usar SIEMPRE el script del repo — NUNCA `cmake`, `ninja`, `make` u otros comandos manuales (los scripts hacen compilar + copiar deps + lanzar el ejecutable en un solo paso):
  - Windows: `cd QtClient && ./compilar_dev.bat`
  - macOS: `cd QtClient && ./compilar_dev.sh`
- No ejecutar manualmente el binario despues de correr los scripts (ellos lo lanzan automaticamente).
- No hacer builds limpios automaticamente. No borrar, vaciar ni recrear `QtClient/build/` salvo pedido explicito. Preservar la cache incremental.
- Si la compilacion falla, intentar corregir el problema SIN limpiar primero.
- El servidor Python (`init.py`, `LGA_QtAdapter_OpenInNukeX.py`) NO se compila — solo se recarga NukeX para probarlo.

## Politica de idioma

Convencion LGA cross-app:

- **Texto visible en UI** (labels, botones, titulos de ventana, mensajes al usuario): siempre en INGLES.
- **Comentarios de codigo y mensajes de log de debug**: siempre en CASTELLANO.
- **Strings de error internas** que no se muestran al usuario final: pueden estar en castellano.

## Versionado y ChangeLog

- Changelog principal: `docs/ChangeLog.md` (raiz del repo, cubre ambas partes — Python y Qt).
- Sincronizacion: el repo tiene los DOS scripts: `sync_version.bat` (auto-bump por entrada de changelog) y `bump_version.bat` (bump manual). Usar el que corresponda segun el patron de la sesion.

### Fuente unica de verdad para el numero de version

- **`QtClient/CMakeLists.txt` es la unica fuente de verdad** del numero de version (via `project(LGA_OpenInNukeX VERSION x.y.z ...)`).
- Expuesta al C++ como `OPENINNUKEX_VERSION` (macro de CMake via `target_compile_definitions`).
- **NO hardcodear la version** en `main.cpp`, `configwindow.cpp`, README o `Info.plist` — la macro CMake ya la propaga.
- La sincronizacion la hace `sync_version.bat`/`.sh` (auto) o `bump_version.bat`/`.sh` (manual) — nunca a mano.
- Registro append-only: **nunca modificar entradas ya agregadas**, incluso las de la misma sesion.
- Al final de cada entrada, sugerencia entre `[ ]` para el commit. Ejemplo: `[ OpenInNukeX - Fix TCP timeout ]`. Linea en blanco despues.
- Si el cambio evoluciona en la misma sesion, agregar OTRA entrada nueva arriba (no editar).

## Help / About tab

Si se agrega un Help tab visible al usuario:

- Version del Help sale de `OPENINNUKEX_VERSION` (macro CMake), NO hardcodear.
- Colores con constantes `k*` al top del `.cpp`, no literales hex sueltos.
- Iconos en `resources/icons/help/` (crear si no existen), declarados en el `.qrc`, renderizados con `QSvgRenderer`.
- Al bumpear version, revisar en la misma tanda que los textos descriptivos del Help sigan coherentes con lo que la app hace.

## Debug logs

- **No** agregar debug flags nuevos ni logs de debug por defecto en cada cambio.
- Crear flags especificos SOLO cuando haga falta diagnosticar un bug importante, un problema real dificil de reproducir o una zona con riesgo tecnico claro.
- Para features normales, cambios triviales o ajustes de UI, NO agregar logs de debug nuevos.
- Errores reales y warnings recuperables deben quedar siempre visibles (no callarse). Si un error impide completar una accion del usuario, mostrar tambien una notificacion breve; el detalle tecnico va al log.
- El servidor Python de NukeX ya loguea con el sistema estandar de Nuke. En el cliente Qt, usar `logger.cpp` (ver `QtClient/src/`).

## Commits

- **NUNCA** hacer commits automaticamente. Solo commitear cuando el usuario lo pide explicitamente.
- Cuando el usuario pide commit, usar la sugerencia entre `[ ]` de la ultima entrada del ChangeLog como base del mensaje.

## GitHub, autoria y menciones

- En GitHub, el unico colaborador/autorizado esperado para este repo es `legandrop`.
- No agregar, invitar, aceptar, configurar ni modificar colaboradores, equipos, bots, apps, deploy keys, permisos de repo, reglas de acceso o cualquier ajuste que pueda hacer aparecer otro colaborador o identidad con acceso al repo.
- Si una herramienta, integracion o flujo pide autenticar, autorizar o agregar otro usuario/app con permisos sobre GitHub, detenerse y consultar al usuario antes de continuar.
- Todos los commits y pushes deben hacerse con la identidad de Git del usuario `legandrop`. No usar coautores, autores alternativos ni footers de atribucion automatica.
- No mencionar herramientas, agentes, modelos de IA ni asistentes automatizados en commits, changelogs, README, documentacion, PRs, issues, releases, comentarios de codigo, logs visibles o cualquier texto versionado del repo.
- Los mensajes de commit deben describir solamente el cambio funcional o documental hecho en la repo, sin referencias a quien o que lo genero.

### Que cuenta como "herramienta AI" (lista NO exhaustiva, ampliar si aparece algo nuevo)

- **Editores AI**: Cursor, Windsurf, Zed (con features AI), Fleet.
- **Extensiones/plugins AI**: GitHub Copilot, Codeium, Continue, Cody, Tabnine, Supermaven, Amazon Q.
- **Agentes CLI / IDE con agente**: Claude Code, Codex, Aider, Plandex, OpenHands, Devin, Roo Cline.
- **Modelos / chatbots**: ChatGPT, Claude, GPT-4, Gemini, LLaMA, "un LLM", "un modelo".
- **El proceso de descubrimiento con AI**: "mientras el agente compilaba", "cuando pedi a la IA que revise", "el modelo sugirio".

Editores NO AI (VS Code puro sin Copilot, IntelliJ, Vim, Emacs, Sublime, nano) se pueden mencionar SOLO si es estrictamente necesario para describir un problema real de esa herramienta especifica. En 99% de los casos no hace falta.

### Como reescribir un mensaje que menciona AI (aunque sea de reojo)

**Regla raiz**: el commit describe el cambio del PROYECTO en sus propios terminos, no del entorno de desarrollo de quien lo hizo. Si sacas el nombre de la herramienta y el mensaje sigue teniendo sentido, ese es el mensaje correcto.

- ❌ Mal: `Fix pkill generico en compilar.sh que reiniciaba extensiones de Cursor`
- ✅ Bien: `Fix pkill generico en compilar.sh (mataba procesos ajenos por match parcial de nombre)`

- ❌ Mal: `Refactor NukeOpener siguiendo sugerencias del review de la IA`
- ✅ Bien: `Refactor NukeOpener: separar deteccion de version de logica de lanzamiento`

Esta regla aplica IGUAL a: commits, entradas del ChangeLog (incluyendo la sugerencia entre `[ ]`), issues, PRs, comentarios en codigo, mensajes de log visibles al usuario, y todo texto versionado del repo.
