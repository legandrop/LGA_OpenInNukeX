#pragma once

#include <QString>
#include <QStringList>

/**
 * Nuke Bridge: el componente Python que corre DENTRO de NukeX.
 *
 * Que hace y por que existe
 * -------------------------
 * La app sabe abrir un `.nk` lanzando NukeX de cero, pero eso no sirve cuando el artista ya
 * tiene una sesion abierta. El bridge es un servidor chico que `init.py` levanta adentro de
 * NukeX (puerto 54325) y al que la app le manda el path: asi el script se abre en la sesion
 * que ya esta corriendo, en vez de arrancar una segunda.
 *
 * Para eso hacen falta dos cosas, y las dos las hace `install()`:
 *   1. Los `.py` copiados en `<.nuke>/LGA_OpenInNukeX/`.
 *   2. Una linea en el `init.py` de `<.nuke>` que agregue esa carpeta al plugin path.
 *
 * El `VERSION` que se escribe al lado de los `.py` NO es decorativo: es exactamente lo que
 * lee el card de LGA Updates de PipeSync (`UpdateProbe`, `LGA_OpenInNukeX/VERSION`) para
 * saber que version del bridge hay instalada. Cambiar su ubicacion o su formato rompe esa
 * lectura desde el otro lado, sin que nada avise de este lado.
 *
 * La carpeta `.nuke` elegida se publica ademas en el registro compartido de LGA
 * (`lgaregistry.h`), que es de donde la leen las otras apps.
 */
namespace NukeBridge {

/// Nombre de la carpeta del plugin adentro de `.nuke`. **No es configurable**: el propio
/// `init.py` valida que su carpeta se llame asi para ubicarse (`_get_plugin_root`).
QString pluginFolderName();

/// La linea que tiene que estar en el `init.py` de `.nuke`.
QString pluginAddPathLine();

struct Status {
    bool folderPresent = false;   ///< existe `<.nuke>/LGA_OpenInNukeX/`
    bool filesPresent = false;    ///< y adentro estan los `.py` que hacen falta
    bool pathRegistered = false;  ///< el `init.py` de `.nuke` ya tiene la linea
    QString installedVersion;     ///< contenido de `<.nuke>/LGA_OpenInNukeX/VERSION`

    /// Instalado de verdad: los archivos estan Y Nuke los va a cargar.
    bool installed() const { return filesPresent && pathRegistered; }
};

/// `~/.nuke` si existe; string vacio si no.
QString detectNukeDirectory();

/**
 * La carpeta `.nuke` con la que trabaja la app: la del registro compartido si hay una
 * registrada y sigue existiendo, y si no la detectada. Vacio si no hay ninguna.
 */
QString currentNukeDirectory();

/// Estado del bridge en esa carpeta. Con `nukeDir` vacio devuelve todo en false.
Status inspect(const QString& nukeDir);

/// Version del bridge que trae ESTA app (la que instalaria `install()`).
QString bundledVersion();

/**
 * Por que un CODIGO y no un string de error
 * -----------------------------------------
 * El motor no sabe en que idioma esta la ventana, y la app es bilingue con switch en
 * caliente. Si devolviera el texto, el cartel saldria siempre en el idioma que eligio quien
 * escribio esta funcion. El codigo lo traduce la UI contra su tabla; el DETALLE tecnico
 * (rutas, nombres de archivo) va aparte y en castellano, porque su destino es el log.
 */
enum class Error {
    None,
    DirMissing,      ///< la carpeta elegida no existe
    SourceRepo,      ///< es el repo fuente: instalar ahi pisaria los `.py` originales
    PayloadMissing,  ///< el artefacto no trae los `.py` adentro
    WriteFailed,     ///< no se pudo copiar, crear la carpeta o tocar el `init.py`
};

/**
 * Copia los `.py` y el `VERSION` a `<nukeDir>/LGA_OpenInNukeX/`, agrega la linea al
 * `init.py` de `<nukeDir>` si falta, y publica la carpeta en el registro LGA.
 *
 * Nunca reescribe el `init.py` del usuario: solo APPENDEA su linea, y solo si no esta.
 * `detailForLog` recibe el detalle tecnico del fallo (puede ser nullptr).
 */
Error install(const QString& nukeDir, QString* detailForLog);

/**
 * Deja una copia de `LGA_OpenInNukeX/` en `destDir` para que el usuario haga los tres pasos
 * a mano. No toca ningun `init.py` ni el registro.
 */
Error exportPayload(const QString& destDir, QString* detailForLog);

} // namespace NukeBridge
