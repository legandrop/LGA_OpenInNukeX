#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QApplication>
#include <QStandardPaths>
#include <QProcess>
#include <QThread>
#include <QCoreApplication>
#include <QScrollArea>
#include <QGroupBox>
#include "qflowlayout.h"
#include "i18n.h"
#include "logger.h"
#include "nukescanner.h"

class QFlowLayout;
class QShowEvent;
class QResizeEvent;
class QTimer;

class ConfigWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWindow(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    /// Distingue el resize del USUARIO del que hace la app: ver `userResizedHeight`.
    void resizeEvent(QResizeEvent *event) override;
    /// Idem para el movimiento: ver `userMovedWindow`.
    void moveEvent(QMoveEvent *event) override;

private slots:
    void browseNukePath();
    void saveConfiguration();
    void applyFileAssociation();
    void onScanStarted();
    void onScanProgress(const QString &currentPath);
    void onVersionFound(const NukeVersion &version);
    void onScanFinished(const QList<NukeVersion> &versions);
    void onVersionButtonClicked();

    // Nuke Bridge
    void browseNukeDirectory();
    void installBridge();
    void toggleManualPanel();
    void exportBridgeFiles();
    void copyPluginLine();

private:
    void setupUI();
    void loadCurrentPath();
    void loadStyleSheet();
    void saveNukePath(const QString &path);
    QString getNukePathFromFile();
    bool executeCommand(const QString &program, const QStringList &arguments);

    // Scanner de versiones
    void initializeScanner();
    void createVersionButtons(const QList<NukeVersion> &versions);
    /// Si la ruta guardada apunta a una version de Nuke que ya no esta instalada, la reemplaza
    /// por la mas nueva de las encontradas por el escaner. Ver el comentario del cuerpo.
    void healStalePath(const QList<NukeVersion> &versions);
    void calculateAndResizeWindow();
    /// Alto que pide el contenido, ya recortado al tope de pantalla. Es tambien el maximo al
    /// que se puede estirar la ventana a mano: mas alto que esto solo agrega fondo vacio.
    int preferredWindowHeight() const;
    /// El 80% del alto USABLE de la pantalla actual. Es el techo duro, independiente de lo que
    /// pida el contenido.
    int screenHeightCap() const;
    /// Aplica limites y, si `targetHeight` es positivo, tambien el alto, marcando todo como
    /// propio de la app. Todo cambio de geometria pasa por aca: ver `programmaticResize` para
    /// por que no alcanza con envolver el `resize()`.
    void applyWindowHeight(int maxHeight, int targetHeight);
    /// Centra la ventana en la pantalla actual. No hace nada si el usuario ya la movio o
    /// redimensiono a mano: desde ese momento la posicion es suya.
    void centerOnScreen();

    // Idioma
    void setLanguage(I18n::Lang lang);
    /// Reescribe TODO string visible. Es la contracara de no usar tr(): los textos ya estan
    /// adentro de los widgets, asi que cambiar el idioma es volver a setearlos uno por uno.
    void retranslateUi();

    // Nuke Bridge
    void setupBridgeGroup(QWidget *parent, QVBoxLayout *centralLayout);
    void refreshBridgeStatus();
    /// Refresca el `class`/`state` de un widget para que el QSS vuelva a evaluar el selector:
    /// cambiar la propiedad sola no repinta nada.
    static void repolish(QWidget *widget);

#ifdef Q_OS_WIN
    /// Devuelve la lista de pasos que fallaron (vacia si salio todo bien). No muestra
    /// ningun cartel: eso lo hace el llamador, para que salga UNO solo.
    QStringList executeRegistryCommands();
    bool cleanRegistry();
    bool registerProgId();
    bool setFileAssociation();
#else
    void executeMacAssociation();
    QString getAppBundlePath() const;
    QString resolveNukeBinaryFromBundle(const QString &bundlePath) const;
#endif

    /// El area de scroll de toda la ventana. Es miembro porque el panel manual necesita
    /// pedirle que desplace hasta el contenido recien desplegado.
    QScrollArea *scrollArea;

    QLineEdit *nukePathEdit;
    QPushButton *browseButton;
    QPushButton *saveButton;
    QPushButton *applyButton;
    QLabel *descriptionLabel;
    QLabel *nukeVersionDescLabel;

    // Scanner de versiones
    NukeScanner *nukeScanner;
    QWidget *versionsContainer;
    QLabel *scanningLabel;
    QLabel *foundVersionsLabel;
    QWidget *versionsButtonsWidget;
    QVBoxLayout *versionsLayout;
    /// Cuantas versiones encontro el ultimo escaneo: `retranslateUi()` necesita el numero
    /// para volver a armar el label, que lleva la cantidad adentro del texto.
    int foundVersionsCount;
    bool scanFinished;

    // Nuke Bridge
    QLabel *bridgeDescLabel;
    QLabel *bridgeChipLabel;
    QLineEdit *bridgeDirEdit;
    QPushButton *bridgeBrowseButton;
    QPushButton *bridgeInstallButton;
    QLabel *bridgeHintLabel;
    QPushButton *manualToggleButton;
    QWidget *manualPanel;
    /// Los tres pasos van en labels SEPARADOS dentro de una grilla, y no en un unico
    /// `<ol>`: la caja con la linea de `pluginAddPath` tiene que quedar sangrada exactamente
    /// como el texto del paso 3, y la sangria que Qt le da a una lista rica no es un numero
    /// que se pueda replicar desde el layout.
    QLabel *manualStepNumbers[3];
    QLabel *manualStepTexts[3];
    QLabel *manualCodeLabel;
    QPushButton *exportButton;
    QPushButton *copyLineButton;
    QTimer *copyFeedbackTimer;

    // Pie
    QPushButton *langEnButton;
    QPushButton *langEsButton;
    QLabel *versionLabel;

    /// True apenas el usuario estira o achica la ventana a mano. Desde ese momento la app no
    /// vuelve a imponerle un alto: desplegar el panel manual solo agrega contenido y desplaza
    /// hasta el, en vez de agrandar la ventana por debajo de las manos del usuario.
    bool userResizedHeight;
    /// Marca los resize que hace la propia app, para que `resizeEvent()` no los confunda con
    /// los del usuario. Tiene que envolver TAMBIEN a `setMinimumHeight()`/`setMaximumHeight()`:
    /// `QWidget::setMaximumSize()` hace un `resize()` adentro cuando el maximo nuevo queda por
    /// debajo del alto actual, y ese resize llega a `resizeEvent()` como cualquier otro.
    bool programmaticResize;
    /// Ultimo alto que impuso la app. Es la segunda linea de defensa del flag de arriba: el
    /// `processEvents()` del recalculo despacha resizes pendientes FUERA de la ventana en la
    /// que el flag esta prendido, y sin esto cualquiera de ellos se registraba como un gesto
    /// del usuario.
    int lastAppliedHeight;
    /// True apenas el usuario arrastra la ventana. Desde ahi la app deja de re-centrarla: la
    /// posicion pasa a ser suya, igual que el alto con `userResizedHeight`.
    bool userMovedWindow;
    /// Marca los `move()` que hace la propia app, para que `moveEvent()` no los confunda con
    /// un arrastre del usuario.
    bool programmaticMove;
};

#endif // CONFIGWINDOW_H
