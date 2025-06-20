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
#include <QFileInfo>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#endif

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
    void saveNukePath(const QString &path);
    QString getNukePathFromFile();
    void associateNkFiles();
    QString getCurrentExecutablePath();
    bool registerOfficialProgId(const QString &progId, const QString &exePath);
    bool executeSetUserFTA(const QString &extension, const QString &progId);
    void verifyFinalResult(const QString &extension, const QString &progId);
    void cleanupFileAssociations();
    
    QLineEdit *nukePathEdit;
    QPushButton *browseButton;
    QPushButton *saveButton;
    QPushButton *applyButton;
    QLabel *titleLabel;
    QLabel *descriptionLabel;
};

#endif // CONFIGWINDOW_H 