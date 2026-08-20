#include "configwindow.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QPalette>
#include <QProcess>
#include <QScreen>
#include <QStyle>
#include <QThread>
#include <QScrollArea>
#include <QGroupBox>
#include <QTimer>
#include <QSizePolicy>
#include <QShowEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QGridLayout>
#include <QScrollBar>
#include <QRegularExpression>
#include <QPointer>
#include <tuple>
#include "appsettings.h"
#include "dialogs.h"
#include "dialogstyle.h"
#include "lgaregistry.h"
#include "logger.h"
#include "nukebridge.h"
#include "qflowlayout.h"
#ifdef Q_OS_MACOS
#include "macintegration.h"
#endif

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <dwmapi.h>
#endif

namespace {

/// Ancho fijo de la ventana. Antes había DOS: 900 en el constructor y 800 en el resize, así
/// que la ventana cambiaba de ancho sola al terminar el escaneo.
constexpr int kWindowWidth = 800;

/// Alto mínimo de la ventana.
constexpr int kMinWindowHeight = 400;

// ============================================================================
// 🔲🔲🔲  SEPARACION ENTRE CARDS — CENTRALIZADA ACA  🔲🔲🔲
// ----------------------------------------------------------------------------
// Espacio vertical, en píxeles, entre los tres bloques de la ventana (File
// Association, Preferred Nuke Version, Nuke Bridge). Cambiar este valor los
// separa o los junta a TODOS de una.
// ============================================================================
constexpr int kCardSpacing = 12;

/// La ventana nunca ocupa más que esta fracción del alto USABLE de la pantalla (o sea ya
/// descontados la barra de menú y el Dock). Con el panel manual desplegado el contenido pide
/// más de lo que entra en un portátil, y una ventana pegada a los dos bordes se lee como si
/// estuviera rota; el resto lo resuelve el scroll.
constexpr double kMaxScreenFraction = 0.8;

#ifdef Q_OS_MACOS
/// Cuanto se espera la respuesta al cartel de permiso de macOS antes de volver a habilitar
/// APPLY. Generoso a proposito: el cartel lo contesta una persona, y reactivar el boton no
/// invalida una respuesta que llegue despues.
constexpr int kAssocWatchdogMs = 90000;
#endif

#ifdef Q_OS_WIN
void applyDarkTitleBar(QWidget *window)
{
    if (!window) {
        return;
    }

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const BOOL enableDarkMode = TRUE;

    // Windows 10 20H1+ usa 20; versiones anteriores compatibles usan 19.
    HRESULT result = DwmSetWindowAttribute(
        hwnd, 20, &enableDarkMode, sizeof(enableDarkMode));
    if (FAILED(result)) {
        DwmSetWindowAttribute(
            hwnd, 19, &enableDarkMode, sizeof(enableDarkMode));
    }

    // Windows 11 permite fijar caption y texto sin depender del tema global.
    const COLORREF captionColor = RGB(22, 22, 22);
    const COLORREF textColor = RGB(242, 242, 242);
    DwmSetWindowAttribute(hwnd, 35, &captionColor, sizeof(captionColor));
    DwmSetWindowAttribute(hwnd, 36, &textColor, sizeof(textColor));
}
#endif
}

ConfigWindow::ConfigWindow(QWidget *parent)
    : QWidget(parent)
    , scrollArea(nullptr)
    , nukePathEdit(nullptr)
    , browseButton(nullptr)
    , saveButton(nullptr)
    , applyButton(nullptr)
    , descriptionLabel(nullptr)
    , nukeVersionDescLabel(nullptr)
    , nukeScanner(nullptr)
    , versionsContainer(nullptr)
    , scanningLabel(nullptr)
    , foundVersionsLabel(nullptr)
    , versionsButtonsWidget(nullptr)
    , versionsLayout(nullptr)
    , foundVersionsCount(0)
    , scanFinished(false)
    , bridgeDescLabel(nullptr)
    , bridgeChipLabel(nullptr)
    , bridgeDirEdit(nullptr)
    , bridgeBrowseButton(nullptr)
    , bridgeInstallButton(nullptr)
    , bridgeHintLabel(nullptr)
    , manualToggleButton(nullptr)
    , manualPanel(nullptr)
    , manualStepNumbers{nullptr, nullptr, nullptr}
    , manualStepTexts{nullptr, nullptr, nullptr}
    , manualCodeLabel(nullptr)
    , exportButton(nullptr)
    , copyLineButton(nullptr)
    , copyFeedbackTimer(nullptr)
    , langEnButton(nullptr)
    , langEsButton(nullptr)
    , versionLabel(nullptr)
    , userResizedHeight(false)
    , programmaticResize(false)
    , lastAppliedHeight(0)
    , userMovedWindow(false)
    , programmaticMove(false)
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
    // Ancho fijo, alto libre: el contenido es una columna de bloques de ancho fijo, así que
    // estirar en X solo agrega fondo a los costados. En Y sí hay algo que ganar, porque el
    // contenido puede no entrar en la pantalla.
    setFixedWidth(kWindowWidth);
    // El techo se fija YA, y no recién cuando termina el escaneo de versiones: hasta ese
    // momento la ventana se podía estirar más allá del 80% de la pantalla, y ese estirón
    // además contaba como un gesto del usuario y le desactivaba el auto-alto por el resto de
    // la sesión. Se usa el tope de pantalla y no el del contenido porque acá el layout todavía
    // no corrió: su sizeHint mediría de menos y dejaría la ventana corta hasta el primer
    // recalculo.
    const int cap = screenHeightCap();
    applyWindowHeight(cap, qMin(600, cap));
    Logger::logInfo("✓ Ventana configurada");

    // Cargar estilo QSS
    Logger::logInfo("Cargando estilo QSS...");
    loadStyleSheet();
    Logger::logInfo("✓ Estilo QSS cargado");

    // Una sola pasada de texto para toda la ventana, DESPUES del QSS: `retranslateUi()` es
    // tambien el lugar donde se arman los textos que dependen del estado (el chip del bridge,
    // el hint, el label del escaner), y `repolish()` necesita la hoja ya aplicada para que el
    // selector del chip valga.
    Logger::logInfo("Aplicando textos e inspeccionando el Nuke Bridge...");
    retranslateUi();
    Logger::logInfo("✓ Textos aplicados");

    // Inicializar y comenzar el escaneo de versiones
    Logger::logInfo("Iniciando proceso de escáner...");
    initializeScanner();
    Logger::logInfo("✓ initializeScanner() llamado");
    
    Logger::logInfo("=== CONSTRUCTOR ConfigWindow COMPLETADO ===");
}

void ConfigWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

#ifdef Q_OS_WIN
    applyDarkTitleBar(this);
