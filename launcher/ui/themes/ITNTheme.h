// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - Minecraft Launcher
 *
 *  ITN theme — PineCraft-inspired pine→gold gradients.
 */

#pragma once

#include "FusionTheme.h"

class ITNTheme : public FusionTheme {
   public:
    virtual ~ITNTheme() {}

    QString id() override;
    QString name() override;
    QString tooltip() override;
    bool hasStyleSheet() override;
    QString appStyleSheet() override;
    QPalette colorScheme() override;
    double fadeAmount() override;
    QColor fadeColor() override;
};
