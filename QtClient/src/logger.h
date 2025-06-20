#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QApplication>
#include <QDir>

class Logger
{
public:
    static void log(const QString &message);
    static void logError(const QString &error);
    static void logInfo(const QString &info);
    
private:
    static QString getLogFilePath();
    static void writeToLog(const QString &level, const QString &message);
};

#endif // LOGGER_H 