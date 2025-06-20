#include "configwindow.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFont>
#include <QPalette>
#include <QProcess>
#include <QThread>
#include <QScrollArea>
#include <QGroupBox>
#include <QTimer>
#include <QSizePolicy>
#include "logger.h"

ConfigWindow::ConfigWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadCurrentPath();

    // Configurar ventana
    setWindowTitle("LGA OpenInNukeX Config");
    setFixedSize(900, 600);

    // Cargar estilo QSS
    loadStyleSheet();
}

void ConfigWindow::setupUI()
{
    // Crear el layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Crear un QScrollArea para permitir desplazamiento
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName("darkScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    // Asegurar que el QScrollArea ocupe todo el espacio disponible
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Contenedor central para limitar el ancho y centrar el contenido
    QWidget *centralWidget = new QWidget();
    centralWidget->setObjectName("centralSettingsWidget");
    
    // Permitir que se ajuste al contenido
    centralWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    
    // Crear un layout para centrar el contenido horizontalmente
    QHBoxLayout *horizontalCenterLayout = new QHBoxLayout(centralWidget);
    horizontalCenterLayout->setContentsMargins(0, 0, 0, 20);
    horizontalCenterLayout->setSpacing(0);
    
    // Crear un widget contenedor para el contenido real
    QWidget *contentWidget = new QWidget();
    contentWidget->setObjectName("contentSettingsWidget");
    contentWidget->setFixedWidth(700); // Ancho para todas las cajas principales

    // Layout para el contenido
    QVBoxLayout *centralLayout = new QVBoxLayout(contentWidget);
    centralLayout->setContentsMargins(20, 20, 20, 20);
    centralLayout->setSpacing(15);
    
    // Agregar el widget de contenido al layout horizontal centrado
    horizontalCenterLayout->addStretch();
    horizontalCenterLayout->addWidget(contentWidget);
    horizontalCenterLayout->addStretch();
    
    // Asignar el widget central al área de desplazamiento
    scrollArea->setWidget(centralWidget);
    
    // Agregar el área de desplazamiento al layout principal
    mainLayout->addWidget(scrollArea, 1);
    
    // ------------------------------------------------------------------------------------------------
    // Grupo de configuración de File Association
    QGroupBox *fileAssociationGroup = new QGroupBox(contentWidget);
    fileAssociationGroup->setObjectName("settingsGroup");
    fileAssociationGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVBoxLayout *fileAssociationLayout = new QVBoxLayout(fileAssociationGroup);
    fileAssociationLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    fileAssociationLayout->setContentsMargins(20, 10, 20, 10);
    fileAssociationLayout->setSpacing(10);
    
    // Título principal de File Association
    QLabel *fileAssociationTitle = new QLabel("File Association", fileAssociationGroup);
    fileAssociationTitle->setObjectName("sectionTitle");
    fileAssociationLayout->addWidget(fileAssociationTitle);
    fileAssociationLayout->addSpacing(10);

    // Texto descriptivo
    descriptionLabel = new QLabel("Associate .nk files with LGA_OpenInNukeX to open them directly in your preferred NukeX version", fileAssociationGroup);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet("QLabel { color: #8A8A8A; font-size: 14px; }");
    fileAssociationLayout->addWidget(descriptionLabel);
    fileAssociationLayout->addSpacing(15);

    // Botón Apply para asociación de archivos
    QHBoxLayout *applyLayout = new QHBoxLayout();
    applyLayout->setContentsMargins(0, 0, 0, 0);
    applyLayout->setSpacing(10);
    
    applyButton = new QPushButton("APPLY", this);
    applyButton->setFixedHeight(40);
    applyButton->setProperty("class", "secondary");
    
    applyLayout->addStretch();
    applyLayout->addWidget(applyButton);
    
    fileAssociationLayout->addLayout(applyLayout);
    
    // Agregar el grupo de File Association al layout central
    centralLayout->addWidget(fileAssociationGroup);
    
    // ------------------------------------------------------------------------------------------------
    // Grupo de configuración de Preferred Nuke Version
    QGroupBox *nukeVersionGroup = new QGroupBox(contentWidget);
    nukeVersionGroup->setObjectName("settingsGroup");
    nukeVersionGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVBoxLayout *nukeVersionLayout = new QVBoxLayout(nukeVersionGroup);
    nukeVersionLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    nukeVersionLayout->setContentsMargins(20, 0, 20, 10);
    
    // Título principal de Preferred Nuke Version
    QLabel *nukeVersionTitle = new QLabel("Preferred Nuke Version", nukeVersionGroup);
    nukeVersionTitle->setObjectName("sectionTitle");
    nukeVersionLayout->addWidget(nukeVersionTitle);
    nukeVersionLayout->addSpacing(10);

    // Crear layout horizontal para el campo de path y el botón browse
    QHBoxLayout *nukePathLayout = new QHBoxLayout();
    nukePathLayout->setContentsMargins(0, 0, 0, 0);
    nukePathLayout->setSpacing(10);
    
    // Campo de entrada para la ruta de NukeX
    nukePathEdit = new QLineEdit(nukeVersionGroup);
    nukePathEdit->setPlaceholderText("Path to NukeX executable");
    
    // Botón Browse para seleccionar la ruta
    browseButton = new QPushButton("BROWSE", nukeVersionGroup);
    browseButton->setFixedHeight(40);
    browseButton->setProperty("class", "secondary");
    
    // Agregar los elementos al layout horizontal
    nukePathLayout->addWidget(nukePathEdit);
    nukePathLayout->addWidget(browseButton);
    
    // Agregar el layout horizontal al layout vertical de Nuke Version
    nukeVersionLayout->addLayout(nukePathLayout);
    nukeVersionLayout->addSpacing(10);
    
    // Crear etiqueta y posicionarla absolutamente ENCIMA del campo
    QLabel *nukePathLabel = new QLabel("NukeX Path", contentWidget);
    nukePathLabel->setObjectName("fieldLabel");
    nukePathLabel->adjustSize();
    
    // Posicionar la etiqueta después de que el layout se haya establecido
    QTimer::singleShot(0, [=]() {
        // Mapear las coordenadas del nukeVersionGroup al contentWidget
        QPoint groupPos = nukeVersionGroup->mapTo(contentWidget, QPoint(0, 0));
        
        // Posicionar etiqueta sobre el campo con ajuste en Y
        nukePathLabel->move(groupPos.x() + nukePathEdit->x() + 50, 
                           groupPos.y() + nukePathEdit->y() + 88);

        // Forzar que esté por encima de todo
        nukePathLabel->raise();
        
        // Asegurar que el label esté en el tope de la pila de widgets
        nukePathLabel->setParent(contentWidget);
        
        // Volver a mostrar y ajustar después del cambio de padre
        nukePathLabel->show();
        nukePathLabel->adjustSize();
    });
    
    // Agregar el grupo de Nuke Version al layout central
    centralLayout->addWidget(nukeVersionGroup);

    // Boton de SAVE
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 10, 0, 0);
    buttonLayout->setSpacing(10);

    saveButton = new QPushButton("SAVE", contentWidget);
    saveButton->setFixedHeight(40);
    saveButton->setMinimumWidth(100);
    saveButton->setProperty("class", "action");

    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);

    centralLayout->addLayout(buttonLayout);
    
    // Agregar un stretch al final para evitar que los widgets se estiren verticalmente
    centralLayout->addStretch(1);

    // Conectar señales y slots
    connect(browseButton, &QPushButton::clicked, this, &ConfigWindow::browseNukePath);
    connect(saveButton, &QPushButton::clicked, this, &ConfigWindow::saveConfiguration);
    connect(applyButton, &QPushButton::clicked, this, &ConfigWindow::applyFileAssociation);
}

