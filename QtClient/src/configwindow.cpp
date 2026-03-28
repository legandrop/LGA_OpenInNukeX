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
#include "qflowlayout.h"

ConfigWindow::ConfigWindow(QWidget *parent)
    : QWidget(parent)
    , nukeScanner(nullptr)
    , versionsContainer(nullptr)
    , scanningLabel(nullptr)
    , foundVersionsLabel(nullptr)
    , versionsButtonsWidget(nullptr)
    , versionsLayout(nullptr)
{
    Logger::logInfo("=== CONSTRUCTOR ConfigWindow INICIADO ===");
    
    Logger::logInfo("Llamando a setupUI()...");
    setupUI();
    Logger::logInfo("✓ setupUI() completado");
    
    Logger::logInfo("Llamando a loadCurrentPath()...");
    loadCurrentPath();
    Logger::logInfo("✓ loadCurrentPath() completado");

    // Configurar ventana
    Logger::logInfo("Configurando ventana...");
    setWindowTitle("OpenInNukeX Config");
    setFixedSize(900, 600);
    Logger::logInfo("✓ Ventana configurada");

    // Cargar estilo QSS
    Logger::logInfo("Cargando estilo QSS...");
    loadStyleSheet();
    Logger::logInfo("✓ Estilo QSS cargado");
    
    // Inicializar y comenzar el escaneo de versiones
    Logger::logInfo("Iniciando proceso de escáner...");
    initializeScanner();
    Logger::logInfo("✓ initializeScanner() llamado");
    
    Logger::logInfo("=== CONSTRUCTOR ConfigWindow COMPLETADO ===");
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
    horizontalCenterLayout->setContentsMargins(0, 20, 0, 20);
    horizontalCenterLayout->setSpacing(0);
    
    // Crear un widget contenedor para el contenido real
    QWidget *contentWidget = new QWidget();
    contentWidget->setObjectName("contentSettingsWidget");
    contentWidget->setFixedWidth(700); // Ancho para todas las cajas principales

    // Layout para el contenido
    QVBoxLayout *centralLayout = new QVBoxLayout(contentWidget);
    centralLayout->setContentsMargins(20, 20, 20, 20);
    centralLayout->setSpacing(24);
    
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
    descriptionLabel = new QLabel("Associate .nk files with OpenInNukeX to open them directly in your preferred NukeX version", fileAssociationGroup);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet("QLabel { color: #8A8A8A; font-size: 15px; }");
    fileAssociationLayout->addWidget(descriptionLabel);
    fileAssociationLayout->addSpacing(15);

    // Botón Apply para asociación de archivos
    QHBoxLayout *applyLayout = new QHBoxLayout();
    applyLayout->setContentsMargins(0, 0, 0, 0);
    applyLayout->setSpacing(10);
    
    applyButton = new QPushButton("APPLY", this);
    applyButton->setFixedHeight(40);
    applyButton->setProperty("class", "action");
    
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

    // ===== SECCIÓN DE VERSIONES ENCONTRADAS (ARRIBA) =====
    Logger::logInfo("=== CREANDO SECCIÓN DE VERSIONES ENCONTRADAS ===");
    
    // Crear contenedor para las versiones encontradas
    versionsContainer = new QWidget(nukeVersionGroup);
    versionsContainer->setObjectName("versionsMainContainer");
    // NO aplicar setStyleSheet aquí porque sobrescribe los estilos de los botones hijos
    Logger::logInfo("✓ versionsContainer creado");
    
    versionsLayout = new QVBoxLayout(versionsContainer);
    versionsLayout->setContentsMargins(0, 0, 0, 0);
    versionsLayout->setSpacing(10);
    Logger::logInfo("✓ versionsLayout creado y configurado");
    
    // Etiqueta descriptiva
    foundVersionsLabel = new QLabel("Choose one of the found versions or browse your own:", versionsContainer);
    foundVersionsLabel->setStyleSheet("QLabel { color: #8A8A8A; font-size: 15px; }");
    foundVersionsLabel->setWordWrap(true);
    versionsLayout->addWidget(foundVersionsLabel);
    Logger::logInfo("✓ foundVersionsLabel creado y agregado");
    
    // Etiqueta de scanning (inicialmente visible)
    scanningLabel = new QLabel("🔍 Scanning for installed Nuke versions...", versionsContainer);
    scanningLabel->setStyleSheet("QLabel { color: #4A9EFF; font-size: 15px; font-style: italic; }");
    versionsLayout->addWidget(scanningLabel);
    Logger::logInfo("✓ scanningLabel creado y agregado");
    
    // Widget contenedor para los botones de versiones (inicialmente oculto)
    versionsButtonsWidget = new QWidget(versionsContainer);
    versionsButtonsWidget->setObjectName("versionsButtonsContainer");
    versionsButtonsWidget->setVisible(false);
    // NO establecer fondo transparente para que los botones mantengan sus estilos
    Logger::logInfo("✓ versionsButtonsWidget creado y ocultado");
    
    // Layout de flujo para organizar los botones en filas
    QFlowLayout *flowLayout = new QFlowLayout(versionsButtonsWidget, 0, 10, 10);
    Logger::logInfo("✓ QFlowLayout creado para versionsButtonsWidget");
    
    versionsLayout->addWidget(versionsButtonsWidget);
    Logger::logInfo("✓ versionsButtonsWidget agregado a versionsLayout");
    
    // Agregar el contenedor de versiones al grupo
    nukeVersionLayout->addWidget(versionsContainer);
    nukeVersionLayout->addSpacing(15);
    Logger::logInfo("✓ versionsContainer agregado a nukeVersionLayout");
    
    Logger::logInfo("=== SECCIÓN DE VERSIONES COMPLETADA ===");

    // Crear layout horizontal para el campo de path y el botón browse
    QHBoxLayout *nukePathLayout = new QHBoxLayout();
    nukePathLayout->setContentsMargins(0, 0, 0, 0);
    nukePathLayout->setSpacing(20);
    
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
    

    
    // Boton de SAVE (DENTRO del grupo Nuke Version)
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 15, 0, 0);
    buttonLayout->setSpacing(10);

    saveButton = new QPushButton("SAVE", nukeVersionGroup);
    saveButton->setFixedHeight(40);
    saveButton->setMinimumWidth(70);
    saveButton->setProperty("class", "action");

    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);

    nukeVersionLayout->addLayout(buttonLayout);
    
    // Agregar el grupo de Nuke Version al layout central
    centralLayout->addWidget(nukeVersionGroup);
    
    // Agregar un stretch al final para evitar que los widgets se estiren verticalmente
    centralLayout->addSpacing(-20);

    // Añadir label de versión y autor
    QLabel *versionLabel = new QLabel("v1.54 | Lega", contentWidget);
    versionLabel->setObjectName("versionLabel"); // Añadir un objectName para posible estilo futuro
    versionLabel->setStyleSheet("QLabel { color: #8A8A8A; font-size: 15px; }"); // Mismo estilo que descriptionLabel
    versionLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter); // Alinear a la derecha y al centro verticalmente
    
    // Crear un layout horizontal para contener el versionLabel y empujarlo a la derecha
    QHBoxLayout *versionLayout = new QHBoxLayout();
    versionLayout->addStretch(1); // Empuja el label a la derecha
    versionLayout->addWidget(versionLabel);
    versionLayout->setContentsMargins(0, 0, 0, 0); // Márgenes para el layout de la versión

    centralLayout->addLayout(versionLayout); // Añadir el layout de la versión al layout central
    centralLayout->addSpacing(-40);

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

