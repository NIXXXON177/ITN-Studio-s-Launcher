// SPDX-License-Identifier: GPL-3.0-only
#include "ui/ITNUpdateSplash.h"

#include <QFont>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPixmap>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QtMath>

ITNUpdateSplash::ITNUpdateSplash(QWidget* parent) : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setModal(true);
    setFixedSize(860, 500);
    setAttribute(Qt::WA_TranslucentBackground, false);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(56, 48, 56, 44);
    root->setSpacing(10);

    m_logo = new QLabel(this);
    m_logo->setAlignment(Qt::AlignCenter);
    QPixmap logo(QStringLiteral(":/itn/logo"));
    if (!logo.isNull()) {
        m_logo->setPixmap(logo.scaled(112, 112, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    root->addWidget(m_logo, 0, Qt::AlignHCenter);

    m_brand = new QLabel(QStringLiteral("ITN"), this);
    {
        QFont f = m_brand->font();
        f.setPointSize(48);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 10);
        m_brand->setFont(f);
    }
    m_brand->setAlignment(Qt::AlignCenter);
    m_brand->setStyleSheet(QStringLiteral("color: #f0fdf4;"));
    root->addWidget(m_brand);

    auto* studio = new QLabel(QStringLiteral("STUDIO LAUNCHER"), this);
    {
        QFont f = studio->font();
        f.setPointSize(10);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 4);
        studio->setFont(f);
    }
    studio->setAlignment(Qt::AlignCenter);
    studio->setStyleSheet(QStringLiteral("color: #4ade80;"));
    root->addWidget(studio);

    root->addSpacing(8);

    m_title = new QLabel(tr("Проверка обновлений…"), this);
    {
        QFont f = m_title->font();
        f.setPointSize(15);
        f.setBold(true);
        m_title->setFont(f);
    }
    m_title->setAlignment(Qt::AlignCenter);
    m_title->setStyleSheet(QStringLiteral("color: #bbf7d0;"));
    root->addWidget(m_title);

    m_detail = new QLabel(this);
    m_detail->setAlignment(Qt::AlignCenter);
    m_detail->setWordWrap(true);
    m_detail->setStyleSheet(QStringLiteral("color: #86efac;"));
    root->addWidget(m_detail);

    root->addStretch(1);

    m_bar = new QProgressBar(this);
    m_bar->setRange(0, 0);  // indeterminate until first progress
    m_bar->setTextVisible(false);
    m_bar->setFixedHeight(12);
    m_bar->setStyleSheet(QStringLiteral(
        "QProgressBar { background: rgba(8,14,10,200); border: 1px solid #166534; border-radius: 6px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        " stop:0 #15803d, stop:0.45 #4ade80, stop:1 #22c55e); border-radius: 5px; }"));
    root->addWidget(m_bar);

    m_fade = new QGraphicsOpacityEffect(this);
    m_fade->setOpacity(0.0);
    setGraphicsEffect(m_fade);

    m_pulseAnim = new QPropertyAnimation(this, "brandPulse", this);
    m_pulseAnim->setDuration(1800);
    m_pulseAnim->setStartValue(0.94);
    m_pulseAnim->setEndValue(1.08);
    m_pulseAnim->setEasingCurve(QEasingCurve::InOutSine);
    m_pulseAnim->setLoopCount(-1);

    connect(&m_shimmer, &QTimer::timeout, this, [this] {
        m_shimmerPhase += 0.07;
        if (m_shimmerPhase > 6.28) {
            m_shimmerPhase = 0;
        }
        update();
    });
}

void ITNUpdateSplash::setBrandPulse(qreal v)
{
    m_brandPulse = v;
    if (m_brand) {
        QFont f = m_brand->font();
        f.setPointSizeF(48.0 * m_brandPulse);
        m_brand->setFont(f);
    }
}

void ITNUpdateSplash::setPhase(const QString& title, const QString& detail)
{
    m_title->setText(title);
    m_detail->setText(detail);
}

void ITNUpdateSplash::setProgress(int percent)
{
    if (percent < 0) {
        m_bar->setRange(0, 0);
        return;
    }
    if (m_bar->maximum() == 0) {
        m_bar->setRange(0, 100);
    }
    m_bar->setValue(qBound(0, percent, 100));
}

void ITNUpdateSplash::finishAndAccept()
{
    m_pulseAnim->stop();
    m_shimmer.stop();
    auto* fadeOut = new QPropertyAnimation(m_fade, "opacity", this);
    fadeOut->setDuration(480);
    fadeOut->setStartValue(m_fade->opacity());
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::OutCubic);
    connect(fadeOut, &QPropertyAnimation::finished, this, [this] { accept(); });
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}

void ITNUpdateSplash::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    startAnimations();
}

void ITNUpdateSplash::startAnimations()
{
    m_shownAt.start();
    auto* fadeIn = new QPropertyAnimation(m_fade, "opacity", this);
    fadeIn->setDuration(560);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    m_pulseAnim->start();
    m_shimmer.start(30);
}

void ITNUpdateSplash::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient bg(0, 0, width(), height());
    bg.setColorAt(0.0, QColor(6, 14, 10));
    bg.setColorAt(0.4, QColor(12, 26, 17));
    bg.setColorAt(1.0, QColor(5, 10, 8));
    p.fillRect(rect(), bg);

    // Banner soft wash
    QPixmap banner(QStringLiteral(":/itn/banner"));
    if (!banner.isNull()) {
        p.setOpacity(0.28);
        p.drawPixmap(rect(), banner.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        p.setOpacity(1.0);
    }

    // Soft radial glow behind brand
    {
        const QPointF c(width() * 0.5, height() * 0.28);
        QRadialGradient glow(c, width() * 0.42);
        glow.setColorAt(0.0, QColor(34, 197, 94, 55));
        glow.setColorAt(0.45, QColor(22, 163, 74, 18));
        glow.setColorAt(1.0, QColor(22, 163, 74, 0));
        p.fillRect(rect(), glow);
    }

    // Floating particles
    for (int i = 0; i < 18; ++i) {
        const qreal t = m_shimmerPhase + i * 0.37;
        const qreal x = (0.08 + 0.84 * ((qSin(t * 0.7 + i) + 1.0) * 0.5)) * width();
        const qreal y = (0.12 + 0.7 * ((qCos(t * 0.55 + i * 1.3) + 1.0) * 0.5)) * height();
        const qreal r = 1.2 + (i % 4) * 0.7;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(74, 222, 128, 35 + (i % 5) * 8));
        p.drawEllipse(QPointF(x, y), r, r);
    }

    // Animated green sweep line
    const qreal x = (0.5 + 0.5 * qSin(m_shimmerPhase)) * width();
    QLinearGradient sweep(x - 140, 0, x + 140, 0);
    sweep.setColorAt(0.0, QColor(34, 197, 94, 0));
    sweep.setColorAt(0.5, QColor(74, 222, 128, 48));
    sweep.setColorAt(1.0, QColor(34, 197, 94, 0));
    p.fillRect(rect(), sweep);

    // Soft vignette frame
    QPen pen(QColor(74, 222, 128, 100));
    pen.setWidth(2);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);
}
