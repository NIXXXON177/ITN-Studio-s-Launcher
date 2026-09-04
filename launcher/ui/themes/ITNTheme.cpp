// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - Minecraft Launcher
 *
 *  ITN green theme.
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
    itnPalette.setColor(QPalette::Window, QColor(13, 22, 15));
    itnPalette.setColor(QPalette::WindowText, QColor(232, 245, 233));
    itnPalette.setColor(QPalette::Base, QColor(10, 17, 12));
    itnPalette.setColor(QPalette::AlternateBase, QColor(16, 27, 19));
    itnPalette.setColor(QPalette::ToolTipBase, QColor(232, 245, 233));
    itnPalette.setColor(QPalette::ToolTipText, QColor(13, 22, 15));
    itnPalette.setColor(QPalette::Text, QColor(232, 245, 233));
    itnPalette.setColor(QPalette::Button, QColor(22, 38, 26));
    itnPalette.setColor(QPalette::ButtonText, QColor(232, 245, 233));
    itnPalette.setColor(QPalette::BrightText, QColor(255, 90, 90));
    itnPalette.setColor(QPalette::Link, QColor(74, 222, 128));
    itnPalette.setColor(QPalette::Highlight, QColor(34, 197, 94));
    itnPalette.setColor(QPalette::HighlightedText, Qt::black);
    itnPalette.setColor(QPalette::PlaceholderText, QColor(110, 140, 115));
    return fadeInactive(itnPalette, fadeAmount(), fadeColor());
}

double ITNTheme::fadeAmount()
{
    return 0.5;
}

QColor ITNTheme::fadeColor()
{
    return QColor(13, 22, 15);
}

bool ITNTheme::hasStyleSheet()
{
    return true;
}

QString ITNTheme::appStyleSheet()
{
    return QStringLiteral(
        "QToolTip { color: #e8f5e9; background-color: #166534; border: 1px solid #22c55e; }"
        "QPushButton { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #22c55e, stop: 1 #15803d);"
        " color: #04120a; border: 1px solid #14532d; border-radius: 4px; padding: 4px 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #4ade80, stop: 1 #16a34a); }"
        "QPushButton:pressed { background-color: #15803d; }"
        "QPushButton:disabled { background-color: #1a2b1f; color: #6e8c73; border: 1px solid #24382a; }"
        "QProgressBar { background-color: #0a110c; border: 1px solid #14532d; border-radius: 4px; text-align: center; color: #e8f5e9; }"
        "QProgressBar::chunk { background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #16a34a, stop: 1 #4ade80); border-radius: 3px; }"
        "QTabBar::tab:selected { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #15803d, stop: 1 #0d1610); color: #e8f5e9; }"
        "QMainWindow { background-color: #0d1610; background-image: url(:/itn/banner); background-position: center; background-repeat: no-repeat; background-attachment: fixed; }");
}

QString ITNTheme::tooltip()
{
    return "";
}
