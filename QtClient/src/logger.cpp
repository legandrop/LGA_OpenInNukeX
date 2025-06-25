#include "logger.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>

QString Logger::getLogFilePath()
{
    // Crear carpeta de logs en AppData junto con la configuración
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir); // Crear directorio si no existe
    
    return QDir(configDir).filePath("OpenInNukeX.log");
}

void Logger::writeToLog(const QString &level, const QString &message)
{
    QFile logFile(getLogFilePath());
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        stream << QString("[%1] %2: %3").arg(timestamp, level, message) << Qt::endl;
        logFile.close();
    }
}

void Logger::log(const QString &message)
{
    writeToLog("INFO", message);
}

void Logger::logError(const QString &error)
{
    writeToLog("ERROR", error);
}

void Logger::logInfo(const QString &info)
{
    writeToLog("INFO", info);
}

void Logger::clearLogFile()
{
    QFile logFile(getLogFilePath());
    if (logFile.exists()) {
        logFile.remove();
    }
} 