#include "pipesync/UserSettingsTab.h"
#include "ui_UserSettingsTab.h"
#include "pipesync/SecureConfig.h"
#include "pipesync/AppPathManager.h"
#include "pipesync/debug_flags.h"
#include "pipesync/MessagePopover.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QDateTime>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QUrl>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QProcess>
#include <QTimer>
#include <QStandardPaths>
#include <QFile>
#include <QSizePolicy>
#include <QScrollArea>
#include <QApplication>
#include <QWheelEvent>
#include <QScrollBar>
#include <QThread>

UserSettingsTab::UserSettingsTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserSettingsTab),
    m_flowPermissionGroup(""),
    m_flowTestedAfterChanges(true) // Inicialmente asumimos que no hay cambios pendientes
{
    ui->setupUi(this);
    
    // Configurar la interfaz de usuario
    setupUi();
    
    // Cargar la configuración
    loadSettings();
}

UserSettingsTab::~UserSettingsTab()
{
    delete ui;
}

void UserSettingsTab::setupUi()
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
    
    // Eliminar la altura fija y permitir que se ajuste al contenido
    centralWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    
    // Crear un layout para centrar el contenido horizontalmente
    QHBoxLayout *horizontalCenterLayout = new QHBoxLayout(centralWidget);
    horizontalCenterLayout->setContentsMargins(0, 0, 0, 20);
    horizontalCenterLayout->setSpacing(0);
    
    // Crear un widget contenedor para el contenido real
    QWidget *contentWidget = new QWidget();
    contentWidget->setObjectName("contentSettingsWidget");
    contentWidget->setFixedWidth(840); // ✅ Ancho para todas las cajas principales (Wasabi, Flow)

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
    
    // Habilitar el seguimiento del mouse para el scroll con la rueda
    scrollArea->setMouseTracking(true);
    scrollArea->viewport()->setMouseTracking(true);
    scrollArea->setFocusPolicy(Qt::WheelFocus);
    
    // Instalar un filtro de eventos para capturar eventos de rueda del mouse
    scrollArea->viewport()->installEventFilter(this);
    scrollArea->installEventFilter(this);
    this->installEventFilter(this);
    
    // Agregar el área de desplazamiento al layout principal
    mainLayout->addWidget(scrollArea, 1);
    
    // Instalar un filtro de eventos para detectar cambios de tamaño
    this->installEventFilter(this);
    
    
    // ------------------------------------------------------------------------------------------------
    // Grupo de configuración de Wasabi (S3)
    QGroupBox *wasabiGroup = new QGroupBox(contentWidget);
    wasabiGroup->setObjectName("wasabiGroup");
    wasabiGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVBoxLayout *wasabiLayout = new QVBoxLayout(wasabiGroup);
    wasabiLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    wasabiLayout->setContentsMargins(20, 10, 20, 10); // Margen izquierdo, superior, derecho, inferior
    wasabiLayout->setSpacing(10);
    
    // Título principal de Wasabi
    QLabel *wasabiTitle = new QLabel("Wasabi", wasabiGroup);
    wasabiTitle->setObjectName("sectionTitle");
    wasabiLayout->addWidget(wasabiTitle);
    wasabiLayout->addSpacing(10);

    // Crear los campos de entrada
    wasabiAccessKeyEdit = new QLineEdit(wasabiGroup);
    wasabiAccessKeyEdit->setPlaceholderText("Access Key");
    
    wasabiSecretKeyEdit = new QLineEdit(wasabiGroup);
    wasabiSecretKeyEdit->setPlaceholderText("Secret Key");
    wasabiSecretKeyEdit->setEchoMode(QLineEdit::Password);
    
    wasabiEndpointCombo = new QComboBox(wasabiGroup);
    wasabiEndpointCombo->setEditable(true);
    wasabiEndpointCombo->addItems(SecureConfig::getInstance().getWasabiEndpoints());
    wasabiEndpointCombo->setPlaceholderText("Endpoint");
    
    wasabiRegionCombo = new QComboBox(wasabiGroup);
    wasabiRegionCombo->addItems({"us-east-1", "us-east-2", "us-west-1", "eu-central-1", "ap-northeast-1"});
    wasabiRegionCombo->setPlaceholderText("Region");
    
    // Agregar los campos al layout (aca es deonde realmente se posicionan los campos en la UI)
    wasabiLayout->addWidget(wasabiAccessKeyEdit);
    wasabiLayout->addSpacing(10);
    wasabiLayout->addWidget(wasabiSecretKeyEdit);
    wasabiLayout->addSpacing(10);
    wasabiLayout->addWidget(wasabiEndpointCombo);
    wasabiLayout->addSpacing(10);
    wasabiLayout->addWidget(wasabiRegionCombo);
    wasabiLayout->addSpacing(10);  // Espacio antes del área de resultados
    
    // Crear etiquetas y posicionarlas absolutamente ENCIMA de los campos
    QLabel *accessKeyLabel = new QLabel("Access Key", contentWidget);
    accessKeyLabel->setObjectName("fieldLabel");
    accessKeyLabel->adjustSize();
    
    QLabel *secretKeyLabel = new QLabel("Secret Key", contentWidget);
    secretKeyLabel->setObjectName("fieldLabel");
    secretKeyLabel->adjustSize();
    
    QLabel *endpointLabel = new QLabel("Endpoint", contentWidget);
    endpointLabel->setObjectName("fieldLabel");
    endpointLabel->adjustSize();
    
    QLabel *regionLabel = new QLabel("Region", contentWidget);
    regionLabel->setObjectName("fieldLabel");
    regionLabel->adjustSize();

        // Posicionar las etiquetas después de que el layout se haya establecido
        QTimer::singleShot(0, [=]()
                           {
        // Mapear las coordenadas del wasabiGroup al contentWidget
        QPoint groupPos = wasabiGroup->mapTo(contentWidget, QPoint(0, 0));
        
        // Posicionar etiquetas sobre los campos con ajuste en Y, usando las coordenadas mapeadas
        accessKeyLabel->move(groupPos.x() + wasabiAccessKeyEdit->x() + 50, 
                             groupPos.y() + wasabiAccessKeyEdit->y() + 88);
        secretKeyLabel->move(groupPos.x() + wasabiSecretKeyEdit->x() + 50, 
                             groupPos.y() + wasabiSecretKeyEdit->y() + 150);
        endpointLabel->move(groupPos.x() + wasabiEndpointCombo->x() + 50, 
                            groupPos.y() + wasabiEndpointCombo->y() + 212);
        regionLabel->move(groupPos.x() + wasabiRegionCombo->x() + 50,
                          groupPos.y() + wasabiRegionCombo->y() + 266);

        // Forzar que estén por encima de todo
        accessKeyLabel->raise();
        secretKeyLabel->raise();
        endpointLabel->raise();
        regionLabel->raise();
        
        // Asegurar que los labels estén en el tope de la pila de widgets
        accessKeyLabel->setParent(contentWidget);
        secretKeyLabel->setParent(contentWidget);
        endpointLabel->setParent(contentWidget);
        regionLabel->setParent(contentWidget);
        
        // Volver a mostrar y ajustar después del cambio de padre
        accessKeyLabel->show();
        secretKeyLabel->show();
        endpointLabel->show();
        regionLabel->show();
        
        // Ajustar tamaño final
        accessKeyLabel->adjustSize();
        secretKeyLabel->adjustSize();
        endpointLabel->adjustSize();
        regionLabel->adjustSize(); });

    // Botón de prueba de conexión
    QHBoxLayout *testLayout = new QHBoxLayout();
    testLayout->setContentsMargins(0, 0, 0, 0);
    testLayout->setSpacing(10);  // Añadir espacio entre los elementos
    
    // Crear un QTextEdit para mostrar el estado de la conexión (seleccionable)
    wasabiStatusLabel = new QTextEdit(this);
    wasabiStatusLabel->setObjectName("wasabiStatusLabel");
    wasabiStatusLabel->setReadOnly(true);
    wasabiStatusLabel->setFixedHeight(40);
    wasabiStatusLabel->setFixedWidth(400);
    wasabiStatusLabel->setFrameStyle(QFrame::NoFrame);
    wasabiStatusLabel->setStyleSheet("");

    testWasabiButton = new QPushButton("TEST", this);
    testWasabiButton->setFixedHeight(40);  // Altura fija para el botón
    testWasabiButton->setProperty("class", "secondary");  // Aplicar estilo secundario
    

    // Elementos del layout de prueba de conexión de Wasabi
    testLayout->addWidget(wasabiStatusLabel);
    testLayout->addStretch();
    testLayout->addWidget(testWasabiButton);
    
    wasabiLayout->addLayout(testLayout);
    
    // Agregar el grupo de Wasabi al layout central
    centralLayout->addWidget(wasabiGroup);
    

    // ------------------------------------------------------------------------------------------------
    // Grupo de configuración de Flow
    QGroupBox *flowGroup = new QGroupBox(contentWidget);
    flowGroup->setObjectName("wasabiGroup");
    flowGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVBoxLayout *flowLayout = new QVBoxLayout(flowGroup);
    flowLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    flowLayout->setContentsMargins(20, 0, 20, 10); // Margen izquierdo, superior, derecho, inferior
    //flowLayout->setSpacing(10);
    
    // Título principal de Flow
    QLabel *flowTitle = new QLabel("Flow", flowGroup);
    flowTitle->setObjectName("sectionTitle");
    flowLayout->addWidget(flowTitle);
    flowLayout->addSpacing(10);


    // Crear los campos de entrada
    flowUrlEdit = new QLineEdit(flowGroup);
    flowUrlEdit->setPlaceholderText("URL");
    
    flowLoginEdit = new QLineEdit(flowGroup);
    flowLoginEdit->setPlaceholderText("Login");
    
    flowPasswordEdit = new QLineEdit(flowGroup);
    flowPasswordEdit->setPlaceholderText("Password");
    flowPasswordEdit->setEchoMode(QLineEdit::Password);
    
    // Agregar los campos al layout
    flowLayout->addWidget(flowUrlEdit);
    flowLayout->addSpacing(10);
    flowLayout->addWidget(flowLoginEdit);
    flowLayout->addSpacing(10);
    flowLayout->addWidget(flowPasswordEdit);
    flowLayout->addSpacing(10);

    // Crear etiquetas y posicionarlas absolutamente ENCIMA de los campos
    QLabel *urlLabel = new QLabel("URL", flowGroup);
    urlLabel->setObjectName("fieldLabel");
    urlLabel->adjustSize();
    
    QLabel *loginLabel = new QLabel("Login", flowGroup);
    loginLabel->setObjectName("fieldLabel");
    loginLabel->adjustSize();
    
    QLabel *passwordLabel = new QLabel("Password", flowGroup);
    passwordLabel->setObjectName("fieldLabel");
    passwordLabel->adjustSize();
    
    // Posicionar las etiquetas después de que el layout se haya establecido
    QTimer::singleShot(0, [=]() {
        // Posicionar etiquetas sobre los campos
        urlLabel->move(flowUrlEdit->x() + 31, flowUrlEdit->y() + 54);
        loginLabel->move(flowLoginEdit->x() + 31, flowLoginEdit->y() + 112);
        passwordLabel->move(flowPasswordEdit->x() + 31, flowPasswordEdit->y() + 171);
        
        // Asegurar que las etiquetas estén por ENCIMA en el eje Z
        urlLabel->raise();
        loginLabel->raise();
        passwordLabel->raise();
        
        // Volver a ajustar el tamaño después de aplicar el estilo
        urlLabel->adjustSize();
        loginLabel->adjustSize();
        passwordLabel->adjustSize();
    });
    
    // Botón de prueba de conexión
    QHBoxLayout *flowTestLayout = new QHBoxLayout();
    flowTestLayout->setContentsMargins(0, 0, 0, 0);
    flowTestLayout->setSpacing(10);  // Añadir espacio entre los elementos

    // Crear un QTextEdit para mostrar el estado de la conexión (seleccionable)
    flowStatusLabel = new QTextEdit(this);
    flowStatusLabel->setObjectName("flowStatusLabel");
    flowStatusLabel->setReadOnly(true);
    flowStatusLabel->setFixedHeight(40);  // Misma altura que el botón
    flowStatusLabel->setFixedWidth(400);  // Ancho fijo para el área de resultados
    flowStatusLabel->setFrameStyle(QFrame::NoFrame);

    testFlowButton = new QPushButton("TEST", this);
    testFlowButton->setFixedHeight(40);  // Altura fija para el botón
    testFlowButton->setProperty("class", "secondary");  // Aplicar estilo secundario

    // Agregar los elementos al layout en el orden deseado
    flowTestLayout->addWidget(flowStatusLabel);
    flowTestLayout->addStretch(); // Esto empujará los elementos a la derecha
    flowTestLayout->addWidget(testFlowButton);

    flowLayout->addLayout(flowTestLayout);
    
    // Agregar el grupo de Flow al layout central
    centralLayout->addWidget(flowGroup);

    // ------------------------------------------------------------------------------------------------
    // Grupo de configuración de NukeX
    QGroupBox *nukeXGroup = new QGroupBox(contentWidget);
    nukeXGroup->setObjectName("wasabiGroup");
    nukeXGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVBoxLayout *nukeXLayout = new QVBoxLayout(nukeXGroup);
    nukeXLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    nukeXLayout->setContentsMargins(20, 0, 20, 10); // Margen izquierdo, superior, derecho, inferior
    
    // Título principal de NukeX
    QLabel *nukeXTitle = new QLabel("NukeX", nukeXGroup);
    nukeXTitle->setObjectName("sectionTitle");
    nukeXLayout->addWidget(nukeXTitle);
    nukeXLayout->addSpacing(10);

    // Crear layout horizontal para el campo de path y el botón browse
    QHBoxLayout *nukeXPathLayout = new QHBoxLayout();
    nukeXPathLayout->setContentsMargins(0, 0, 0, 0);
    nukeXPathLayout->setSpacing(10);
    
    // Campo de entrada para la ruta de NukeX
    nukeXPathEdit = new QLineEdit(nukeXGroup);
    nukeXPathEdit->setPlaceholderText("Path to NukeX executable");
    
    // Botón Browse para seleccionar la ruta
    nukeXBrowseButton = new QPushButton("BROWSE", nukeXGroup);
    nukeXBrowseButton->setFixedHeight(40);  // Misma altura que el campo de entrada
    nukeXBrowseButton->setProperty("class", "secondary");  // Aplicar estilo secundario
    
    // Agregar los elementos al layout horizontal
    nukeXPathLayout->addWidget(nukeXPathEdit);
    nukeXPathLayout->addWidget(nukeXBrowseButton);
    
    // Agregar el layout horizontal al layout vertical de NukeX
    nukeXLayout->addLayout(nukeXPathLayout);
    nukeXLayout->addSpacing(10);
    
    // Crear etiqueta y posicionarla absolutamente ENCIMA del campo
    QLabel *nukeXPathLabel = new QLabel("NukeX Path", contentWidget);
    nukeXPathLabel->setObjectName("fieldLabel");
    nukeXPathLabel->adjustSize();
    
    // Agregar el grupo de NukeX al layout central
    centralLayout->addWidget(nukeXGroup);

    // Boton de SAVE
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 10, 0, 0);
    buttonLayout->setSpacing(10);

    QPushButton *saveButton = new QPushButton("SAVE", contentWidget);
    saveButton->setFixedHeight(40);
    saveButton->setMinimumWidth(100);
    saveButton->setProperty("class", "action"); // Aplicar estilo de acción principal

    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);

    centralLayout->addLayout(buttonLayout);
    
    // Agregar un stretch al final para evitar que los widgets se estiren verticalmente
    centralLayout->addStretch(1);

    // Conectar señales y slots
    connect(saveButton, &QPushButton::clicked, this, &UserSettingsTab::onSaveButtonClicked);
    connect(testWasabiButton, &QPushButton::clicked, this, &UserSettingsTab::onTestWasabiConnectionClicked);
    connect(testFlowButton, &QPushButton::clicked, this, &UserSettingsTab::onTestFlowConnectionClicked);
    connect(nukeXBrowseButton, &QPushButton::clicked, this, &UserSettingsTab::onNukeXBrowseClicked);
}