void ConfigWindow::loadCurrentPath()
{
    QString currentPath = getNukePathFromFile();
    if (!currentPath.isEmpty())
    {
        nukePathEdit->setText(currentPath);
    }
}

QString ConfigWindow::getNukePathFromFile()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir); // Crear directorio si no existe
    QString filepath = QDir(configDir).filePath("nukeXpath.txt");

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }

    QTextStream in(&file);
    QString nukePath = in.readLine().trimmed();
    file.close();

    return nukePath;
}

void ConfigWindow::browseNukePath()
{
    QString currentPath = nukePathEdit->text();
    QString startDir = currentPath.isEmpty() ? "C:/Program Files/" : QDir(currentPath).absolutePath();

    QString selectedPath = QFileDialog::getOpenFileName(
        this,
        "Seleccionar NukeX.exe o Nuke.exe",
        startDir,
        "Executable Files (*.exe);;All Files (*)");

    if (!selectedPath.isEmpty())
    {
        nukePathEdit->setText(selectedPath);
    }
}

void ConfigWindow::saveConfiguration()
{
    QString nukePath = nukePathEdit->text().trimmed();

    if (nukePath.isEmpty())
    {
        QMessageBox::warning(this, "Error", "Por favor, selecciona una ruta válida para NukeX.");
        return;
    }

    // Verificar que el archivo existe
    if (!QFile::exists(nukePath))
    {
        QMessageBox::warning(this, "Error", "El archivo seleccionado no existe.");
        return;
    }

    // Verificar que es un ejecutable de Nuke
    QString fileName = QFileInfo(nukePath).fileName().toLower();
    if (!fileName.contains("nuke"))
    {
        QMessageBox::warning(this, "Advertencia", "El archivo seleccionado no parece ser un ejecutable de Nuke.");
    }

    saveNukePath(nukePath);

    QMessageBox::information(this, "Configuración Guardada", "La ruta de NukeX se ha guardado correctamente.");

    // Cerrar la aplicación
    QApplication::quit();
}

