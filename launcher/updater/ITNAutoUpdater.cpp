// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - automatic updates from GitHub Releases
 */

#include "updater/ITNAutoUpdater.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QProcess>
#include <QSettings>
#include <QTextStream>
#include <QUrl>

#include "Application.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "QObjectPtr.h"
#include "Version.h"
#include "archive/ExtractZipTask.h"

ITNAutoUpdater::ITNAutoUpdater(QObject* parent) : QObject(parent) {}

QString ITNAutoUpdater::localVersionPath()
{
    return FS::PathCombine(QCoreApplication::applicationDirPath(), "itn-version.txt");
}

QString ITNAutoUpdater::localGameVersionPath()
{
    return FS::PathCombine(QCoreApplication::applicationDirPath(), "itn-game-version.txt");
}

QString ITNAutoUpdater::readLocalVersion()
{
    QFile f(localVersionPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("0");
    }
    return QString::fromUtf8(f.readAll()).trimmed();
}

QString ITNAutoUpdater::readLocalGameVersion()
{
    QFile f(localGameVersionPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("0");
    }
    return QString::fromUtf8(f.readAll()).trimmed();
}

void ITNAutoUpdater::writeLocalVersion(const QString& tag)
{
    QFile f(localVersionPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(tag.toUtf8());
        f.write("\n");
    }
}

void ITNAutoUpdater::writeLocalGameVersion(const QString& tag)
{
    QFile f(localGameVersionPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(tag.toUtf8());
        f.write("\n");
    }
}

void ITNAutoUpdater::checkForUpdates(bool silent)
{
    m_silent = silent;
    m_mode = Mode::LegacyCheck;
    if (m_reply) {
        return;
    }

    emit statusMessage(tr("Проверка обновлений ITN…"));
    emit phaseChanged(tr("Проверка обновлений…"), tr("Лаунчер"));
    emit progressChanged(-1);

    QNetworkRequest req{ QUrl(QString::fromUtf8(kRepoApi)) };
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ITNLauncher-Updater"));
    req.setRawHeader("Accept", "application/vnd.github+json");
    m_reply = m_nam.get(req);
    connect(m_reply, &QNetworkReply::finished, this, &ITNAutoUpdater::onReleasesFinished);
}

void ITNAutoUpdater::runStartupSequence()
{
    m_silent = true;
    m_mode = Mode::StartupLauncher;
    m_restarting = false;
    if (m_reply) {
        return;
    }

    emit phaseChanged(tr("Проверка обновлений…"), tr("Сверяем версию лаунчера"));
    emit progressChanged(-1);
    emit statusMessage(tr("Проверка обновлений ITN…"));

    QNetworkRequest req{ QUrl(QString::fromUtf8(kRepoApi)) };
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ITNLauncher-Updater"));
    req.setRawHeader("Accept", "application/vnd.github+json");
    m_reply = m_nam.get(req);
    connect(m_reply, &QNetworkReply::finished, this, &ITNAutoUpdater::onReleasesFinished);
}

void ITNAutoUpdater::finishStartup(bool restarted)
{
    emit progressChanged(100);
    emit phaseChanged(tr("Готово"), restarted ? tr("Перезапуск…") : tr("Всё актуально"));
    emit startupFinished(restarted);
}

