// SPDX-License-Identifier: GPL-3.0-only
#include "ui/ITNUiAudio.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

ITNUiAudio::ITNUiAudio(QObject* parent) : QObject(parent)
{
    m_cacheDir = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(QStringLiteral("itn-ui-audio"));
    QDir().mkpath(m_cacheDir);
}

ITNUiAudio* ITNUiAudio::instance()
{
    static ITNUiAudio* s = nullptr;
    if (!s) {
        s = new ITNUiAudio(qApp);
    }
    return s;
}

void ITNUiAudio::setVolume(qreal volume)
{
    m_volume = qBound(0.0, volume, 1.0);
}

void ITNUiAudio::playHover()
{
    playResource(QStringLiteral(":/itn/ui-hover"));
}

void ITNUiAudio::playClick()
{
    playResource(QStringLiteral(":/itn/ui-click"));
}

void ITNUiAudio::playSuccess()
{
    playResource(QStringLiteral(":/itn/ui-success"));
}

QString ITNUiAudio::cacheWav(const QString& resourcePath)
{
    const QString name = resourcePath.section('/', -1) + QStringLiteral(".wav");
    const QString dest = QDir(m_cacheDir).filePath(name);
    if (!QFile::exists(dest)) {
        QFile src(resourcePath);
        if (!src.open(QIODevice::ReadOnly)) {
            return {};
        }
        QFile out(dest);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return {};
        }
        out.write(src.readAll());
    }
    return dest;
}

void ITNUiAudio::playResource(const QString& resourcePath)
{
    if (!m_enabled || m_volume <= 0.01) {
        return;
    }
#ifdef Q_OS_WIN
    const QString path = cacheWav(resourcePath);
    if (path.isEmpty()) {
        return;
    }
    // SND_ASYNC | SND_FILENAME | SND_NODEFAULT — non-blocking soft UI blip
    PlaySoundW(reinterpret_cast<LPCWSTR>(path.utf16()), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
#else
    Q_UNUSED(resourcePath);
#endif
}
