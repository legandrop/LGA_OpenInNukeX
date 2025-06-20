#include "configwindow.h"
#include "logger.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFont>
#include <QPalette>
#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>

ConfigWindow::ConfigWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadCurrentPath();
    
    // Configurar ventana
    setWindowTitle("LGA OpenInNukeX - v0.14");
    setFixedSize(600, 280);
    
    // Establecer color de fondo #161616
    setStyleSheet("QWidget { background-color: #161616; color: #ffffff; }");
}

void ConfigWindow::setupUI()
{
    // Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // Título
    titleLabel = new QLabel("NukeX", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { color: #ffffff; margin-bottom: 5px; }");
    
    // Texto descriptivo
    descriptionLabel = new QLabel("Use this app to open .nk files in your preferred NukeX version", this);
    QFont descFont = descriptionLabel->font();
    descFont.setPointSize(10);
    descriptionLabel->setFont(descFont);
    descriptionLabel->setAlignment(Qt::AlignCenter);
    descriptionLabel->setStyleSheet("QLabel { color: #cccccc; margin-bottom: 10px; }");
    
    // Botón Apply
    applyButton = new QPushButton("APPLY", this);
    applyButton->setFixedSize(120, 35);
    applyButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #0078d4;"
        "    border: none;"
        "    border-radius: 6px;"
        "    color: #ffffff;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #106ebe;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #005a9e;"
        "}"
    );
    
    // Campo de texto para la ruta
    nukePathEdit = new QLineEdit(this);
    nukePathEdit->setPlaceholderText("Selecciona la ruta de NukeX.exe o Nuke.exe");
    nukePathEdit->setStyleSheet(
        "QLineEdit {"
        "    background-color: #2a2a2a;"
        "    border: 1px solid #404040;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "    font-size: 12px;"
        "    color: #ffffff;"
        "}"
        "QLineEdit:focus {"
        "    border: 1px solid #0078d4;"
        "}"
    );
    
    // Layout horizontal para el campo de texto y botón Browse
    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(nukePathEdit);
    
    // Botón Browse
    browseButton = new QPushButton("BROWSE", this);
    browseButton->setFixedSize(80, 32);
    browseButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #404040;"
        "    border: 1px solid #505050;"
        "    border-radius: 4px;"
        "    color: #ffffff;"
        "    font-weight: bold;"
        "    font-size: 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #505050;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #303030;"
        "}"
    );
    
    pathLayout->addWidget(browseButton);
    
    // Botón Save
    saveButton = new QPushButton("SAVE", this);
    saveButton->setFixedSize(100, 35);
    saveButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #28a745;"
        "    border: none;"
        "    border-radius: 6px;"
        "    color: #ffffff;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #218838;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1e7e34;"
        "}"
    );
    
    // Layout horizontal para los botones Apply y Save
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addStretch();
    
    // Agregar widgets al layout principal
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(pathLayout);
    
    // Conectar señales
    connect(browseButton, &QPushButton::clicked, this, &ConfigWindow::browseNukePath);
    connect(saveButton, &QPushButton::clicked, this, &ConfigWindow::saveConfiguration);
    connect(applyButton, &QPushButton::clicked, this, &ConfigWindow::applyFileAssociation);
}

void ConfigWindow::loadCurrentPath()
{
    QString currentPath = getNukePathFromFile();
    if (!currentPath.isEmpty()) {
        nukePathEdit->setText(currentPath);
    }
}

QString ConfigWindow::getNukePathFromFile()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir); // Crear directorio si no existe
    QString filepath = QDir(configDir).filePath("nukeXpath.txt");
    
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
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
        "Executable Files (*.exe);;All Files (*)"
    );
    
    if (!selectedPath.isEmpty()) {
        nukePathEdit->setText(selectedPath);
    }
}

