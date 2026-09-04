// SPDX-License-Identifier: GPL-3.0-only
#include "ui/ITNHoverFilter.h"

#include "ui/ITNUiAudio.h"

#include <QAbstractButton>
#include <QDateTime>
#include <QEvent>

bool ITNHoverFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (auto* btn = qobject_cast<QAbstractButton*>(watched)) {
        if (!btn->isEnabled()) {
            return QObject::eventFilter(watched, event);
        }
        if (event->type() == QEvent::Enter) {
            // Debounce: avoid spam when cursor skims many tools
            static qint64 lastHoverMs = 0;
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (now - lastHoverMs > 70) {
                lastHoverMs = now;
                ITNUiAudio::instance()->playHover();
            }
        } else if (event->type() == QEvent::MouseButtonPress) {
            ITNUiAudio::instance()->playClick();
        }
    }
    return QObject::eventFilter(watched, event);
}
