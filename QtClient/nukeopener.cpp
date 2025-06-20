#include "nukeopener.h"
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QProcess>
#include <QDebug>

NukeOpener::NukeOpener(QObject *parent)
    : QObject(parent)
    , socket(nullptr)
    , timeoutTimer(new QTimer(this))
{
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, &NukeOpener::onSocketTimeout);
}

void NukeOpener::showMessage(const QString &message)
{
    QMessageBox::information(nullptr, "Resultado", message);
}

QString NukeOpener::getNukePathFromFile()
{
    // Obtiene la ruta del directorio donde se encuentra el ejecutable
    QString appDir = QApplication::applicationDirPath();
    QString filepath = QDir(appDir).filePath("nukeXpath.txt");
    
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showMessage(QString("Error: Archivo de ruta de Nuke no encontrado en el directorio del ejecutable. %1").arg(file.errorString()));
        return QString();
    }
    
    QTextStream in(&file);
    QString nukePath = in.readLine().trimmed();
    file.close();
    
    if (nukePath.isEmpty()) {
        showMessage("Error: El archivo nukeXpath.txt está vacío.");
        return QString();
    }
    
    return nukePath;
}

void NukeOpener::openNukeWithFile(const QString &nukeExecutablePath, const QString &nkFilepath)
{
    // Crea un comando para abrir Nuke con el archivo .nk
    QStringList arguments;
    arguments << "--nukex" << nkFilepath;
    
    QProcess *process = new QProcess(this);
    bool started = process->startDetached(nukeExecutablePath, arguments);
    
    if (!started) {
        showMessage(QString("Error al abrir Nuke con el archivo: %1").arg(process->errorString()));
    }
    
    process->deleteLater();
}

void NukeOpener::sendToNuke(const QString &filepath, const QString &host, int port)
{
    currentFilepath = filepath;
    
    // Crear socket
    socket = new QTcpSocket(this);
    
    // Conectar señales
    connect(socket, &QTcpSocket::connected, this, &NukeOpener::onConnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &NukeOpener::onErrorOccurred);
    
    // Configurar timeout de 10 segundos
    timeoutTimer->start(10000);
    
    // Intentar conectar
    socket->connectToHost(host, port);
}

void NukeOpener::onConnected()
{
    timeoutTimer->stop();
    
    // Normalizar la ruta del archivo (convertir \ a /)
    QString normalizedPath = QDir::fromNativeSeparators(currentFilepath);
    QString fullCommand = QString("run_script||%1").arg(normalizedPath);
    
    socket->write(fullCommand.toUtf8());
    socket->waitForBytesWritten();
    
    // Leer respuesta
    socket->waitForReadyRead(5000);
    QByteArray response = socket->readAll();
    
    // Comentado para que no muestre el mensaje de respuesta como en el Python original
    // showMessage(QString("Recibido: %1").arg(QString::fromUtf8(response)));
    
    socket->close();
    socket->deleteLater();
    socket = nullptr;
    
    // Cerrar la aplicación después de enviar el comando
    QApplication::quit();
}

void NukeOpener::onErrorOccurred(QAbstractSocket::SocketError error)
{
    timeoutTimer->stop();
    
    if (error == QAbstractSocket::ConnectionRefusedError) {
        QString nukeExecutablePath = getNukePathFromFile();
        if (!nukeExecutablePath.isEmpty()) {
            // showMessage("No se pudo conectar a Nuke. Intentando abrir NukeX con el archivo.");
            openNukeWithFile(nukeExecutablePath, currentFilepath);
        } else {
            showMessage("Archivo de ruta de Nuke no encontrado o no se puede leer.");
        }
    } else if (error == QAbstractSocket::RemoteHostClosedError) {
        showMessage("La conexión fue cerrada por el servidor.");
    } else {
        showMessage(QString("Error de conexión: %1").arg(socket->errorString()));
    }
    
    if (socket) {
        socket->deleteLater();
        socket = nullptr;
    }
    
    // Cerrar la aplicación
    QApplication::quit();
}

void NukeOpener::onSocketTimeout()
{
    showMessage("La conexión a Nuke ha expirado.");
    
    if (socket) {
        socket->abort();
        socket->deleteLater();
        socket = nullptr;
    }
    
    // Cerrar la aplicación
    QApplication::quit();
} 