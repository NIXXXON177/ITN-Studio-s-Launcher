// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - automatic updates from GitHub Releases
 */
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

/** Checks GitHub Releases and auto-downloads/installs portable updates. */
class ITNAutoUpdater : public QObject {
    Q_OBJECT
   public:
    explicit ITNAutoUpdater(QObject* parent = nullptr);

    /** Start background check (call after main window is up). */
    void checkForUpdates(bool silent = true);

    static QString localVersionPath();
    static QString readLocalVersion();
    static void writeLocalVersion(const QString& tag);

   signals:
    void statusMessage(QString message);
    void updateFinished(bool applied);

   private slots:
    void onReleasesFinished();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();

   private:
    void downloadAsset(const QString& url, const QString& tag, const QString& name);
    void applyUpdate(const QString& zipPath, const QString& tag);
    bool shouldPreservePath(const QString& relativePath) const;
    void scheduleRestartApply(const QString& stagingDir, const QString& tag);

    QNetworkAccessManager m_nam;
    QNetworkReply* m_reply = nullptr;
    QString m_pendingTag;
    QString m_pendingName;
    QString m_downloadPath;
    bool m_silent = true;

    static constexpr const char* kRepoApi =
        "https://api.github.com/repos/NIXXXON177/ITN-Studio-s-Launcher/releases/latest";
    static constexpr const char* kAssetNeedle = "ITNLauncher-windows";
};