#ifdef Q_OS_WIN
    QString startDir = currentPath.isEmpty() ? "C:/Program Files/" : QDir(currentPath).absolutePath();
    QString selectedPath = QFileDialog::getOpenFileName(
        this,
        "Select NukeX.exe or Nuke.exe",
        startDir,
        "Executable Files (*.exe);;All Files (*)");
    if (!selectedPath.isEmpty())
        nukePathEdit->setText(selectedPath);
#else
    // On macOS, start at /Applications and accept any file (incl. .app bundles)
    QString startDir = currentPath.isEmpty() ? "/Applications" : QDir(currentPath).absolutePath();
    QString selectedPath = QFileDialog::getOpenFileName(
        this,
        "Select Nuke application or binary",
        startDir,
        "All Files (*)");
    if (!selectedPath.isEmpty()) {
        // If the user picked a .app bundle, resolve the binary inside it
        if (selectedPath.endsWith(".app", Qt::CaseInsensitive)) {
            QString resolved = resolveNukeBinaryFromBundle(selectedPath);
            if (!resolved.isEmpty())
                selectedPath = resolved;
        }
        nukePathEdit->setText(selectedPath);
    }
#endif
}

void ConfigWindow::saveConfiguration()
{
    QString nukePath = nukePathEdit->text().trimmed();

    if (nukePath.isEmpty())
    {
        QMessageBox::warning(this, "Error", "Please select a valid path for NukeX.");
        return;
    }

    // Verificar que el archivo existe
    if (!QFile::exists(nukePath))
    {
        QMessageBox::warning(this, "Error", "The selected file does not exist.");
        return;
    }

    // Verificar que parece un ejecutable de Nuke
    QString fileName = QFileInfo(nukePath).fileName().toLower();
    if (!fileName.contains("nuke"))
    {
        QMessageBox::warning(this, "Warning", "The selected file does not appear to be a Nuke executable.");
    }

#ifndef Q_OS_WIN
    // On macOS, if the user somehow saved a .app path, resolve the binary now
    if (nukePath.endsWith(".app", Qt::CaseInsensitive)) {
        QString resolved = resolveNukeBinaryFromBundle(nukePath);
        if (!resolved.isEmpty()) {
            nukePath = resolved;
            nukePathEdit->setText(nukePath);
        }
    }
#endif

    saveNukePath(nukePath);

    QMessageBox::information(this, "Configuration Saved", "The NukeX path has been saved successfully.");

    // Cerrar la aplicación
    // QApplication::quit();
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
        QMessageBox::warning(this, "Error", "Please select a valid path for NukeX first.");
        return;
    }

    if (!QFile::exists(nukePath)) {
        Logger::logError(QString("Error: El archivo no existe: %1").arg(nukePath));
        QMessageBox::warning(this, "Error", "The selected file does not exist.");
        return;
    }

    Logger::logInfo("Guardando configuración de NukeX");
    saveNukePath(nukePath);

