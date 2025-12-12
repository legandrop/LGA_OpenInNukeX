"""
Qt compatibility layer for LGA_OpenInNukeX
Provides unified imports for PySide6 (Nuke 16) and PySide2 (Nuke 15)
"""

try:
    from PySide6 import QtWidgets, QtGui, QtCore  # type: ignore
    from PySide6.QtWidgets import QApplication  # type: ignore
    from PySide6.QtGui import QAction, QGuiApplication  # type: ignore
    from PySide6.QtCore import QTimer  # type: ignore
    PYSIDE_VER = 6
    print("LGA_OpenInNukeX: Using PySide6 (Qt6)")
except ImportError:  # PySide2 (Nuke ≤15)
    from PySide2 import QtWidgets, QtGui, QtCore  # type: ignore
    from PySide2.QtWidgets import QApplication  # type: ignore
    try:
        from PySide2.QtGui import QAction  # type: ignore
    except ImportError:
        from PySide2.QtWidgets import QAction  # type: ignore
    from PySide2.QtGui import QGuiApplication  # type: ignore
    from PySide2.QtCore import QTimer  # type: ignore
    PYSIDE_VER = 2
    print("LGA_OpenInNukeX: Using PySide2 (Qt5) - fallback")

__all__ = ["QtWidgets", "QtGui", "QtCore", "QApplication", "QAction", "QGuiApplication", "QTimer", "PYSIDE_VER", "get_screen_geometry", "get_screen_at"]

# Handle deprecated APIs
def get_screen_geometry():
    """Get screen geometry with Qt6/Qt5 compatibility"""
    try:
        # Qt6/PySide6 way
        app = QApplication.instance()
        if app:
            screen = app.primaryScreen()
            if screen:
                return screen.availableGeometry()
        # Fallback to primary screen
        return QGuiApplication.primaryScreen().availableGeometry()
    except:
        # Qt5/PySide2 fallback - QDesktopWidget deprecated in Qt6
        if PYSIDE_VER == 2:
            from PySide2.QtWidgets import QDesktopWidget
            desktop = QDesktopWidget()
            return desktop.availableGeometry(desktop.primaryScreen())
        else:
            # Fallback for Qt6
            return QGuiApplication.primaryScreen().availableGeometry()

def get_screen_at(pos):
    """Get screen at position with Qt6/Qt5 compatibility"""
    try:
        # Qt6/PySide6 way
        return QGuiApplication.screenAt(pos)
    except:
        # Qt5/PySide2 fallback - QDesktopWidget deprecated in Qt6
        if PYSIDE_VER == 2:
            from PySide2.QtWidgets import QDesktopWidget
            desktop = QDesktopWidget()
            return desktop.screenGeometry(pos)
        else:
            # Fallback for Qt6 - approximate with primary screen
            screen = QGuiApplication.primaryScreen()
            return screen.geometry()