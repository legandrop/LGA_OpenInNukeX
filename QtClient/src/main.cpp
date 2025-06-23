#include <QApplication>
#include <QMessageBox>
#include <QStringList>
#include <QStandardPaths>
#include "nukeopener.h"
#include "configwindow.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Configurar información de la organización para QStandardPaths
    QApplication::setOrganizationName("LGA");
    QApplication::setApplicationName("OpenInNukeX");
    
    // Limpiar archivo de logs al inicio
    Logger::clearLogFile();
    
    Logger::logInfo("=== INICIANDO OpenInNukeX ===");
    Logger::logInfo(QString("Directorio de configuración: %1").arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)));
    
    QStringList arguments = QApplication::arguments();
    Logger::logInfo(QString("Argumentos recibidos: %1").arg(arguments.join(" | ")));
    
    if (arguments.size() > 1) {
        // El primer argumento después del nombre del programa es la ruta del archivo
        QString filepath = arguments.at(1);
        Logger::logInfo(QString("Modo cliente: procesando archivo %1").arg(filepath));
        
        NukeOpener opener;
        opener.sendToNuke(filepath);
        return app.exec(); // Cambio importante: esperar a que termine la conexión
    } else {
        Logger::logInfo("Modo configuración: mostrando ventana de configuración");
        // Si no hay argumentos, mostrar ventana de configuración
        ConfigWindow configWindow;
        configWindow.show();
        return app.exec();
    }
} 