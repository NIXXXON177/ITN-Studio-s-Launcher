/// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLaucher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ProgressDialog.h"
#include "ui_ProgressDialog.h"

#include <QDebug>
#include <QFont>
#include <QKeyEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QUrl>
#include <limits>

#include "tasks/Task.h"

#include "ui/widgets/SubTaskProgressBar.h"

ProgressDialog::ProgressDialog(QWidget* parent) : QDialog(parent), ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);

    // ITN: compact frameless progress card
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setModal(true);
    setFixedSize(300, 340);
    setAttribute(Qt::WA_QuitOnClose, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setSizeGripEnabled(false);

    ui->taskProgressScrollArea->setVisible(false);
    ui->taskProgressScrollArea->setMaximumHeight(0);

    QPixmap logo(QStringLiteral(":/itn/logo"));
    if (!logo.isNull()) {
        ui->logoLabel->setPixmap(logo.scaled(120, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    {
        QFont f = ui->brandLabel->font();
        f.setPointSize(16);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
        ui->brandLabel->setFont(f);
        ui->brandLabel->setStyleSheet(QStringLiteral("color: #e8f2ea;"));
    }
    {
        QFont f = ui->globalStatusLabel->font();
        f.setPointSize(11);
        f.setBold(true);
        ui->globalStatusLabel->setFont(f);
        ui->globalStatusLabel->setStyleSheet(QStringLiteral("color: #c9d8cc;"));
    }
    const QString mute = QStringLiteral("color: #7a9080; font-size: 11px;");
    ui->remainingCaptionLabel->setStyleSheet(mute);
    ui->globalStatusDetailsLabel->setStyleSheet(mute);
    ui->sizeLabel->setStyleSheet(mute);
    ui->speedLabel->setStyleSheet(mute);

    ui->globalProgressBar->setTextVisible(false);
    ui->globalProgressBar->setFixedHeight(6);
    ui->globalProgressBar->setStyleSheet(QStringLiteral(
        "QProgressBar {"
        "  background: #1a2420; border: none; border-radius: 3px;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 #f0b429, stop:1 #35c26e);"
        "  border-radius: 3px;"
        "}"));

    ui->skipButton->setCursor(Qt::PointingHandCursor);
    ui->skipButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #1a2420; color: #e8f2ea; border: 1px solid #2c4434;"
        "  border-radius: 8px; font-weight: 800; letter-spacing: 1px;"
        "}"
        "QPushButton:hover { background: #223028; border-color: #35c26e; }"
        "QPushButton:pressed { background: #13211a; }"
        "QPushButton:disabled { color: #5c7263; border-color: #1e2f25; }"));

    setStyleSheet(QStringLiteral(
        "ProgressDialog, QDialog#ProgressDialog { background: transparent; }"
        "QLabel { background: transparent; }"));

    changeProgress(0, 100);
    setSkipButton(true, tr("ОТМЕНА"));
    updateSize(true);
}

void ProgressDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0.0, QColor(17, 25, 22));
    bg.setColorAt(1.0, QColor(10, 17, 13));
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(rect(), 12, 12);

    {
        QRadialGradient g(width() * 0.5, 0, width() * 0.7);
        g.setColorAt(0.0, QColor(53, 194, 110, 28));
        g.setColorAt(1.0, QColor(53, 194, 110, 0));
        p.setBrush(g);
        p.drawRoundedRect(rect(), 12, 12);
    }

    QPen pen(QColor(44, 68, 52, 210));
    pen.setWidth(1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12);
}

void ProgressDialog::setSkipButton(bool present, QString label)
{
    ui->skipButton->setAutoDefault(false);
    ui->skipButton->setDefault(false);
    ui->skipButton->setFocusPolicy(Qt::ClickFocus);
    ui->skipButton->setEnabled(present);
    ui->skipButton->setVisible(true);  // always show compact cancel slot
    ui->skipButton->setText(label.isEmpty() ? tr("ОТМЕНА") : label.toUpper());
    updateSize();
}

