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
#include "logger.h"

class ConfigWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWindow(QWidget *parent = nullptr);

private slots:
    void browseNukePath();
    void saveConfiguration();
    void applyFileAssociation();

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

    QLineEdit *nukePathEdit;
    QPushButton *browseButton;
    QPushButton *saveButton;
    QPushButton *applyButton;
    QLabel *descriptionLabel;
};

#endif // CONFIGWINDOW_H