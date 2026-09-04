// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - Minecraft Launcher
 *
 *  Import settings, servers, packs, mods and skins from other launchers.
 */

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>

class MinecraftInstance;

struct ITNSourceInstance {
    QString name;
    QString mcDir;
    QStringList mods;
    QStringList skins;
    bool hasOptions = false;
    bool hasServers = false;
    bool hasResPacks = false;
    bool hasShaders = false;
};

class ITNImportDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ITNImportDialog(MinecraftInstance* inst, QWidget* parent = 0);

   signals:
    void imported();

   private slots:
    void refreshLaunchers();
    void refreshInstances();
    void runImport();

   private:
    struct LauncherRoot {
        QString name;
        QString path;
    };
    QList<LauncherRoot> detectLaunchers();
    QList<ITNSourceInstance> scanRoot(const QString& root);
    ITNSourceInstance scanInstanceDir(const QString& dir, const QString& fallbackName);
    bool doImport(const ITNSourceInstance& src, QStringList& log);
    static bool mergeServersDat(const QString& dstPath, const QString& srcPath);

   private:
    MinecraftInstance* m_inst = nullptr;
    QComboBox* m_launcherBox = nullptr;
    QListWidget* m_instanceList = nullptr;
    QCheckBox* m_optOptions = nullptr;
    QCheckBox* m_optServers = nullptr;
    QCheckBox* m_optResPacks = nullptr;
    QCheckBox* m_optShaders = nullptr;
    QCheckBox* m_optMods = nullptr;
    QCheckBox* m_optSkins = nullptr;
    QListWidget* m_modsList = nullptr;
    QListWidget* m_skinsList = nullptr;
    QPushButton* m_importBtn = nullptr;
    QList<ITNSourceInstance> m_sources;
};
