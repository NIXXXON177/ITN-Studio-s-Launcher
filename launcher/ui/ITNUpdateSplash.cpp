// SPDX-License-Identifier: GPL-3.0-only
#include "ui/ITNUpdateSplash.h"

#include <QFont>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QtMath>

ITNUpdateSplash::ITNUpdateSplash(QWidget* parent) : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setModal(true);
    // Compact Majestic / PineCraft updater window
    setFixedSize(464, 358);
    setAttribute(Qt::WA_TranslucentBackground, false);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(8);

    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);
    m_logo = new QLabel(this);
    QPixmap logo(QStringLiteral(":/itn/logo"));
    if (!logo.isNull()) {
        m_logo->setPixmap(logo.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    titleRow->addWidget(m_logo);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(0);
    m_brand = new QLabel(QStringLiteral("ITN"), this);
    {
        QFont f = m_brand->font();
        f.setPointSize(14);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        m_brand->setFont(f);
    }
    m_brand->setStyleSheet(QStringLiteral("color: #e8f2ea;"));
    titleCol->addWidget(m_brand);

    auto* channel = new QLabel(QStringLiteral("STUDIO · STABLE"), this);
    {
        QFont f = channel->font();
        f.setPointSize(8);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        channel->setFont(f);
    }
    channel->setStyleSheet(QStringLiteral("color: #5c7263;"));
    titleCol->addWidget(channel);
    titleRow->addLayout(titleCol, 1);
    root->addLayout(titleRow);

    root->addSpacing(6);

    m_title = new QLabel(tr("Проверка обновлений…"), this);
    {
        QFont f = m_title->font();
        f.setPointSize(12);
        f.setBold(true);
        m_title->setFont(f);
    }
    m_title->setStyleSheet(QStringLiteral("color: #e8f2ea;"));
    root->addWidget(m_title);

    m_detail = new QLabel(this);
    m_detail->setWordWrap(true);
    m_detail->setStyleSheet(QStringLiteral("color: #93a89a;"));
    root->addWidget(m_detail);

    root->addStretch(1);

    m_bar = new QProgressBar(this);
    m_bar->setRange(0, 0);
    m_bar->setTextVisible(false);
    m_bar->setFixedHeight(12);
    m_bar->setStyleSheet(QStringLiteral(
        "QProgressBar { background: rgba(0,0,0,115); border: 1px solid #1e2f25; border-radius: 2px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        " stop:0 #8a6a14, stop:0.55 #f0b429, stop:1 #35c26e); border-radius: 1px; }"));
    root->addWidget(m_bar);

    m_fade = new QGraphicsOpacityEffect(this);
    m_fade->setOpacity(0.0);
    setGraphicsEffect(m_fade);

    m_pulseAnim = new QPropertyAnimation(this, "brandPulse", this);
    m_pulseAnim->setDuration(1600);
    m_pulseAnim->setStartValue(0.96);
    m_pulseAnim->setEndValue(1.04);
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
        f.setPointSizeF(14.0 * m_brandPulse);
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
    fadeOut->setDuration(420);
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
    fadeIn->setDuration(480);
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

    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0.0, QColor(19, 33, 26));
    bg.setColorAt(1.0, QColor(13, 23, 18));
    p.fillRect(rect(), bg);

    {
        QRadialGradient g(width() * 0.85, -height() * 0.1, width() * 0.7);
        g.setColorAt(0.0, QColor(53, 194, 110, 28));
        g.setColorAt(1.0, QColor(53, 194, 110, 0));
        p.fillRect(rect(), g);
    }
    {
        QRadialGradient g(-width() * 0.05, height() * 1.05, width() * 0.65);
        g.setColorAt(0.0, QColor(240, 180, 41, 22));
        g.setColorAt(1.0, QColor(240, 180, 41, 0));
        p.fillRect(rect(), g);
    }

    QPixmap banner(QStringLiteral(":/itn/banner"));
    if (!banner.isNull()) {
        p.setOpacity(0.12);
        p.drawPixmap(rect(), banner.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        p.setOpacity(1.0);
    }

    const qreal x = (0.5 + 0.5 * qSin(m_shimmerPhase)) * width();
    QLinearGradient sweep(x - 100, 0, x + 100, height());
    sweep.setColorAt(0.0, QColor(240, 180, 41, 0));
    sweep.setColorAt(0.45, QColor(240, 180, 41, 28));
    sweep.setColorAt(0.7, QColor(53, 194, 110, 22));
    sweep.setColorAt(1.0, QColor(53, 194, 110, 0));
    p.fillRect(rect(), sweep);

    QPen pen(QColor(44, 68, 52, 200));
    pen.setWidth(1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);

    QLinearGradient edge(0, 0, width(), 0);
    edge.setColorAt(0.0, QColor(240, 180, 41, 180));
    edge.setColorAt(0.55, QColor(255, 214, 107, 120));
    edge.setColorAt(1.0, QColor(53, 194, 110, 180));
    p.fillRect(QRect(1, 1, width() - 2, 2), edge);
}