#ifdef Q_OS_WIN
    try {
        Logger::logInfo("Ejecutando comandos de registro (Windows)");
        executeRegistryCommands();
        Logger::logInfo("Asociación completada exitosamente");
        QMessageBox::information(this, "Association Completed",
            "The .nk file association has been successfully configured.\n\n"
            "You can now double-click .nk files to open them with NukeX.");
    } catch (const std::exception& e) {
        Logger::logError(QString("Excepción durante asociación: %1").arg(e.what()));
        QMessageBox::critical(this, "Error",
            QString("Error configuring file association:\n%1").arg(e.what()));
    }
#else
    executeMacAssociation();
#endif
}

// ─── executeCommand: shared by Windows and macOS ─────────────────────────────
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
    if (!stdOut.isEmpty())
        Logger::logInfo(QString("Salida estándar: %1").arg(stdOut.trimmed()));
    if (!stdErr.isEmpty())
        Logger::logInfo(QString("Error estándar: %1").arg(stdErr.trimmed()));

    if (exitCode != 0) {
        // Para 'reg delete', código 1 es normal si la clave no existe
        if (program == "reg" && arguments.contains("delete") && exitCode == 1) {
            Logger::logInfo("Código 1 en reg delete es normal (clave no existe)");
            return true;
        }
        Logger::logError(QString("Comando falló con código: %1").arg(exitCode));
        return false;
    }

    Logger::logInfo("Comando ejecutado exitosamente");
    return true;
}

// ─── Windows-only: registry functions ────────────────────────────────────────
#ifdef Q_OS_WIN