void ITNAutoUpdater::onReleasesFinished()
{
    auto* reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "ITN update check failed:" << reply->errorString();
        if (!m_silent) {
            QMessageBox::warning(nullptr, tr("Обновление"), tr("Не удалось проверить обновления:\n%1").arg(reply->errorString()));
        }
        emit updateFinished(false);
        if (m_mode != Mode::LegacyCheck) {
            // Offline / API fail — still open launcher
            emit phaseChanged(tr("Офлайн-режим"), tr("Не удалось проверить обновления"));
            finishStartup(false);
        }
        return;
    }

    const auto doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit updateFinished(false);
        if (m_mode != Mode::LegacyCheck)
            finishStartup(false);
        return;
    }
    const auto obj = doc.object();
    m_lastRelease = obj;
    const QString tag = obj.value(QStringLiteral("tag_name")).toString().trimmed();
    if (tag.isEmpty()) {
        emit updateFinished(false);
        if (m_mode != Mode::LegacyCheck)
            finishStartup(false);
        return;
    }

    const QString local = readLocalVersion();
    const Version remoteVer(tag.startsWith('v') ? tag.mid(1) : tag);
    const Version localVer(local.startsWith('v') ? local.mid(1) : local);
    const bool newer = (local == QStringLiteral("0")) || ((localVer <=> remoteVer) == std::strong_ordering::less);

    if (!newer) {
        qDebug() << "ITN: up to date" << local << "vs" << tag;
        if (!m_silent && m_mode == Mode::LegacyCheck) {
            QMessageBox::information(nullptr, tr("Обновление"), tr("У вас актуальная версия (%1).").arg(local));
        }
        emit updateFinished(false);
        if (m_mode == Mode::StartupLauncher) {
            beginGameCheck(obj);
        }
        return;
    }

    QString url;
    QString name;
    const auto assets = obj.value(QStringLiteral("assets")).toArray();
    for (const auto& a : assets) {
        const auto ao = a.toObject();
        const QString n = ao.value(QStringLiteral("name")).toString();
        const QString u = ao.value(QStringLiteral("browser_download_url")).toString();
        if (n.contains(QString::fromUtf8(kAssetNeedle), Qt::CaseInsensitive) && n.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
            url = u;
            name = n;
            break;
        }
    }
    if (url.isEmpty()) {
        qWarning() << "ITN: no windows zip asset in release" << tag;
        emit updateFinished(false);
        if (m_mode == Mode::StartupLauncher) {
            beginGameCheck(obj);
        }
        return;
    }

    qInfo() << "ITN: update available" << local << "->" << tag << name;
    emit statusMessage(tr("Скачивание обновления %1…").arg(tag));
    emit phaseChanged(tr("Обновление лаунчера"), tr("Скачивание %1").arg(tag));
    emit progressChanged(0);
    downloadAsset(url, tag, name);
}

void ITNAutoUpdater::beginGameCheck(const QJsonObject& releaseObj)
{
    m_mode = Mode::StartupGame;
    emit phaseChanged(tr("Проверка игры…"), tr("Сверяем модпак"));
    emit progressChanged(-1);

    const QString tag = releaseObj.value(QStringLiteral("tag_name")).toString().trimmed();
    QString url;
    QString name;
    const auto assets = releaseObj.value(QStringLiteral("assets")).toArray();
    for (const auto& a : assets) {
        const auto ao = a.toObject();
        const QString n = ao.value(QStringLiteral("name")).toString();
        const QString u = ao.value(QStringLiteral("browser_download_url")).toString();
        if (n.contains(QString::fromUtf8(kGameNeedle), Qt::CaseInsensitive) &&
            (n.endsWith(QStringLiteral(".mrpack"), Qt::CaseInsensitive) || n.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive))) {
            // Prefer mrpack
            if (n.endsWith(QStringLiteral(".mrpack"), Qt::CaseInsensitive) || url.isEmpty()) {
                url = u;
                name = n;
                if (n.endsWith(QStringLiteral(".mrpack"), Qt::CaseInsensitive))
                    break;
            }
        }
    }

    if (url.isEmpty()) {
        // No separate game asset — treat launcher tag as game baseline
        if (readLocalGameVersion() == QStringLiteral("0") && !tag.isEmpty()) {
            writeLocalGameVersion(tag);
        }
        emit phaseChanged(tr("Игра актуальна"), {});
        finishStartup(false);
        return;
    }

    // Version key: release tag + asset name so re-uploads bump
    const QString remoteGame = tag + QLatin1Char('/') + name;
    const QString localGame = readLocalGameVersion();
    if (localGame == remoteGame) {
        emit phaseChanged(tr("Игра актуальна"), name);
        finishStartup(false);
        return;
    }

    emit phaseChanged(tr("Обновление игры"), tr("Скачивание %1").arg(name));
    emit progressChanged(0);
    downloadGameAsset(url, remoteGame, name);
}

