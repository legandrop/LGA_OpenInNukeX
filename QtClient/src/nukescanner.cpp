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
    
    // Usar QTimer para realizar el escaneo de forma asíncrona sin mover el objeto a otro hilo
    QTimer::singleShot(100, this, &NukeScanner::performScan);
}

void NukeScanner::performScan()
{
    Logger::logInfo("=== INICIANDO ESCANEO COMPLETO DE VERSIONES NUKE ===");
    
    QStringList searchPaths = getCommonNukePaths();
    Logger::logInfo(QString("Directorios Nuke encontrados para escanear: %1").arg(searchPaths.size()));
    
    if (searchPaths.isEmpty()) {
        Logger::logInfo("❌ No se encontraron directorios de Nuke para escanear");
        emit scanFinished(foundVersions);
        return;
    }
    
    for (const QString &path : searchPaths) {
        emit scanProgress(path);
        Logger::logInfo(QString("=== ESCANEANDO DIRECTORIO: %1 ===").arg(path));
        
        QList<NukeVersion> versions = scanDirectory(path);
        Logger::logInfo(QString("Ejecutables encontrados en %1: %2").arg(path).arg(versions.size()));
        
        for (const NukeVersion &version : versions) {
            foundVersions.append(version);
            emit versionFound(version);
            Logger::logInfo(QString("✓ VERSIÓN AGREGADA: %1 -> %2").arg(version.displayName, version.path));
        }
    }
    
    Logger::logInfo(QString("=== ESCANEO COMPLETADO: %1 versiones totales encontradas ===").arg(foundVersions.size()));
    
    // Mostrar resumen de versiones encontradas
    if (foundVersions.isEmpty()) {
        Logger::logInfo("❌ RESULTADO FINAL: No se encontraron versiones válidas de Nuke");
    } else {
        Logger::logInfo("✅ RESULTADO FINAL: Versiones encontradas:");
        for (const NukeVersion &version : foundVersions) {
            Logger::logInfo(QString("  - %1: %2").arg(version.displayName, version.path));
        }
    }
    
    // Emitir señal de finalización
    emit scanFinished(foundVersions);
}

QStringList NukeScanner::getCommonNukePaths()
{
    Logger::logInfo("=== INICIANDO BÚSQUEDA DINÁMICA DE DIRECTORIOS NUKE ===");
    
    QStringList foundPaths;
    
    // Directorios base donde buscar
    QStringList baseDirs = {
        "C:/Program Files",
        "C:/Program Files (x86)",
        "C:/Program Files/Foundry"
    };
    
    for (const QString &baseDir : baseDirs) {
        Logger::logInfo(QString("Escaneando directorio base: %1").arg(baseDir));
        
        QDir dir(baseDir);
        if (!dir.exists()) {
            Logger::logInfo(QString("Directorio no existe: %1").arg(baseDir));
            continue;
        }
        
        // Buscar todas las carpetas que contengan "Nuke"
        QStringList filters;
        filters << "*Nuke*";
        
        QFileInfoList subdirs = dir.entryInfoList(filters, QDir::Dirs | QDir::NoDotAndDotDot);
        
        Logger::logInfo(QString("Encontradas %1 carpetas con 'Nuke' en %2").arg(subdirs.size()).arg(baseDir));
        
        for (const QFileInfo &subdir : subdirs) {
            QString nukePath = subdir.absoluteFilePath();
            Logger::logInfo(QString("Evaluando directorio: %1").arg(nukePath));
            
            // Verificar que realmente parece un directorio de Nuke
            if (isValidNukeDirectory(nukePath)) {
                foundPaths << nukePath;
                Logger::logInfo(QString("✓ Directorio Nuke válido agregado: %1").arg(nukePath));
            } else {
                Logger::logInfo(QString("✗ Directorio no válido (sin ejecutables Nuke): %1").arg(nukePath));
            }
        }
    }
    
    Logger::logInfo(QString("=== BÚSQUEDA COMPLETADA: %1 directorios Nuke encontrados ===").arg(foundPaths.size()));
    
    return foundPaths;
}

bool NukeScanner::isValidNukeDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return false;
    }
    
    // Buscar ejecutables de Nuke en el directorio
    QStringList filters;
    filters << "Nuke*.exe" << "nuke*.exe";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    // Debe tener al menos un ejecutable de Nuke
    for (const QFileInfo &file : files) {
        if (isValidNukeExecutable(file.absoluteFilePath())) {
            return true;
        }
    }
    
    return false;
}

QList<NukeVersion> NukeScanner::scanDirectory(const QString &dirPath)
{
    QList<NukeVersion> versions;
    QDir dir(dirPath);
    
    if (!dir.exists()) {
        return versions;
    }
    
    // Buscar ejecutables de Nuke en el directorio
    QStringList filters;
    filters << "Nuke*.exe" << "nuke*.exe";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    
    for (const QFileInfo &fileInfo : files) {
        if (isValidNukeExecutable(fileInfo.absoluteFilePath())) {
            NukeVersion version = parseNukeExecutable(fileInfo.absoluteFilePath());
            if (!version.name.isEmpty()) {
                versions.append(version);
            }
        }
    }
    
    return versions;
}

NukeVersion NukeScanner::parseNukeExecutable(const QString &executablePath)
{
    NukeVersion version;
    QFileInfo fileInfo(executablePath);
    
    version.path = executablePath;
    version.name = fileInfo.baseName();
    
    // Extraer versión del nombre del archivo o directorio
    QRegularExpression versionRegex(R"((\d+\.\d+(?:v\d+)?))");
    QRegularExpressionMatch match = versionRegex.match(executablePath);
    
    if (match.hasMatch()) {
        version.version = match.captured(1);
        version.displayName = QString("Nuke %1").arg(version.version);
    } else {
        // Si no se puede extraer la versión, usar el nombre del directorio padre
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

bool NukeScanner::isValidNukeExecutable(const QString &executablePath)
{
    QFileInfo fileInfo(executablePath);
    
    // Verificar que es un archivo ejecutable
    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isExecutable()) {
        return false;
    }
    
    QString fileName = fileInfo.fileName().toLower();
    
    // Verificar que es un .exe
    if (fileInfo.suffix().toLower() != "exe") {
        return false;
    }
    
    // Verificar que el nombre contiene "nuke"
    if (!fileName.contains("nuke")) {
        return false;
    }
    
    // FILTRAR archivos que NO son el ejecutable principal de Nuke
    // Excluir archivos como nukeCrashFeedback.exe, etc.
    if (fileName.contains("crash") || 
        fileName.contains("feedback") || 
        fileName.contains("update") || 
        fileName.contains("uninstall") ||
        fileName.contains("setup") ||
        fileName.contains("install")) {
        Logger::logInfo(QString("✗ Ejecutable filtrado (no es Nuke principal): %1").arg(fileName));
        return false;
    }
    
    // Verificar que es un ejecutable principal de Nuke
    // Ejemplos válidos: Nuke.exe, NukeX.exe, Nuke13.1.exe, Nuke15.1.exe, etc.
    if (fileName == "nuke.exe" || 
        fileName == "nukex.exe" ||
        (fileName.startsWith("nuke") && fileName.endsWith(".exe") && !fileName.contains("crash"))) {
        Logger::logInfo(QString("✓ Ejecutable válido de Nuke: %1").arg(fileName));
        return true;
    }
    
    Logger::logInfo(QString("✗ Ejecutable no reconocido como Nuke principal: %1").arg(fileName));
    return false;
} 