void ConfigWindow::saveNukePath(const QString &path)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir); // Crear directorio si no existe
    QString filepath = QDir(configDir).filePath("nukeXpath.txt");

    QFile file(filepath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out << path;
        file.close();
    }
}

void ConfigWindow::applyFileAssociation()
{
    Logger::logInfo("=== INICIANDO ASOCIACIÓN DE ARCHIVOS .NK ===");
    
    QString nukePath = nukePathEdit->text().trimmed();
    Logger::logInfo(QString("Ruta de NukeX: %1").arg(nukePath));
    
    if (nukePath.isEmpty()) {
        Logger::logError("Error: Ruta de NukeX vacía");
        QMessageBox::warning(this, "Error", "Por favor, selecciona una ruta válida para NukeX primero.");
        return;
    }
    
    if (!QFile::exists(nukePath)) {
        Logger::logError(QString("Error: El archivo no existe: %1").arg(nukePath));
        QMessageBox::warning(this, "Error", "El archivo seleccionado no existe.");
        return;
    }
    
    Logger::logInfo("Mostrando diálogo de confirmación al usuario");
    // Mostrar mensaje de confirmación
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "Confirmar Asociación", 
        "¿Deseas asociar los archivos .nk con esta aplicación?\n\n"
        "Esto modificará el registro de Windows para que los archivos .nk se abran con NukeX.",
        QMessageBox::Yes | QMessageBox::No);
        
    if (reply != QMessageBox::Yes) {
        Logger::logInfo("Usuario canceló la asociación");
        return;
    }
    
    Logger::logInfo("Usuario confirmó la asociación - iniciando proceso");
    
    try {
        // Primero guardar la configuración
        Logger::logInfo("Guardando configuración de NukeX");
        saveNukePath(nukePath);
        
        // Ejecutar los comandos de registro
        Logger::logInfo("Ejecutando comandos de registro");
        executeRegistryCommands();
        
        Logger::logInfo("Asociación completada exitosamente");
        QMessageBox::information(this, "Asociación Completada", 
            "La asociación de archivos .nk se ha configurado correctamente.\n\n"
            "Ahora puedes hacer doble click en archivos .nk para abrirlos con NukeX.");
            
    } catch (const std::exception& e) {
        Logger::logError(QString("Excepción durante asociación: %1").arg(e.what()));
        QMessageBox::critical(this, "Error", 
            QString("Error al configurar la asociación de archivos:\n%1").arg(e.what()));
    }
}

