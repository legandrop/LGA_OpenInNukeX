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
#include <QPushButton>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

NukeOpener::NukeOpener(QObject *parent)
    : QObject(parent)
    , socket(nullptr)
    , timeoutTimer(new QTimer(this))
    , countdownTimer(new QTimer(this))
    , autoCloseMessageBox(nullptr)
{
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, &NukeOpener::onSocketTimeout);
    
    // Configurar timer para countdown
    countdownTimer->setSingleShot(false);
    connect(countdownTimer, &QTimer::timeout, this, &NukeOpener::updateCountdownMessage);
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
    
    // Configurar timeout de 2 segundos
    Logger::logInfo("Configurando timeout de 2 segundos...");
    timeoutTimer->start(2000);
    
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
            // Primero abrir NukeX
            openNukeWithFile(nukeExecutablePath, currentFilepath);
            
            // Luego mostrar mensaje con countdown (no bloquea)
            showAutoCloseMessage();
            
            // Esperar 4 segundos antes de cerrar la aplicación (3 segundos del mensaje + 1 segundo de margen)
            QTimer::singleShot(4000, [this]() {
                Logger::logInfo("Cerrando aplicación tras error de conexión y finalización del mensaje");
                QApplication::quit();
            });
        } else {
            Logger::logError("No se encontró la ruta de NukeX en el archivo de configuración");
            showMessage("Nuke path file not found or cannot be read.");
            
            // Si no hay ruta válida, cerrar inmediatamente
            Logger::logInfo("Cerrando aplicación tras error de conexión sin ruta válida");
            QApplication::quit();
        }
    } else if (error == QAbstractSocket::RemoteHostClosedError) {
        Logger::logError("La conexión fue cerrada por el servidor");
        showMessage("The connection was closed by the server.");
        Logger::logInfo("Cerrando aplicación tras cierre remoto");
        QApplication::quit();
    } else {
        Logger::logError(QString("Error de conexión desconocido: %1").arg(socket->errorString()));
        showMessage(QString("Connection error: %1").arg(socket->errorString()));
        Logger::logInfo("Cerrando aplicación tras error desconocido");
        QApplication::quit();
    }
    
    if (socket) {
        socket->deleteLater();
        socket = nullptr;
    }
}

void NukeOpener::showAutoCloseMessage()
{
    if (!ShowNewNukeMsg) {
        Logger::logInfo("Flag ShowNewNukeMsg está deshabilitado, no se mostrará mensaje");
        return;
    }
    
    Logger::logInfo("=== MOSTRANDO MENSAJE DE NUEVA INSTANCIA ===");
    Logger::logInfo("Mensaje: No NukeX instance found, opening a new one...");
    
    countdownSeconds = 3;
    autoCloseMessageBox = new QMessageBox();
    autoCloseMessageBox->setWindowTitle("NukeX Launcher");
    autoCloseMessageBox->setIcon(QMessageBox::Information);
    autoCloseMessageBox->setText("No NukeX instance found, opening a new one...");
    
    // Configurar botón personalizado
    QPushButton *closeButton = new QPushButton("Closing in 3 seconds");
    closeButton->setEnabled(false);
    autoCloseMessageBox->addButton(closeButton, QMessageBox::AcceptRole);
    
    // Hacer que el diálogo no sea modal para que no bloquee la apertura de NukeX
    autoCloseMessageBox->setModal(false);
    autoCloseMessageBox->show();
    
    Logger::logInfo("Iniciando cuenta regresiva de 3 segundos...");
    
    // Iniciar countdown
    countdownTimer->start(1000);
    
    // Auto-cerrar después de 3 segundos
    QTimer::singleShot(3000, [this]() {
        Logger::logInfo("Cuenta regresiva completada, cerrando mensaje automáticamente");
        if (autoCloseMessageBox) {
            autoCloseMessageBox->accept();
            autoCloseMessageBox->deleteLater();
            autoCloseMessageBox = nullptr;
        }
        if (countdownTimer->isActive()) {
            countdownTimer->stop();
        }
    });
}

void NukeOpener::updateCountdownMessage()
{
    if (!autoCloseMessageBox) {
        countdownTimer->stop();
        return;
    }
    
    countdownSeconds--;
    Logger::logInfo(QString("Cuenta regresiva: %1 segundo%2 restante%3").arg(countdownSeconds).arg(countdownSeconds == 1 ? "" : "s").arg(countdownSeconds == 1 ? "" : "s"));
    
    if (countdownSeconds <= 0) {
        countdownTimer->stop();
        return;
    }
    
    // Actualizar texto del botón
    QList<QAbstractButton*> buttons = autoCloseMessageBox->buttons();
    if (!buttons.isEmpty()) {
        QPushButton *button = qobject_cast<QPushButton*>(buttons.first());
        if (button) {
            button->setText(QString("Closing in %1 second%2").arg(countdownSeconds).arg(countdownSeconds == 1 ? "" : "s"));
        }
    }
}

void NukeOpener::onSocketTimeout()
{
    Logger::logInfo("Timeout de conexión alcanzado. Abriendo nueva instancia de NukeX...");
    
    QString nukeExecutablePath = getNukePathFromFile();
    Logger::logInfo(QString("Ruta de NukeX obtenida: %1").arg(nukeExecutablePath));
    
    if (!nukeExecutablePath.isEmpty()) {
        // Primero abrir NukeX
        openNukeWithFile(nukeExecutablePath, currentFilepath);
        
        // Luego mostrar mensaje con countdown (no bloquea)
        showAutoCloseMessage();
        
        // Esperar 4 segundos antes de cerrar la aplicación (3 segundos del mensaje + 1 segundo de margen)
        QTimer::singleShot(4000, [this]() {
            Logger::logInfo("Cerrando aplicación tras timeout y finalización del mensaje");
            QApplication::quit();
        });
    } else {
        Logger::logError("No se encontró la ruta de NukeX en el archivo de configuración");
        showMessage("Nuke path file not found or cannot be read.");
        
        // Si no hay ruta válida, cerrar inmediatamente
        Logger::logInfo("Cerrando aplicación tras timeout sin ruta válida");
        QApplication::quit();
    }
    
    if (socket) {
        socket->deleteLater();
        socket = nullptr;
    }
} 