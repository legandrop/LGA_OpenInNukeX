#pragma once

#include <QString>

/**
 * Settings NO sensitivos de la app, en un INI de texto plano (convencion LGA: lo sensitivo
 * iria a un SecureConfig encriptado, y aca no hay nada de eso).
 *
 * Que NO vive aca
 * ---------------
 * - **La carpeta `.nuke`**: es un dato COMPARTIDO entre las apps LGA y su fuente de verdad es
 *   el registro comun (`lgaregistry.h`, `nuke.json`). Guardarla ademas aca crearia dos
 *   escritores para el mismo dato y la copia local ganaria en silencio cuando divergieran.
 * - **La ruta del ejecutable de NukeX**: sigue en `nukeXpath.txt`, que es lo que lee PipeSync
 *   (`UserSettingsTab.cpp`). Mudarla a este INI romperia esa lectura.
 */
namespace AppSettings {

/// "en" o "es". Default "en".
QString language();
void setLanguage(const QString& code);

} // namespace AppSettings