#endif

    // Centrado también acá, y no solo desde `applyWindowHeight()`: el del constructor corre
    // con la ventana todavía sin marco, así que no puede descontar la barra de título. Este es
    // el primero que la ve, y es el que decide dónde APARECE.
    centerOnScreen();
}

void ConfigWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (programmaticResize || !isVisible()) {
        return;
    }
    // `oldSize().height() > 0`: durante el armado de la ventana llegan resizes que no son del
    // usuario y que tampoco pasan por el flag (los del propio layout al mostrarse).
    if (event->oldSize().height() <= 0
        || event->size().height() == event->oldSize().height()) {
        return;
    }
    // Segunda línea de defensa: si el alto al que se llegó es exactamente el último que impuso
    // la app, el resize es suyo aunque haya llegado con el flag apagado. Pasa de verdad: el
    // `processEvents()` del recálculo despacha resizes pendientes fuera de esa ventana.
    if (event->size().height() == lastAppliedHeight) {
        return;
    }

    if (!userResizedHeight) {
        Logger::logInfo("El usuario redimensionó la ventana: la app deja de imponer el alto");
    }
    userResizedHeight = true;
}

void ConfigWindow::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);

    if (programmaticMove || !isVisible()) {
        return;
    }
    if (!userMovedWindow) {
        Logger::logInfo("El usuario movió la ventana: la app deja de re-centrarla");
    }
    userMovedWindow = true;
}

void ConfigWindow::centerOnScreen()
{
    // Una vez que el usuario la movió o la redimensionó, la geometría es suya: re-centrarla
    // seria arrancarle la ventana de donde la dejó.
    if (userMovedWindow || userResizedHeight) {
        return;
    }

    QScreen *screen = this->screen();
    if (!screen) {
        return;
    }

    // Se centra el FRAME y no el cliente: `frameGeometry()` incluye la barra de título, y
    // centrar el cliente deja la ventana un poco baja, que es justo el síntoma que esto viene
    // a corregir. El alto del marco solo se conoce una vez que la ventana existe, así que si
    // todavía no hay marco se usa el alto del cliente y se recentra en la próxima pasada.
    const QRect available = screen->availableGeometry();
    const QRect frame = frameGeometry();
    const int frameHeight = frame.height() > 0 ? frame.height() : height();
    const int titleBar = qMax(0, frameHeight - height());

    const int x = available.x() + (available.width() - width()) / 2;
    const int y = available.y() + (available.height() - frameHeight) / 2 + titleBar;

    programmaticMove = true;
    move(x, y);
    programmaticMove = false;
}

