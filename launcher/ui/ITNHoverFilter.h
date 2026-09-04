// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QEvent>
#include <QObject>

/** Plays soft hover/click sounds on buttons and tool buttons (Majestic-style). */
class ITNHoverFilter : public QObject {
    Q_OBJECT
   public:
    explicit ITNHoverFilter(QObject* parent = nullptr) : QObject(parent) {}

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};
