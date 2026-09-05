// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QObject>
#include <QString>

/** Soft UI sounds (hover / click) — feedback without Qt Multimedia. */
class ITNUiAudio : public QObject {
    Q_OBJECT
   public:
    explicit ITNUiAudio(QObject* parent = nullptr);

    static ITNUiAudio* instance();

    void playHover();
    void playClick();
    void playSuccess();

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    void setVolume(qreal volume);  // 0..1

   private:
    void playResource(const QString& resourcePath);
    QString cacheWav(const QString& resourcePath);

    bool m_enabled = true;
    qreal m_volume = 0.55;
    QString m_cacheDir;
};
