#include <QApplication>
#include <QFileOpenEvent>
#include <QStringList>
#include <QStandardPaths>
#include <QTimer>
#include "nukeopener.h"
#include "configwindow.h"
#include "logger.h"

// ─────────────────────────────────────────────────────────────────────────────
// NukeApp: subclase de QApplication que captura QFileOpenEvent.
//
// En macOS, cuando el Finder abre un .nk con esta app, NO pasa el path como
// argumento de línea de comandos. En su lugar envía un Apple Event que Qt
// convierte a QFileOpenEvent y entrega a QApplication::event().
//
// No necesita Q_OBJECT porque solo overridea un método virtual.
// ─────────────────────────────────────────────────────────────────────────────
class NukeApp : public QApplication {
public:
    NukeApp(int &argc, char **argv) : QApplication(argc, argv) {}

    bool fileEventReceived = false;
    QString pendingFile;

protected:
    bool event(QEvent *event) override {
        if (event->type() == QEvent::FileOpen) {
            pendingFile = static_cast<QFileOpenEvent *>(event)->file();
            fileEventReceived = true;
            Logger::logInfo(QString("QFileOpenEvent recibido: %1").arg(pendingFile));
            return true;
        }
        return QApplication::event(event);
    }
};

int main(int argc, char *argv[])
{
    NukeApp app(argc, argv);

    app.setOrganizationName("LGA");
    app.setApplicationName("OpenInNukeX");

    Logger::clearLogFile();
    Logger::logInfo("=== INICIANDO OpenInNukeX ===");
    Logger::logInfo(QString("Config dir: %1").arg(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)));

    QStringList arguments = QApplication::arguments();
    Logger::logInfo(QString("Argumentos: %1").arg(arguments.join(" | ")));

    // ── Modo cliente por línea de comandos (Windows, o Mac desde terminal) ────
    if (arguments.size() > 1) {
        QString filepath = arguments.at(1);
        // Ignorar flags de macOS como -psn_0_xxxxxx o -NSDocumentRevisionsDebugMode
        if (!filepath.startsWith("-")) {
            Logger::logInfo(QString("Modo cliente (arg): %1").arg(filepath));
            NukeOpener opener;
            opener.sendToNuke(filepath);
            return app.exec();
        }
    }

#ifdef Q_OS_MAC
    // ── macOS: el archivo llega via QFileOpenEvent (Apple Event del Finder) ───
    //
    // El QFileOpenEvent se despacha al procesar los primeros eventos del loop.
    // Usamos un timer de 200ms: si llega el evento primero → abrimos el archivo.
    // Si vence el timer sin evento → el usuario lanzó la app directamente → ConfigWindow.

    NukeOpener *opener = new NukeOpener();

    QTimer *startupTimer = new QTimer();
    startupTimer->setSingleShot(true);

    QObject::connect(startupTimer, &QTimer::timeout, [&app, opener, startupTimer]() {
        startupTimer->deleteLater();

        if (app.fileEventReceived) {
            Logger::logInfo(QString("Abriendo via FileOpenEvent: %1").arg(app.pendingFile));
            opener->sendToNuke(app.pendingFile);
            // opener se autodestruye al llamar QApplication::quit() dentro de sendToNuke
        } else {
            Logger::logInfo("Sin archivo, mostrando ConfigWindow");
            opener->deleteLater();
            ConfigWindow *configWindow = new ConfigWindow();
            configWindow->show();
        }
    });

    startupTimer->start(200);
    return app.exec();

#else
    // ── Windows sin argumento: ventana de configuración ───────────────────────
    Logger::logInfo("Modo configuración");
    ConfigWindow configWindow;
    configWindow.show();
    return app.exec();
#endif
}