void UserSettingsTab::loadSettings()
{
    SecureConfig& config = SecureConfig::getInstance();
    
    // Cargar configuración de Wasabi
    wasabiEndpointCombo->setCurrentText(config.getWasabiEndpoint());
    wasabiRegionCombo->setCurrentText(config.getWasabiRegion());
    wasabiAccessKeyEdit->setText(config.getWasabiAccessKey());
    wasabiSecretKeyEdit->setText(config.getWasabiSecretKey());
    
    // Cargar configuración de Flow
    m_originalFlowUrl = config.getFlowUrl();
    m_originalFlowLogin = config.getFlowLogin();
    m_originalFlowPassword = config.getFlowPassword();
    
    flowUrlEdit->setText(m_originalFlowUrl);
    flowLoginEdit->setText(m_originalFlowLogin);
    flowPasswordEdit->setText(m_originalFlowPassword);
    
    // Mostrar el grupo de permisos guardado si existe
    QString permissionGroup = config.getFlowPermissionGroup();
    if (!permissionGroup.isEmpty()) {
        flowStatusLabel->setText("  " + permissionGroup);
        flowStatusLabel->setStyleSheet("color: #777777; background-color: transparent;");
    }
    
    // Cargar configuración de NukeX
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/config.ini", QSettings::IniFormat);
    QString nukeXPath = settings.value("NukeX/Path", "C:/Program Files/Nuke15.1v6/Nuke15.1.exe").toString();
    nukeXPathEdit->setText(nukeXPath);
    
    // Conectar señales de cambio en los campos de Flow
    connect(flowUrlEdit, &QLineEdit::textEdited, this, [this]() {
        m_flowTestedAfterChanges = false;
    });
    
    connect(flowLoginEdit, &QLineEdit::textEdited, this, [this]() {
        m_flowTestedAfterChanges = false;
    });
    
    connect(flowPasswordEdit, &QLineEdit::textEdited, this, [this]() {
        m_flowTestedAfterChanges = false;
    });
}

