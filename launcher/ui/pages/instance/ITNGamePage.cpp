// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - Minecraft Launcher
 *
 *  In-launcher Minecraft game settings editor (options.txt).
 */

#include "ui/pages/instance/ITNGamePage.h"

#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QTextStream>
#include <QVBoxLayout>

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "ui/dialogs/ITNImportDialog.h"

ITNGamePage::ITNGamePage(MinecraftInstance* inst, QWidget* parent) : QWidget(parent), m_inst(inst)
{
    auto* mainLayout = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    mainLayout->addLayout(form);

    m_chunks = new QSpinBox(this);
    m_chunks->setRange(2, 32);
    m_chunks->setSuffix(tr(" чанков"));
    form->addRow(tr("Дальность прорисовки:"), m_chunks);

    m_graphics = new QComboBox(this);
    m_graphics->addItem(tr("Быстрая"), 0);
    m_graphics->addItem(tr("Красивая"), 1);
    m_graphics->addItem(tr("Роскошная!"), 2);
    form->addRow(tr("Графика:"), m_graphics);

    m_clouds = new QComboBox(this);
    m_clouds->addItem(tr("Выкл"), QString("false"));
    m_clouds->addItem(tr("Быстрые"), QString("fast"));
    m_clouds->addItem(tr("Красивые"), QString("true"));
    form->addRow(tr("Облака:"), m_clouds);

    m_particles = new QComboBox(this);
    m_particles->addItem(tr("Минимум"), 0);
    m_particles->addItem(tr("Уменьшенные"), 1);
    m_particles->addItem(tr("Все"), 2);
    form->addRow(tr("Частицы:"), m_particles);

    auto* fovRow = new QHBoxLayout();
    m_fov = new QSlider(Qt::Horizontal, this);
    m_fov->setRange(30, 110);
    m_fovLabel = new QLabel(this);
    m_fovLabel->setMinimumWidth(50);
    connect(m_fov, &QSlider::valueChanged, this, [this](int v) { m_fovLabel->setText(QString::number(v)); });
    fovRow->addWidget(m_fov, 1);
    fovRow->addWidget(m_fovLabel);
    form->addRow(tr("Угол обзора:"), fovRow);

    auto* gammaRow = new QHBoxLayout();
    m_gamma = new QSlider(Qt::Horizontal, this);
    m_gamma->setRange(0, 100);
    m_gammaLabel = new QLabel(this);
    m_gammaLabel->setMinimumWidth(50);
    connect(m_gamma, &QSlider::valueChanged, this, [this](int v) { m_gammaLabel->setText(QString("%1%").arg(v)); });
    gammaRow->addWidget(m_gamma, 1);
    gammaRow->addWidget(m_gammaLabel);
    form->addRow(tr("Яркость:"), gammaRow);

    m_vsync = new QCheckBox(tr("Вкл"), this);
    form->addRow(tr("Вертикальная синхронизация:"), m_vsync);

    m_fps = new QComboBox(this);
    m_fps->addItem(tr("30"), 30);
    m_fps->addItem(tr("60"), 60);
    m_fps->addItem(tr("120"), 120);
    m_fps->addItem(tr("144"), 144);
    m_fps->addItem(tr("Без лимита"), 260);
    form->addRow(tr("Частота кадров:"), m_fps);

    m_fullscreen = new QCheckBox(tr("Вкл"), this);
    form->addRow(tr("Полный экран:"), m_fullscreen);

    auto* musicRow = new QHBoxLayout();
    m_music = new QSlider(Qt::Horizontal, this);
    m_music->setRange(0, 100);
    m_musicLabel = new QLabel(this);
    m_musicLabel->setMinimumWidth(50);
    connect(m_music, &QSlider::valueChanged, this, [this](int v) { m_musicLabel->setText(QString("%1%").arg(v)); });
    musicRow->addWidget(m_music, 1);
    musicRow->addWidget(m_musicLabel);
    form->addRow(tr("Музыка:"), musicRow);

    m_langLabel = new QLabel(QStringLiteral("ru_ru"), this);
    form->addRow(tr("Язык:"), m_langLabel);

    auto* btnRow = new QHBoxLayout();
    auto* resetBtn = new QPushButton(tr("Сбросить к ITN"), this);
    connect(resetBtn, &QPushButton::clicked, this, &ITNGamePage::resetToITN);
    btnRow->addWidget(resetBtn);
    // ITN: migration import is opt-in — portable builds ship ready instances
    if (APPLICATION->settings()->get("ITNShowMigrationImport").toBool()) {
        auto* importBtn = new QPushButton(tr("Импорт из другого лаунчера..."), this);
        connect(importBtn, &QPushButton::clicked, this, &ITNGamePage::openImportDialog);
        btnRow->addWidget(importBtn);
    }
    btnRow->addStretch(1);
    mainLayout->addLayout(btnRow);
    mainLayout->addStretch(1);
}

