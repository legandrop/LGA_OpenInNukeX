#include "nukebridge.h"

#include "lgaregistry.h"
#include "logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>

namespace {

/// Los `.py` que forman el bridge. `init.py` es el servidor; el adapter resuelve el binding
/// de Qt que use esa version de Nuke (PySide2 / PySide6).
const QStringList kPayloadFiles = {
    QStringLiteral("init.py"),
    QStringLiteral("LGA_QtAdapter_OpenInNukeX.py"),
};

/**
 * De donde salen los `.py` que se instalan.
 *
 * Van DENTRO del artefacto —`Contents/Resources/bridge` en macOS, `bridge/` al lado del
 * `.exe` en Windows— y no se leen del repo: la app que corre en la maquina del artista no
 * tiene el repo al lado. El CMakeLists los copia ahi en las dos plataformas.
 */
QString payloadDirectory()
{
    const QDir exeDir(QCoreApplication::applicationDirPath());
#ifdef Q_OS_MACOS
    const QString inBundle =
        QDir::cleanPath(exeDir.absoluteFilePath(QStringLiteral("../Resources/bridge")));
    if (QFileInfo::exists(inBundle)) {
        return inBundle;
    }
#endif
    const QString besideExe = exeDir.absoluteFilePath(QStringLiteral("bridge"));
    if (QFileInfo::exists(besideExe)) {
        return besideExe;
    }
    return QString();
}

QString readTrimmedFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

/**
 * Una carpeta que contiene `QtClient/CMakeLists.txt` es el REPO de OpenInNukeX, no una copia
 * instalada del plugin.
 *
 * Distinguirlos importa por donde vive el repo: `~/.nuke/LGA_OpenInNukeX/`, que es
 * EXACTAMENTE la carpeta destino de la instalacion. En una maquina de desarrollo, apretar
 * INSTALL con `~/.nuke` en el campo pisaria los `.py` fuente con la copia empaquetada y se
 * perderia todo lo que estuviera sin commitear.
 *
 * OJO con QUE se le pasa. El chequeo que importa es el de la carpeta DESTINO
 * (`<.nuke>/LGA_OpenInNukeX`): hecho SOLO sobre la `.nuke` no dispara nunca, porque
 * `~/.nuke/QtClient/` no existe — y ese fue el primer intento, que pasó la lectura del código
 * y no la prueba real. Los dos llamadores lo corren igual sobre las dos carpetas, porque en el
 * export el destino lo elige el usuario y bien puede apuntar directo a la raíz del repo.
 */
bool looksLikeSourceRepo(const QString& dirPath)
{
    return QFileInfo::exists(
        QDir(dirPath).filePath(QStringLiteral("QtClient/CMakeLists.txt")));
}

/**
 * Si el `init.py` tiene una linea ACTIVA que agrega la carpeta del plugin al plugin path.
 *
 * Va linea por linea y saltea las comentadas, y eso NO es un detalle: comentar la linea es el
 * gesto natural para desactivar el bridge un rato. Matcheando contra el archivo entero, un
 * `# nuke.pluginAddPath('./LGA_OpenInNukeX')` daba positivo, asi que el chip decia "instalado"
 * y ademas INSTALL informaba exito sin appendear nada — con Nuke sin cargar el plugin y sin
 * forma de darse cuenta desde la UI.
 *
 * Tolerante a comillas simples o dobles, a espacios y a que el path venga con o sin "./": lo
 * que importa es que Nuke vaya a cargar esa carpeta, no como esta escrito.
 *
 * LIMITE CONOCIDO: una linea adentro de un docstring o de un `if False:` cuenta como activa.
 * Detectarlo pedirea parsear Python, y el costo no se justifica para un caso que nadie hace.
 */
bool hasActivePluginPathLine(const QString& initPyContents)
{
    static const QRegularExpression kLineRe(
        QStringLiteral(R"(nuke\s*\.\s*pluginAddPath\s*\(\s*['"][^'"]*LGA_OpenInNukeX['"])"));

    const QStringList lines = initPyContents.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        if (line.trimmed().startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (kLineRe.match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

/// Copia pisando el destino: `QFile::copy` falla si el archivo ya existe.
bool copyOver(const QString& from, const QString& to, QString* errorOut)
{
    if (QFile::exists(to) && !QFile::remove(to)) {
        *errorOut = QStringLiteral("No se pudo reemplazar %1").arg(to);
        return false;
    }
    if (!QFile::copy(from, to)) {
        *errorOut = QStringLiteral("No se pudo copiar %1").arg(QFileInfo(from).fileName());
        return false;
    }
    // Lo copiado de adentro del bundle hereda los permisos del origen, que puede venir de
    // solo lectura. NO es para poder reinstalar —`copyOver` borra el destino primero, y en
    // POSIX borrar depende de los permisos del DIRECTORIO, no del archivo—: es para que el
    // usuario pueda tocar o borrar el `.py` a mano en su propia carpeta `.nuke`.
    if (!QFile(to).setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup
                                  | QFile::ReadOther)) {
        Logger::logInfo(QString("NukeBridge: no se pudieron ajustar los permisos de %1").arg(to));
    }
    return true;
}

/// Si el artefacto trae los `.py` adentro. Se chequea aparte para poder distinguir "el build
/// quedo incompleto" de "no se pudo escribir en la carpeta destino": son dos problemas de
/// quien los tiene que arreglar completamente distintos.
bool payloadPresent()
{
    const QString sourceDir = payloadDirectory();
    if (sourceDir.isEmpty()) {
        return false;
    }
    for (const QString& name : kPayloadFiles) {
        if (!QFileInfo::exists(QDir(sourceDir).filePath(name))) {
            return false;
        }
    }
    return true;
}

/// Copia el payload completo (los `.py` + el `VERSION`) a `targetDir`, que ya tiene que existir.
bool writePayloadInto(const QString& targetDir, QString* errorOut)
{
    const QString sourceDir = payloadDirectory();
    if (sourceDir.isEmpty()) {
        *errorOut = QStringLiteral(
            "Los archivos del bridge no estan en la aplicacion. El artefacto quedo incompleto.");
        return false;
    }

    for (const QString& name : kPayloadFiles) {
        const QString from = QDir(sourceDir).filePath(name);
        if (!QFileInfo::exists(from)) {
            *errorOut = QStringLiteral("Falta %1 dentro de la aplicacion.").arg(name);
            return false;
        }
        if (!copyOver(from, QDir(targetDir).filePath(name), errorOut)) {
            return false;
        }
    }

    // El VERSION se escribe aca y no se copia: la fuente de verdad de la version es la macro
    // del CMake, no un archivo suelto que pueda quedar atrasado dentro del payload.
    QSaveFile versionFile(QDir(targetDir).filePath(QStringLiteral("VERSION")));
    if (!versionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *errorOut = QStringLiteral("No se pudo escribir el VERSION en %1").arg(targetDir);
        return false;
    }
    versionFile.write((NukeBridge::bundledVersion() + QLatin1Char('\n')).toUtf8());
    if (!versionFile.commit()) {
        *errorOut = QStringLiteral("No se pudo escribir el VERSION en %1").arg(targetDir);
        return false;
    }
    return true;
}

/**
 * Agrega la linea al `init.py` de `.nuke` si no esta. **Solo appendea**: el `init.py` del
 * usuario puede tener toda su configuracion de Nuke y reescribirlo seria imperdonable.
 */
bool ensurePluginPathLine(const QString& nukeDir, QString* errorOut)
{
    const QString initPath = QDir(nukeDir).filePath(QStringLiteral("init.py"));

    QString existing;
    if (QFileInfo::exists(initPath)) {
        QFile file(initPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            *errorOut = QStringLiteral("No se pudo leer %1").arg(initPath);
            return false;
        }
        existing = QString::fromUtf8(file.readAll());
    }

    if (hasActivePluginPathLine(existing)) {
        return true;
    }

    QFile file(initPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        *errorOut = QStringLiteral("No se pudo escribir en %1").arg(initPath);
        return false;
    }
    QTextStream out(&file);
    if (!existing.isEmpty() && !existing.endsWith(QLatin1Char('\n'))) {
        out << "\n";
    }
    out << "\n# LGA OpenInNukeX\n" << NukeBridge::pluginAddPathLine() << "\n";
    return true;
}

} // namespace

namespace NukeBridge {

QString pluginFolderName()
{
    return QStringLiteral("LGA_OpenInNukeX");
}

QString pluginAddPathLine()
{
    return QStringLiteral("nuke.pluginAddPath('./%1')").arg(pluginFolderName());
}

QString bundledVersion()
{
    return QString::fromLatin1(OPENINNUKEX_VERSION);
}

QString detectNukeDirectory()
{
    const QString home = QDir::homePath();
    const QString candidate = QDir(home).filePath(QStringLiteral(".nuke"));
    return QFileInfo(candidate).isDir() ? QDir::cleanPath(candidate) : QString();
}

QString currentNukeDirectory()
{
    const QString registered = LgaRegistry::readNukeDirectory();
    if (!registered.isEmpty() && QFileInfo(registered).isDir()) {
        return QDir::cleanPath(QDir::fromNativeSeparators(registered));
    }
    return detectNukeDirectory();
}

Status inspect(const QString& nukeDir)
{
    Status status;
    if (nukeDir.isEmpty() || !QFileInfo(nukeDir).isDir()) {
        return status;
    }

    const QString pluginDir = QDir(nukeDir).filePath(pluginFolderName());
    status.folderPresent = QFileInfo(pluginDir).isDir();
    if (status.folderPresent) {
        status.filesPresent = true;
        for (const QString& name : kPayloadFiles) {
            if (!QFileInfo::exists(QDir(pluginDir).filePath(name))) {
                status.filesPresent = false;
                break;
            }
        }
        status.installedVersion =
            readTrimmedFile(QDir(pluginDir).filePath(QStringLiteral("VERSION")));
    }

    const QString initPath = QDir(nukeDir).filePath(QStringLiteral("init.py"));
    if (QFileInfo::exists(initPath)) {
        status.pathRegistered = hasActivePluginPathLine(readTrimmedFile(initPath));
    }

    return status;
}

Error install(const QString& nukeDir, QString* detailForLog)
{
    QString ignored;
    QString& detail = detailForLog ? *detailForLog : ignored;
    detail.clear();

    const QString clean = QDir::cleanPath(nukeDir.trimmed());
    if (clean.isEmpty() || !QFileInfo(clean).isDir()) {
        detail = QStringLiteral("La carpeta no existe: '%1'").arg(nukeDir);
        return Error::DirMissing;
    }

    const QString pluginDir = QDir(clean).filePath(pluginFolderName());
    if (looksLikeSourceRepo(pluginDir) || looksLikeSourceRepo(clean)) {
        detail = QStringLiteral("Es el repositorio fuente: %1").arg(pluginDir);
        return Error::SourceRepo;
    }

    // El chequeo del payload va ANTES del mkpath. Al reves, un artefacto incompleto dejaba
    // creada una `<.nuke>/LGA_OpenInNukeX/` VACIA y devolvia error igual: Nuke se quedaba con
    // una carpeta vacia en el plugin path y el proximo inspect() la veia como media instalada.
    if (!payloadPresent()) {
        detail = QStringLiteral("El artefacto no trae el payload del bridge.");
        return Error::PayloadMissing;
    }
    if (!QDir().mkpath(pluginDir)) {
        detail = QStringLiteral("No se pudo crear %1").arg(pluginDir);
        return Error::WriteFailed;
    }
    if (!writePayloadInto(pluginDir, &detail)) {
        return Error::WriteFailed;
    }
    if (!ensurePluginPathLine(clean, &detail)) {
        return Error::WriteFailed;
    }

    // Recien cuando la instalacion salio bien se publica la carpeta: registrar una `.nuke`
    // en la que el bridge no quedo instalado le daria a las otras apps LGA una ruta que no
    // sirve para lo unico que van a hacer con ella.
    LgaRegistry::saveNukeDirectory(clean);

    Logger::logInfo(QString("NukeBridge: instalado v%1 en %2").arg(bundledVersion(), pluginDir));
    return Error::None;
}

Error exportPayload(const QString& destDir, QString* detailForLog)
{
    QString ignored;
    QString& detail = detailForLog ? *detailForLog : ignored;
    detail.clear();

    const QString clean = QDir::cleanPath(destDir.trimmed());
    if (clean.isEmpty() || !QFileInfo(clean).isDir()) {
        detail = QStringLiteral("La carpeta no existe: '%1'").arg(destDir);
        return Error::DirMissing;
    }

    const QString targetDir = QDir(clean).filePath(pluginFolderName());
    if (looksLikeSourceRepo(targetDir) || looksLikeSourceRepo(clean)) {
        detail = QStringLiteral("Es el repositorio fuente: %1").arg(targetDir);
        return Error::SourceRepo;
    }
    if (!payloadPresent()) {
        detail = QStringLiteral("El artefacto no trae el payload del bridge.");
        return Error::PayloadMissing;
    }
    if (!QDir().mkpath(targetDir)) {
        detail = QStringLiteral("No se pudo crear %1").arg(targetDir);
        return Error::WriteFailed;
    }
    if (!writePayloadInto(targetDir, &detail)) {
        return Error::WriteFailed;
    }

    Logger::logInfo(QString("NukeBridge: exportado a %1").arg(targetDir));
    return Error::None;
}

} // namespace NukeBridge
