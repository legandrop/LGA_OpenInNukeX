#include "nukescanner.h"
#include "logger.h"
#include <QStandardPaths>
#include <QRegularExpression>
#include <QApplication>
#include <QTimer>

NukeScanner::NukeScanner(QObject *parent)
    : QObject(parent)
    , workerThread(nullptr)
{
}

void NukeScanner::startScan()
{
    Logger::logInfo("=== INICIANDO ESCANEO DE VERSIONES DE NUKE ===");

    foundVersions.clear();
    emit scanStarted();

    QTimer::singleShot(100, this, &NukeScanner::performScan);
}

void NukeScanner::performScan()
{
    Logger::logInfo("=== INICIANDO ESCANEO COMPLETO DE VERSIONES NUKE ===");

    QStringList searchPaths = getCommonNukePaths();
    Logger::logInfo(QString("Directorios Nuke encontrados para escanear: %1").arg(searchPaths.size()));

    if (searchPaths.isEmpty()) {
        Logger::logInfo("No se encontraron directorios de Nuke para escanear");
        emit scanFinished(foundVersions);
        return;
    }

    for (const QString &path : searchPaths) {
        emit scanProgress(path);
        Logger::logInfo(QString("=== ESCANEANDO: %1 ===").arg(path));

        QList<NukeVersion> versions = scanDirectory(path);
        Logger::logInfo(QString("Ejecutables encontrados en %1: %2").arg(path).arg(versions.size()));

        for (const NukeVersion &version : versions) {
            foundVersions.append(version);
            emit versionFound(version);
            Logger::logInfo(QString("VERSION AGREGADA: %1 -> %2").arg(version.displayName, version.path));
        }
    }

    Logger::logInfo(QString("=== ESCANEO COMPLETADO: %1 versiones totales ===").arg(foundVersions.size()));
    emit scanFinished(foundVersions);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Windows implementation
// ─────────────────────────────────────────────────────────────────────────────
#ifdef Q_OS_WIN

QStringList NukeScanner::getCommonNukePaths()
{
    Logger::logInfo("=== BUSQUEDA WINDOWS: Program Files ===");

    QStringList foundPaths;
    QStringList baseDirs = {
        "C:/Program Files",
        "C:/Program Files (x86)",
        "C:/Program Files/Foundry"
    };

    for (const QString &baseDir : baseDirs) {
        QDir dir(baseDir);
        if (!dir.exists()) continue;

        QFileInfoList subdirs = dir.entryInfoList(QStringList() << "*Nuke*",
                                                  QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &subdir : subdirs) {
            QString nukePath = subdir.absoluteFilePath();
            if (isValidNukeDirectory(nukePath)) {
                foundPaths << nukePath;
                Logger::logInfo(QString("Directorio Nuke valido: %1").arg(nukePath));
            }
        }
    }

    Logger::logInfo(QString("Directorios encontrados: %1").arg(foundPaths.size()));
    return foundPaths;
}

bool NukeScanner::isValidNukeDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) return false;

    QFileInfoList files = dir.entryInfoList(
        QStringList() << "Nuke*.exe" << "nuke*.exe", QDir::Files);

    for (const QFileInfo &file : files) {
        if (isValidNukeExecutable(file.absoluteFilePath()))
            return true;
    }
    return false;
}

QList<NukeVersion> NukeScanner::scanDirectory(const QString &dirPath)
{
    QList<NukeVersion> versions;
    QDir dir(dirPath);
    if (!dir.exists()) return versions;

    QFileInfoList files = dir.entryInfoList(
        QStringList() << "Nuke*.exe" << "nuke*.exe", QDir::Files);

    for (const QFileInfo &fileInfo : files) {
        if (isValidNukeExecutable(fileInfo.absoluteFilePath())) {
            NukeVersion version = parseNukeExecutable(fileInfo.absoluteFilePath());
            if (!version.name.isEmpty())
                versions.append(version);
        }
    }
    return versions;
}

bool NukeScanner::isValidNukeExecutable(const QString &executablePath)
{
    QFileInfo fileInfo(executablePath);
    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isExecutable())
        return false;

    if (fileInfo.suffix().toLower() != "exe")
        return false;

    QString fileName = fileInfo.fileName().toLower();
    if (!fileName.contains("nuke"))
        return false;

    // Filtrar herramientas auxiliares
    if (fileName.contains("crash") || fileName.contains("feedback") ||
        fileName.contains("update") || fileName.contains("uninstall") ||
        fileName.contains("setup") || fileName.contains("install"))
        return false;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  macOS implementation
// ─────────────────────────────────────────────────────────────────────────────
#else // Q_OS_MAC / Unix