void UserSettingsTab::saveSettings()
{
    SecureConfig& config = SecureConfig::getInstance();
    
    // Guardar configuración de Wasabi
    config.setWasabiEndpoint(wasabiEndpointCombo->currentText());
    config.setWasabiRegion(wasabiRegionCombo->currentText());
    config.setWasabiAccessKey(wasabiAccessKeyEdit->text());
    config.setWasabiSecretKey(wasabiSecretKeyEdit->text());
    
    // Guardar configuración de Flow
    config.setFlowUrl(flowUrlEdit->text());
    config.setFlowLogin(flowLoginEdit->text());
    config.setFlowPassword(flowPasswordEdit->text());
    
    // Guardar el grupo de permisos si se obtuvo del test
    if (!m_flowPermissionGroup.isEmpty()) {
        config.setFlowPermissionGroup(m_flowPermissionGroup);
        CONDITIONAL_DEBUG("paths", "Guardando grupo de permisos en configuración: " << m_flowPermissionGroup);
    }
    
    // Guardar configuración de NukeX
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/config.ini", QSettings::IniFormat);
    settings.setValue("NukeX/Path", nukeXPathEdit->text());
    settings.sync();
    
    // Sincronizar la configuración
    config.sync();
}

void UserSettingsTab::onNukeXBrowseClicked()
{
    QString currentPath = nukeXPathEdit->text();
    
    // Si no hay ruta actual, usar el directorio de archivos de programa
    if (currentPath.isEmpty()) {
        currentPath = "C:/Program Files";
    } else {
        // Usar el directorio padre del archivo actual
        QFileInfo fileInfo(currentPath);
        currentPath = fileInfo.absolutePath();
    }
    
    QString selectedFile = QFileDialog::getOpenFileName(
        this,
        "Seleccionar ejecutable de NukeX",
        currentPath,
        "Ejecutables (*.exe);;Todos los archivos (*.*)"
    );
    
    if (!selectedFile.isEmpty()) {
        nukeXPathEdit->setText(selectedFile);
    }
}

