# El Nuke Bridge

Cómo OpenInNukeX instala su componente Python adentro de Nuke, y con qué contratos externos
tiene que cumplir esa instalación.

---

## Qué es

La app sabe abrir un `.nk` lanzando NukeX de cero. Eso no alcanza cuando el artista **ya
tiene una sesión abierta**: lo que quiere es que el script se abra ahí, no que arranque un
segundo Nuke.

El bridge es un servidor chico que `init.py` levanta dentro de NukeX (puerto 54325) y al que
la app le manda el path. Para que exista hacen falta dos cosas:

1. Los `.py` en `<.nuke>/LGA_OpenInNukeX/`.
2. Una línea en el `init.py` de `<.nuke>` que agregue esa carpeta al plugin path.

Eso es todo lo que hace la instalación, la haga el botón INSTALL de la app
(`NukeBridge::install`), el `installer_mac.sh` del release, o el usuario a mano con el panel
desplegable.

## Los tres caminos, y por qué son tres

| Camino | Quién lo usa | Código |
|---|---|---|
| Botón **INSTALL** | el usuario, desde la ventana de config | `src/nukebridge.cpp` |
| `installer_mac.sh` | **PipeSync**, desde su card de LGA Updates | `QtClient/installer_mac.sh` |
| Panel manual | el usuario cuando los otros dos no aplican (permisos, carpeta rara, red) | export + tres pasos |

Los tres hacen lo mismo y **ninguno reescribe el `init.py` del usuario**: solo appendean su
línea, y solo si no está. Ese archivo puede tener toda la configuración de Nuke de esa
persona.

## Los contratos que NO se pueden cambiar de este lado

Son cuatro, y los cuatro los consume otro repo. Romper cualquiera falla del otro lado, sin
que nada avise acá.

**1. La carpeta se llama `LGA_OpenInNukeX`.** Con guiones bajos, aunque la app se llame `LGA
OpenInNukeX`. No es un nombre de producto: el propio `init.py` recorre `nuke.pluginPath()`
buscando un componente que se llame exactamente así para ubicarse (`_get_plugin_root`), y el
catálogo de PipeSync lo tiene hardcodeado.

**2. El `VERSION` va en `<.nuke>/LGA_OpenInNukeX/VERSION`, texto plano.** Es lo que lee
`UpdateProbe` de PipeSync para saber qué versión hay instalada
(`buildLocalVersionFile("LGA_OpenInNukeX/VERSION")`). Lo escribe el instalador tomando el
número de la macro del CMake, **no** se copia un archivo del payload: la fuente de verdad es
`project(... VERSION ...)`, y un `VERSION` copiado puede quedar atrasado.

**3. El motor del release se llama `installer_mac.sh`.** El `UpdateRunner::locateEngine()` de
PipeSync busca ese nombre exacto, recursivamente, dentro del zip extraído, y lo corre con
`/bin/bash <ruta> --non-interactive --nuke-dir <carpeta>`. En Windows el nombre que busca es
`i_win_engine.ps1`.

**4. El asset del release se llama `LGA_OpenInNukeX_v<version>_mac.zip`.** Es el patrón que
declara `UpdateCatalog.cpp` de PipeSync. No sigue la convención genérica
`<App>_Mac_v<version>.zip` del template, y es a propósito: la convención dice "el nombre que
busca el consumidor", y para esta app el consumidor es PipeSync y no `FM_UpdateService`.

## La forma del zip

Igual que la del `_win.zip` que ya se publicaba: el release lleva **la app y el componente de
Nuke juntos**, porque hay un solo release para las dos cosas.

```
LGA_OpenInNukeX/        init.py + LGA_QtAdapter_OpenInNukeX.py + VERSION
installer_mac.sh        el entry point que invoca PipeSync
LGA OpenInNukeX.app     la aplicación, firmada ad-hoc
```

El payload del zip se toma **del bundle ya firmado**, no del repo: así lo que se publica es
exactamente lo que la app instala, y no dos copias que puedan divergir.

## De dónde saca la app los `.py`

De adentro del artefacto: `Contents/Resources/bridge/` en macOS, `bridge/` al lado del `.exe`
en Windows. Los copia ahí el `CMakeLists.txt` desde la raíz del repo.

**No se leen del repo en runtime.** La máquina del artista no tiene el repo al lado — es el
error obvio, y el que hace que todo funcione perfecto en la máquina de desarrollo.

Los dos scripts de deploy **cortan** si el payload no está adentro del artefacto. Sin ese
chequeo, el fallo aparece recién cuando un usuario aprieta INSTALL y no pasa nada.

## El guard del repo fuente

`install()` se niega a instalar si el destino contiene `QtClient/CMakeLists.txt`.

El motivo es específico de esta app y no es hipotético: **el repo vive en
`~/.nuke/LGA_OpenInNukeX/`**, que es exactamente la carpeta destino. En una máquina de
desarrollo, apretar INSTALL con `~/.nuke` en el campo pisaría los `.py` fuente con la copia
empaquetada, y se perdería todo lo que estuviera sin commitear.

**El chequeo va contra la carpeta destino (`<.nuke>/LGA_OpenInNukeX`), no contra la
`.nuke`.** Hecho sobre la `.nuke` no dispara nunca, porque `~/.nuke/QtClient/` no existe —
ése fue el primer intento, y pasó la revisión de código pero no la prueba real.

## El registro compartido

Cuando la instalación sale bien, `install()` publica la carpeta `.nuke` elegida en el
registro compartido de LGA (`~/Library/Application Support/LGA/nuke.json`), que es de donde
la lee PipeSync. Se escribe **después** de instalar y no antes: registrar una `.nuke` en la
que el bridge no quedó instalado le daría a las otras apps una ruta que no sirve para lo
único que van a hacer con ella.

Ver `../../Desktop/Codin/LGA_Base_QT_C_Py/docs/Doc_Registro_LGA.md`.