void ConfigWindow::executeRegistryCommands()
{
    Logger::logInfo("=== EJECUTANDO COMANDOS DE REGISTRO ===");
    QStringList errors;
    
    // Paso 1: Limpiar registro
    Logger::logInfo("PASO 1: Limpiando registro...");
    if (!cleanRegistry()) {
        errors << "Error al limpiar el registro";
        Logger::logError("PASO 1 FALLÓ: Error al limpiar el registro");
    } else {
        Logger::logInfo("PASO 1 COMPLETADO: Registro limpiado exitosamente");
    }
    
    // Esperar un poco para que Windows procese
    Logger::logInfo("Esperando 1 segundo para que Windows procese...");
    QThread::msleep(1000);
    
    // Paso 2: Registrar ProgID
    Logger::logInfo("PASO 2: Registrando ProgID...");
    if (!registerProgId()) {
        errors << "Error al registrar ProgID";
        Logger::logError("PASO 2 FALLÓ: Error al registrar ProgID");
    } else {
        Logger::logInfo("PASO 2 COMPLETADO: ProgID registrado exitosamente");
    }
    
    // Paso 3: Configurar asociación
    Logger::logInfo("PASO 3: Configurando asociación...");
    if (!setFileAssociation()) {
        errors << "Error al configurar asociación";
        Logger::logError("PASO 3 FALLÓ: Error al configurar asociación");
    } else {
        Logger::logInfo("PASO 3 COMPLETADO: Asociación configurada exitosamente");
    }
    
    if (!errors.isEmpty()) {
        Logger::logError(QString("Se encontraron errores: %1").arg(errors.join("; ")));
        QMessageBox::warning(this, "Advertencias", 
            "Se encontraron algunos problemas:\n" + errors.join("\n") + 
            "\n\nLa asociación puede no funcionar completamente.");
    }
    
    // Notificar cambios al explorador
    Logger::logInfo("PASO 4: Notificando cambios al Explorador...");
    if (executeCommand("rundll32.exe", QStringList() << "shell32.dll,SHChangeNotify" << "0x08000000,0x0000,0,0")) {
        Logger::logInfo("PASO 4 COMPLETADO: Explorador notificado exitosamente");
    } else {
        Logger::logError("PASO 4 FALLÓ: Error al notificar al Explorador");
    }
    
    Logger::logInfo("=== COMANDOS DE REGISTRO COMPLETADOS ===");
}