bool UserSettingsTab::haveFlowSettingsChanged() const
{
    return flowUrlEdit->text() != m_originalFlowUrl ||
           flowLoginEdit->text() != m_originalFlowLogin ||
           flowPasswordEdit->text() != m_originalFlowPassword;
}

void UserSettingsTab::onSaveButtonClicked()
{
    // Verificar si se han cambiado los ajustes de Flow y si se ha realizado una prueba después
    if (haveFlowSettingsChanged() && !m_flowTestedAfterChanges) {
        // Crear y mostrar el MessagePopover con la advertencia
        MessagePopover* popover = new MessagePopover(this);
        
        QString message = "<span style='color:#B2B2B2;'>Hacer prueba de conexión a Flow antes de guardar";
        
        popover->setMessage(message);
        popover->showPopover();
        
        return; // No permitir guardar sin prueba
    }
    
    // Si pasó la validación, guardar normalmente
    saveSettings();
    
    // Mostrar mensaje de confirmación con MessagePopover en lugar de QMessageBox
    MessagePopover* successPopover = new MessagePopover(this);
    QString successMessage = "<span style='color:#B2B2B2;'>The configuration has been saved</span>";
    successPopover->setMessage(successMessage);
    successPopover->showPopover();
}

void UserSettingsTab::onTestWasabiConnectionClicked()
{
    // Deshabilitar el botón para evitar múltiples clics
    testWasabiButton->setEnabled(false);
    
    // Mostrar indicador de carga
    wasabiStatusLabel->setText("Probando conexión...");
    wasabiStatusLabel->setStyleSheet("background-color: transparent;");
    
    // Solo guardar temporalmente las credenciales de Wasabi para la prueba
    SecureConfig& config = SecureConfig::getInstance();
    config.setWasabiAccessKey(wasabiAccessKeyEdit->text());
    config.setWasabiSecretKey(wasabiSecretKeyEdit->text());
    config.setWasabiEndpoint(wasabiEndpointCombo->currentText());
    config.setWasabiRegion(wasabiRegionCombo->currentText());
    config.sync();
    
    // Obtener la ruta del Python embebido
    QString pythonPath = AppPathManager::getPythonRuntimePath(QCoreApplication::applicationDirPath());
    
    // Verificar si el archivo existe
    QFileInfo pythonFileInfo(pythonPath);
    if (!pythonFileInfo.exists() || !pythonFileInfo.isExecutable()) {
        wasabiStatusLabel->setText("Error: Python no encontrado en " + pythonPath);
        wasabiStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
        testWasabiButton->setEnabled(true);
        return;
    }
    
    // Obtener la ruta del script de prueba de Wasabi usando AppPathManager
    QString scriptPath = AppPathManager::getPythonScriptPath(QCoreApplication::applicationDirPath(), "s3_testCredencials.py");
    
    if (!QFileInfo(scriptPath).exists()) {
        wasabiStatusLabel->setText("Error: Script de prueba no encontrado");
        wasabiStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
        testWasabiButton->setEnabled(true);
        return;
    }
    
    // Crear el proceso de Python en un hilo separado para evitar bloquear la UI
    CONDITIONAL_DEBUG("paths", "Iniciando prueba de Wasabi en proceso separado: " << pythonPath << scriptPath);
    
    // Crear el proceso y moverlo a un hilo propio
    QProcess* pythonProcess = new QProcess();
    
    // Configurar el proceso antes de iniciarlo
    pythonProcess->setProcessChannelMode(QProcess::MergedChannels); // Combinar stdout y stderr
    
    // Conectar las señales para manejar el resultado de manera asíncrona
    connect(pythonProcess, &QProcess::errorOccurred, this, [this, pythonProcess](QProcess::ProcessError error) {
        // Manejar errores del proceso
        QString errorMessage = "Error al ejecutar Python: ";
        switch (error) {
            case QProcess::FailedToStart:
                errorMessage += "El proceso no pudo iniciarse";
                break;
            case QProcess::Crashed:
                errorMessage += "El proceso se detuvo de forma inesperada";
                break;
            default:
                errorMessage += "Error desconocido";
                break;
        }
        
        wasabiStatusLabel->setText(errorMessage);
        wasabiStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
        testWasabiButton->setEnabled(true);
        
        // Eliminar el proceso cuando se haya completado
        pythonProcess->deleteLater();
    });
    
    connect(pythonProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
            this, [this, pythonProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        // Leer la salida del proceso
        QString output = QString::fromUtf8(pythonProcess->readAllStandardOutput());
        
        CONDITIONAL_DEBUG("paths", "Salida del script Python:" << output);
        
        if (exitStatus == QProcess::NormalExit) {
            // Filtrar advertencias comunes de SSL
            QStringList outputLines = output.split('\n');
            QStringList filteredLines;
            
            for (const QString &line : outputLines) {
                if (!line.contains("InsecureRequestWarning") && 
                    !line.contains("warnings.warn(") &&
                    !line.isEmpty()) {
                    filteredLines.append(line);
                }
            }
            
            // Buscar la última línea que no sea un log
            QString lastLine;
            for (int i = filteredLines.size() - 1; i >= 0; i--) {
                if (!filteredLines[i].startsWith("[LOG]")) {
                    lastLine = filteredLines[i].trimmed();
                    break;
                }
            }
            
            // Determinar el resultado basado en la última línea
            if (lastLine.contains("CONEXION EXITOSA")) {
                wasabiStatusLabel->setText("Conexión exitosa");
                wasabiStatusLabel->setStyleSheet("color: #489348; background-color: transparent;");
            } else if (lastLine.contains("ERROR:")) {
                wasabiStatusLabel->setText(lastLine);
                wasabiStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
            } else if (!lastLine.isEmpty()) {
                wasabiStatusLabel->setText(lastLine);
                wasabiStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
            } else {
                wasabiStatusLabel->setText("Error desconocido al conectar con Wasabi");
                wasabiStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
            }
        } else {
            wasabiStatusLabel->setText("Error: El proceso de Python terminó de forma anormal");
            wasabiStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
        }
        
        // Re-habilitar el botón
        testWasabiButton->setEnabled(true);
        
        // Eliminar el proceso cuando se haya completado
        pythonProcess->deleteLater();
    });
    
    // Iniciar el proceso
    pythonProcess->start(pythonPath, QStringList() << scriptPath);
}