void ConfigWindow::saveConfiguration()
{
    QString nukePath = nukePathEdit->text().trimmed();
    
    if (nukePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Por favor, selecciona una ruta válida para NukeX.");
        return;
    }
    
    // Verificar que el archivo existe
    if (!QFile::exists(nukePath)) {
        QMessageBox::warning(this, "Error", "El archivo seleccionado no existe.");
        return;
    }
    
    // Verificar que es un ejecutable de Nuke
    QString fileName = QFileInfo(nukePath).fileName().toLower();
    if (!fileName.contains("nuke")) {
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
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << path;
        file.close();
    }
}

void ConfigWindow::applyFileAssociation()
{
    QString currentExePath = getCurrentExecutablePath();
    
    if (currentExePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "No se pudo obtener la ruta del ejecutable actual.");
        return;
    }
    
    // Confirmar con el usuario
    int reply = QMessageBox::question(this, "Asociar Archivos .nk", 
        QString("¿Deseas asociar todos los archivos .nk para que se abran con:\n%1?\n\n"
                "Esto limpiará asociaciones previas y modificará el registro de Windows.").arg(currentExePath),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Primero limpiar asociaciones existentes
        cleanupFileAssociations();
        // Luego asociar archivos
        associateNkFiles();
    }
}

QString ConfigWindow::getCurrentExecutablePath()
{
    return QCoreApplication::applicationFilePath();
}

void ConfigWindow::associateNkFiles()
{
    Logger::log("=== INICIANDO PROCESO DE ASOCIACIÓN COMO EN EL .BAT ===");
    
    QString exePath = getCurrentExecutablePath();
    QString progId = "LGA.NukeScript.1";
    QString extension = ".nk";
    
    // PASO 1: Limpiar COMPLETAMENTE el registro (como en el .bat)
    Logger::log("PASO 1: Limpiando registro COMPLETAMENTE...");
    cleanupFileAssociations();
    
    // PASO 2: Registrar SOLO el ProgID (como en el .bat)
    Logger::log("PASO 2: Registrando ProgID limpio...");
    if (!registerOfficialProgId(progId, exePath)) {
        Logger::log("❌ ERROR: Falló el registro del ProgID");
        QMessageBox::warning(this, "Error", "No se pudo registrar el ProgID correctamente.");
        return;
    }
    
    // PASO 3: Usar SetUserFTA (como en el .bat)
    Logger::log("PASO 3: Usando SetUserFTA...");
    if (!executeSetUserFTA(extension, progId)) {
        Logger::log("❌ ERROR: SetUserFTA falló");
        return;
    }
    
    // PASO 4: SOLO verificar (SIN escribir nada más)
    Logger::log("PASO 4: Verificando resultado...");
    verifyFinalResult(extension, progId);
    
    Logger::log("=== PROCESO COMPLETADO ===");
}