bool ConfigWindow::executeCommand(const QString &program, const QStringList &arguments)
{
    QString commandStr = QString("%1 %2").arg(program, arguments.join(" "));
    Logger::logInfo(QString("=== EJECUTANDO COMANDO ==="));
    Logger::logInfo(QString("Comando: %1").arg(commandStr));
    
    QProcess process;
    process.start(program, arguments);
    
    if (!process.waitForStarted(3000)) {
        Logger::logError(QString("Error al iniciar comando: %1").arg(commandStr));
        Logger::logError(QString("Error del proceso: %1").arg(process.errorString()));
        return false;
    }
    
    if (!process.waitForFinished(10000)) {
        Logger::logError(QString("Timeout esperando comando: %1").arg(commandStr));
        process.kill();
        return false;
    }
    
    int exitCode = process.exitCode();
    QString stdOut = process.readAllStandardOutput();
    QString stdErr = process.readAllStandardError();
    
    Logger::logInfo(QString("Código de salida: %1").arg(exitCode));
    if (!stdOut.isEmpty()) {
        Logger::logInfo(QString("Salida estándar: %1").arg(stdOut.trimmed()));
    }
    if (!stdErr.isEmpty()) {
        Logger::logInfo(QString("Error estándar: %1").arg(stdErr.trimmed()));
    }
    
    if (exitCode != 0) {
        // Para comandos reg delete, el código de salida 1 es normal si la clave no existe
        if (program == "reg" && arguments.contains("delete") && exitCode == 1) {
            Logger::logInfo("Código 1 en reg delete es normal (clave no existe)");
            return true; // No es realmente un error
        }
        Logger::logError(QString("Comando falló con código: %1").arg(exitCode));
        return false;
    }
    
    Logger::logInfo("Comando ejecutado exitosamente");
    return true;
}

bool ConfigWindow::cleanRegistry()
{
    Logger::logInfo("Iniciando limpieza del registro...");
    
    // Lista de claves a eliminar
    QStringList keysToDelete = {
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.nk\\UserChoice",
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.nk",
        "HKEY_CURRENT_USER\\Software\\Classes\\.nk",
        "HKEY_CURRENT_USER\\Software\\Classes\\NukeScript",
        "HKEY_CURRENT_USER\\Software\\Classes\\nuke_auto_file",
        "HKEY_CURRENT_USER\\Software\\Classes\\Foundry.Nuke.Script",
        "HKEY_CURRENT_USER\\Software\\Classes\\LGA.NukeScript",
        "HKEY_CURRENT_USER\\Software\\Classes\\LGA.NukeScript.1",
        "HKEY_CURRENT_USER\\Software\\Classes\\Applications\\LGA_OpenInNukeX.exe",
        "HKEY_CURRENT_USER\\Software\\Classes\\Applications\\Nuke15.0.exe"
    };
    
    Logger::logInfo(QString("Ejecutando %1 comandos de limpieza...").arg(keysToDelete.size()));
    
    bool allSuccess = true;
    int successCount = 0;
    
    for (const QString& key : keysToDelete) {
        QStringList args;
        args << "delete" << key << "/f";
        
        if (executeCommand("reg", args)) {
            successCount++;
        } else {
            allSuccess = false;
        }
    }
    
    Logger::logInfo(QString("Limpieza completada: %1/%2 comandos exitosos").arg(successCount).arg(keysToDelete.size()));
    return allSuccess;
}

bool ConfigWindow::registerProgId()
{
    Logger::logInfo("Iniciando registro de ProgID...");
    
    QString progId = "LGA.NukeScript.1";
    QString exePath = QCoreApplication::applicationFilePath();
    QString iconPath = QDir(QCoreApplication::applicationDirPath()).filePath("app_icon.ico");
    
    Logger::logInfo(QString("ProgID: %1").arg(progId));
    Logger::logInfo(QString("Ejecutable: %1").arg(exePath));
    Logger::logInfo(QString("Icono: %1").arg(iconPath));
    
    QString escapedExePath = exePath;
    escapedExePath.replace("/", "\\");
    
    bool allSuccess = true;
    int successCount = 0;
    int totalCommands = 0;
    
    // 1. Registrar ProgID principal
    {
        QStringList args;
        args << "add" << QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(progId) 
             << "/ve" << "/t" << "REG_SZ" << "/d" << "Nuke Script File" << "/f";
        totalCommands++;
        if (executeCommand("reg", args)) {
            successCount++;
        } else {
            allSuccess = false;
        }
    }
    
    // 2. Registrar comando de apertura
    {
        QStringList args;
        args << "add" << QString("HKEY_CURRENT_USER\\Software\\Classes\\%1\\shell\\open\\command").arg(progId)
             << "/ve" << "/t" << "REG_SZ" << "/d" << QString("\"%1\" \"%2\"").arg(escapedExePath, "%1") << "/f";
        totalCommands++;
        if (executeCommand("reg", args)) {
            successCount++;
        } else {
            allSuccess = false;
        }
    }
    
    // 3. Registrar icono si existe
    if (QFile::exists(iconPath)) {
        Logger::logInfo("Icono encontrado, registrándolo...");
        QString escapedIconPath = iconPath;
        escapedIconPath.replace("/", "\\");
        
        QStringList args;
        args << "add" << QString("HKEY_CURRENT_USER\\Software\\Classes\\%1\\DefaultIcon").arg(progId)
             << "/ve" << "/t" << "REG_SZ" << "/d" << QString("\"%1\",0").arg(escapedIconPath) << "/f";
        totalCommands++;
        if (executeCommand("reg", args)) {
            successCount++;
        } else {
            allSuccess = false;
        }
    } else {
        Logger::logInfo("Icono no encontrado, omitiendo registro de icono");
    }
    
    Logger::logInfo(QString("Registro de ProgID completado: %1/%2 comandos exitosos").arg(successCount).arg(totalCommands));
    return allSuccess;
}

