#include "configwindow.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFont>
#include <QPalette>

ConfigWindow::ConfigWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadCurrentPath();
    
    // Configurar ventana
    setWindowTitle("NukeX Path Configuration");
    setFixedSize(500, 200);
    
    // Establecer color de fondo #161616
    setStyleSheet("QWidget { background-color: #161616; color: #ffffff; }");
}

void ConfigWindow::setupUI()
{
    // Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // Título
    titleLabel = new QLabel("NukeX", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { color: #ffffff; margin-bottom: 10px; }");
    
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
    
    // Agregar widgets al layout principal
    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(pathLayout);
    mainLayout->addWidget(saveButton, 0, Qt::AlignCenter);
    
    // Conectar señales
    connect(browseButton, &QPushButton::clicked, this, &ConfigWindow::browseNukePath);
    connect(saveButton, &QPushButton::clicked, this, &ConfigWindow::saveConfiguration);
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
    QString appDir = QApplication::applicationDirPath();
    QString filepath = QDir(appDir).filePath("nukeXpath.txt");
    
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
    QString appDir = QApplication::applicationDirPath();
    QString filepath = QDir(appDir).filePath("nukeXpath.txt");
    
    QFile file(filepath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << path;
        file.close();
    }
} 