QStringList NukeScanner::getCommonNukePaths()
{
    Logger::logInfo("=== BUSQUEDA macOS: /Applications ===");

    QStringList foundBundles;

    // Nuke installs under /Applications, typically as:
    //   /Applications/Nuke15.0v1/Nuke15.0v1.app   (subdirectory + bundle)
    //   /Applications/Nuke15.0v1.app               (direct bundle)
    QStringList baseDirs = { "/Applications" };

    for (const QString &baseDir : baseDirs) {
        QDir dir(baseDir);
        if (!dir.exists()) continue;

        QFileInfoList entries = dir.entryInfoList(
            QStringList() << "Nuke*",
            QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QFileInfo &entry : entries) {
            QString entryPath = entry.absoluteFilePath();

            if (entryPath.endsWith(".app", Qt::CaseInsensitive)) {
                // Direct .app bundle in /Applications
                if (isValidNukeAppBundle(entryPath)) {
                    foundBundles << entryPath;
                    Logger::logInfo(QString("Bundle directo: %1").arg(entryPath));
                }
            } else if (entry.isDir()) {
                // Subdirectory — look for .app bundles inside
                QDir nukeDir(entryPath);
                QFileInfoList appBundles = nukeDir.entryInfoList(
                    QStringList() << "Nuke*.app",
                    QDir::Dirs | QDir::NoDotAndDotDot);

                for (const QFileInfo &bundle : appBundles) {
                    if (isValidNukeAppBundle(bundle.absoluteFilePath())) {
                        foundBundles << bundle.absoluteFilePath();
                        Logger::logInfo(QString("Bundle en subdir: %1").arg(bundle.absoluteFilePath()));
                    }
                }
            }
        }
    }

    Logger::logInfo(QString("Bundles encontrados: %1").arg(foundBundles.size()));
    return foundBundles;
}

bool NukeScanner::isValidNukeAppBundle(const QString &bundlePath)
{
    // Must end in .app and contain Contents/MacOS with a Nuke binary
    if (!bundlePath.endsWith(".app", Qt::CaseInsensitive))
        return false;

    QDir macosDir(bundlePath + "/Contents/MacOS");
    if (!macosDir.exists())
        return false;

    QFileInfoList files = macosDir.entryInfoList(QDir::Files | QDir::Executable);
    for (const QFileInfo &f : files) {
        if (isValidNukeExecutable(f.absoluteFilePath()))
            return true;
    }
    return false;
}

// On macOS, scanDirectory receives a .app bundle path.
// It returns the Nuke binary inside Contents/MacOS.
QList<NukeVersion> NukeScanner::scanDirectory(const QString &bundlePath)
{
    QList<NukeVersion> versions;

    QDir macosDir(bundlePath + "/Contents/MacOS");
    if (!macosDir.exists()) return versions;

    QFileInfoList files = macosDir.entryInfoList(QDir::Files | QDir::Executable);
    for (const QFileInfo &fileInfo : files) {
        if (isValidNukeExecutable(fileInfo.absoluteFilePath())) {
            NukeVersion version = parseNukeExecutable(fileInfo.absoluteFilePath());
            if (!version.name.isEmpty())
                versions.append(version);
        }
    }
    return versions;
}

// On macOS there's no .exe — validate by name, exclude .dylib and other non-executables
bool NukeScanner::isValidNukeExecutable(const QString &executablePath)
{
    QFileInfo fileInfo(executablePath);
    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isExecutable())
        return false;

    // Exclude shared libraries and other non-executable file types
    QString suffix = fileInfo.suffix().toLower();
    if (suffix == "dylib" || suffix == "so" || suffix == "framework" ||
        suffix == "a" || suffix == "o")
        return false;

    QString fileName = fileInfo.fileName().toLower();
    if (!fileName.contains("nuke"))
        return false;

    // Filter auxiliary tools
    if (fileName.contains("crash") || fileName.contains("feedback") ||
        fileName.contains("update") || fileName.contains("uninstall") ||
        fileName.contains("setup") || fileName.contains("install") ||
        fileName.contains("python") || fileName.contains("helper"))
        return false;

    return true;
}

// isValidNukeDirectory is only used on Windows, but stub it for Unix builds
bool NukeScanner::isValidNukeDirectory(const QString &dirPath)
{
    Q_UNUSED(dirPath)
    return false;
}

#endif // Q_OS_WIN / macOS

// ─────────────────────────────────────────────────────────────────────────────
//  Shared: parse version number from path (works on both platforms)
// ─────────────────────────────────────────────────────────────────────────────
NukeVersion NukeScanner::parseNukeExecutable(const QString &executablePath)
{
    NukeVersion version;
    QFileInfo fileInfo(executablePath);

    version.path = executablePath;
    version.name = fileInfo.baseName();

    // Extract version from path, e.g. "15.0v4" from "Nuke15.0v4" or directory name
    QRegularExpression versionRegex(R"((\d+\.\d+(?:v\d+)?))");
    QRegularExpressionMatch match = versionRegex.match(executablePath);

    if (match.hasMatch()) {
        version.version = match.captured(1);
        version.displayName = QString("Nuke %1").arg(version.version);
    } else {
        QString parentDir = fileInfo.dir().dirName();
        QRegularExpressionMatch parentMatch = versionRegex.match(parentDir);
        if (parentMatch.hasMatch()) {
            version.version = parentMatch.captured(1);
            version.displayName = QString("Nuke %1").arg(version.version);
        } else {
            version.version = "Unknown";
            version.displayName = QString("Nuke (%1)").arg(fileInfo.baseName());
        }
    }

    return version;
}
