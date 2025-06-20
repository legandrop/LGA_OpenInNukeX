#ifndef NUKEOPENER_H
#define NUKEOPENER_H

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QProcess>

class NukeOpener : public QObject
{
    Q_OBJECT

public:
    explicit NukeOpener(QObject *parent = nullptr);
    void sendToNuke(const QString &filepath, const QString &host = "localhost", int port = 54325);

private slots:
    void onConnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onSocketTimeout();

private:
    QString getNukePathFromFile();
    void openNukeWithFile(const QString &nukeExecutablePath, const QString &nkFilepath);
    void showMessage(const QString &message);
    
    QTcpSocket *socket;
    QTimer *timeoutTimer;
    QString currentFilepath;
};

#endif // NUKEOPENER_H 