bool ConfigWindow::registerOfficialProgId(const QString &progId, const QString &exePath)
{
#ifdef Q_OS_WIN
    Logger::log(QString("=== REGISTRANDO PROGID EXACTAMENTE COMO EN EL .BAT ==="));
    Logger::log(QString("ProgID: %1").arg(progId));
    Logger::log(QString("Ejecutable: %1").arg(exePath));
    
    // Verificar que el ejecutable existe
    if (!QFile::exists(exePath)) {
        Logger::logError(QString("❌ ERROR: El ejecutable no existe: %1").arg(exePath));
        return false;
    }
    
    try {
        // SOLO registrar el ProgID principal (como en el .bat)
        QString progIdKey = QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(progId);
        QSettings progIdSettings(progIdKey, QSettings::NativeFormat);
        
        // Descripción del ProgID
        progIdSettings.setValue("Default", "Nuke Script File");
        Logger::log("✓ Descripción registrada");
        
        // Comando de apertura (EXACTAMENTE como en el .bat)
        QString command = QString("\"%1\" \"%%1\"").arg(exePath);
        progIdSettings.setValue("shell/open/command/Default", command);
        Logger::log(QString("✓ Comando registrado: %1").arg(command));
        
        // Icono (opcional, como en el .bat)
        QString iconPath = QDir(QCoreApplication::applicationDirPath()).filePath("app_icon.ico");
        if (QFile::exists(iconPath)) {
            QString iconValue = QString("\"%1\",0").arg(QDir::toNativeSeparators(iconPath));
            progIdSettings.setValue("DefaultIcon/Default", iconValue);
            Logger::log(QString("✓ Icono registrado: %1").arg(iconValue));
        } else {
            Logger::log("⚠ Icono no encontrado, continuando sin icono");
        }
        
        // Sincronizar
        progIdSettings.sync();
        Logger::log("✓ ProgID sincronizado");
        
        // Verificar que se guardó
        QString savedCommand = progIdSettings.value("shell/open/command/Default").toString();
        if (savedCommand.isEmpty()) {
            Logger::logError("❌ ERROR: El comando no se guardó correctamente");
            return false;
        }
        
        Logger::log(QString("✅ ProgID registrado exitosamente: %1").arg(savedCommand));
        return true;
        
    } catch (const std::exception& e) {
        Logger::logError(QString("❌ EXCEPCIÓN: %1").arg(e.what()));
        return false;
    } catch (...) {
        Logger::logError("❌ ERROR DESCONOCIDO");
        return false;
    }
#else
    Q_UNUSED(progId)
    Q_UNUSED(exePath)
    return false;
#endif
}

bool ConfigWindow::executeSetUserFTA(const QString &extension, const QString &progId)
{
    Logger::log("=== EJECUTANDO SETUSERFTA EXACTAMENTE COMO EN EL .BAT ===");
    
    QString executablePath = QCoreApplication::applicationFilePath();
    QFileInfo appDir(executablePath);
    QString setUserFTAPath = appDir.absolutePath() + "/SetUserFTA.exe";
    
    // Verificar si SetUserFTA.exe existe
    if (!QFileInfo::exists(setUserFTAPath)) {
        Logger::log("❌ ERROR: SetUserFTA.exe no encontrado en: " + setUserFTAPath);
        Logger::log("📥 Descarga desde: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/");
        
        QMessageBox::warning(this, "❌ SetUserFTA Requerido", 
            "🚨 IMPORTANTE: SetUserFTA.exe no encontrado\n\n"
            "Para asociaciones de archivos en Windows 10/11 necesitas:\n"
            "📥 Descargar SetUserFTA.exe\n"
            "📁 Colocarlo junto a LGA_OpenInNukeX.exe\n\n"
            "⚠️ Sin SetUserFTA, Windows detecta las asociaciones como\n"
            "'hijacking' y las resetea automáticamente.\n\n"
            "Descarga desde: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/");
        return false;
    }
    
    // Ejecutar SetUserFTA EXACTAMENTE como en el .bat
    QProcess process;
    QStringList arguments;
    arguments << extension << progId;
    
    Logger::log(QString("🔧 Ejecutando: %1 %2").arg(setUserFTAPath, arguments.join(" ")));
    
    process.setProgram(setUserFTAPath);
    process.setArguments(arguments);
    process.setWorkingDirectory(appDir.absolutePath());
    
    process.start();
    
    if (!process.waitForStarted(5000)) {
        Logger::log("❌ ERROR: No se pudo iniciar SetUserFTA");
        QMessageBox::warning(this, "Error", "No se pudo ejecutar SetUserFTA.exe");
        return false;
    }
    
    if (!process.waitForFinished(15000)) {
        Logger::log("❌ ERROR: SetUserFTA timeout");
        process.kill();
        QMessageBox::warning(this, "Error", "SetUserFTA tardó demasiado en ejecutarse");
        return false;
    }
    
    int exitCode = process.exitCode();
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();
    
    Logger::log(QString("SetUserFTA terminó con código: %1").arg(exitCode));
    if (!output.isEmpty()) {
        Logger::log("📤 Salida: " + output.trimmed());
    }
    if (!error.isEmpty()) {
        Logger::log("❌ Error: " + error.trimmed());
    }
    
    if (exitCode == 0) {
        Logger::log("✅ SetUserFTA ejecutado exitosamente");
        
        // Notificar al explorador de cambios (como en el .bat)
#ifdef Q_OS_WIN
        Sleep(500);
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
#endif
        Logger::log("✅ Explorador notificado de cambios");
        
        QMessageBox::information(this, "Asociación Exitosa", 
            "✅ Los archivos .nk ahora se abrirán con LGA_OpenInNukeX.\n\n"
            "🔧 La asociación se estableció usando SetUserFTA (método oficial)\n"
            "🛡️ Windows no detectará esto como 'hijacking'\n\n"
            "Si no funciona inmediatamente:\n"
            "• Reinicia el Explorador de Windows\n"
            "• Cierra y vuelve a abrir las ventanas del explorador");
        
        return true;
    } else {
        Logger::log("❌ ERROR: SetUserFTA falló con código: " + QString::number(exitCode));
        
        QString errorMsg = "SetUserFTA falló al establecer la asociación.\n\n";
        errorMsg += QString("Código de error: %1\n").arg(exitCode);
        
        if (!error.isEmpty()) {
            errorMsg += "Error: " + error + "\n";
        }
        
        errorMsg += "\nPosibles soluciones:\n";
        errorMsg += "• Ejecuta la aplicación como administrador\n";
        errorMsg += "• Verifica que SetUserFTA.exe no esté bloqueado por antivirus\n";
        errorMsg += "• Reinicia Windows y vuelve a intentar";
        
        QMessageBox::warning(this, "Error en SetUserFTA", errorMsg);
        return false;
    }
}

