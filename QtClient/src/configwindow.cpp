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
                "Esto modificará el registro de Windows.").arg(currentExePath),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        associateNkFiles();
    }
}

QString ConfigWindow::getCurrentExecutablePath()
{
    return QCoreApplication::applicationFilePath();
}

void ConfigWindow::associateNkFiles()
{
    QString exePath = getCurrentExecutablePath();
    QString iconPath = QDir(QCoreApplication::applicationDirPath()).filePath("app_icon.ico");
    
    // Verificar que el icono existe
    if (!QFile::exists(iconPath)) {
        Logger::log("⚠ Icono no encontrado, buscando en deploy...");
        iconPath = QDir(QCoreApplication::applicationDirPath()).filePath("deploy/app_icon.ico");
        if (!QFile::exists(iconPath)) {
            iconPath.clear();
        }
    }
    
    // Establecer como aplicación predeterminada usando SetUserFTA
    bool success = setAsDefaultApp();
        
    if (success) {
        // Notificar al shell que las asociaciones han cambiado
        notifyShellOfChanges();
        
        QMessageBox::information(this, "Éxito", 
            "Asociación de archivos .nk configurada exitosamente.\n\n"
            "Los archivos .nk ahora se abrirán con LGA_OpenInNukeX.");
    } else {
        QMessageBox::warning(this, "Advertencia", 
            "La aplicación se registró pero no se pudo establecer como predeterminada.\n"
            "Puedes seleccionarla manualmente desde 'Abrir con...'");
    }
}

bool ConfigWindow::setAsDefaultApp()
{
    Logger::log("=== ESTABLECIENDO COMO APLICACIÓN PREDETERMINADA (SETUSERFTA) ===");
    
    QString executablePath = QCoreApplication::applicationFilePath();
    QString progId = "LGA.NukeScript";
    QString extension = ".nk";
    
    // Verificar si SetUserFTA.exe existe en el directorio de la aplicación
    QFileInfo appDir(executablePath);
    QString setUserFTAPath = appDir.absolutePath() + "/SetUserFTA.exe";
    
    if (!QFileInfo::exists(setUserFTAPath)) {
        Logger::log("❌ ERROR: SetUserFTA.exe no encontrado en: " + setUserFTAPath);
        Logger::log("📥 Descarga SetUserFTA desde: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/");
        
        QMessageBox::warning(this, "SetUserFTA requerido", 
            "Para establecer asociaciones de archivos correctamente en Windows 10/11,\n"
            "necesitas descargar SetUserFTA.exe y colocarlo junto a LGA_OpenInNukeX.exe\n\n"
            "Descarga desde: https://kolbi.cz/blog/2017/10/25/setuserfta-userchoice-hash-defeated-set-file-type-associations-per-user/\n\n"
            "SetUserFTA es la única herramienta que puede establecer asociaciones\n"
            "sin que Windows las detecte como 'hijacking' y las resetee.");
        return false;
    }
    
    // Registrar solo ProgID básico (SIN UserChoice - lo hace SetUserFTA)
    QString iconPath = QDir(QCoreApplication::applicationDirPath()).filePath("app_icon.ico");
    if (!registerProgIdOnly(extension, progId, "Nuke Script File", executablePath, iconPath)) {
        Logger::log("❌ ERROR: Falló el registro básico de ProgID");
        return false;
    }
    
    // Usar SetUserFTA para establecer la asociación con el hash correcto
    QProcess process;
    QStringList arguments;
    arguments << extension << progId;
    
    Logger::log("🔧 Ejecutando: " + setUserFTAPath + " " + arguments.join(" "));
    
    process.start(setUserFTAPath, arguments);
    process.waitForFinished(10000); // 10 segundos timeout
    
    int exitCode = process.exitCode();
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();
    
    if (exitCode == 0) {
        Logger::log("✅ SetUserFTA ejecutado exitosamente");
        if (!output.isEmpty()) {
            Logger::log("📤 Salida: " + output);
        }
        
        // Notificar al explorador
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
        Logger::log("✅ Explorador notificado de cambios");
        
        // Verificar el resultado
        verifyAssociation(extension, progId);
        
        QMessageBox::information(this, "Asociación Exitosa", 
            "Los archivos .nk ahora se abrirán con LGA_OpenInNukeX.\n\n"
            "La asociación se estableció correctamente usando SetUserFTA,\n"
            "evitando que Windows la detecte como modificación no autorizada.");
            
        return true;
    } else {
        Logger::log("❌ ERROR: SetUserFTA falló con código: " + QString::number(exitCode));
        if (!output.isEmpty()) {
            Logger::log("📤 Salida: " + output);
        }
        if (!error.isEmpty()) {
            Logger::log("❌ Error: " + error);
        }
        
        QMessageBox::warning(this, "Error en SetUserFTA", 
            "SetUserFTA falló al establecer la asociación.\n\n"
            "Código de error: " + QString::number(exitCode) + "\n"
            "Error: " + error + "\n\n"
            "Verifica que SetUserFTA.exe esté en el directorio correcto\n"
            "y que tengas permisos para ejecutarlo.");
        return false;
    }
}

