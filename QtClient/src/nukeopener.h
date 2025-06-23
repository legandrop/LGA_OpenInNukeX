#ifndef NUKEOPENER_H
#define NUKEOPENER_H

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QPushButton>

class NukeOpener : public QObject
{
    Q_OBJECT

public:
    explicit NukeOpener(QObject *parent = nullptr);
    void sendToNuke(const QString &filepath, const QString &host = "localhost", int port = 54325);
    void setShowNewNukeMsg(bool enabled) { ShowNewNukeMsg = enabled; }
    bool getShowNewNukeMsg() const { return ShowNewNukeMsg; }

private slots:
    void onConnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onSocketTimeout();
    void updateCountdownMessage();

private:
    QString getNukePathFromFile();
    void openNukeWithFile(const QString &nukeExecutablePath, const QString &nkFilepath);
    void showMessage(const QString &message);
    void showAutoCloseMessage();
    
    QTcpSocket *socket;
    QTimer *timeoutTimer;
    QTimer *countdownTimer;
    QMessageBox *autoCloseMessageBox;
    QString currentFilepath;
    bool ShowNewNukeMsg = false;  // ✅✅Flag para controlar si mostrar el mensaje
    int countdownSeconds = 3;    // Contador para el mensaje automático
};

#endif // NUKEOPENER_H 