void ITNAutoUpdater::downloadAsset(const QString& url, const QString& tag, const QString& name)
{
    m_pendingTag = tag;
    m_pendingName = name;

    const QString updatesDir = FS::PathCombine(QCoreApplication::applicationDirPath(), "updates");
    FS::ensureFolderPathExists(updatesDir);
    m_downloadPath = FS::PathCombine(updatesDir, name);

    QNetworkRequest req{ QUrl(url) };
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ITNLauncher-Updater"));
    m_reply = m_nam.get(req);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &ITNAutoUpdater::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &ITNAutoUpdater::onDownloadFinished);
}

void ITNAutoUpdater::downloadGameAsset(const QString& url, const QString& version, const QString& name)
{
    m_pendingTag = version;
    m_pendingName = name;

    const QString updatesDir = FS::PathCombine(QCoreApplication::applicationDirPath(), "updates");
    FS::ensureFolderPathExists(updatesDir);
    m_downloadPath = FS::PathCombine(updatesDir, name);

    QNetworkRequest req{ QUrl(url) };
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ITNLauncher-Updater"));
    m_reply = m_nam.get(req);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &ITNAutoUpdater::onGameDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &ITNAutoUpdater::onGameDownloadFinished);
}

void ITNAutoUpdater::onDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0) {
        const int pct = static_cast<int>((received * 100) / total);
        emit progressChanged(pct);
        emit statusMessage(tr("Скачивание обновления… %1 / %2 МБ")
                               .arg(QString::number(received / (1024.0 * 1024.0), 'f', 1))
                               .arg(QString::number(total / (1024.0 * 1024.0), 'f', 1)));
        emit phaseChanged(tr("Обновление лаунчера"),
                          tr("%1 / %2 МБ").arg(QString::number(received / (1024.0 * 1024.0), 'f', 1),
                                               QString::number(total / (1024.0 * 1024.0), 'f', 1)));
    }
}

void ITNAutoUpdater::onGameDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0) {
        emit progressChanged(static_cast<int>((received * 100) / total));
        emit phaseChanged(tr("Обновление игры"),
                          tr("%1 / %2 МБ").arg(QString::number(received / (1024.0 * 1024.0), 'f', 1),
                                               QString::number(total / (1024.0 * 1024.0), 'f', 1)));
    }
}

void ITNAutoUpdater::onDownloadFinished()
{
    auto* reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "ITN download failed:" << reply->errorString();
        if (!m_silent) {
            QMessageBox::warning(nullptr, tr("Обновление"), tr("Ошибка загрузки:\n%1").arg(reply->errorString()));
        }
        emit updateFinished(false);
        if (m_mode == Mode::StartupLauncher) {
            beginGameCheck(m_lastRelease);
        }
        return;
    }

    QFile out(m_downloadPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "ITN cannot write" << m_downloadPath;
        emit updateFinished(false);
        if (m_mode == Mode::StartupLauncher)
            beginGameCheck(m_lastRelease);
        return;
    }
    out.write(reply->readAll());
    out.close();

    qInfo() << "ITN: downloaded" << m_downloadPath;
    emit statusMessage(tr("Установка обновления %1…").arg(m_pendingTag));
    emit phaseChanged(tr("Установка лаунчера"), m_pendingTag);
    emit progressChanged(95);
    applyUpdate(m_downloadPath, m_pendingTag);
}

void ITNAutoUpdater::onGameDownloadFinished()
{
    auto* reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "ITN game download failed:" << reply->errorString();
        emit phaseChanged(tr("Игра без обновления"), reply->errorString());
        finishStartup(false);
        return;
    }

    QFile out(m_downloadPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        finishStartup(false);
        return;
    }
    out.write(reply->readAll());
    out.close();

    emit phaseChanged(tr("Установка модпака"), m_pendingName);
    emit progressChanged(90);
    applyGamePack(m_downloadPath, m_pendingTag);
    finishStartup(false);
}