QString ITNGamePage::optionsPath() const
{
    return FS::PathCombine(m_inst->gameRoot(), "options.txt");
}

void ITNGamePage::openedImpl()
{
    loadOptions();
}

void ITNGamePage::loadOptions()
{
    m_values.clear();
    m_order.clear();
    QFile f(optionsPath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine();
            int sep = line.indexOf(':');
            if (sep <= 0) {
                continue;
            }
            QString key = line.left(sep);
            QString val = line.mid(sep + 1);
            if (!m_values.contains(key)) {
                m_order.append(key);
            }
            m_values[key] = val;
        }
    }
    auto get = [this](const QString& key, const QString& def) { return m_values.value(key, def); };

    m_chunks->setValue(get("renderDistance", "12").toInt());
    int gfx = get("graphicsMode", "1").toInt();
    m_graphics->setCurrentIndex(m_graphics->findData(gfx));
    QString clouds = get("renderClouds", "true");
    int ci = m_clouds->findData(clouds);
    m_clouds->setCurrentIndex(ci < 0 ? 2 : ci);
    int part = get("particles", "2").toInt();
    m_particles->setCurrentIndex(m_particles->findData(part));
    double fov = get("fov", "0.0").toDouble();
    int fovDeg = qRound(30.0 + fov * 80.0);
    m_fov->setValue(qBound(30, fovDeg, 110));
    m_gamma->setValue(qRound(get("gamma", "0.5").toDouble() * 100.0));
    m_vsync->setChecked(get("enableVsync", "false") == "true");
    int fps = get("maxFps", "120").toInt();
    int fi = m_fps->findData(fps);
    if (fi < 0) {
        m_fps->setCurrentIndex(1);
    } else {
        m_fps->setCurrentIndex(fi);
    }
    m_fullscreen->setChecked(get("fullscreen", "false") == "true");
    m_music->setValue(qRound(get("soundCategory_music", "1.0").toDouble() * 100.0));
    m_langLabel->setText(get("lang", "ru_ru"));
    refreshImportState();
}

void ITNGamePage::refreshImportState()
{
    // nothing dynamic for now
}

bool ITNGamePage::apply()
{
    m_values["renderDistance"] = QString::number(m_chunks->value());
    m_values["graphicsMode"] = QString::number(m_graphics->currentData().toInt());
    m_values["renderClouds"] = QString("\"%1\"").arg(m_clouds->currentData().toString());
    m_values["particles"] = QString::number(m_particles->currentData().toInt());
    double fov = (m_fov->value() - 30) / 80.0;
    m_values["fov"] = QString::number(fov, 'f', 4);
    m_values["gamma"] = QString::number(m_gamma->value() / 100.0, 'f', 4);
    m_values["enableVsync"] = m_vsync->isChecked() ? "true" : "false";
    m_values["maxFps"] = QString::number(m_fps->currentData().toInt());
    m_values["fullscreen"] = m_fullscreen->isChecked() ? "true" : "false";
    m_values["soundCategory_music"] = QString::number(m_music->value() / 100.0, 'f', 4);
    m_values["lang"] = "ru_ru";
    for (const QString& key : { QString("renderDistance"), QString("graphicsMode"), QString("renderClouds"), QString("particles"),
                                QString("fov"), QString("gamma"), QString("enableVsync"), QString("maxFps"),
                                QString("fullscreen"), QString("soundCategory_music"), QString("lang") }) {
        if (!m_order.contains(key)) {
            m_order.append(key);
        }
    }
    saveOptions();
    return true;
}

void ITNGamePage::saveOptions()
{
    QFile f(optionsPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }
    QTextStream out(&f);
    for (const QString& key : m_order) {
        out << key << ":" << m_values.value(key) << "\n";
    }
}

void ITNGamePage::applyITNDefaults()
{
    m_chunks->setValue(12);
    m_graphics->setCurrentIndex(m_graphics->findData(1));
    m_clouds->setCurrentIndex(m_clouds->findData(QString("true")));
    m_particles->setCurrentIndex(m_particles->findData(2));
    m_fov->setValue(110);
    m_gamma->setValue(50);
    m_vsync->setChecked(false);
    m_fps->setCurrentIndex(m_fps->findData(260));
    m_fullscreen->setChecked(false);
    m_music->setValue(0);
}

void ITNGamePage::resetToITN()
{
    applyITNDefaults();
}

void ITNGamePage::openImportDialog()
{
    ITNImportDialog dlg(m_inst, this);
    connect(&dlg, &ITNImportDialog::imported, this, [this] { loadOptions(); });
    dlg.exec();
}