void ConfigWindow::verifyFinalResult(const QString &extension, const QString &progId)
{
#ifdef Q_OS_WIN
    Logger::log("=== VERIFICANDO RESULTADO FINAL (SOLO LECTURA) ===");
    
    // SOLO verificar UserChoice (NO escribir nada)
    QString userChoiceKey = QString("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1\\UserChoice").arg(extension);
    QSettings userChoiceSettings(userChoiceKey, QSettings::NativeFormat);
    QString currentProgId = userChoiceSettings.value("ProgId").toString();
    QString hash = userChoiceSettings.value("Hash").toString();
    
    Logger::log(QString("🔍 UserChoice ProgId: %1").arg(currentProgId.isEmpty() ? "VACÍO" : currentProgId));
    Logger::log(QString("🔍 UserChoice Hash: %1").arg(hash.isEmpty() ? "VACÍO" : hash));
    
    if (currentProgId == progId && !hash.isEmpty()) {
        Logger::log("✅ UserChoice configurado correctamente por SetUserFTA");
    } else if (currentProgId.isEmpty()) {
        Logger::log("❌ UserChoice VACÍO - SetUserFTA no funcionó");
    } else {
        Logger::log(QString("⚠ UserChoice diferente: esperado %1, encontrado %2").arg(progId, currentProgId));
    }
    
    // SOLO verificar que el ProgID existe (NO escribir nada)
    QString progIdKey = QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(progId);
    QSettings progIdSettings(progIdKey, QSettings::NativeFormat);
    QString description = progIdSettings.value("Default").toString();
    QString command = progIdSettings.value("shell/open/command/Default").toString();
    
    Logger::log(QString("🔍 ProgID Descripción: %1").arg(description.isEmpty() ? "VACÍA" : description));
    Logger::log(QString("🔍 ProgID Comando: %1").arg(command.isEmpty() ? "VACÍO" : command));
    
    if (!command.isEmpty()) {
        Logger::log("✅ ProgID registrado correctamente");
    } else {
        Logger::log("❌ ProgID VACÍO o mal registrado");
    }
    
    // Resumen final
    Logger::log("=== RESUMEN FINAL ===");
    if (currentProgId == progId && !hash.isEmpty() && !command.isEmpty()) {
        Logger::log("✅ ASOCIACIÓN EXITOSA - Todo configurado correctamente");
    } else {
        Logger::log("❌ ASOCIACIÓN PROBLEMÁTICA - Revisar logs arriba");
    }
    
    Logger::log("=== FIN VERIFICACIÓN ===");
    
#else
    Q_UNUSED(extension)
    Q_UNUSED(progId)
#endif
}

