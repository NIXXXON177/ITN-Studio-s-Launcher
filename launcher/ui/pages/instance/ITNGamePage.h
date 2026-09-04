// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - Minecraft Launcher
 *
 *  In-launcher Minecraft game settings editor (options.txt).
 */

#pragma once

#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStringList>
#include <QWidget>

#include "ui/pages/BasePage.h"

class MinecraftInstance;

class ITNGamePage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit ITNGamePage(MinecraftInstance* inst, QWidget* parent = 0);
    virtual ~ITNGamePage() = default;

    void openedImpl() override;
    bool apply() override;

    virtual QString displayName() const override { return tr("Настройки игры"); }
    virtual QIcon icon() const override { return QIcon::fromTheme("settings"); }
    virtual QString id() const override { return "itngame"; }
    virtual QString helpPage() const override { return QString(); }
    void retranslate() override {}

   private slots:
    void resetToITN();
    void openImportDialog();
    void refreshImportState();

   private:
    void loadOptions();
    void saveOptions();
    void applyITNDefaults();
    QString optionsPath() const;

   private:
    MinecraftInstance* m_inst = nullptr;
    // raw options.txt lines: key -> value, plus order
    QMap<QString, QString> m_values;
    QStringList m_order;

    QSpinBox* m_chunks = nullptr;
    QComboBox* m_graphics = nullptr;
    QComboBox* m_clouds = nullptr;
    QComboBox* m_particles = nullptr;
    QSlider* m_fov = nullptr;
    QLabel* m_fovLabel = nullptr;
    QSlider* m_gamma = nullptr;
    QLabel* m_gammaLabel = nullptr;
    QCheckBox* m_vsync = nullptr;
    QComboBox* m_fps = nullptr;
    QCheckBox* m_fullscreen = nullptr;
    QSlider* m_music = nullptr;
    QLabel* m_musicLabel = nullptr;
    QLabel* m_langLabel = nullptr;
};