void ProgressDialog::on_skipButton_clicked(bool checked)
{
    Q_UNUSED(checked);
    if (ui->skipButton->isEnabled() && m_task)
        m_task->abort();
}

ProgressDialog::~ProgressDialog()
{
    for (auto conn : this->m_taskConnections) {
        disconnect(conn);
    }
    delete ui;
}

void ProgressDialog::updateSize(bool recenterParent)
{
    QPoint lastPos = this->pos();
    QSize lastSize = this->size();

    // Keep the card compact and fixed — no growing with details text
    setFixedSize(300, 340);

    auto parent = this->parentWidget();
    if (recenterParent && parent) {
        auto newX = std::max(0, parent->x() + ((parent->width() - width()) / 2));
        auto newY = std::max(0, parent->y() + ((parent->height() - height()) / 2));
        this->move(newX, newY);
    } else if (lastSize != size()) {
        QSize sizeDiff = lastSize - size();
        auto newX = std::max(0, lastPos.x() + (sizeDiff.width() / 2));
        auto newY = std::max(0, lastPos.y() + (sizeDiff.height() / 2));
        this->move(newX, newY);
    }
}

int ProgressDialog::execWithTask(Task* task)
{
    this->m_task = task;

    if (!task) {
        qDebug() << "Programmer error: Progress dialog created with null task.";
        return QDialog::DialogCode::Accepted;
    }

    QDialog::DialogCode result{};
    if (handleImmediateResult(result)) {
        return result;
    }

    this->m_taskConnections.push_back(connect(task, &Task::started, this, &ProgressDialog::onTaskStarted));
    this->m_taskConnections.push_back(connect(task, &Task::failed, this, &ProgressDialog::onTaskFailed));
    this->m_taskConnections.push_back(connect(task, &Task::succeeded, this, &ProgressDialog::onTaskSucceeded));
    this->m_taskConnections.push_back(connect(task, &Task::status, this, &ProgressDialog::changeStatus));
    this->m_taskConnections.push_back(connect(task, &Task::details, this, &ProgressDialog::changeStatus));
    this->m_taskConnections.push_back(connect(task, &Task::stepProgress, this, &ProgressDialog::changeStepProgress));
    this->m_taskConnections.push_back(connect(task, &Task::progress, this, &ProgressDialog::changeProgress));
    this->m_taskConnections.push_back(connect(task, &Task::aborted, this, &ProgressDialog::hide));
    this->m_taskConnections.push_back(connect(task, &Task::abortStatusChanged, ui->skipButton, &QPushButton::setEnabled));
    this->m_taskConnections.push_back(connect(task, &Task::abortButtonTextChanged, this, [this](const QString& text) {
        ui->skipButton->setText(text.isEmpty() ? tr("ОТМЕНА") : text.toUpper());
    }));

    m_is_multi_step = task->isMultiStep();
    ui->taskProgressScrollArea->setVisible(false);
    updateSize(true);

    if (!task->isRunning()) {
        QMetaObject::invokeMethod(task, &Task::start, Qt::QueuedConnection);
    } else {
        changeStatus(task->getStatus());
        changeProgress(task->getProgress(), task->getTotalProgress());
    }

    return QDialog::exec();
}

int ProgressDialog::execWithTask(std::unique_ptr<Task>&& task)
{
    connect(this, &ProgressDialog::destroyed, task.get(), &Task::deleteLater);
    return execWithTask(task.release());
}
int ProgressDialog::execWithTask(std::unique_ptr<Task>& task)
{
    connect(this, &ProgressDialog::destroyed, task.get(), &Task::deleteLater);
    return execWithTask(task.release());
}

bool ProgressDialog::handleImmediateResult(QDialog::DialogCode& result)
{
    if (m_task->isFinished()) {
        if (m_task->wasSuccessful()) {
            result = QDialog::Accepted;
        } else {
            result = QDialog::Rejected;
        }
        return true;
    }
    return false;
}