void ConfigWindow::setupUI()
{
    // Crear el layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Crear un QScrollArea para permitir desplazamiento
    scrollArea = new QScrollArea(this);
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
    // Ver `kCardSpacing` al tope del archivo: es la perilla para separarlos o juntarlos.
    centralLayout->setSpacing(kCardSpacing);
    
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

    QLabel *fileAssociationTitle = new QLabel("File Association", fileAssociationGroup);
    fileAssociationTitle->setObjectName("sectionTitle");
    fileAssociationLayout->addWidget(fileAssociationTitle);

    // APPLY va a la derecha del TEXTO, no del título, y los dos centrados verticalmente entre
    // sí: alineado con el título quedaba a dos renglones de distancia de la explicación de lo
    // que hace, con un hueco vacío en el medio que no significaba nada.
    QHBoxLayout *fileAssociationBody = new QHBoxLayout();
    fileAssociationBody->setContentsMargins(0, 0, 0, 0);
    fileAssociationBody->setSpacing(20);

    descriptionLabel = new QLabel(TR(DescFileAssociation), fileAssociationGroup);
    descriptionLabel->setObjectName("sectionDescription");
    descriptionLabel->setWordWrap(true);
    // El centrado vertical se hace DENTRO del label y no con un flag de alineación en el
    // layout: con el flag, la caja se encoge a su sizeHint y un label con wordWrap mide mal
    // (Qt no le consulta el heightForWidth). Así el label ocupa el alto de la fila —que lo
    // fija el botón— y es el texto el que se centra adentro.
    descriptionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    applyButton = new QPushButton(TR(BtnApply), fileAssociationGroup);
    applyButton->setFixedHeight(40);
    applyButton->setMinimumWidth(158);
    applyButton->setProperty("class", "action");

    fileAssociationBody->addWidget(descriptionLabel, 1);
    fileAssociationBody->addWidget(applyButton, 0, Qt::AlignVCenter);

    fileAssociationLayout->addLayout(fileAssociationBody);

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
    nukeVersionLayout->addSpacing(4);

    nukeVersionDescLabel = new QLabel(TR(DescNukeVersion), nukeVersionGroup);
    nukeVersionDescLabel->setObjectName("sectionDescription");
    nukeVersionDescLabel->setWordWrap(true);
    nukeVersionLayout->addWidget(nukeVersionDescLabel);
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
    foundVersionsLabel = new QLabel(TR(ScanChoose), versionsContainer);
    foundVersionsLabel->setObjectName("sectionDescription");
    foundVersionsLabel->setWordWrap(true);
    versionsLayout->addWidget(foundVersionsLabel);
    Logger::logInfo("✓ foundVersionsLabel creado y agregado");

    // Etiqueta de scanning (inicialmente visible)
    scanningLabel = new QLabel(TR(ScanScanning), versionsContainer);
    scanningLabel->setObjectName("scanStatus");
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
    nukePathEdit->setPlaceholderText(TR(PlaceholderNukePath));

    // Botón Browse para seleccionar la ruta
    browseButton = new QPushButton(TR(BtnBrowse), nukeVersionGroup);
    browseButton->setFixedHeight(40);
    browseButton->setProperty("class", "secondary");

    // Agregar los elementos al layout horizontal
    // SAVE va en la MISMA fila, a la derecha de BROWSE: los dos actuan sobre el campo de al
    // lado, asi que separarlos en dos filas obligaba a barrer la vista de vuelta para arriba.
    saveButton = new QPushButton(TR(BtnSave), nukeVersionGroup);
    saveButton->setFixedHeight(40);
    saveButton->setMinimumWidth(70);
    saveButton->setProperty("class", "action");

    nukePathLayout->addWidget(nukePathEdit);
    nukePathLayout->addWidget(browseButton);
    nukePathLayout->addWidget(saveButton);

    // Agregar el layout horizontal al layout vertical de Nuke Version
    nukeVersionLayout->addLayout(nukePathLayout);
    nukeVersionLayout->addSpacing(10);

    // Agregar el grupo de Nuke Version al layout central
    centralLayout->addWidget(nukeVersionGroup);

    // ------------------------------------------------------------------------------------------------
    // Grupo del Nuke Bridge
    setupBridgeGroup(contentWidget, centralLayout);

    // ------------------------------------------------------------------------------------------------
    // Pie: selector de idioma a la izquierda, version a la derecha.
    QHBoxLayout *footerLayout = new QHBoxLayout();
    footerLayout->setContentsMargins(4, 0, 4, 0);
    footerLayout->setSpacing(0);

    langEnButton = new QPushButton("EN", contentWidget);
    langEsButton = new QPushButton("ES", contentWidget);
    for (QPushButton *button : {langEnButton, langEsButton}) {
        button->setProperty("class", "lang");
        button->setFixedHeight(24);
        button->setFixedWidth(38);
        button->setCursor(Qt::PointingHandCursor);
    }
    footerLayout->addWidget(langEnButton);
    footerLayout->addWidget(langEsButton);
    footerLayout->addStretch(1);

    // La version sale de la macro del CMake, nunca escrita a mano (convencion LGA: la fuente
    // unica de verdad es el `project(... VERSION ...)`).
    versionLabel = new QLabel(QString("v%1 | Lega").arg(OPENINNUKEX_VERSION), contentWidget);
    versionLabel->setObjectName("versionLabel");
    versionLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    footerLayout->addWidget(versionLabel);

    centralLayout->addLayout(footerLayout);

    // Conectar señales y slots
    connect(browseButton, &QPushButton::clicked, this, &ConfigWindow::browseNukePath);
    connect(saveButton, &QPushButton::clicked, this, &ConfigWindow::saveConfiguration);
    connect(applyButton, &QPushButton::clicked, this, &ConfigWindow::applyFileAssociation);
    connect(langEnButton, &QPushButton::clicked, this,
            [this]() { setLanguage(I18n::Lang::En); });
    connect(langEsButton, &QPushButton::clicked, this,
            [this]() { setLanguage(I18n::Lang::Es); });

    // Marca el idioma activo al arrancar (el que quedo persistido).
    langEnButton->setProperty("selected", I18n::lang() == I18n::Lang::En);
    langEsButton->setProperty("selected", I18n::lang() == I18n::Lang::Es);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Nuke Bridge
// ─────────────────────────────────────────────────────────────────────────────

void ConfigWindow::setupBridgeGroup(QWidget *parent, QVBoxLayout *centralLayout)
{
    QGroupBox *bridgeGroup = new QGroupBox(parent);
    bridgeGroup->setObjectName("settingsGroup");
    bridgeGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QVBoxLayout *bridgeLayout = new QVBoxLayout(bridgeGroup);
    bridgeLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    bridgeLayout->setContentsMargins(20, 10, 20, 10);
    bridgeLayout->setSpacing(10);

    QLabel *bridgeTitle = new QLabel("Nuke Bridge", bridgeGroup);
    bridgeTitle->setObjectName("sectionTitle");
    bridgeLayout->addWidget(bridgeTitle);
    bridgeLayout->addSpacing(4);

    // El chip de estado NO va acá arriba junto al título: se crea ahora porque
    // `refreshBridgeStatus()` lo necesita existiendo, pero se ubica más abajo, en la fila del
    // toggle de instalación manual. Ver el comentario de esa fila.
    bridgeChipLabel = new QLabel(TR(ChipNotInstalled), bridgeGroup);
    bridgeChipLabel->setObjectName("bridgeChip");
    bridgeChipLabel->setProperty("state", "off");

    bridgeDescLabel = new QLabel(TR(DescNukeBridge), bridgeGroup);
    bridgeDescLabel->setObjectName("sectionDescription");
    bridgeDescLabel->setWordWrap(true);
    bridgeLayout->addWidget(bridgeDescLabel);
    bridgeLayout->addSpacing(6);

    // Carpeta .nuke + acciones.
    QHBoxLayout *bridgeDirLayout = new QHBoxLayout();
    bridgeDirLayout->setContentsMargins(0, 0, 0, 0);
    bridgeDirLayout->setSpacing(20);

    bridgeDirEdit = new QLineEdit(bridgeGroup);
    bridgeDirEdit->setPlaceholderText(TR(PlaceholderNukeDir));

    bridgeBrowseButton = new QPushButton(TR(BtnBrowse), bridgeGroup);
    bridgeBrowseButton->setFixedHeight(40);
    bridgeBrowseButton->setProperty("class", "secondary");

    bridgeInstallButton = new QPushButton(TR(BtnInstall), bridgeGroup);
    bridgeInstallButton->setFixedHeight(40);
    bridgeInstallButton->setMinimumWidth(70);
    bridgeInstallButton->setProperty("class", "action");

    bridgeDirLayout->addWidget(bridgeDirEdit);
    bridgeDirLayout->addWidget(bridgeBrowseButton);
    bridgeDirLayout->addWidget(bridgeInstallButton);
    bridgeLayout->addLayout(bridgeDirLayout);

    bridgeHintLabel = new QLabel(bridgeGroup);
    bridgeHintLabel->setObjectName("bridgeHint");
    bridgeHintLabel->setWordWrap(true);
    bridgeHintLabel->setTextFormat(Qt::RichText);
    bridgeLayout->addWidget(bridgeHintLabel);

    // Es un QPushButton y no un QLabel con <a href> para heredar foco y teclado sin manejar
    // linkActivated a mano. Y se ve COMO un botón: pintado de azul y sin caja parecía un
    // enlace de texto, o sea algo que abre otra cosa, cuando en realidad despliega contenido
    // acá mismo.
    manualToggleButton = new QPushButton(TR(LinkManualShow), bridgeGroup);
    manualToggleButton->setProperty("class", "toggle");
    manualToggleButton->setFixedHeight(32);
    manualToggleButton->setCursor(Qt::PointingHandCursor);

    // El chip de estado comparte esta fila, pegado al borde derecho. Arriba, junto al título,
    // quedaba lejos del control que lo cambia: el botón de instalar está a media caja de
    // distancia, y el usuario lo apretaba sin ver que el chip se actualizaba. Acá queda
    // inmediatamente debajo de ese botón y en el mismo renglón que la salida manual, que es la
    // otra forma de cambiar lo que el chip informa.
    QHBoxLayout *manualToggleLayout = new QHBoxLayout();
    manualToggleLayout->setContentsMargins(0, 0, 0, 0);
    manualToggleLayout->setSpacing(10);
    manualToggleLayout->addWidget(manualToggleButton);
    manualToggleLayout->addStretch(1);
    manualToggleLayout->addWidget(bridgeChipLabel, 0, Qt::AlignVCenter);
    bridgeLayout->addLayout(manualToggleLayout);

    // ── panel manual, colapsado por defecto ──────────────────────────────────
    manualPanel = new QWidget(bridgeGroup);
    manualPanel->setObjectName("manualPanel");
    manualPanel->setVisible(false);

    QVBoxLayout *manualLayout = new QVBoxLayout(manualPanel);
    manualLayout->setContentsMargins(0, 10, 0, 0);
    manualLayout->setSpacing(14);

    // Grilla de dos columnas: los números en la primera y el contenido en la segunda. Lo que
    // esto compra es que la caja de código pueda ir en la columna del contenido, o sea con la
    // misma sangría que el texto del paso 3 y por lo tanto leyéndose como parte de ese paso.
    QGridLayout *stepsGrid = new QGridLayout();
    stepsGrid->setContentsMargins(0, 0, 0, 0);
    stepsGrid->setHorizontalSpacing(6);
    stepsGrid->setVerticalSpacing(4);
    stepsGrid->setColumnStretch(1, 1);

    for (int i = 0; i < 3; ++i) {
        manualStepNumbers[i] = new QLabel(QString("%1.").arg(i + 1), manualPanel);
        manualStepNumbers[i]->setObjectName("manualSteps");
        manualStepNumbers[i]->setAlignment(Qt::AlignRight | Qt::AlignTop);

        manualStepTexts[i] = new QLabel(manualPanel);
        manualStepTexts[i]->setObjectName("manualSteps");
        manualStepTexts[i]->setTextFormat(Qt::RichText);
        manualStepTexts[i]->setWordWrap(true);

        stepsGrid->addWidget(manualStepNumbers[i], i, 0, Qt::AlignTop);
        stepsGrid->addWidget(manualStepTexts[i], i, 1);
    }

    // La línea a pegar en el `init.py` y su botón de copiar, en la fila siguiente al paso 3 y
    // en su misma columna. El botón va a la derecha de la línea, que es lo que copia: abajo,
    // junto al de exportar, no se sabía cuál de las dos cosas copiaba. La caja se queda con el
    // sobrante de la fila (stretch 1) en vez de ajustarse al texto, así el botón queda siempre
    // en la misma posición y no se corre de lugar si algún día cambia el largo de la línea.
    QHBoxLayout *codeRow = new QHBoxLayout();
    codeRow->setContentsMargins(0, 0, 0, 0);
    codeRow->setSpacing(8);

    manualCodeLabel = new QLabel(NukeBridge::pluginAddPathLine(), manualPanel);
    manualCodeLabel->setObjectName("manualCode");
    manualCodeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    copyLineButton = new QPushButton(TR(BtnCopyLine), manualPanel);
    copyLineButton->setFixedHeight(34);
    copyLineButton->setMinimumWidth(120);
    copyLineButton->setProperty("class", "secondary");

    codeRow->addWidget(manualCodeLabel, 1);
    codeRow->addWidget(copyLineButton, 0);

    stepsGrid->addLayout(codeRow, 3, 1);
    manualLayout->addLayout(stepsGrid);

    QHBoxLayout *manualButtons = new QHBoxLayout();
    manualButtons->setContentsMargins(0, 0, 0, 0);
    manualButtons->setSpacing(10);

    exportButton = new QPushButton(TR(BtnExport), manualPanel);
    exportButton->setFixedHeight(34);
    exportButton->setProperty("class", "action");

    manualButtons->addWidget(exportButton);
    manualButtons->addStretch(1);
    manualLayout->addLayout(manualButtons);

    bridgeLayout->addWidget(manualPanel);

    centralLayout->addWidget(bridgeGroup);

    // El estado se recalcula también al editar el campo a mano: sin esto, pegar otra `.nuke`
    // que ya tiene el bridge dejaba el chip en "Not installed" hasta apretar INSTALL.
    // editingFinished y no textChanged: inspeccionar el disco en cada tecla es trabajo de I/O
    // en el hilo de UI por cada caracter tipeado.
    connect(bridgeDirEdit, &QLineEdit::editingFinished, this, &ConfigWindow::refreshBridgeStatus);
    connect(bridgeBrowseButton, &QPushButton::clicked, this, &ConfigWindow::browseNukeDirectory);
    connect(bridgeInstallButton, &QPushButton::clicked, this, &ConfigWindow::installBridge);
    connect(manualToggleButton, &QPushButton::clicked, this, &ConfigWindow::toggleManualPanel);
    connect(exportButton, &QPushButton::clicked, this, &ConfigWindow::exportBridgeFiles);
    connect(copyLineButton, &QPushButton::clicked, this, &ConfigWindow::copyPluginLine);

    // El campo arranca con la carpeta que ya conocemos: la registrada por otra app LGA, o la
    // detectada en el home. Vacío solo si no hay ninguna.
    bridgeDirEdit->setText(QDir::toNativeSeparators(NukeBridge::currentNukeDirectory()));
}

void ConfigWindow::repolish(QWidget *widget)
{
    if (!widget) {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void ConfigWindow::refreshBridgeStatus()
{
    if (!bridgeChipLabel) {
        return;
    }

    const QString nukeDir = bridgeDirEdit ? bridgeDirEdit->text().trimmed() : QString();
    const NukeBridge::Status status = NukeBridge::inspect(nukeDir);
    const QString bundled = NukeBridge::bundledVersion();

    if (!status.installed()) {
        bridgeChipLabel->setText(TR(ChipNotInstalled));
        bridgeChipLabel->setProperty("state", "off");
        bridgeInstallButton->setText(TR(BtnInstall));
        bridgeInstallButton->setProperty("class", "action");
    } else if (status.installedVersion.isEmpty()) {
        // Instalado pero sin `VERSION` legible: pasa con un bridge puesto a mano, con uno de
        // una versión anterior de la app, o con el archivo sin permiso de lectura.
        //
        // Antes este caso caía en el `else` y el chip mostraba en verde la versión que la app
        // TRAERÍA, no la que hay: el usuario veía "al día" sobre un bridge viejo, que es
        // exactamente lo que este chip existe para detectar. Se muestra como pendiente.
        bridgeChipLabel->setText(TR(ChipInstalledUnknown));
        bridgeChipLabel->setProperty("state", "stale");
        bridgeInstallButton->setText(TR(BtnReinstall));
        bridgeInstallButton->setProperty("class", "action");
    } else if (status.installedVersion != bundled) {
        // Instalado pero de otra version: el botón vuelve a ser la acción destacada, porque
        // acá sí hay algo que hacer. Con "Installed" a secas el usuario no se entera.
        bridgeChipLabel->setText(TR(ChipNeedsUpdate).arg(status.installedVersion));
        bridgeChipLabel->setProperty("state", "stale");
        bridgeInstallButton->setText(TR(BtnReinstall));
        bridgeInstallButton->setProperty("class", "action");
    } else {
        bridgeChipLabel->setText(TR(ChipInstalled).arg(status.installedVersion));
        bridgeChipLabel->setProperty("state", "on");
        bridgeInstallButton->setText(TR(BtnReinstall));
        bridgeInstallButton->setProperty("class", "secondary");
    }

    repolish(bridgeChipLabel);
    repolish(bridgeInstallButton);

    // La condición mira que la carpeta EXISTA, no sólo que el campo tenga texto: con una ruta
    // tipeada a mano que no existe, el hint afirmaba haberla encontrado mientras el chip decía
    // "Not installed" y INSTALL fallaba con "esa carpeta no existe". Tres mensajes, dos de
    // ellos falsos.
    if (!QFileInfo(nukeDir).isDir()) {
        bridgeHintLabel->setText(TR(HintFolderNotFound));
        bridgeHintLabel->setVisible(true);
    } else if (!status.installed()) {
        // toHtmlEscaped: el label es RichText y un path puede traer `&` o `<` (por ejemplo
        // /Users/x/R&D/.nuke), que sin escapar se renderiza mal o se come el resto del texto.
        bridgeHintLabel->setText(TR(HintFolderFound).arg(
            QDir::toNativeSeparators(nukeDir).toHtmlEscaped()));
        bridgeHintLabel->setVisible(true);
    } else {
        // Ya instalado: el hint explicaba qué carpeta usar, y eso ya no aporta nada.
        bridgeHintLabel->setVisible(false);
    }
}

void ConfigWindow::browseNukeDirectory()
{
    const QString current = bridgeDirEdit->text().trimmed();
    const QString startDir = QFileInfo(current).isDir() ? current : QDir::homePath();

    // La `.nuke` está oculta: sin DontUseNativeDialog el panel de macOS no la muestra y el
    // usuario no puede llegar a ella salvo con Cmd+Shift+. , que casi nadie conoce.
    QFileDialog dialog(this, TR(PickNukeFolder), startDir);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setFilter(dialog.filter() | QDir::Hidden);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QStringList selected = dialog.selectedFiles();
    if (selected.isEmpty()) {
        return;
    }
    bridgeDirEdit->setText(QDir::toNativeSeparators(QDir::cleanPath(selected.first())));
    refreshBridgeStatus();
}

namespace {

/// El motor devuelve un CODIGO; el texto que ve el usuario sale de la tabla del idioma
/// activo. Ver el comentario de NukeBridge::Error sobre por que estan separados.
QString bridgeErrorMessage(NukeBridge::Error error)
{
    switch (error) {
    case NukeBridge::Error::None:           return QString();
    case NukeBridge::Error::DirMissing:     return TR(MsgBridgeDirMissing);
    case NukeBridge::Error::SourceRepo:     return TR(MsgBridgeSourceIsTarget);
    case NukeBridge::Error::PayloadMissing: return TR(MsgBridgePayloadMissing);
    case NukeBridge::Error::WriteFailed:    return TR(MsgBridgeWriteFailed);
    }
    return TR(MsgBridgeWriteFailed);
}

} // namespace

void ConfigWindow::installBridge()
{
    const QString nukeDir = bridgeDirEdit->text().trimmed();

    QString detail;
    const NukeBridge::Error error = NukeBridge::install(nukeDir, &detail);
    if (error == NukeBridge::Error::None) {
        refreshBridgeStatus();
        Dialogs::info(this, TR(TitleBridgeInstalled),
                      TR(MsgBridgeInstalled).arg(Dialogs::colorizePath(
                          QDir::toNativeSeparators(QDir(nukeDir).filePath(
                              NukeBridge::pluginFolderName())))));
        return;
    }

    // El detalle tecnico va al log y NO al cartel: al usuario le sirve saber que hacer, no
    // que ruta fallo. Al que diagnostica le sirve la ruta, y esa la tiene en el log.
    Logger::logError(QString("NukeBridge: instalacion fallida en '%1': %2").arg(nukeDir, detail));
    Dialogs::error(this, TR(TitleBridgeFailed), bridgeErrorMessage(error));
}

void ConfigWindow::toggleManualPanel()
{
    const bool nowVisible = !manualPanel->isVisible();
    manualPanel->setVisible(nowVisible);
    manualToggleButton->setText(nowVisible ? TR(LinkManualHide) : TR(LinkManualShow));

    // Agranda la ventana para que las instrucciones entren — salvo que el usuario ya la haya
    // dimensionado a mano, en cuyo caso `calculateAndResizeWindow()` no le toca el alto y lo
    // único que pasa es que aparece contenido nuevo más abajo.
    calculateAndResizeWindow();

    if (nowVisible && scrollArea) {
        // Diferido: recién después de que el layout acomodó el panel recién mostrado tiene
        // sentido preguntarle dónde quedó. En la misma pasada, su geometría todavía es la que
        // tenía estando oculto.
        QTimer::singleShot(0, this, [this]() {
            if (!scrollArea || !manualPanel || !manualPanel->isVisible()) {
                return;
            }
            QWidget *content = scrollArea->widget();
            if (!content) {
                return;
            }
            // Se desplaza al TOPE del panel y no con `ensureWidgetVisible()`: ese, cuando el
            // widget es más alto que el viewport —que es justo lo que pasa con la ventana
            // achicada a mano—, lo centra, y entonces el paso 1 queda arriba del borde visible.
            const int top = manualPanel->mapTo(content, QPoint(0, 0)).y();
            scrollArea->verticalScrollBar()->setValue(top - 20);
        });
    }
}

void ConfigWindow::exportBridgeFiles()
{
    const QString destDir = QFileDialog::getExistingDirectory(
        this, TR(PickExportFolder), QDir::homePath());
    if (destDir.isEmpty()) {
        return;
    }

    QString detail;
    const NukeBridge::Error error = NukeBridge::exportPayload(destDir, &detail);
    if (error == NukeBridge::Error::None) {
        Dialogs::info(this, TR(TitleBridgeExported),
                      TR(MsgBridgeExported).arg(Dialogs::colorizePath(
                          QDir::toNativeSeparators(QDir(destDir).filePath(
                              NukeBridge::pluginFolderName())))));
        return;
    }

    Logger::logError(QString("NukeBridge: exportacion fallida a '%1': %2").arg(destDir, detail));
    Dialogs::error(this, TR(TitleBridgeFailed), bridgeErrorMessage(error));
}

void ConfigWindow::copyPluginLine()
{
    QApplication::clipboard()->setText(NukeBridge::pluginAddPathLine());

    // El feedback va en el propio botón y no en un cartel: es una confirmación de un gesto
    // trivial, y un modal para esto obliga a un click extra para volver a lo que se estaba
    // haciendo.
    copyLineButton->setText(TR(BtnCopied));
    if (!copyFeedbackTimer) {
        copyFeedbackTimer = new QTimer(this);
        copyFeedbackTimer->setSingleShot(true);
        connect(copyFeedbackTimer, &QTimer::timeout, this,
                [this]() { copyLineButton->setText(TR(BtnCopyLine)); });
    }
    copyFeedbackTimer->start(1500);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Idioma
// ─────────────────────────────────────────────────────────────────────────────

void ConfigWindow::setLanguage(I18n::Lang lang)
{
    if (I18n::lang() == lang) {
        return;
    }
    I18n::setLang(lang);

    langEnButton->setProperty("selected", lang == I18n::Lang::En);
    langEsButton->setProperty("selected", lang == I18n::Lang::Es);
    repolish(langEnButton);
    repolish(langEsButton);

    retranslateUi();
    calculateAndResizeWindow();
}

void ConfigWindow::refreshApplyButtonText()
{
    if (!applyButton)
        return;

    // El boton dice lo que va a HACER. Si esta app ya es la que abre los `.nk`, apretarlo no
    // asocia nada nuevo: rehace el registro, que es lo que sirve cuando la asociacion quedo
    // apuntando a una copia vieja o el Finder dejo de reconocerla.
    //
    // En Windows la consulta equivalente lee UserChoice/UserChoiceLatest y compara el ProgID.
#ifdef Q_OS_MACOS
    applyButton->setText(MacIntegration::isDefaultNkHandler() ? TR(BtnReapply) : TR(BtnApply));
#else
    applyButton->setText(WinFileAssociation::isNkAssociatedWithUs() ? TR(BtnReapply)
                                                                     : TR(BtnApply));
#endif
}

void ConfigWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    // Al volver al frente se relee: el usuario pudo haber cambiado la asociacion en Finder, o
    // haber contestado el cartel del sistema con la ventana atras.
    if (event->type() == QEvent::ActivationChange && isActiveWindow())
        refreshApplyButtonText();
}

void ConfigWindow::retranslateUi()
{
    refreshApplyButtonText();
    descriptionLabel->setText(TR(DescFileAssociation));

    nukeVersionDescLabel->setText(TR(DescNukeVersion));
    nukePathEdit->setPlaceholderText(TR(PlaceholderNukePath));
    browseButton->setText(TR(BtnBrowse));
    saveButton->setText(TR(BtnSave));

    // El label del escáner dice cosas distintas según en qué punto esté: reconstruirlo pide
    // saber en cuál, y por eso el resultado del último escaneo queda guardado.
    if (!scanFinished) {
        foundVersionsLabel->setText(TR(ScanChoose));
        foundVersionsLabel->setVisible(true);
        scanningLabel->setText(TR(ScanScanning));
    } else if (foundVersionsCount == 0) {
        // Sin versiones encontradas, "Elegí una de las encontradas" es falso y además queda
        // arriba del mensaje que dice que no hay ninguna. Antes tampoco se reescribía al
        // cambiar de idioma, así que los dos renglones quedaban en idiomas distintos.
        foundVersionsLabel->setVisible(false);
        scanningLabel->setText(TR(ScanNone));
    } else {
        foundVersionsLabel->setText(TR(ScanFound).arg(foundVersionsCount));
        foundVersionsLabel->setVisible(true);
    }

    bridgeDescLabel->setText(TR(DescNukeBridge));
    bridgeDirEdit->setPlaceholderText(TR(PlaceholderNukeDir));
    bridgeBrowseButton->setText(TR(BtnBrowse));
    manualToggleButton->setText(manualPanel->isVisible() ? TR(LinkManualHide)
                                                         : TR(LinkManualShow));
    manualStepTexts[0]->setText(TR(ManualStep1));
    manualStepTexts[1]->setText(TR(ManualStep2));
    manualStepTexts[2]->setText(TR(ManualStep3));
    exportButton->setText(TR(BtnExport));
    copyLineButton->setText(TR(BtnCopyLine));

    // El chip, el hint y el texto del botón de instalar dependen del estado, no solo del
    // idioma: los reconstruye el mismo lugar que los calcula.
    refreshBridgeStatus();
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
        TR(PickNukeExecutable),
        startDir,
        "Executable Files (*.exe);;All Files (*)");
    if (!selectedPath.isEmpty())
        nukePathEdit->setText(selectedPath);
#else
    // On macOS, start at /Applications and accept any file (incl. .app bundles)
    QString startDir = currentPath.isEmpty() ? "/Applications" : QDir(currentPath).absolutePath();
    QString selectedPath = QFileDialog::getOpenFileName(
        this,
        TR(PickNukeExecutable),
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
        // TitleWarning y no TitleError: no es un error, es un requisito sin cumplir.
        Dialogs::warn(this, TR(TitleWarning), TR(MsgPathEmpty));
        return;
    }

    // Verificar que el archivo existe
    if (!QFile::exists(nukePath))
    {
        Dialogs::error(this, TR(TitleError), TR(MsgPathMissing));
        return;
    }

    // Verificar que parece un ejecutable de Nuke
    QString fileName = QFileInfo(nukePath).fileName().toLower();
    if (!fileName.contains("nuke"))
    {
        Dialogs::warn(this, TR(TitleWarning), TR(MsgNotNukeExecutable));
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

    // El path va coloreado y en su PROPIA linea, debajo del texto (convencion de Doc_Dialogs):
    // metido en el medio de la oracion se corta por donde cae el wrap y se vuelve ilegible.
    Dialogs::info(this, TR(TitleSaved),
                  TR(MsgSaved).arg(Dialogs::colorizePath(QDir::toNativeSeparators(nukePath))));
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
        Dialogs::warn(this, TR(TitleWarning), TR(MsgPathEmpty));
        return;
    }

    if (!QFile::exists(nukePath)) {
        Logger::logError(QString("Error: El archivo no existe: %1").arg(nukePath));
        Dialogs::error(this, TR(TitleError), TR(MsgPathMissing));
        return;
    }

    Logger::logInfo("Guardando configuración de NukeX");
    saveNukePath(nukePath);

#ifdef Q_OS_WIN
    try {
        Logger::logInfo("Ejecutando asociacion nativa de Windows");
        const WinFileAssociation::ApplyOutcome outcome =
            WinFileAssociation::apply(reinterpret_cast<HWND>(winId()));
        if (outcome.result == WinFileAssociation::ApplyResult::Success) {
            Logger::logInfo("Asociación completada exitosamente");
            refreshApplyButtonText();
            Dialogs::info(this, TR(TitleAssocDone), TR(MsgAssocDone));
        } else if (outcome.result == WinFileAssociation::ApplyResult::NeedsUserConfirmation) {
            Logger::logInfo("Asociación registrada; falta confirmación del usuario en Windows");
            if (!outcome.errors.isEmpty()) {
                Logger::logError(QString("Asociación con advertencias: %1")
                                     .arg(outcome.errors.join("; ")));
            }
            Dialogs::info(this, TR(TitleAssocWindowsConfirm), TR(MsgAssocWindowsConfirm));
        } else {
            const QStringList errors =
                outcome.errors.isEmpty()
                    ? QStringList{QStringLiteral("Error al configurar asociación")}
                    : outcome.errors;
            Logger::logError(QString("Asociación con errores: %1").arg(errors.join("; ")));
            Dialogs::warn(this, TR(TitleAssocWarnings),
                          DialogStyle::emphasis(errors.join(QStringLiteral("<br>"))));
        }
    } catch (const std::exception &e) {
        Logger::logError(QString("Excepción durante asociación: %1").arg(e.what()));
        Dialogs::error(this, TR(TitleError),
                       QString::fromUtf8(e.what()));
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

QStringList ConfigWindow::executeRegistryCommands()
{
    const WinFileAssociation::ApplyOutcome outcome = WinFileAssociation::apply();
    return outcome.errors;
}

#endif // Q_OS_WIN

// ─────────────────────────────────────────────────────────────────────────────
//  macOS: file association via Launch Services
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

    // Lo que se asocia al `.nk` es la RUTA de ESTE bundle. Desde el arbol de build eso deja al
    // Finder apuntando a una carpeta de compilacion que el proximo `limpiar.sh` borra, y el
    // usuario se queda sin poder abrir un `.nk` con doble click sin entender por que. Antes no
    // pasaba porque `duti` recibia el bundle ID y resolvia el que estuviera registrado.
    if (LgaRegistry::runsFromDevTree()) {
        Logger::logError(QString("Asociacion cancelada: la app corre desde una salida de "
                                 "desarrollo (%1)").arg(bundlePath));
        Dialogs::warn(this, TR(TitleWarning), TR(MsgAssocDevTree));
        return;
    }

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

    // Paso 2: quedar como handler por defecto de los `.nk`. Esto lo hacia `duti`, que es un
    // binario de Homebrew que el usuario tenia que instalarse aparte y que la mayoria no tiene:
    // sin el, APPLY terminaba siempre en el cartel de "Casi listo" mandando a hacer a mano el
    // Get Info + Change All. `duti` no hace ninguna magia — por dentro le pide el cambio a Launch
    // Services, que es lo mismo que ahora se pide desde adentro de la app.
    //
    // La llamada es asincrona porque macOS pone SU PROPIO cartel de confirmacion antes de
    // cambiar una asociacion, y hasta que el usuario no lo contesta no hay resultado. Esa
    // confirmacion no se puede saltear ni desde la app ni con duti: es el sistema protegiendo
    // una preferencia del usuario. Lo que si cambia es que ahora es UN cartel del sistema en vez
    // de cuatro pasos manuales en Finder.
    //
    // Mientras el cartel del sistema esta arriba, APPLY queda apagado para no encolar un segundo
    // pedido. Lo reactiva el callback, y tambien un watchdog: si el usuario deja el cartel sin
    // contestar, el callback no llega nunca, y sin la segunda red el boton se quedaba muerto
    // hasta reiniciar la app. El watchdog NO cancela nada — la respuesta tardia sigue valiendo.
    if (applyButton)
        applyButton->setEnabled(false);
    QPointer<ConfigWindow> self(this);
    QTimer::singleShot(kAssocWatchdogMs, this, [self]() {
        if (self && self->applyButton)
            self->applyButton->setEnabled(true);
    });

    // `self` y no `this`: el callback llega despues de que el usuario conteste el cartel, y para
    // entonces la ventana pudo haberse cerrado.
    MacIntegration::setAsDefaultNkHandler([self](bool ok, const QString &error) {
        if (!self)
            return;
        if (self->applyButton)
            self->applyButton->setEnabled(true);
        self->refreshApplyButtonText();
        if (ok) {
            Logger::logInfo("Handler por defecto de .nk configurado");
            Dialogs::info(self, TR(TitleAssocDone), TR(MsgAssocDone));
            return;
        }
        // El fallo mas comun no es un bug sino un "No" en el cartel del sistema, asi que el
        // cartel que queda es el de los pasos manuales y no uno de error.
        Logger::logError(QString("No se pudo fijar el handler por defecto de .nk: %1")
                             .arg(error.isEmpty() ? QStringLiteral("sin detalle") : error));
        Dialogs::info(self, TR(TitleAssocAlmost), TR(MsgAssocAlmost));
    });
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
    
    scanFinished = false;
    foundVersionsCount = 0;

    if (scanningLabel) {
        scanningLabel->setText(TR(ScanScanning));
        scanningLabel->setProperty("state", "busy");
        repolish(scanningLabel);
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
        scanningLabel->setText(TR(ScanScanningPath).arg(shortPath));
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

    // Se guardan para poder rearmar los labels al cambiar de idioma: llevan la cantidad
    // adentro del texto, asi que no alcanza con volver a pedir el string traducido.
    scanFinished = true;
    foundVersionsCount = versions.size();

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
            scanningLabel->setText(TR(ScanNone));
            scanningLabel->setProperty("state", "empty");
            repolish(scanningLabel);
            scanningLabel->setVisible(true);
            Logger::logInfo("✓ Mensaje de 'no encontrado' mostrado");
        }
        return;
    }
    
    Logger::logInfo("Creando botones para las versiones encontradas...");
    // Crear botones para las versiones encontradas
    createVersionButtons(versions);

    // Recien aca se sabe que hay instalado, asi que recien aca se puede corregir una ruta muerta.
    healStalePath(versions);
    
    // Mostrar el contenedor de botones
    if (versionsButtonsWidget) {
        versionsButtonsWidget->setVisible(true);
        Logger::logInfo("✓ versionsButtonsWidget mostrado");
    } else {
        Logger::logError("ERROR: versionsButtonsWidget es nullptr al intentar mostrar");
    }
    
    // Actualizar mensaje descriptivo
    if (foundVersionsLabel) {
        foundVersionsLabel->setText(TR(ScanFound).arg(versions.size()));
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

namespace {

/// Convierte "16.0v9" en una tupla comparable (16, 0, 9). Lo que no parsea queda en cero, asi
/// que una version rara pierde contra cualquiera bien formada en vez de ganar por accidente.
std::tuple<int, int, int> versionSortKey(const QString &version)
{
    static const QRegularExpression re(QStringLiteral("^(\\d+)\\.(\\d+)v(\\d+)"));
    const QRegularExpressionMatch m = re.match(version);
    if (!m.hasMatch())
        return {0, 0, 0};
    return {m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt()};
}

} // namespace

// El campo se carga desde nukeXpath.txt ANTES de que termine el escaneo, asi que puede quedar
// mostrando una version que el usuario desinstalo hace meses. Sin esto, la ventana se veia
// coherente —listaba las versiones que si estan— pero APPLY y SAVE fallaban con "ese archivo ya
// no existe" sobre una ruta que el usuario no eligio en esa sesion y que no tenia motivo para
// mirar. Se repone sola con la mas nueva encontrada, y se persiste: si no se guardara, el doble
// click desde el Finder seguiria intentando lanzar el ejecutable muerto.
void ConfigWindow::healStalePath(const QList<NukeVersion> &versions)
{
    if (!nukePathEdit || versions.isEmpty())
        return;

    const QString currentPath = nukePathEdit->text().trimmed();
    if (!currentPath.isEmpty() && QFile::exists(currentPath))
        return;

    const NukeVersion *newest = &versions.first();
    for (const NukeVersion &version : versions) {
        if (versionSortKey(version.version) > versionSortKey(newest->version))
            newest = &version;
    }

    if (currentPath.isEmpty()) {
        Logger::logInfo(QString("Sin ruta configurada: se toma la version mas nueva encontrada (%1)")
                            .arg(newest->displayName));
    } else {
        Logger::logInfo(QString("La ruta guardada ya no existe (%1): se repone con %2")
                            .arg(currentPath, newest->displayName));
    }

    nukePathEdit->setText(newest->path);
    saveNukePath(newest->path);
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

int ConfigWindow::screenHeightCap() const
{
    // El techo real es la PANTALLA, no un número fijo: con el panel manual desplegado el
    // contenido pide más de lo que entra en un portátil, y una ventana más alta que el
    // escritorio deja el pie —el selector de idioma y la versión— abajo del borde. Se toma el
    // 80% del área USABLE (ya sin barra de menú ni Dock) para que además quede aire alrededor.
    // Lo que no entra lo resuelve el QScrollArea que ya envuelve todo el contenido.
    if (const QScreen *screen = this->screen()) {
        const int cap = static_cast<int>(screen->availableGeometry().height() * kMaxScreenFraction);
        return qMax(cap, kMinWindowHeight);
    }
    // Sin pantalla que consultar no hay techo que calcular: no se inventa uno, porque un tope
    // arbitrario recortaría contenido en un monitor grande.
    return QWIDGETSIZE_MAX;
}

void ConfigWindow::applyWindowHeight(int maxHeight, int targetHeight)
{
    // El flag envuelve TAMBIÉN a los setters de límites, no sólo al `resize()`: cuando el
    // máximo nuevo queda por debajo del alto actual, `QWidget::setMaximumSize()` hace un
    // `resize()` adentro, y ese resize llegaba a `resizeEvent()` con el flag apagado. O sea
    // que plegar el panel manual —que baja el máximo— se registraba como si el usuario hubiera
    // redimensionado la ventana, y desde ahí volver a desplegarlo ya no la agrandaba nunca
    // más. Justo el caso que tiene que funcionar.
    programmaticResize = true;
    setMinimumHeight(kMinWindowHeight);
    setMaximumHeight(maxHeight);
    if (targetHeight > 0) {
        resize(kWindowWidth, targetHeight);
    }
    lastAppliedHeight = height();
    programmaticResize = false;

    // Cada vez que la app cambia el alto vuelve a centrar: si no, la ventana crece hacia ABAJO
    // desde donde estaba y termina descentrada. El escaneo de versiones y el panel manual
    // cambian el alto después de abrir, así que centrar una sola vez al arranque no alcanza.
    if (targetHeight > 0) {
        centerOnScreen();
    }
}

int ConfigWindow::preferredWindowHeight() const
{
    const QWidget *centralWidget = this->findChild<QWidget*>("centralSettingsWidget");
    if (!centralWidget) {
        return kMinWindowHeight;
    }

    // Márgenes de la ventana más el ancho de una posible barra de scroll: sin ese colchón la
    // ventana queda exactamente un pelo corta y aparece scroll cuando no hacía falta.
    const int margins = 40;
    const int scrollBarSpace = 20;
    const int totalHeight = qMin(centralWidget->sizeHint().height() + margins + scrollBarSpace,
                                 screenHeightCap());

    return qMax(totalHeight, kMinWindowHeight);
}

void ConfigWindow::calculateAndResizeWindow()
{
    Logger::logInfo("=== CALCULANDO Y REDIMENSIONANDO VENTANA ===");

    // El `processEvents()` de abajo despacha eventos pendientes, y entre ellos puede venir un
    // click sobre INSTALL o sobre el propio toggle del panel manual — que vuelve a entrar acá
    // mientras la pasada anterior no terminó (y en el caso de INSTALL, abre un modal desde
    // adentro del resize). El guard corta la reentrada; la pasada de afuera termina el
    // trabajo igual.
    static bool resizing = false;
    if (resizing) {
        Logger::logInfo("Resize reentrante ignorado");
        return;
    }
    resizing = true;
    struct ResizeGuard {
        bool& flag;
        ~ResizeGuard() { flag = false; }
    } guard{resizing};

    // Forzar que todos los widgets calculen su tamaño
    this->updateGeometry();
    QApplication::processEvents();
    
    if (!this->findChild<QWidget*>("centralSettingsWidget")) {
        Logger::logError("ERROR: No se pudo encontrar centralSettingsWidget");
        return;
    }

    const int totalHeight = preferredWindowHeight();
    Logger::logInfo(QString("Altura preferida calculada: %1").arg(totalHeight));

    // El tope de la ventana es la altura que PIDE el contenido: más allá de eso, estirar solo
    // agrega fondo vacío abajo. El piso permite achicarla, y ahí se hace cargo el QScrollArea.
    //
    // Si el usuario ya eligió un alto no se lo pisamos: se le aplican los límites y nada más.
    // El recorte al nuevo máximo lo hace `setMaximumHeight()` por su cuenta cuando hace falta
    // —por ejemplo al plegar el panel manual, que deja a la ventana más alta que su contenido—
    // y por eso ese setter también va envuelto en el flag.
    applyWindowHeight(totalHeight, userResizedHeight ? 0 : totalHeight);
    Logger::logInfo(QString("Ventana: alto %1, maximo %2, impuesto por la app: %3")
                        .arg(height()).arg(totalHeight)
                        .arg(userResizedHeight ? "no" : "si"));

    // Clampear la altura NO alcanza: la ventana crece hacia ABAJO desde donde ya estaba, así
    // que una que entra en la pantalla igual puede terminar con el pie por debajo del borde.
    // Medido: 1024 px de alto arrancando en y=224, sobre un escritorio de 1117 — el selector
    // de idioma quedaba fuera y, al ser la ventana de tamaño fijo, no había forma de llegar.
    if (QScreen *screen = this->screen()) {
        const QRect available = screen->availableGeometry();
        const QRect frame = frameGeometry();
        if (frame.bottom() > available.bottom()) {
            const int desiredTop = qMax(available.top(), available.bottom() - frame.height());
            // Se mueve por DELTA sobre la posición del cliente: frameGeometry() incluye la
            // barra de título, así que un move() con su `top` correría la ventana de más.
            move(x(), y() + (desiredTop - frame.top()));
            Logger::logInfo(QString("Ventana reubicada para entrar en la pantalla (y=%1)").arg(y()));
        }
    }
}
