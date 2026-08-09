#pragma once

#include <QString>

/**
 * Registro COMPARTIDO entre las apps LGA.
 *
 * Que es y por que existe
 * -----------------------
 * Las apps LGA necesitan saber unas de otras dos cosas: donde esta instalada cada app, y cual
 * es la carpeta `.nuke` del usuario. El card de LGA Updates de PipeSync es el primer consumidor
 * —chequea que hay instalado y que version—, pero el dato no es suyo: lo sabe cada app sobre si
 * misma. Por eso se escribe en un lugar comun, en CLARO, y no en el config privado de nadie.
 *
 * Formato
 * -------
 * Un JSON por app, mas uno para la carpeta de Nuke, todos bajo el mismo directorio:
 *
 *   macOS    ~/Library/Application Support/LGA/
 *   Windows  %APPDATA%/LGA/                        (o sea AppData/Roaming/LGA)
 *
 *   nuke.json          { "nukeDir": "/Users/x/.nuke", "updatedAt": "<ISO-8601>" }
 *   <AppName>.json     { "name": "...", "version": "...", "installPath": "...",
 *                        "executable": "...", "updatedAt": "<ISO-8601>" }
 *
 * `installPath` es la carpeta que CONTIENE la app: en macOS el `.app` (no el binario de adentro),
 * en Windows el directorio de instalacion. Es lo que sirve para ubicar o relanzar la app.
 *
 * Decisiones que conviene no revisar dos veces
 * -------------------------------------------
 * - **En claro, no encriptado.** Son rutas, no credenciales. Un `SecureConfig` acopla a las apps
 *   por una clave y un esquema privados, y si una cambia el formato la otra se rompe en silencio.
 * - **La app se auto-registra al arrancar**, en vez de que lo escriba el instalador. Funciona
 *   igual en las dos plataformas, sobrevive a que el usuario mueva la app de lugar, y no depende
 *   de que el instalador se haya corrido (en macOS directamente no hay instalador).
 * - **Escritura atomica** (archivo temporal + rename): dos apps LGA pueden arrancar a la vez, y
 *   un lector no puede encontrarse un JSON a medio escribir.
 * - **Fallar no molesta al usuario.** Si el registro no se puede escribir, la app sigue andando:
 *   lo unico que se pierde es que otra app la vea. Va al log y nada mas.
 */
namespace LgaRegistry {

/** Directorio del registro. Lo crea si no existe. Vacio si no se pudo resolver. */
QString directory();

/**
 * Registra esta app: nombre visible, version e `installPath` (deducido del ejecutable que
 * corre). Se llama una vez al arrancar. Devuelve false si no se pudo escribir.
 */
bool registerThisApp(const QString& appName, const QString& version);

/** Guarda la carpeta `.nuke` que el usuario eligio, para que la vean las demas apps LGA. */
bool saveNukeDirectory(const QString& nukeDir);

/** Lee la carpeta `.nuke` registrada. String vacio si no hay ninguna. */
QString readNukeDirectory();

} // namespace LgaRegistry
