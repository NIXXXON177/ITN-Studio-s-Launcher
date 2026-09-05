// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - PineCraft-inspired theme (deep forest + pine→gold gradients)
 */
#include "ITNTheme.h"

#include <QObject>

QString ITNTheme::id()
{
    return "itn";
}

QString ITNTheme::name()
{
    return QObject::tr("ITN");
}

QPalette ITNTheme::colorScheme()
{
    QPalette itnPalette;
    // PineCraft: --bg-deep / --bg-panel / --ink
    itnPalette.setColor(QPalette::Window, QColor(10, 17, 13));
    itnPalette.setColor(QPalette::WindowText, QColor(232, 242, 234));
    itnPalette.setColor(QPalette::Base, QColor(13, 21, 16));
    itnPalette.setColor(QPalette::AlternateBase, QColor(19, 33, 26));
    itnPalette.setColor(QPalette::ToolTipBase, QColor(232, 242, 234));
    itnPalette.setColor(QPalette::ToolTipText, QColor(10, 17, 13));
    itnPalette.setColor(QPalette::Text, QColor(232, 242, 234));
    itnPalette.setColor(QPalette::Button, QColor(19, 33, 26));
    itnPalette.setColor(QPalette::ButtonText, QColor(232, 242, 234));
    itnPalette.setColor(QPalette::BrightText, QColor(229, 99, 107));
    itnPalette.setColor(QPalette::Link, QColor(111, 220, 154));
    itnPalette.setColor(QPalette::Highlight, QColor(240, 180, 41));
    itnPalette.setColor(QPalette::HighlightedText, QColor(29, 26, 8));
    itnPalette.setColor(QPalette::PlaceholderText, QColor(92, 114, 99));
    return fadeInactive(itnPalette, fadeAmount(), fadeColor());
}

double ITNTheme::fadeAmount()
{
    return 0.5;
}

QColor ITNTheme::fadeColor()
{
    return QColor(10, 17, 13);
}

bool ITNTheme::hasStyleSheet()
{
    return true;
}

QString ITNTheme::appStyleSheet()
{
    // Gradients: pine (#35c26e) ↔ gold (#f0b429), not flat green
    return QStringLiteral(
        "QToolTip {"
        "  color: #e8f2ea; background-color: #13211a;"
        "  border: 1px solid #2c4434; padding: 6px 10px;"
        "}"
        "QPushButton {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #ffd66b, stop:0.45 #f0b429, stop:1 #35c26e);"
        "  color: #1d1a08; border: 1px solid rgba(0,0,0,0.45);"
        "  border-radius: 4px; padding: 7px 16px; font-weight: 800;"
        "}"
        "QPushButton:hover {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #ffe08a, stop:0.4 #f0b429, stop:1 #6fdc9a);"
        "}"
        "QPushButton:pressed {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #c99220, stop:1 #1c7a45);"
        "  padding-top: 8px; padding-bottom: 6px;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #13211a; color: #5c7263; border: 1px solid #1e2f25;"
        "}"
        "QToolButton:hover {"
        "  background-color: rgba(240, 180, 41, 40); border-radius: 4px;"
        "}"
        "QToolButton:pressed {"
        "  background-color: rgba(53, 194, 110, 55);"
        "}"
        "QProgressBar {"
        "  background-color: rgba(0,0,0,0.45); border: 1px solid #1e2f25;"
        "  border-radius: 2px; text-align: center; color: #e8f2ea; min-height: 10px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #8a6a14, stop:0.55 #f0b429, stop:1 #35c26e);"
        "  border-radius: 1px;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #1c7a45, stop:1 #0a110d);"
        "  color: #e8f2ea; border-bottom: 2px solid #f0b429;"
        "}"
        "QTabBar::tab:!selected { color: #93a89a; }"
        "QTabBar::tab:hover:!selected { color: #6fdc9a; }"
        "QMenuBar::item:selected {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 rgba(53,194,110,40), stop:1 rgba(240,180,41,35));"
        "}"
        "QMenu::item:selected {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 rgba(53,194,110,55), stop:1 rgba(240,180,41,40));"
        "  color: #e8f2ea;"
        "}"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
        "  background: #2c4434; border-radius: 3px; min-height: 24px;"
        "}"
        "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #35c26e, stop:1 #f0b429);"
        "}"
        "QLineEdit, QComboBox, QSpinBox, QTextEdit, QPlainTextEdit {"
        "  background: #0d1510; border: 1px solid #1e2f25; border-radius: 4px;"
        "  padding: 4px 8px; color: #e8f2ea; selection-background-color: #f0b429;"
        "  selection-color: #1d1a08;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus {"
        "  border: 1px solid #35c26e;"
        "}"
        "QToolBar {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #13211a, stop:1 #0a110d);"
        "  border: none; border-right: 1px solid #1e2f25; spacing: 4px;"
        "}"
        "QStatusBar {"
        "  background: #0d1510; color: #5c7263; border-top: 1px solid #1e2f25;"
        "}"
        "QMainWindow {"
        "  background-color: #0a110d;"
        "  background-image: url(:/itn/banner);"
        "  background-position: center; background-repeat: no-repeat;"
        "  background-attachment: fixed;"
        "}");
}

QString ITNTheme::tooltip()
{
    return QObject::tr("ITN — хвойный градиент (pine → gold)");
}
