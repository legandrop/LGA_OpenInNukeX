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
class QTimer;

class ConfigWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWindow(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

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
    void calculateAndResizeWindow();

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
    QLabel *manualStepsLabel;
    QLabel *manualCodeLabel;
    QPushButton *exportButton;
    QPushButton *copyLineButton;
    QTimer *copyFeedbackTimer;

    // Pie
    QPushButton *langEnButton;
    QPushButton *langEsButton;
    QLabel *versionLabel;
};

#endif // CONFIGWINDOW_H
