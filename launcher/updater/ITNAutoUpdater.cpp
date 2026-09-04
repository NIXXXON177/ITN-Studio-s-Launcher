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
#include <QTextStream>
#include <QUrl>

#include "FileSystem.h"
#include "QObjectPtr.h"
#include "Version.h"
#include "archive/ExtractZipTask.h"

ITNAutoUpdater::ITNAutoUpdater(QObject* parent) : QObject(parent) {}

QString ITNAutoUpdater::localVersionPath()
{
    return FS::PathCombine(QCoreApplication::applicationDirPath(), "itn-version.txt");
}

QString ITNAutoUpdater::readLocalVersion()
{
    QFile f(localVersionPath());
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

void ITNAutoUpdater::checkForUpdates(bool silent)
{
    m_silent = silent;
    if (m_reply) {
        return;
    }

    emit statusMessage(tr("Проверка обновлений ITN…"));

    QNetworkRequest req{ QUrl(QString::fromUtf8(kRepoApi)) };
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ITNLauncher-Updater"));
    req.setRawHeader("Accept", "application/vnd.github+json");
    m_reply = m_nam.get(req);
    connect(m_reply, &QNetworkReply::finished, this, &ITNAutoUpdater::onReleasesFinished);
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
        return;
    }

    const auto doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit updateFinished(false);
        return;
    }
    const auto obj = doc.object();
    const QString tag = obj.value(QStringLiteral("tag_name")).toString().trimmed();
    if (tag.isEmpty()) {
        emit updateFinished(false);
        return;
    }

    const QString local = readLocalVersion();
    const Version remoteVer(tag.startsWith('v') ? tag.mid(1) : tag);
    const Version localVer(local.startsWith('v') ? local.mid(1) : local);
    const bool newer = (local == QStringLiteral("0")) || ((localVer <=> remoteVer) == std::strong_ordering::less);
    if (!newer) {
        qDebug() << "ITN: up to date" << local << "vs" << tag;
        if (!m_silent) {
            QMessageBox::information(nullptr, tr("Обновление"), tr("У вас актуальная версия (%1).").arg(local));
        }
        emit updateFinished(false);
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
        return;
    }

    qInfo() << "ITN: update available" << local << "->" << tag << name;
    emit statusMessage(tr("Скачивание обновления %1…").arg(tag));
    downloadAsset(url, tag, name);
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

void ITNAutoUpdater::onDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0) {
        emit statusMessage(tr("Скачивание обновления… %1 / %2 МБ")
                               .arg(QString::number(received / (1024.0 * 1024.0), 'f', 1))
                               .arg(QString::number(total / (1024.0 * 1024.0), 'f', 1)));
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
        return;
    }

    QFile out(m_downloadPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "ITN cannot write" << m_downloadPath;
        emit updateFinished(false);
        return;
    }
    out.write(reply->readAll());
    out.close();

    qInfo() << "ITN: downloaded" << m_downloadPath;
    emit statusMessage(tr("Установка обновления %1…").arg(m_pendingTag));
    applyUpdate(m_downloadPath, m_pendingTag);
}

bool ITNAutoUpdater::shouldPreservePath(const QString& relativePath) const
{
    const QString p = relativePath.replace('\\', '/').toLower();
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
    return false;
}

void ITNAutoUpdater::applyUpdate(const QString& zipPath, const QString& tag)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString staging = FS::PathCombine(appDir, "updates", "staging");
    FS::deletePath(staging);
    FS::ensureFolderPathExists(staging);

    auto task = makeShared<MMCZip::ExtractZipTask>(zipPath, QDir(staging), QString());
    // Run synchronously via local event loop pattern used elsewhere — start and wait with processEvents
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
        return;
    }

    // Zip root is often ITNLauncher-windows/ — peel one directory if needed
    QDir stageDir(staging);
    QString contentRoot = staging;
    const auto entries = stageDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (entries.size() == 1 && !QFile::exists(FS::PathCombine(staging, "ITNLauncher.exe"))) {
        contentRoot = stageDir.absoluteFilePath(entries.first());
    }

    // Copy files into appDir, preserving user data
    QDirIterator it(contentRoot, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        const QString rel = QDir(contentRoot).relativeFilePath(fi.absoluteFilePath());
        if (fi.isDir())
            continue;
        if (shouldPreservePath(rel) && QFile::exists(FS::PathCombine(appDir, rel)))
            continue;
        // Never overwrite the running updater staging/download mid-flight oddly
        if (rel.startsWith(QStringLiteral("updates/"), Qt::CaseInsensitive))
            continue;

        const QString dest = FS::PathCombine(appDir, rel);
        // Skipping the currently running exe — leave for restart script
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

void ITNAutoUpdater::scheduleRestartApply(const QString& /*stagingDir*/, const QString& tag)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString bat = FS::PathCombine(appDir, "itn-apply-update.bat");
    const QString exe = FS::PathCombine(appDir, "ITNLauncher.exe");
    const QString exeNew = exe + QStringLiteral(".new");

    QFile f(bat);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "ITN: cannot write apply script";
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

    QMessageBox::information(nullptr, tr("Обновление ITN"),
                             tr("Обновление %1 скачано и установлено.\nЛаунчер перезапустится.").arg(tag));

    QProcess::startDetached(QStringLiteral("cmd.exe"), { QStringLiteral("/c"), bat }, appDir);
    QCoreApplication::quit();
}