bool ConfigWindow::registerFileAssociation(const QString &extension, const QString &progId, 
                                          const QString &description, const QString &exePath, 
                                          const QString &iconPath)
{
#ifdef Q_OS_WIN
    // Logging detallado para debugging
    Logger::log(QString("=== INICIANDO REGISTRO DE ASOCIACIÓN ==="));
    Logger::log(QString("Extensión: %1").arg(extension));
    Logger::log(QString("ProgID: %1").arg(progId));
    Logger::log(QString("Descripción: %1").arg(description));
    Logger::log(QString("Ejecutable: %1").arg(exePath));
    Logger::log(QString("Icono: %1").arg(iconPath));
    
    // Verificar que el ejecutable existe
    if (!QFile::exists(exePath)) {
        Logger::logError(QString("❌ ERROR: El ejecutable no existe: %1").arg(exePath));
        return false;
    }
    
    bool success = true;
    
    try {
        // 1. Registrar el ProgID principal (USAR HKCU\Software\Classes)
        QSettings progIdSettings(QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(progId), QSettings::NativeFormat);
        progIdSettings.setValue("Default", description);
        Logger::log(QString("✓ ProgID registrado: HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(progId));
        
        // 2. Configurar el comando de apertura (BEST PRACTICE: comillas en ambos)
        QString command = QString("\"%1\" \"%2\"").arg(exePath, "%1");
        progIdSettings.setValue("shell/open/command/Default", command);
        Logger::log(QString("✓ Comando registrado: %1").arg(command));
        
        // 3. Configurar el icono si está disponible
        if (!iconPath.isEmpty() && QFile::exists(iconPath)) {
            QString iconValue = QString("\"%1\",0").arg(QDir::toNativeSeparators(iconPath));
            progIdSettings.setValue("DefaultIcon/Default", iconValue);
            Logger::log(QString("✓ Icono registrado: %1").arg(iconValue));
        } else {
            Logger::log("⚠ Icono no registrado (archivo no existe o ruta vacía)");
        }
        
        // 4. Registrar la extensión (USAR HKCU\Software\Classes)
        QSettings extensionSettings(QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(extension), QSettings::NativeFormat);
        extensionSettings.setValue("Default", progId);
        Logger::log(QString("✓ Extensión registrada: HKEY_CURRENT_USER\\Software\\Classes\\%1 -> %2").arg(extension, progId));
        
        // 5. Agregar a OpenWithProgids para mejor integración (USAR HKCU\Software\Classes)
        QSettings openWithSettings(QString("HKEY_CURRENT_USER\\Software\\Classes\\%1\\OpenWithProgids").arg(extension), QSettings::NativeFormat);
        openWithSettings.setValue(progId, "");
        Logger::log(QString("✓ OpenWithProgids registrado"));
        
        // 6. Registrar en Applications para aparecer en "Abrir con..." (USAR HKCU\Software\Classes)
        QString appFileName = QFileInfo(exePath).fileName();
        QSettings appSettings(QString("HKEY_CURRENT_USER\\Software\\Classes\\Applications\\%1").arg(appFileName), QSettings::NativeFormat);
        appSettings.setValue("shell/open/command/Default", command);
        appSettings.setValue(QString("SupportedTypes/%1").arg(extension), "");
        Logger::log(QString("✓ Applications registrado: %1").arg(appFileName));
        
        // 7. NO establecer UserChoice manualmente - SetUserFTA lo hará con el hash correcto
        Logger::log("⚠ UserChoice NO establecido manualmente - SetUserFTA lo manejará");
        
        // 8. Sincronizar todos los cambios (excepto UserChoice que maneja SetUserFTA)
        progIdSettings.sync();
        extensionSettings.sync();
        openWithSettings.sync();
        appSettings.sync();
        Logger::log("✓ Todos los settings sincronizados (UserChoice lo maneja SetUserFTA)");
        
        // 9. Verificar que los valores se guardaron correctamente
        progIdSettings.sync();
        QString savedCommand = progIdSettings.value("shell/open/command/Default").toString();
        Logger::log(QString("✓ Verificación - Comando guardado: %1").arg(savedCommand));
        
        Logger::log("=== REGISTRO COMPLETADO EXITOSAMENTE ===");
        
    } catch (const std::exception& e) {
        Logger::logError(QString("❌ EXCEPCIÓN durante el registro: %1").arg(e.what()));
        success = false;
    } catch (...) {
        Logger::logError("❌ ERROR DESCONOCIDO durante el registro");
        success = false;
    }
    
    return success;
#else
    Q_UNUSED(extension)
    Q_UNUSED(progId)
    Q_UNUSED(description)
    Q_UNUSED(exePath)
    Q_UNUSED(iconPath)
    return false;
#endif
}

void ConfigWindow::notifyShellOfChanges()
{
#ifdef Q_OS_WIN
    // Notificar al shell que las asociaciones de archivos han cambiado
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    
    // También notificar cambios específicos en el registro
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, L"HKEY_CLASSES_ROOT", NULL);
    
    // Forzar actualización del registro
    Sleep(100); // Pequeña pausa para asegurar que Windows procese los cambios
#endif
}

bool ConfigWindow::registerProgIdOnly(const QString &extension, const QString &progId, 
                                     const QString &description, const QString &exePath, 
                                     const QString &iconPath)
{
#ifdef Q_OS_WIN
    Logger::log(QString("=== REGISTRANDO SOLO PROGID (SIN USERCHOICE) ==="));
    Logger::log(QString("Extensión: %1").arg(extension));
    Logger::log(QString("ProgID: %1").arg(progId));
    Logger::log(QString("Descripción: %1").arg(description));
    Logger::log(QString("Ejecutable: %1").arg(exePath));
    Logger::log(QString("Icono: %1").arg(iconPath));
    
    // Verificar que el ejecutable existe
    if (!QFile::exists(exePath)) {
        Logger::logError(QString("❌ ERROR: El ejecutable no existe: %1").arg(exePath));
        return false;
    }
    
    bool success = true;
    
    try {
        // 1. Registrar el ProgID principal (USAR HKCU\Software\Classes)
        QSettings progIdSettings(QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(progId), QSettings::NativeFormat);
        progIdSettings.setValue("Default", description);
        Logger::log(QString("✓ ProgID registrado: HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(progId));
        
        // 2. Configurar el comando de apertura
        QString command = QString("\"%1\" \"%2\"").arg(exePath, "%1");
        progIdSettings.setValue("shell/open/command/Default", command);
        Logger::log(QString("✓ Comando registrado: %1").arg(command));
        
        // 3. Configurar el icono si está disponible
        if (!iconPath.isEmpty() && QFile::exists(iconPath)) {
            QString iconValue = QString("\"%1\",0").arg(QDir::toNativeSeparators(iconPath));
            progIdSettings.setValue("DefaultIcon/Default", iconValue);
            Logger::log(QString("✓ Icono registrado: %1").arg(iconValue));
        } else {
            Logger::log("⚠ Icono no registrado (archivo no existe o ruta vacía)");
        }
        
        // 4. Registrar en Applications para aparecer en "Abrir con..."
        QString appFileName = QFileInfo(exePath).fileName();
        QSettings appSettings(QString("HKEY_CURRENT_USER\\Software\\Classes\\Applications\\%1").arg(appFileName), QSettings::NativeFormat);
        appSettings.setValue("shell/open/command/Default", command);
        appSettings.setValue(QString("SupportedTypes/%1").arg(extension), "");
        Logger::log(QString("✓ Applications registrado: %1").arg(appFileName));
        
        // 5. Sincronizar cambios
        progIdSettings.sync();
        appSettings.sync();
        Logger::log("✓ ProgID y Applications sincronizados");
        
        // 6. Verificar que los valores se guardaron correctamente
        progIdSettings.sync();
        QString savedCommand = progIdSettings.value("shell/open/command/Default").toString();
        Logger::log(QString("✓ Verificación - Comando guardado: %1").arg(savedCommand));
        
        Logger::log("=== REGISTRO DE PROGID COMPLETADO ===");
        
    } catch (const std::exception& e) {
        Logger::logError(QString("❌ EXCEPCIÓN durante el registro: %1").arg(e.what()));
        success = false;
    } catch (...) {
        Logger::logError("❌ ERROR DESCONOCIDO durante el registro");
        success = false;
    }
    
    return success;
#else
    Q_UNUSED(extension)
    Q_UNUSED(progId)
    Q_UNUSED(description)
    Q_UNUSED(exePath)
    Q_UNUSED(iconPath)
    return false;
#endif
}

void ConfigWindow::verifyAssociation(const QString &extension, const QString &progId)
{
#ifdef Q_OS_WIN
    Logger::log("=== VERIFICANDO ASOCIACIÓN ===");
    
    // Verificar UserChoice
    QString userChoiceKey = QString("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1\\UserChoice").arg(extension);
    QSettings userChoiceSettings(userChoiceKey, QSettings::NativeFormat);
    QString currentProgId = userChoiceSettings.value("ProgId").toString();
    
    if (currentProgId == progId) {
        Logger::log(QString("✅ Verificación exitosa: %1 -> %2").arg(extension, progId));
    } else {
        Logger::log(QString("⚠ Verificación: se esperaba %1 pero se encontró %2").arg(progId, currentProgId));
    }
    
    // Verificar ProgID existe
    QString progIdKey = QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(progId);
    QSettings progIdSettings(progIdKey, QSettings::NativeFormat);
    QString progIdCommand = progIdSettings.value("shell/open/command/Default").toString();
    
    if (!progIdCommand.isEmpty()) {
        Logger::log(QString("✅ ProgID comando OK: %1").arg(progIdCommand));
    } else {
        Logger::log(QString("❌ ProgID comando VACÍO"));
    }
#else
    Q_UNUSED(extension)
    Q_UNUSED(progId)
#endif
} 