#include "i18n.h"

#include "appsettings.h"

#include <array>

namespace {

struct Row {
    const char* en;
    const char* es;
};

// El orden TIENE que ser el mismo que el de I18n::Str: la tabla se indexa por el enum, no se
// busca por clave. El static_assert de abajo cubre el olvido de agregar la fila, que es el
// error facil; agregarla en el LUGAR equivocado no lo detecta nadie mas que la vista.
constexpr std::array<Row, static_cast<size_t>(I18n::Str::Count_)> kTable = {{
    // ── descripciones de los tres bloques ────────────────────────────────────
    {"Associate .nk files with OpenInNukeX to open them directly in your preferred "
     "NukeX version.",
     "Asocia los archivos .nk con OpenInNukeX para abrirlos directamente en tu version "
     "preferida de NukeX."},
    {"When no NukeX session is running, .nk files will open using this NukeX version.",
     "Cuando no hay una sesion de NukeX abierta, los .nk se abren con esta version de NukeX."},
    {"Allows OpenInNukeX to detect a running NukeX session and open .nk files directly in it.",
     "Permite que OpenInNukeX detecte una sesion de NukeX abierta y abra los .nk directamente "
     "ahi."},

    // ── estado del bridge ────────────────────────────────────────────────────
    {"Not installed", "Sin instalar"},
    {"Installed · v%1", "Instalado · v%1"},
    {"Update available · v%1", "Hay actualizacion · v%1"},
    {"Installed · unknown version", "Instalado · version desconocida"},

    // ── botones ──────────────────────────────────────────────────────────────
    {"APPLY", "APPLY"},
    {"BROWSE", "BROWSE"},
    {"SAVE", "SAVE"},
    {"INSTALL", "INSTALAR"},
    {"REINSTALL", "REINSTALAR"},
    {"EXPORT BRIDGE FILES…", "EXPORTAR ARCHIVOS…"},
    {"COPY LINE", "COPIAR LINEA"},
    {"COPIED", "COPIADO"},

    // ── bridge: hints y panel manual ─────────────────────────────────────────
    {"Found your Nuke folder at <b>%1</b>. Change it if you use a different one.",
     "Encontramos tu carpeta de Nuke en <b>%1</b>. Cambiala si usas otra."},
    {"No Nuke folder found. Pick the one you use before installing.",
     "No encontramos la carpeta de Nuke. Elegi la que usas antes de instalar."},
    {"Install manually instead…", "Instalar manualmente…"},
    {"Hide manual instructions", "Ocultar las instrucciones"},
    {"Export the bridge files with the button below.",
     "Exporta los archivos del bridge con el boton de abajo."},
    {"Copy the <code>LGA_OpenInNukeX</code> folder into your <code>.nuke</code> folder.",
     "Copia la carpeta <code>LGA_OpenInNukeX</code> dentro de tu carpeta <code>.nuke</code>."},
    {"Add this line to the <code>init.py</code> inside <code>.nuke</code>:",
     "Agrega esta linea al <code>init.py</code> que esta dentro de <code>.nuke</code>:"},

    // ── scanner de versiones ─────────────────────────────────────────────────
    {"Choose one of the found versions or browse your own:",
     "Elegi una de las versiones encontradas o busca la tuya:"},
    {"Scanning for installed Nuke versions…",
     "Buscando versiones de Nuke instaladas…"},
    {"Scanning: %1", "Buscando: %1"},
    {"%1 Nuke versions found:", "%1 versiones de Nuke encontradas:"},
    {"No Nuke installations found in common locations",
     "No se encontro ninguna instalacion de Nuke en las ubicaciones habituales"},

    // ── placeholders y titulos de file dialogs ───────────────────────────────
    {"Path to NukeX executable", "Ruta al ejecutable de NukeX"},
    {"Path to your .nuke folder", "Ruta a tu carpeta .nuke"},
    {"Select NukeX application or binary", "Elegi la aplicacion o el binario de NukeX"},
    {"Select your .nuke folder", "Elegi tu carpeta .nuke"},
    {"Select where to export the bridge files",
     "Elegi donde exportar los archivos del bridge"},

    // ── carteles ─────────────────────────────────────────────────────────────
    {"Error", "Error"},
    {"Warning", "Atencion"},
    {"Choose a NukeX version first.", "Elegi primero una version de NukeX."},
    {"That file no longer exists.", "Ese archivo ya no existe."},
    {"That file does not look like a Nuke executable. Saving it anyway.",
     "Ese archivo no parece un ejecutable de Nuke. Igual se guarda."},
    {"Nuke version saved", "Version de Nuke guardada"},
    {".nk files will open with this NukeX build:<br>%1",
     "Los .nk se van a abrir con esta version de NukeX:<br>%1"},

    {"Nuke Bridge installed", "Nuke Bridge instalado"},
    {"The bridge is in place. <b>Restart NukeX</b> for it to start listening.<br>%1",
     "El bridge quedo instalado. <b>Reinicia NukeX</b> para que empiece a escuchar.<br>%1"},
    {"Could not install the Nuke Bridge", "No se pudo instalar el Nuke Bridge"},
    {"Bridge files exported", "Archivos del bridge exportados"},
    {"Follow the three steps with these files:<br>%1",
     "Segui los tres pasos con estos archivos:<br>%1"},
    {"That folder is the OpenInNukeX source repository, not a <code>.nuke</code> folder. "
     "Installing there would overwrite the source files.",
     "Esa carpeta es el repositorio de OpenInNukeX, no una carpeta <code>.nuke</code>. "
     "Instalar ahi pisaria los archivos fuente."},
    {"That folder does not exist.", "Esa carpeta no existe."},
    {"This copy of OpenInNukeX does not carry the bridge files. The build is incomplete — "
     "download the app again.",
     "Esta copia de OpenInNukeX no trae los archivos del bridge. El artefacto quedo "
     "incompleto: volve a descargar la app."},
    {"Could not write to that folder. Check that you have permission on it.",
     "No se pudo escribir en esa carpeta. Fijate que tengas permiso sobre ella."},

    {"Association completed", "Asociacion completada"},
    {"Double-clicking a .nk file now opens it with OpenInNukeX.",
     "Ahora al hacer doble click en un .nk se abre con OpenInNukeX."},
    {"Almost done", "Casi listo"},
    {"The app is registered with your system. To finish, right-click any .nk file in Finder, "
     "choose <b>Get Info</b>, pick OpenInNukeX under <b>Open with</b> and click "
     "<b>Change All</b>.<br><br>Installing <code>duti</code> "
     "(<code>brew install duti</code>) automates this step.",
     "La app quedo registrada en el sistema. Para terminar, boton derecho sobre cualquier .nk "
     "en Finder, <b>Get Info</b>, elegi OpenInNukeX en <b>Open with</b> y toca "
     "<b>Change All</b>.<br><br>Instalando <code>duti</code> "
     "(<code>brew install duti</code>) este paso se automatiza."},
    {"Association finished with warnings", "La asociacion termino con advertencias"},
}};

static_assert(kTable.size() == static_cast<size_t>(I18n::Str::Count_),
              "Falta (o sobra) una fila en kTable: la tabla se indexa por I18n::Str.");

I18n::Lang g_lang = I18n::Lang::En;
bool g_loaded = false;

} // namespace

namespace I18n {

Lang lang()
{
    if (!g_loaded) {
        g_lang = AppSettings::language() == QLatin1String("es") ? Lang::Es : Lang::En;
        g_loaded = true;
    }
    return g_lang;
}

void setLang(Lang value)
{
    g_lang = value;
    g_loaded = true;
    AppSettings::setLanguage(value == Lang::Es ? QStringLiteral("es") : QStringLiteral("en"));
}

QString t(Str id)
{
    const auto index = static_cast<size_t>(id);
    if (index >= kTable.size()) {
        return QString();
    }
    const Row& row = kTable[index];
    return QString::fromUtf8(lang() == Lang::Es ? row.es : row.en);
}

} // namespace I18n