bool ITNAutoUpdater::shouldPreservePath(const QString& relativePath) const
{
    QString p = relativePath;
    p.replace('\\', '/');
    p = p.toLower();
    if (p.endsWith(QStringLiteral("accounts.json")))
        return true;
    if (p.contains(QStringLiteral("/saves/")))
        return true;
    if (p.contains(QStringLiteral("/screenshots/")))
        return true;
    if (p.contains(QStringLiteral("/logs/")))
        return true;
    if (p.endsWith(QStringLiteral("itn-modded.imported")))
        return true;
    if (p.endsWith(QStringLiteral("itn-game-version.txt")))
        return true;
    return false;
}

void ITNAutoUpdater::applyUpdate(const QString& zipPath, const QString& tag)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString staging = FS::PathCombine(appDir, "updates", "staging");
    FS::deletePath(staging);
    FS::ensureFolderPathExists(staging);

    auto task = makeShared<MMCZip::ExtractZipTask>(zipPath, QDir(staging), QString());
    QEventLoop loop;
    bool ok = false;
    connect(task.get(), &Task::succeeded, &loop, [&] {
        ok = true;
        loop.quit();
    });
    connect(task.get(), &Task::failed, &loop, [&](QString) { loop.quit(); });
    connect(task.get(), &Task::aborted, &loop, &QEventLoop::quit);
    task->start();
    loop.exec();

    if (!ok) {
        qWarning() << "ITN: extract failed";
        if (!m_silent) {
            QMessageBox::warning(nullptr, tr("Обновление"), tr("Не удалось распаковать обновление."));
        }
        emit updateFinished(false);
        if (m_mode == Mode::StartupLauncher)
            beginGameCheck(m_lastRelease);
        return;
    }

    QDir stageDir(staging);
    QString contentRoot = staging;
    const auto entries = stageDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (entries.size() == 1 && !QFile::exists(FS::PathCombine(staging, "ITNLauncher.exe"))) {
        contentRoot = stageDir.absoluteFilePath(entries.first());
    }

    QDirIterator it(contentRoot, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        const QString rel = QDir(contentRoot).relativeFilePath(fi.absoluteFilePath());
        if (fi.isDir())
            continue;
        if (shouldPreservePath(rel) && QFile::exists(FS::PathCombine(appDir, rel)))
            continue;
        if (rel.startsWith(QStringLiteral("updates/"), Qt::CaseInsensitive))
            continue;

        const QString dest = FS::PathCombine(appDir, rel);
        if (fi.fileName().compare(QStringLiteral("ITNLauncher.exe"), Qt::CaseInsensitive) == 0) {
            const QString pending = dest + QStringLiteral(".new");
            QFile::remove(pending);
            QFile::copy(fi.absoluteFilePath(), pending);
            continue;
        }
        FS::ensureFilePathExists(dest);
        QFile::remove(dest);
        if (!QFile::copy(fi.absoluteFilePath(), dest)) {
            qWarning() << "ITN: failed to copy" << fi.absoluteFilePath() << "->" << dest;
        }
    }

    writeLocalVersion(tag);
    scheduleRestartApply(contentRoot, tag);
    emit updateFinished(true);
}