Task* ProgressDialog::getTask()
{
    return m_task;
}

void ProgressDialog::onTaskStarted() {}

void ProgressDialog::onTaskFailed([[maybe_unused]] QString failure)
{
    reject();
    hide();
}

void ProgressDialog::onTaskSucceeded()
{
    accept();
    hide();
}

void ProgressDialog::changeStatus([[maybe_unused]] const QString& status)
{
    QString title = m_task->getStatus();
    if (title.length() > 48) {
        title = title.left(45) + QStringLiteral("…");
    }
    ui->globalStatusLabel->setText(title);

    // Prefer structured details from last step summary; fall back to task details
    QString details = m_task->getDetails().trimmed();
    if (!details.isEmpty() && !details.contains(QLatin1String("://"))) {
        if (details.length() > 36) {
            details = details.left(33) + QStringLiteral("…");
        }
        ui->globalStatusDetailsLabel->setText(details);
    }
}

void ProgressDialog::addTaskProgress(TaskStepProgress const& progress)
{
    Q_UNUSED(progress);
    // ITN: compact card — no per-file sub-bars
}

void ProgressDialog::changeStepProgress(TaskStepProgress const& task_progress)
{
    m_is_multi_step = true;
    updateITNSummary(task_progress);
}

void ProgressDialog::updateITNSummary(TaskStepProgress const& task_progress)
{
    QString name = task_progress.status;
    if (name.contains(QLatin1String("://"))) {
        QUrl url(name.section(QLatin1Char('\n'), 0, 0).trimmed());
        if (!url.fileName().isEmpty()) {
            name = url.fileName();
        }
    }
    if (name.length() > 28) {
        name = name.left(25) + QStringLiteral("…");
    }

    QString sizes;
    QString speed;
    const QStringList lines = task_progress.details.split(QLatin1Char('\n'));
    if (!lines.isEmpty()) {
        sizes = lines.value(0).trimmed();
    }
    if (lines.size() > 1) {
        speed = lines.value(1).trimmed();
    }

    ui->globalStatusDetailsLabel->setText(name.isEmpty() ? QStringLiteral("…") : name);
    ui->sizeLabel->setText(sizes.isEmpty() ? QStringLiteral("—") : sizes);
    ui->speedLabel->setText(speed);
}

void ProgressDialog::changeProgress(qint64 current, qint64 total)
{
    if (total <= 0) {
        ui->globalProgressBar->setRange(0, 0);  // indeterminate
        return;
    }
    if (ui->globalProgressBar->maximum() == 0) {
        ui->globalProgressBar->setRange(0, 1000);
    }
    ui->globalProgressBar->setMaximum(static_cast<int>(total));
    ui->globalProgressBar->setValue(static_cast<int>(current));

    // Show step counter under brand area when no file-size stats yet
    if (ui->sizeLabel->text().isEmpty() || ui->sizeLabel->text() == QStringLiteral("—") ||
        ui->sizeLabel->text() == QStringLiteral("0 / 0")) {
        ui->sizeLabel->setText(tr("%1 из %2").arg(current).arg(total));
    }
}

void ProgressDialog::keyPressEvent(QKeyEvent* e)
{
    if (ui->skipButton->isVisible()) {
        if (e->key() == Qt::Key_Escape) {
            on_skipButton_clicked(true);
            return;
        } else if (e->key() == Qt::Key_Tab) {
            ui->skipButton->setFocusPolicy(Qt::StrongFocus);
            ui->skipButton->setFocus();
            ui->skipButton->setAutoDefault(true);
            ui->skipButton->setDefault(true);
            return;
        }
    }
    QDialog::keyPressEvent(e);
}

void ProgressDialog::closeEvent(QCloseEvent* e)
{
    if (m_task && m_task->isRunning()) {
        e->ignore();
    } else {
        QDialog::closeEvent(e);
    }
}
