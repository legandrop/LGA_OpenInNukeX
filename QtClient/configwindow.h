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

class ConfigWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWindow(QWidget *parent = nullptr);

private slots:
    void browseNukePath();
    void saveConfiguration();

private:
    void setupUI();
    void loadCurrentPath();
    void saveNukePath(const QString &path);
    QString getNukePathFromFile();
    
    QLineEdit *nukePathEdit;
    QPushButton *browseButton;
    QPushButton *saveButton;
    QLabel *titleLabel;
};

#endif // CONFIGWINDOW_H 