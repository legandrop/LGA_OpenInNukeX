#include <QApplication>
#include <QMessageBox>
#include <QStringList>
#include "nukeopener.h"
#include "configwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QStringList arguments = QApplication::arguments();
    
    if (arguments.size() > 1) {
        // El primer argumento después del nombre del programa es la ruta del archivo
        QString filepath = arguments.at(1);
        NukeOpener opener;
        opener.sendToNuke(filepath);
        return 0;
    } else {
        // Si no hay argumentos, mostrar ventana de configuración
        ConfigWindow configWindow;
        configWindow.show();
        return app.exec();
    }
} 