#include "lgaregistry.h"
#include "logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

/**
 * En macOS `AppDataLocation` devuelve `~/Library/Application Support/<Org>/<App>` y en Windows
 * `%APPDATA%/<Org>/<App>`. Nosotros queremos el nivel de la ORGANIZACION —`.../LGA`—, que es
 * comun a todas las apps, asi que se sube un nivel desde el de la app.
 */
QString resolveRegistryDir()
{
    const QString appDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDir.isEmpty()) {
        return QString();
    }
    QDir dir(appDir);
    // AppDataLocation ya incluye el nombre de la app; el padre es el de la organizacion.
    // El guard NO es decorativo: Qt saltea los componentes vacios al armar la ruta, asi que una
    // app que setea organizationName pero NO applicationName ya tiene `.../LGA` aca, y el cdUp
    // aterrizaria un nivel mas arriba —fuera del registro— sin error y sin log.
    if (dir.dirName() != QCoreApplication::applicationName() || !dir.cdUp()) {
        return QString();
    }
    return dir.absolutePath();
}

/** Escritura atomica: nadie puede leer un JSON a medio escribir. */
bool writeJson(const QString& filePath, const QJsonObject& obj)
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::logError(QString("LgaRegistry: no se pudo abrir para escribir %1").arg(filePath));
        return false;
    }
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        Logger::logError(QString("LgaRegistry: no se pudo commitear %1").arg(filePath));
        return false;
    }
    return true;
}

QJsonObject readJson(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QJsonObject();
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject();
}

QString nowIso()
{
    // UTC y no hora local: `currentDateTime()` con ISODate no emite offset, asi que dos
    // maquinas en husos distintos producen timestamps que no se pueden comparar.
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

/**
 * La carpeta que CONTIENE la app, que es lo que sirve para ubicarla o relanzarla:
 * en macOS el `.app` completo (el ejecutable vive tres niveles adentro, en
 * `Contents/MacOS/`), en Windows el directorio del `.exe`.
 */
QString resolveInstallPath()
{
    const QString exeDir = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MACOS
    QDir dir(exeDir);
    if (dir.dirName() == QLatin1String("MacOS") && dir.cdUp() && dir.cdUp()) {
        if (dir.dirName().endsWith(QLatin1String(".app"))) {
            return QDir::toNativeSeparators(dir.absolutePath());
        }
    }
#endif
    return QDir::toNativeSeparators(exeDir);
}

} // namespace

namespace LgaRegistry {

QString directory()
{
    const QString dirPath = resolveRegistryDir();
    if (dirPath.isEmpty()) {
        return QString();
    }
    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        Logger::logError(QString("LgaRegistry: no se pudo crear %1").arg(dirPath));
        return QString();
    }
    return dirPath;
}

bool registerThisApp(const QString& appName, const QString& version)
{
    const QString dirPath = directory();
    if (dirPath.isEmpty() || appName.trimmed().isEmpty()) {
        return false;
    }

    QJsonObject obj;
    obj[QStringLiteral("name")] = appName;
    obj[QStringLiteral("version")] = version;
    obj[QStringLiteral("installPath")] = resolveInstallPath();
    obj[QStringLiteral("executable")] =
        QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    obj[QStringLiteral("updatedAt")] = nowIso();

    const QString filePath = QDir(dirPath).filePath(appName + QStringLiteral(".json"));
    const bool ok = writeJson(filePath, obj);
    if (ok) {
        Logger::logInfo(QString("LgaRegistry: registrada %1 v%2 en %3")
                            .arg(appName, version, obj[QStringLiteral("installPath")].toString()));
    }
    return ok;
}

bool saveNukeDirectory(const QString& nukeDir)
{
    const QString dirPath = directory();
    if (dirPath.isEmpty()) {
        return false;
    }
    const QString clean = QDir::cleanPath(nukeDir.trimmed());
    if (clean.isEmpty()) {
        return false;
    }

    const QString filePath = QDir(dirPath).filePath(QStringLiteral("nuke.json"));
    QJsonObject obj = readJson(filePath);
    obj[QStringLiteral("nukeDir")] = QDir::toNativeSeparators(clean);
    obj[QStringLiteral("updatedAt")] = nowIso();

    const bool ok = writeJson(filePath, obj);
    if (ok) {
        Logger::logInfo(QString("LgaRegistry: carpeta .nuke registrada en %1").arg(clean));
    }
    return ok;
}

QString readNukeDirectory()
{
    const QString dirPath = resolveRegistryDir();
    if (dirPath.isEmpty()) {
        return QString();
    }
    const QJsonObject obj = readJson(QDir(dirPath).filePath(QStringLiteral("nuke.json")));
    return obj.value(QStringLiteral("nukeDir")).toString();
}

} // namespace LgaRegistry