void ConfigWindow::executeRegistryCommands()
{
    Logger::logInfo("=== EJECUTANDO COMANDOS DE REGISTRO ===");
    QStringList errors;

    Logger::logInfo("PASO 1: Limpiando registro...");
    if (!cleanRegistry()) {
        errors << "Error al limpiar el registro";
        Logger::logError("PASO 1 FALLÓ");
    } else {
        Logger::logInfo("PASO 1 COMPLETADO");
    }

    Logger::logInfo("Esperando 1 segundo para que Windows procese...");
    QThread::msleep(1000);

    Logger::logInfo("PASO 2: Registrando ProgID...");
    if (!registerProgId()) {
        errors << "Error al registrar ProgID";
        Logger::logError("PASO 2 FALLÓ");
    } else {
        Logger::logInfo("PASO 2 COMPLETADO");
    }

    Logger::logInfo("PASO 3: Configurando asociación...");
    if (!setFileAssociation()) {
        errors << "Error al configurar asociación";
        Logger::logError("PASO 3 FALLÓ");
    } else {
        Logger::logInfo("PASO 3 COMPLETADO");
    }

    if (!errors.isEmpty()) {
        Logger::logError(QString("Se encontraron errores: %1").arg(errors.join("; ")));
        QMessageBox::warning(this, "Warnings",
            "Some issues were found:\n" + errors.join("\n") +
            "\n\nFile association may not work completely.");
    }

    Logger::logInfo("PASO 4: Notificando cambios al Explorador...");
    if (executeCommand("rundll32.exe",
            QStringList() << "shell32.dll,SHChangeNotify" << "0x08000000,0x0000,0,0")) {
        Logger::logInfo("PASO 4 COMPLETADO");
    } else {
        Logger::logError("PASO 4 FALLÓ");
    }

    Logger::logInfo("=== COMANDOS DE REGISTRO COMPLETADOS ===");
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

#endif // Q_OS_WIN

// ─────────────────────────────────────────────────────────────────────────────
//  macOS: file association via Launch Services + duti
// ─────────────────────────────────────────────────────────────────────────────
#ifndef Q_OS_WIN

QString ConfigWindow::getAppBundlePath() const
{
    // applicationDirPath() on macOS = .../LGA_OpenInNukeX.app/Contents/MacOS
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp(); // Contents
    dir.cdUp(); // LGA_OpenInNukeX.app
    return dir.absolutePath();
}

QString ConfigWindow::resolveNukeBinaryFromBundle(const QString &bundlePath) const
{
    QDir macosDir(bundlePath + "/Contents/MacOS");
    if (!macosDir.exists()) return QString();

    QFileInfoList files = macosDir.entryInfoList(QDir::Files | QDir::Executable);
    for (const QFileInfo &f : files) {
        if (f.fileName().toLower().contains("nuke"))
            return f.absoluteFilePath();
    }
    return QString();
}

void ConfigWindow::executeMacAssociation()
{
    Logger::logInfo("=== ASOCIACION macOS via Launch Services ===");

    QString bundlePath = getAppBundlePath();
    Logger::logInfo(QString("Bundle path: %1").arg(bundlePath));

    // Step 1: Register the app bundle with Launch Services
    QString lsregister =
        "/System/Library/Frameworks/CoreServices.framework"
        "/Frameworks/LaunchServices.framework/Support/lsregister";

    if (QFile::exists(lsregister)) {
        Logger::logInfo("Registrando bundle con Launch Services...");
        executeCommand(lsregister, {"-f", bundlePath});
        Logger::logInfo("Bundle registrado");
    } else {
        Logger::logError("lsregister no encontrado");
    }

    // Step 2: Try 'duti' to set LGA_OpenInNukeX as default handler for .nk
    bool dutiSuccess = false;
    QProcess dutiCheck;
    dutiCheck.start("which", {"duti"});
    dutiCheck.waitForFinished(3000);

    if (dutiCheck.exitCode() == 0) {
        Logger::logInfo("duti encontrado, configurando handler por defecto...");
        dutiSuccess = executeCommand("duti", {"-s", "com.lga.openinnukex", ".nk", "all"});
        Logger::logInfo(QString("duti resultado: %1").arg(dutiSuccess ? "exitoso" : "fallo"));
    } else {
        Logger::logInfo("duti no encontrado (brew install duti)");
    }

    if (dutiSuccess) {
        QMessageBox::information(this, "Association Completed",
            "The .nk file association has been successfully configured.\n\n"
            "You can now double-click .nk files to open them with LGA_OpenInNukeX.");
    } else {
        QMessageBox::information(this, "Almost done!",
            "The app has been registered with your system.\n\n"
            "To set LGA_OpenInNukeX as the default app for .nk files:\n\n"
            "1. Right-click any .nk file in Finder\n"
            "2. Choose \"Get Info\"  (Cmd+I)\n"
            "3. Under \"Open with:\", select LGA_OpenInNukeX\n"
            "4. Click \"Change All...\"\n\n"
            "Tip: install duti via Homebrew (brew install duti) to automate this step.");
    }
}

#endif // !Q_OS_WIN

void ConfigWindow::loadStyleSheet()
{
#ifdef Q_OS_WIN
    // Windows: QSS lives next to the executable
    QString qssPath = QDir(QCoreApplication::applicationDirPath()).filePath("dark_theme.qss");
#else
    // macOS: QSS is embedded in the bundle at Contents/Resources/
    QString qssPath = QDir(QCoreApplication::applicationDirPath() + "/../Resources").filePath("dark_theme.qss");
    // Fallback: same dir as executable (dev builds without bundle)
    if (!QFile::exists(qssPath))
        qssPath = QDir(QCoreApplication::applicationDirPath()).filePath("dark_theme.qss");
#endif
    
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

// ===== FUNCIONES DEL ESCÁNER DE VERSIONES =====

void ConfigWindow::initializeScanner()
{
    Logger::logInfo("=== INICIALIZANDO ESCÁNER DE VERSIONES DE NUKE ===");
    
    // Verificar que los elementos UI existen
    if (!scanningLabel) {
        Logger::logError("ERROR: scanningLabel es nullptr");
        return;
    }
    if (!versionsButtonsWidget) {
        Logger::logError("ERROR: versionsButtonsWidget es nullptr");
        return;
    }
    if (!foundVersionsLabel) {
        Logger::logError("ERROR: foundVersionsLabel es nullptr");
        return;
    }
    
    Logger::logInfo("✓ Elementos UI verificados correctamente");
    
    // Crear el escáner
    nukeScanner = new NukeScanner(this);
    Logger::logInfo("✓ NukeScanner creado");
    
    // Conectar señales
    connect(nukeScanner, &NukeScanner::scanStarted, this, &ConfigWindow::onScanStarted);
    connect(nukeScanner, &NukeScanner::scanProgress, this, &ConfigWindow::onScanProgress);
    connect(nukeScanner, &NukeScanner::versionFound, this, &ConfigWindow::onVersionFound);
    connect(nukeScanner, &NukeScanner::scanFinished, this, &ConfigWindow::onScanFinished);
    Logger::logInfo("✓ Señales conectadas");
    
    // Iniciar el escaneo
    Logger::logInfo("Iniciando escaneo...");
    nukeScanner->startScan();
}

void ConfigWindow::onScanStarted()
{
    Logger::logInfo("=== EVENTO: ESCANEO INICIADO ===");
    Logger::logInfo("Actualizando UI para mostrar estado de scanning...");
    
    if (scanningLabel) {
        scanningLabel->setText("🔍 Scanning for installed Nuke versions...");
        scanningLabel->setVisible(true);
        Logger::logInfo("✓ scanningLabel actualizado y mostrado");
    } else {
        Logger::logError("ERROR: scanningLabel es nullptr en onScanStarted");
    }
    
    if (versionsButtonsWidget) {
        versionsButtonsWidget->setVisible(false);
        Logger::logInfo("✓ versionsButtonsWidget ocultado");
    } else {
        Logger::logError("ERROR: versionsButtonsWidget es nullptr en onScanStarted");
    }
}

void ConfigWindow::onScanProgress(const QString &currentPath)
{
    if (scanningLabel) {
        QString shortPath = currentPath;
        if (shortPath.length() > 50) {
            shortPath = "..." + shortPath.right(47);
        }
        scanningLabel->setText(QString("🔍 Scanning: %1").arg(shortPath));
    }
}

void ConfigWindow::onVersionFound(const NukeVersion &version)
{
    Logger::logInfo(QString("UI: Nueva versión encontrada - %1").arg(version.displayName));
    // La creación de botones se hace en onScanFinished para mejor rendimiento
}

void ConfigWindow::onScanFinished(const QList<NukeVersion> &versions)
{
    Logger::logInfo(QString("=== EVENTO: ESCANEO COMPLETADO - %1 versiones encontradas ===").arg(versions.size()));
    
    // Ocultar mensaje de scanning
    if (scanningLabel) {
        scanningLabel->setVisible(false);
        Logger::logInfo("✓ scanningLabel ocultado");
    } else {
        Logger::logError("ERROR: scanningLabel es nullptr en onScanFinished");
    }
    
    if (versions.isEmpty()) {
        Logger::logInfo("No se encontraron versiones - mostrando mensaje de error");
        // No se encontraron versiones
        if (scanningLabel) {
            scanningLabel->setText("❌ No Nuke installations found in common locations");
            scanningLabel->setStyleSheet("QLabel { color: #FF6B6B; font-size: 15px; font-style: italic; }");
            scanningLabel->setVisible(true);
            Logger::logInfo("✓ Mensaje de 'no encontrado' mostrado");
        }
        return;
    }
    
    Logger::logInfo("Creando botones para las versiones encontradas...");
    // Crear botones para las versiones encontradas
    createVersionButtons(versions);
    
    // Mostrar el contenedor de botones
    if (versionsButtonsWidget) {
        versionsButtonsWidget->setVisible(true);
        Logger::logInfo("✓ versionsButtonsWidget mostrado");
    } else {
        Logger::logError("ERROR: versionsButtonsWidget es nullptr al intentar mostrar");
    }
    
    // Actualizar mensaje descriptivo
    if (foundVersionsLabel) {
        foundVersionsLabel->setText(QString("%1 Nuke versions found:").arg(versions.size()));
        Logger::logInfo("✓ foundVersionsLabel actualizado");
    } else {
        Logger::logError("ERROR: foundVersionsLabel es nullptr al actualizar");
    }

    // Calcular y ajustar el tamaño de la ventana para acomodar los nuevos elementos
    calculateAndResizeWindow();
}

void ConfigWindow::createVersionButtons(const QList<NukeVersion> &versions)
{
    if (!versionsButtonsWidget) {
        return;
    }
    
    // Obtener el layout de flujo existente
    QFlowLayout *flowLayout = dynamic_cast<QFlowLayout*>(versionsButtonsWidget->layout());
    if (!flowLayout) {
        flowLayout = new QFlowLayout(versionsButtonsWidget, 0, 10, 10);
    }
    
    // Limpiar botones existentes
    QLayoutItem *item;
    while ((item = flowLayout->takeAt(0))) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    // Crear botón para cada versión encontrada
    for (const NukeVersion &version : versions) {
        QPushButton *versionButton = new QPushButton(version.displayName, versionsButtonsWidget);
        versionButton->setFixedHeight(35);
        versionButton->setMinimumWidth(120);
        versionButton->setProperty("class", "version");
        
        // Guardar la ruta en una propiedad del botón
        versionButton->setProperty("nukePath", version.path);
        
        // Conectar el click
        connect(versionButton, &QPushButton::clicked, this, &ConfigWindow::onVersionButtonClicked);
        
        // Agregar al layout
        flowLayout->addWidget(versionButton);
        
        Logger::logInfo(QString("Botón creado para: %1 -> %2").arg(version.displayName, version.path));
    }
}

void ConfigWindow::onVersionButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) {
        return;
    }
    
    QString nukePath = button->property("nukePath").toString();
    if (nukePath.isEmpty()) {
        Logger::logError("Error: No se pudo obtener la ruta del botón de versión");
        return;
    }
    
    Logger::logInfo(QString("Usuario seleccionó versión: %1 -> %2").arg(button->text(), nukePath));
    
    // Actualizar el campo de texto con la ruta seleccionada
    if (nukePathEdit) {
        nukePathEdit->setText(nukePath);
        Logger::logInfo("✓ Campo de texto actualizado con la ruta seleccionada");
    }
}

