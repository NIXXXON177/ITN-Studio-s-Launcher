// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QDialog>
#include <QElapsedTimer>
#include <QLabel>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>

/**
 * Majestic-style startup gate: full-bleed brand screen with status + progress
 * while launcher / game updates are checked and applied.
 */
class ITNUpdateSplash : public QDialog {
    Q_OBJECT
    Q_PROPERTY(qreal brandPulse READ brandPulse WRITE setBrandPulse)
   public:
    explicit ITNUpdateSplash(QWidget* parent = nullptr);

    void setPhase(const QString& title, const QString& detail = {});
    void setProgress(int percent);  // 0..100, -1 indeterminate
    void finishAndAccept();

    qreal brandPulse() const { return m_brandPulse; }
    void setBrandPulse(qreal v);

   protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

   private:
    void startAnimations();

    QLabel* m_logo = nullptr;
    QLabel* m_brand = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_detail = nullptr;
    QProgressBar* m_bar = nullptr;
    QGraphicsOpacityEffect* m_fade = nullptr;
    QPropertyAnimation* m_pulseAnim = nullptr;
    QTimer m_shimmer;
    qreal m_brandPulse = 1.0;
    qreal m_shimmerPhase = 0.0;
    QElapsedTimer m_shownAt;
};
