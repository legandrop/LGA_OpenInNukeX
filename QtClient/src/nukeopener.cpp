#include "nukeopener.h"
#include "logger.h"
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QProcess>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

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
    // Obtiene la ruta de configuración en AppData
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filepath = QDir(configDir).filePath("nukeXpath.txt");
    
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showMessage(QString("Error: Archivo de configuración de Nuke no encontrado. Ejecuta la aplicación sin argumentos para configurar la ruta. %1").arg(file.errorString()));
        return QString();
    }
    
    QTextStream in(&file);
    QString nukePath = in.readLine().trimmed();
    file.close();
    
    if (nukePath.isEmpty()) {
        showMessage("Error: El archivo de configuración está vacío. Ejecuta la aplicación sin argumentos para configurar la ruta.");
        return QString();
    }
    
    return nukePath;
}

void NukeOpener::openNukeWithFile(const QString &nukeExecutablePath, const QString &nkFilepath)
{
    Logger::logInfo(QString("=== ABRIENDO NUEVA INSTANCIA DE NUKEX ==="));
    Logger::logInfo(QString("Ejecutable: %1").arg(nukeExecutablePath));
    Logger::logInfo(QString("Archivo: %1").arg(nkFilepath));
    
    // Verificar que el ejecutable existe
    if (!QFile::exists(nukeExecutablePath)) {
        Logger::logError(QString("El ejecutable de NukeX no existe: %1").arg(nukeExecutablePath));
        showMessage(QString("Error: El ejecutable de NukeX no existe: %1").arg(nukeExecutablePath));
        return;
    }
    
    // Crea un comando para abrir Nuke con el archivo .nk
    QStringList arguments;
    arguments << "--nukex" << nkFilepath;
    
    Logger::logInfo(QString("Argumentos: %1").arg(arguments.join(" ")));
    
    #ifdef Q_OS_WIN
    // En Windows, usar CreateProcess directamente con CREATE_NEW_CONSOLE
    STARTUPINFOW startupInfo = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION processInfo;
    
    QString commandLine = QString("\"%1\" %2").arg(nukeExecutablePath).arg(arguments.join(" "));
    std::wstring wCommandLine = commandLine.toStdWString();
    
    Logger::logInfo(QString("Comando completo: %1").arg(commandLine));
    
    BOOL success = CreateProcessW(
        NULL,                                // No module name (use command line)
        &wCommandLine[0],                    // Command line
        NULL,                                // Process handle not inheritable
        NULL,                                // Thread handle not inheritable
        FALSE,                               // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,                  // Creation flags - CREAR NUEVA CONSOLA
        NULL,                                // Use parent's environment block
        NULL,                                // Use parent's starting directory
        &startupInfo,                        // Pointer to STARTUPINFO structure
        &processInfo                         // Pointer to PROCESS_INFORMATION structure
    );
    
    if (success) {
        Logger::logInfo("NukeX iniciado exitosamente con nueva consola");
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    } else {
        DWORD error = GetLastError();
        Logger::logError(QString("Error al iniciar proceso con CreateProcess: %1").arg(error));
        showMessage(QString("Error al abrir Nuke con el archivo. Código de error: %1").arg(error));
    }
    #else
    // En sistemas Unix, usar QProcess normal
    QProcess *process = new QProcess(this);
    bool started = process->startDetached(nukeExecutablePath, arguments);
    
    if (!started) {
        Logger::logError(QString("Error al iniciar proceso: %1").arg(process->errorString()));
        showMessage(QString("Error al abrir Nuke con el archivo: %1").arg(process->errorString()));
    } else {
        Logger::logInfo("NukeX iniciado exitosamente como proceso independiente");
    }
    
    process->deleteLater();
    #endif
}

void NukeOpener::sendToNuke(const QString &filepath, const QString &host, int port)
{
    Logger::logInfo(QString("=== INICIANDO sendToNuke ==="));
    Logger::logInfo(QString("Archivo a abrir: %1").arg(filepath));
    Logger::logInfo(QString("Host: %1, Puerto: %2").arg(host).arg(port));
    
    currentFilepath = filepath;
    
    // Verificar que el archivo existe
    if (!QFile::exists(filepath)) {
        Logger::logError(QString("El archivo no existe: %1").arg(filepath));
        showMessage(QString("Error: El archivo no existe: %1").arg(filepath));
        return;
    }
    
    // Crear socket
    Logger::logInfo("Creando socket TCP...");
    socket = new QTcpSocket(this);
    
    // Conectar señales
    connect(socket, &QTcpSocket::connected, this, &NukeOpener::onConnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &NukeOpener::onErrorOccurred);
    
    // Configurar timeout de 10 segundos
    Logger::logInfo("Configurando timeout de 10 segundos...");
    timeoutTimer->start(10000);
    
    // Intentar conectar
    Logger::logInfo(QString("Intentando conectar a %1:%2...").arg(host).arg(port));
    socket->connectToHost(host, port);
}

void NukeOpener::onConnected()
{
    Logger::logInfo("¡Conectado al servidor TCP!");
    timeoutTimer->stop();
    
    // Normalizar la ruta del archivo (convertir \ a /)
    QString normalizedPath = QDir::fromNativeSeparators(currentFilepath);
    QString fullCommand = QString("run_script||%1").arg(normalizedPath);
    
    Logger::logInfo(QString("Enviando comando: %1").arg(fullCommand));
    
    socket->write(fullCommand.toUtf8());
    socket->waitForBytesWritten();
    
    Logger::logInfo("Comando enviado, esperando respuesta...");
    
    // Leer respuesta
    socket->waitForReadyRead(5000);
    QByteArray response = socket->readAll();
    
    Logger::logInfo(QString("Respuesta recibida: %1").arg(QString::fromUtf8(response)));
    
    // Comentado para que no muestre el mensaje de respuesta como en el Python original
    // showMessage(QString("Recibido: %1").arg(QString::fromUtf8(response)));
    
    socket->close();
    socket->deleteLater();
    socket = nullptr;
    
    Logger::logInfo("Conexión cerrada exitosamente. Cerrando aplicación.");
    
    // Cerrar la aplicación después de enviar el comando
    QApplication::quit();
}

void NukeOpener::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Logger::logError(QString("Error de socket: %1").arg(error));
    timeoutTimer->stop();
    
    if (error == QAbstractSocket::ConnectionRefusedError) {
        Logger::logInfo("Conexión rechazada. Intentando abrir nueva instancia de NukeX...");
        QString nukeExecutablePath = getNukePathFromFile();
        Logger::logInfo(QString("Ruta de NukeX obtenida: %1").arg(nukeExecutablePath));
        
        if (!nukeExecutablePath.isEmpty()) {
            // showMessage("No se pudo conectar a Nuke. Intentando abrir NukeX con el archivo.");
            openNukeWithFile(nukeExecutablePath, currentFilepath);
        } else {
            Logger::logError("No se encontró la ruta de NukeX en el archivo de configuración");
            showMessage("Archivo de ruta de Nuke no encontrado o no se puede leer.");
        }
    } else if (error == QAbstractSocket::RemoteHostClosedError) {
        Logger::logError("La conexión fue cerrada por el servidor");
        showMessage("La conexión fue cerrada por el servidor.");
    } else {
        Logger::logError(QString("Error de conexión desconocido: %1").arg(socket->errorString()));
        showMessage(QString("Error de conexión: %1").arg(socket->errorString()));
    }
    
    if (socket) {
        socket->deleteLater();
        socket = nullptr;
    }
    
    Logger::logInfo("Cerrando aplicación tras error de conexión");
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