void ITNAutoUpdater::applyGamePack(const QString& mrpackPath, const QString& version)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString destMrpack = FS::PathCombine(appDir, "ITN-Modded.mrpack");
    QFile::remove(destMrpack);
    QFile::copy(mrpackPath, destMrpack);

    // Prefer updating existing instance mods/overrides; keep worlds & options
    QString instRoot;
    if (APPLICATION) {
        instRoot = APPLICATION->instances()->primaryDir();
    }
    if (instRoot.isEmpty()) {
        instRoot = FS::PathCombine(appDir, "instances");
    }

    QString targetInst;
    const auto dirs = QDir(instRoot).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& d : dirs) {
        if (d.contains(QStringLiteral("ITN"), Qt::CaseInsensitive) || d.contains(QStringLiteral("Modded"), Qt::CaseInsensitive)) {
            targetInst = FS::PathCombine(instRoot, d);
            break;
        }
    }
    if (targetInst.isEmpty() && !dirs.isEmpty()) {
        targetInst = FS::PathCombine(instRoot, dirs.first());
    }

    if (!targetInst.isEmpty()) {
        const QString staging = FS::PathCombine(appDir, "updates", "game-staging");
        FS::deletePath(staging);
        FS::ensureFolderPathExists(staging);

        auto task = makeShared<MMCZip::ExtractZipTask>(mrpackPath, QDir(staging), QString());
        QEventLoop loop;
        bool ok = false;
        connect(task.get(), &Task::succeeded, &loop, [&] {
            ok = true;
            loop.quit();
        });
        connect(task.get(), &Task::failed, &loop, [&](QString) { loop.quit(); });
        connect(task.get(), &Task::aborted, &loop, &QEventLoop::quit);
        task->start();
        loop.exec();

        if (ok) {
            const QString overrides = FS::PathCombine(staging, "overrides");
            // Prism portable uses minecraft/; older paths may use .minecraft/
            QString mc = FS::PathCombine(targetInst, "minecraft");
            if (!QDir(mc).exists()) {
                mc = FS::PathCombine(targetInst, ".minecraft");
            }
            FS::ensureFolderPathExists(mc);

            // Sync overrides except user data
            if (QDir(overrides).exists()) {
                QDirIterator oit(overrides, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                while (oit.hasNext()) {
                    oit.next();
                    const QFileInfo fi = oit.fileInfo();
                    if (fi.isDir())
                        continue;
                    const QString rel = QDir(overrides).relativeFilePath(fi.absoluteFilePath());
                    const QString relLower = rel.toLower();
                    if (relLower.contains(QStringLiteral("saves/")) || relLower.contains(QStringLiteral("screenshots/")) ||
                        relLower.endsWith(QStringLiteral("options.txt")) || relLower.endsWith(QStringLiteral("servers.dat")) ||
                        relLower.contains(QStringLiteral("logs/"))) {
                        continue;
                    }
                    const QString dest = FS::PathCombine(mc, rel);
                    FS::ensureFilePathExists(dest);
                    QFile::remove(dest);
                    QFile::copy(fi.absoluteFilePath(), dest);
                }
            }
        }
    }

    writeLocalGameVersion(version);
    emit progressChanged(100);
    emit phaseChanged(tr("Игра обновлена"), version);
}

void ITNAutoUpdater::scheduleRestartApply(const QString& /*stagingDir*/, const QString& tag)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString bat = FS::PathCombine(appDir, "itn-apply-update.bat");
    const QString exe = FS::PathCombine(appDir, "ITNLauncher.exe");
    const QString exeNew = exe + QStringLiteral(".new");

    QFile f(bat);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "ITN: cannot write apply script";
        if (m_mode == Mode::StartupLauncher)
            beginGameCheck(m_lastRelease);
        return;
    }
    QTextStream out(&f);
    out << "@echo off\r\n";
    out << "timeout /t 2 /nobreak >nul\r\n";
    out << "if exist \"" << exeNew << "\" (\r\n";
    out << "  del /f /q \"" << exe << "\" 2>nul\r\n";
    out << "  move /y \"" << exeNew << "\" \"" << exe << "\" >nul\r\n";
    out << ")\r\n";
    out << "start \"\" \"" << exe << "\"\r\n";
    out << "del /f /q \"%~f0\"\r\n";
    f.close();

    m_restarting = true;
    emit phaseChanged(tr("Перезапуск лаунчера"), tag);
    emit progressChanged(100);

    if (m_mode == Mode::LegacyCheck) {
        QMessageBox::information(nullptr, tr("Обновление ITN"),
                                 tr("Обновление %1 скачано и установлено.\nЛаунчер перезапустится.").arg(tag));
    }

    emit startupFinished(true);
    QProcess::startDetached(QStringLiteral("cmd.exe"), { QStringLiteral("/c"), bat }, appDir);
    QCoreApplication::quit();
}