void UserSettingsTab::onTestFlowConnectionClicked()
{
    // Deshabilitar el botón para evitar múltiples clics
    testFlowButton->setEnabled(false);
    
    // Mostrar indicador de carga
    flowStatusLabel->setText("Probando conexión...");
    flowStatusLabel->setStyleSheet("background-color: transparent;");
    
    // Solo guardar temporalmente las credenciales de Flow para la prueba
    SecureConfig& config = SecureConfig::getInstance();
    config.setFlowUrl(flowUrlEdit->text());
    config.setFlowLogin(flowLoginEdit->text());
    config.setFlowPassword(flowPasswordEdit->text());
    config.sync();
    
    // Marcar que se ha realizado una prueba después de los cambios
    m_flowTestedAfterChanges = true;
    
    // Obtener la ruta del Python embebido
    QString pythonPath = AppPathManager::getPythonRuntimePath(QCoreApplication::applicationDirPath());
    
    // Verificar si el archivo existe
    QFileInfo pythonFileInfo(pythonPath);
    if (!pythonFileInfo.exists() || !pythonFileInfo.isExecutable()) {
        flowStatusLabel->setText("Error: Python no encontrado en " + pythonPath);
        flowStatusLabel->setStyleSheet("color: red; background-color: transparent;");
        testFlowButton->setEnabled(true);
        return;
    }
    
    // Obtener la ruta del script de prueba de Flow usando AppPathManager
    QString scriptPath = AppPathManager::getPythonScriptPath(QCoreApplication::applicationDirPath(), "test_flow_loginSettings.py");
    
    if (!QFileInfo(scriptPath).exists()) {
        flowStatusLabel->setText("Error: Script de prueba no encontrado");
        flowStatusLabel->setStyleSheet("color: red; background-color: transparent;");
        testFlowButton->setEnabled(true);
        return;
    }
    
    // Crear el proceso de Python en un hilo separado para evitar bloquear la UI
    CONDITIONAL_DEBUG("paths", "Iniciando prueba de Flow en proceso separado: " << pythonPath << scriptPath);
    
    // Crear el proceso y moverlo a un hilo propio
    QProcess* pythonProcess = new QProcess();
    
    // Configurar el proceso antes de iniciarlo
    pythonProcess->setProcessChannelMode(QProcess::MergedChannels); // Combinar stdout y stderr
    
    // Conectar las señales para manejar el resultado de manera asíncrona
    connect(pythonProcess, &QProcess::errorOccurred, this, [this, pythonProcess](QProcess::ProcessError error) {
        // Manejar errores del proceso
        QString errorMessage = "Error al ejecutar Python: ";
        switch (error) {
            case QProcess::FailedToStart:
                errorMessage += "El proceso no pudo iniciarse";
                break;
            case QProcess::Crashed:
                errorMessage += "El proceso se detuvo de forma inesperada";
                break;
            default:
                errorMessage += "Error desconocido";
                break;
        }
        
        flowStatusLabel->setText(errorMessage);
        flowStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
        testFlowButton->setEnabled(true);
        
        // Eliminar el proceso cuando se haya completado
        pythonProcess->deleteLater();
    });
    
    connect(pythonProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
            this, [this, pythonProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        // Leer la salida del proceso
        QString output = QString::fromUtf8(pythonProcess->readAllStandardOutput());
        
        CONDITIONAL_DEBUG("paths", "Salida del script Python:" << output);
        
        if (exitStatus == QProcess::NormalExit) {
            // Filtrar advertencias comunes de SSL
            QStringList outputLines = output.split('\n');
            QStringList filteredLines;
            
            for (const QString &line : outputLines) {
                if (!line.contains("InsecureRequestWarning") && 
                    !line.contains("warnings.warn(") &&
                    !line.isEmpty()) {
                    filteredLines.append(line);
                }
            }
            
            // Buscar la última línea que no sea un log
            QString lastLine;
            for (int i = filteredLines.size() - 1; i >= 0; i--) {
                if (!filteredLines[i].startsWith("[LOG]")) {
                    lastLine = filteredLines[i].trimmed();
                    break;
                }
            }
            
            // Determinar el resultado basado en la última línea
            if (lastLine.contains("CONEXION EXITOSA")) {
                // Verificar si incluye información del grupo de permisos
                if (lastLine.contains(":")) {
                    // Extraer el grupo de permisos
                    QString permissionGroup = lastLine.split(":").last().trimmed();
                    
                    // Almacenar el grupo de permisos temporalmente
                    m_flowPermissionGroup = permissionGroup;
                    CONDITIONAL_DEBUG("paths", "Grupo de permisos almacenado en memoria: " << permissionGroup);
                    
                    flowStatusLabel->setText("Conexión exitosa: " + permissionGroup);
                } else {
                    flowStatusLabel->setText("Conexión exitosa");
                    m_flowPermissionGroup = ""; // Limpiar si no hay grupo de permisos
                }
                flowStatusLabel->setStyleSheet("color: #489348; background-color: transparent;");
            } else if (lastLine.contains("ERROR:")) {
                flowStatusLabel->setText(lastLine);
                flowStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
            } else if (!lastLine.isEmpty()) {
                flowStatusLabel->setText(lastLine);
                flowStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
            } else {
                flowStatusLabel->setText("Error desconocido al conectar con Flow");
                flowStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
            }
        } else {
            flowStatusLabel->setText("Error: El proceso de Python terminó de forma anormal");
            flowStatusLabel->setStyleSheet("color: #B73C3C; background-color: transparent;");
        }
        
        // Re-habilitar el botón
        testFlowButton->setEnabled(true);
        
        // Eliminar el proceso cuando se haya completado
        pythonProcess->deleteLater();
    });
    
    // Iniciar el proceso
    pythonProcess->start(pythonPath, QStringList() << scriptPath);
    
    CONDITIONAL_DEBUG("paths", "Solicitud de prueba de Flow enviada al proceso Python");
}