bool ConfigWindow::setFileAssociation()
{
    Logger::logInfo("Iniciando configuración de asociación de archivos...");
    
    QString progId = "LGA.NukeScript.1";
    QString setUserFtaPath = QDir(QCoreApplication::applicationDirPath()).filePath("SetUserFTA.exe");
    
    Logger::logInfo(QString("Buscando SetUserFTA en: %1").arg(setUserFtaPath));
    
    if (QFile::exists(setUserFtaPath)) {
        Logger::logInfo("SetUserFTA encontrado, usándolo para la asociación");
        bool result = executeCommand(setUserFtaPath, QStringList() << ".nk" << progId);
        Logger::logInfo(QString("SetUserFTA resultado: %1").arg(result ? "exitoso" : "falló"));
        return result;
    } else {
        Logger::logInfo("SetUserFTA no encontrado, usando asociación directa en registro");
        
        // CAMBIO CRÍTICO: Usar PowerShell en lugar de cmd para mejor compatibilidad
        QString powershellCommand = QString(
            "New-Item -Path 'HKCU:\\Software\\Classes\\.nk' -Force | Out-Null; "
            "Set-ItemProperty -Path 'HKCU:\\Software\\Classes\\.nk' -Name '(Default)' -Value '%1'"
        ).arg(progId);
        
        Logger::logInfo("Usando PowerShell para configurar asociación");
        bool result = executeCommand("powershell.exe", QStringList() << "-Command" << powershellCommand);
        
        if (!result) {
            Logger::logInfo("PowerShell falló, intentando con reg directo como fallback");
            QStringList args;
            args << "add" << "HKEY_CURRENT_USER\\Software\\Classes\\.nk" 
                 << "/ve" << "/t" << "REG_SZ" << "/d" << progId << "/f";
            result = executeCommand("reg", args);
        }
        
        Logger::logInfo(QString("Asociación directa resultado: %1").arg(result ? "exitoso" : "falló"));
        return result;
    }
}

void ConfigWindow::loadStyleSheet()
{
    // Buscar el archivo QSS en el directorio de la aplicación
    QString qssPath = QDir(QCoreApplication::applicationDirPath()).filePath("dark_theme.qss");
    
    QFile file(qssPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString styleSheet = file.readAll();
        file.close();
        
        // Aplicar el estilo a la aplicación
        this->setStyleSheet(styleSheet);
        
        Logger::logInfo(QString("Estilo QSS cargado desde: %1").arg(qssPath));
    } else {
        Logger::logInfo(QString("No se pudo cargar el archivo QSS desde: %1").arg(qssPath));
        
        // Fallback: aplicar estilo básico
        setStyleSheet("QWidget { background-color: #161616; color: #B2B2B2; }");
    }
}