void ConfigWindow::cleanupFileAssociations()
{
#ifdef Q_OS_WIN
    Logger::log("=== LIMPIEZA TOTAL DEL REGISTRO (COMO EN EL .BAT) ===");
    
    QString extension = ".nk";
    
    try {
        // 1. Limpiar UserChoice (lo más importante, como en el .bat)
        Logger::log("Limpiando UserChoice...");
        QString userChoiceKey = QString("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1\\UserChoice").arg(extension);
        QSettings userChoiceSettings(userChoiceKey, QSettings::NativeFormat);
        userChoiceSettings.clear();
        userChoiceSettings.sync();
        
        // También limpiar toda la clave FileExts/.nk
        QString fileExtsKey = QString("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1").arg(extension);
        QSettings fileExtsSettings(fileExtsKey, QSettings::NativeFormat);
        fileExtsSettings.clear();
        fileExtsSettings.sync();
        Logger::log("✓ UserChoice y FileExts limpiados");
        
        // 2. Limpiar TODAS las asociaciones directas (como en el .bat)
        Logger::log("Limpiando asociaciones directas...");
        QString classesKey = QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(extension);
        QSettings classesSettings(classesKey, QSettings::NativeFormat);
        classesSettings.clear();
        classesSettings.sync();
        Logger::log("✓ Classes/.nk limpiado");
        
        // 3. Limpiar TODOS los ProgIDs posibles (como en el .bat)
        Logger::log("Limpiando ProgIDs antiguos...");
        QStringList oldProgIds = {"NukeScript", "nuke_auto_file", "Foundry.Nuke.Script", "LGA.NukeScript", "LGA.NukeScript.1"};
        for (const QString &oldProgId : oldProgIds) {
            QString oldProgIdKey = QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(oldProgId);
            QSettings oldProgIdSettings(oldProgIdKey, QSettings::NativeFormat);
            oldProgIdSettings.clear();
            oldProgIdSettings.sync();
        }
        Logger::log("✓ ProgIDs antiguos limpiados");
        
        // 4. Limpiar Applications (como en el .bat)
        Logger::log("Limpiando Applications...");
        QStringList appKeys = {"LGA_OpenInNukeX.exe", "Nuke15.0.exe"};
        for (const QString &appKey : appKeys) {
            QString applicationKey = QString("HKEY_CURRENT_USER\\Software\\Classes\\Applications\\%1").arg(appKey);
            QSettings applicationSettings(applicationKey, QSettings::NativeFormat);
            applicationSettings.clear();
            applicationSettings.sync();
        }
        Logger::log("✓ Applications limpiadas");
        
        // 5. Notificar cambios (como en el .bat)
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
        Logger::log("✓ Sistema notificado de limpieza");
        
        // 6. Esperar que Windows procese (como en el .bat)
        Logger::log("Esperando que Windows procese la limpieza...");
        Sleep(2000);  // 2 segundos como en el .bat
        
        Logger::log("✓ Limpieza total completada");
        
    } catch (const std::exception& e) {
        Logger::logError(QString("❌ ERROR durante limpieza: %1").arg(e.what()));
    } catch (...) {
        Logger::logError("❌ ERROR DESCONOCIDO durante limpieza");
    }
#endif
}

 