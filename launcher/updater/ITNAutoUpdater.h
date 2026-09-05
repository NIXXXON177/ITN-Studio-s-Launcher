// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - automatic updates from GitHub Releases (launcher + game pack)
 */
#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>

/** Checks GitHub Releases and auto-downloads/installs portable updates. */
class ITNAutoUpdater : public QObject {
    Q_OBJECT
   public:
    explicit ITNAutoUpdater(QObject* parent = nullptr);

    /** Silent background check (legacy). */
    void checkForUpdates(bool silent = true);

    /**
     * Startup flow: check launcher, then game pack, emit UI signals.
     * Call after splash is visible. Emits startupFinished when UI can open.
     */
    void runStartupSequence();

    static QString localVersionPath();
    static QString localGameVersionPath();
    static QString readLocalVersion();
    static QString readLocalGameVersion();
    static void writeLocalVersion(const QString& tag);
    static void writeLocalGameVersion(const QString& tag);

   signals:
    void statusMessage(QString message);
    void phaseChanged(QString title, QString detail);
    void progressChanged(int percent);  // 0..100, -1 indeterminate
    void updateFinished(bool applied);
    void startupFinished(bool restartedForLauncherUpdate);

   private slots:
    void onReleasesFinished();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();
    void onGameDownloadProgress(qint64 received, qint64 total);
    void onGameDownloadFinished();

   private:
    enum class Mode { LegacyCheck, StartupLauncher, StartupGame };

    void downloadAsset(const QString& url, const QString& tag, const QString& name);
    void applyUpdate(const QString& zipPath, const QString& tag);
    bool shouldPreservePath(const QString& relativePath) const;
    void scheduleRestartApply(const QString& stagingDir, const QString& tag);
    void beginGameCheck(const QJsonObject& releaseObj);
    void downloadGameAsset(const QString& url, const QString& version, const QString& name);
    void applyGamePack(const QString& mrpackPath, const QString& version);
    void finishStartup(bool restarted);

    QNetworkAccessManager m_nam;
    QNetworkReply* m_reply = nullptr;
    QString m_pendingTag;
    QString m_pendingName;
    QString m_downloadPath;
    bool m_silent = true;
    Mode m_mode = Mode::LegacyCheck;
    QJsonObject m_lastRelease;
    bool m_restarting = false;

    static constexpr const char* kRepoApi =
        "https://api.github.com/repos/NIXXXON177/ITN-Studio-s-Launcher/releases/latest";
    static constexpr const char* kAssetNeedle = "ITNLauncher-windows";
    static constexpr const char* kGameNeedle = "ITN-Modded";
};