bool UserSettingsTab::eventFilter(QObject *watched, QEvent *event)
{
    // Capturar eventos de rueda del mouse
    if (event->type() == QEvent::Wheel)
    {
        // Verificar si el objeto observado es el viewport del QScrollArea o el QScrollArea mismo
        QScrollArea *scrollArea = findChild<QScrollArea*>("darkScrollArea");
        if (scrollArea && (watched == scrollArea->viewport() || watched == scrollArea))
        {
            // Obtener el evento de rueda
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            
            // Obtener la posición actual del scrollbar
            QScrollBar *vScrollBar = scrollArea->verticalScrollBar();
            int currentValue = vScrollBar->value();
            
            // Calcular el nuevo valor con un paso más pequeño para un desplazamiento más suave
            int delta = wheelEvent->angleDelta().y();
            int step = 20; // Paso más pequeño para un desplazamiento más suave
            int newValue = currentValue - (delta > 0 ? step : -step);
            
            // Limitar el valor para evitar desplazamiento excesivo
            int maxValue = vScrollBar->maximum();
            if (newValue > maxValue) {
                newValue = maxValue;
            } else if (newValue < 0) {
                newValue = 0;
            }
            
            // Establecer el nuevo valor
            vScrollBar->setValue(newValue);
            
            // Aceptar el evento para evitar que se propague
            return true;
        }
        
        // Si el evento es para otro objeto, redirigirlo al QScrollArea
        if (scrollArea)
        {
            QApplication::sendEvent(scrollArea->viewport(), event);
            return true;
        }
    }
    // Capturar eventos de cambio de tamaño
    else if (event->type() == QEvent::Resize && watched == this)
    {
        // Ya no necesitamos ajustar la altura del widget central
        // porque ahora tiene una altura fija
        return false;
    }
    
    // Para otros eventos, usar el comportamiento predeterminado
    return QWidget::eventFilter(watched, event);
} 
