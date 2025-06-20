#ifndef NUKESCANNER_H
#define NUKESCANNER_H

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

struct NukeVersion {
    QString name;           // Ej: "Nuke15.0v4"
    QString path;           // Ruta completa al ejecutable
    QString version;        // Ej: "15.0v4"
    QString displayName;    // Ej: "Nuke 15.0v4"
};

class NukeScanner : public QObject
{
    Q_OBJECT

public:
    explicit NukeScanner(QObject *parent = nullptr);
    void startScan();

signals:
    void scanStarted();
    void scanProgress(const QString &currentPath);
    void versionFound(const NukeVersion &version);
    void scanFinished(const QList<NukeVersion> &versions);

private slots:
    void performScan();

private:
    QStringList getCommonNukePaths();
    QList<NukeVersion> scanDirectory(const QString &dirPath);
    NukeVersion parseNukeExecutable(const QString &executablePath);
    bool isValidNukeExecutable(const QString &executablePath);
    bool isValidNukeDirectory(const QString &dirPath);
    
    QThread *workerThread;
    QList<NukeVersion> foundVersions;
};

#endif // NUKESCANNER_H 