void ConfigWindow::calculateAndResizeWindow()
{
    Logger::logInfo("=== CALCULANDO Y REDIMENSIONANDO VENTANA ===");
    
    // Forzar que todos los widgets calculen su tamaño
    this->updateGeometry();
    QApplication::processEvents();
    
    // Obtener el tamaño recomendado del contenido principal
    QWidget *centralWidget = this->findChild<QWidget*>("centralSettingsWidget");
    if (!centralWidget) {
        Logger::logError("ERROR: No se pudo encontrar centralSettingsWidget");
        return;
    }
    
    // Calcular la altura necesaria basándose en el contenido
    int contentHeight = centralWidget->sizeHint().height();
    Logger::logInfo(QString("Altura sugerida del contenido: %1").arg(contentHeight));
    
    // Agregar márgenes y espacios adicionales
    int margins = 40; // Márgenes superior e inferior
    int scrollBarSpace = 20; // Espacio para posible scroll bar
    int totalHeight = contentHeight + margins + scrollBarSpace;
    
    // Establecer límites mínimos y máximos
    int minHeight = 400;
    int maxHeight = 1000;
    
    // Ajustar dentro de los límites
    if (totalHeight < minHeight) {
        totalHeight = minHeight;
        Logger::logInfo(QString("Altura ajustada al mínimo: %1").arg(totalHeight));
    } else if (totalHeight > maxHeight) {
        totalHeight = maxHeight;
        Logger::logInfo(QString("Altura ajustada al máximo: %1").arg(totalHeight));
    }
    
    Logger::logInfo(QString("Altura final calculada: %1").arg(totalHeight));
    
    // Redimensionar la ventana manteniendo el ancho fijo de 900px
    this->setFixedSize(800, totalHeight);
    
    Logger::logInfo(QString("Ventana redimensionada a: 900x%1").arg(totalHeight));
}