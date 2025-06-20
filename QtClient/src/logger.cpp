#include "logger.h"
#include <QStandardPaths>

QString Logger::getLogFilePath()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir); // Crear directorio si no existe
    return QDir(configDir).filePath("LGA_OpenInNukeX_Qt.log");
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