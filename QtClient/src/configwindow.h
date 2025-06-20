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
#include "logger.h"
#include "nukescanner.h"

class QFlowLayout;

class ConfigWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWindow(QWidget *parent = nullptr);

private slots:
    void browseNukePath();
    void saveConfiguration();
    void applyFileAssociation();
    void onScanStarted();
    void onScanProgress(const QString &currentPath);
    void onVersionFound(const NukeVersion &version);
    void onScanFinished(const QList<NukeVersion> &versions);
    void onVersionButtonClicked();

private:
    void setupUI();
    void loadCurrentPath();
    void loadStyleSheet();
    void saveNukePath(const QString &path);
    QString getNukePathFromFile();
    void executeRegistryCommands();
    bool cleanRegistry();
    bool registerProgId();
    bool setFileAssociation();
    bool executeCommand(const QString &program, const QStringList &arguments);
    
    // Scanner de versiones
    void initializeScanner();
    void createVersionButtons(const QList<NukeVersion> &versions);

    QLineEdit *nukePathEdit;
    QPushButton *browseButton;
    QPushButton *saveButton;
    QPushButton *applyButton;
    QLabel *descriptionLabel;
    
    // Scanner de versiones
    NukeScanner *nukeScanner;
    QWidget *versionsContainer;
    QLabel *scanningLabel;
    QLabel *foundVersionsLabel;
    QWidget *versionsButtonsWidget;
    QVBoxLayout *versionsLayout;
};

#endif // CONFIGWINDOW_H