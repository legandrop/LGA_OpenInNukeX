QT += core widgets network

CONFIG += c++17
CONFIG += console

TARGET = LGA_OpenInNukeX_Qt
TEMPLATE = app

SOURCES += \
    main.cpp \
    nukeopener.cpp \
    configwindow.cpp

HEADERS += \
    nukeopener.h \
    configwindow.h

# Configuración para Windows
win32 {
    CONFIG += windows
    RC_ICONS = ../Developement/Nuke15_102.ico
}

# Optimizaciones para release
CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
    QMAKE_CXXFLAGS_RELEASE += -O2
} 