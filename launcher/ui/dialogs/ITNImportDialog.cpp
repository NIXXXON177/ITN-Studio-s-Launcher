// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ITN Launcher - Minecraft Launcher
 *
 *  Import settings, servers, packs, mods and skins from other launchers.
 */

#include "ui/dialogs/ITNImportDialog.h"

#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QPushButton>

#include <io/stream_reader.h>
#include <tag_compound.h>
#include <tag_list.h>
#include <tag_primitive.h>
#include <tag_string.h>
#include <memory>
#include <sstream>
#include <QSet>

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"

static QString readInstanceName(const QString& dir)
{
    QFile cfg(FS::PathCombine(dir, "instance.cfg"));
    if (cfg.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!cfg.atEnd()) {
            QString line = QString::fromUtf8(cfg.readLine()).trimmed();
            if (line.startsWith("name=")) {
                return line.mid(5);
            }
        }
    }
    return QDir(dir).dirName();
}

ITNImportDialog::ITNImportDialog(MinecraftInstance* inst, QWidget* parent) : QDialog(parent), m_inst(inst)
{
    setWindowTitle(tr("Импорт из другого лаунчера"));
    setMinimumSize(640, 480);

    auto* main = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    main->addLayout(form);

    m_launcherBox = new QComboBox(this);
    form->addRow(tr("Лаунчер:"), m_launcherBox);
    connect(m_launcherBox, &QComboBox::currentIndexChanged, this, &ITNImportDialog::refreshInstances);

    main->addWidget(new QLabel(tr("Сборка:"), this));
    m_instanceList = new QListWidget(this);
    m_instanceList->setSelectionMode(QAbstractItemView::SingleSelection);
    main->addWidget(m_instanceList, 1);

    auto* opts = new QHBoxLayout();
    m_optOptions = new QCheckBox(tr("Настройки игры"), this);
    m_optOptions->setChecked(true);
    m_optServers = new QCheckBox(tr("Серверы"), this);
    m_optServers->setChecked(true);
    m_optResPacks = new QCheckBox(tr("Ресурспаки"), this);
    m_optResPacks->setChecked(true);
    m_optShaders = new QCheckBox(tr("Шейдеры"), this);
    m_optShaders->setChecked(true);
    m_optMods = new QCheckBox(tr("Моды"), this);
    m_optMods->setChecked(true);
    m_optSkins = new QCheckBox(tr("Скины"), this);
    m_optSkins->setChecked(true);
    opts->addWidget(m_optOptions);
    opts->addWidget(m_optServers);
    opts->addWidget(m_optResPacks);
    opts->addWidget(m_optShaders);
    opts->addWidget(m_optMods);
    opts->addWidget(m_optSkins);
    main->addLayout(opts);

    auto* lists = new QHBoxLayout();
    m_modsList = new QListWidget(this);
    m_modsList->setSelectionMode(QAbstractItemView::NoSelection);
    m_skinsList = new QListWidget(this);
    m_skinsList->setSelectionMode(QAbstractItemView::NoSelection);
    lists->addWidget(m_modsList, 1);
    lists->addWidget(m_skinsList, 1);
    main->addLayout(lists, 1);
    connect(m_instanceList, &QListWidget::currentRowChanged, this, [this](int row) {
        m_modsList->clear();
        m_skinsList->clear();
        if (row < 0 || row >= m_sources.size()) {
            return;
        }
        const auto& src = m_sources[row];
        for (const QString& m : src.mods) {
            auto* item = new QListWidgetItem(QFileInfo(m).fileName(), m_modsList);
            item->setCheckState(Qt::Checked);
        }
        for (const QString& s : src.skins) {
            auto* item = new QListWidgetItem(QFileInfo(s).fileName(), m_skinsList);
            item->setCheckState(Qt::Checked);
        }
    });

    auto* target = new QLabel(tr("В сборку: %1").arg(m_inst->name()), this);
    main->addWidget(target);

    auto* btns = new QHBoxLayout();
    btns->addStretch(1);
    m_importBtn = new QPushButton(tr("Импортировать"), this);
    connect(m_importBtn, &QPushButton::clicked, this, &ITNImportDialog::runImport);
    auto* closeBtn = new QPushButton(tr("Закрыть"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btns->addWidget(m_importBtn);
    btns->addWidget(closeBtn);
    main->addLayout(btns);

    refreshLaunchers();
}

QList<ITNImportDialog::LauncherRoot> ITNImportDialog::detectLaunchers()
{
    QList<LauncherRoot> out;
    QString roaming = qgetenv("APPDATA");
    if (roaming.isEmpty()) {
        roaming = QDir::homePath() + "/AppData/Roaming";
    }
    const QStringList candidates = { "ElyPrismLauncher", "PrismLauncher", "PineconeMC", "ITNLauncher" };
    QString selfRoot = QDir(APPLICATION->dataRoot()).canonicalPath();
    for (const QString& c : candidates) {
        QString p = FS::PathCombine(roaming, c);
        if (!QDir(p).exists()) {
            continue;
        }
        if (QDir(p).canonicalPath() == selfRoot) {
            continue;  // do not import from ourselves
        }
        out.append({ c, p });
    }
    QString mc = FS::PathCombine(roaming, ".minecraft");
    if (QDir(mc).exists() && QDir(mc).canonicalPath() != selfRoot) {
        out.append({ tr("Minecraft (.minecraft)"), mc });
    }
    return out;
}

void ITNImportDialog::refreshLaunchers()
{
    m_launcherBox->clear();
    auto roots = detectLaunchers();
    for (const auto& r : roots) {
        m_launcherBox->addItem(r.name, r.path);
    }
    m_launcherBox->addItem(tr("Выбрать папку..."), QString("__custom__"));
    refreshInstances();
}

ITNSourceInstance ITNImportDialog::scanInstanceDir(const QString& dir, const QString& fallbackName)
{
    ITNSourceInstance s;
    s.mcDir = dir;
    QString mc = dir;
    if (QDir(FS::PathCombine(dir, ".minecraft")).exists()) {
        mc = FS::PathCombine(dir, ".minecraft");
        s.name = readInstanceName(dir);
    } else {
        s.name = fallbackName.isEmpty() ? QDir(dir).dirName() : fallbackName;
    }
    s.mcDir = mc;
    s.hasOptions = QFile::exists(FS::PathCombine(mc, "options.txt"));
    s.hasServers = QFile::exists(FS::PathCombine(mc, "servers.dat"));
    s.hasResPacks = QDir(FS::PathCombine(mc, "resourcepacks")).exists();
    s.hasShaders = QDir(FS::PathCombine(mc, "shaderpacks")).exists();
    QDir modsDir(FS::PathCombine(mc, "mods"));
    if (modsDir.exists()) {
        const auto jars = modsDir.entryList({ "*.jar" }, QDir::Files);
        for (const QString& j : jars) {
            s.mods.append(modsDir.filePath(j));
        }
    }
    return s;
}

QList<ITNSourceInstance> ITNImportDialog::scanRoot(const QString& root)
{
    QList<ITNSourceInstance> out;
    QDir r(root);
    // single instance directly?
    if (QFile::exists(FS::PathCombine(root, ".minecraft", "options.txt")) ||
        QFile::exists(FS::PathCombine(root, "options.txt")) || QFile::exists(FS::PathCombine(root, "instance.cfg"))) {
        QString mc = root;
        if (QDir(FS::PathCombine(root, ".minecraft")).exists()) {
            out.append(scanInstanceDir(root, QString()));
        } else {
            ITNSourceInstance s;
            s.mcDir = root;
            s.name = QDir(root).dirName();
            s.hasOptions = QFile::exists(FS::PathCombine(root, "options.txt"));
            s.hasServers = QFile::exists(FS::PathCombine(root, "servers.dat"));
            out.append(s);
        }
        return out;
    }
    // Prism-family instances dir
    QDir inst(FS::PathCombine(root, "instances"));
    if (inst.exists()) {
        const auto dirs = inst.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& d : dirs) {
            QString full = inst.filePath(d);
            if (QDir(FS::PathCombine(full, ".minecraft")).exists() || QFile::exists(FS::PathCombine(full, "instance.cfg"))) {
                out.append(scanInstanceDir(full, d));
            }
        }
    }
    return out;
}

void ITNImportDialog::refreshInstances()
{
    m_sources.clear();
    m_instanceList->clear();
    m_modsList->clear();
    m_skinsList->clear();
    QString path = m_launcherBox->currentData().toString();
    if (path == "__custom__") {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Папка лаунчера или сборки"));
        if (dir.isEmpty()) {
            return;
        }
        path = dir;
    }
    if (path.isEmpty()) {
        return;
    }
    m_sources = scanRoot(path);
    // collect loose skins next to the root
    QStringList skinDirs = { FS::PathCombine(path, "skins"), FS::PathCombine(path, ".minecraft", "skins") };
    QStringList skins;
    for (const QString& sd : skinDirs) {
        QDir d(sd);
        if (d.exists()) {
            const auto pngs = d.entryList({ "*.png" }, QDir::Files);
            for (const QString& p : pngs) {
                skins.append(d.filePath(p));
            }
        }
    }
    for (int i = 0; i < m_sources.size(); ++i) {
        const auto& s = m_sources[i];
        QStringList flags;
        if (s.hasOptions)
            flags << tr("настройки");
        if (s.hasServers)
            flags << tr("серверы");
        if (!s.mods.isEmpty())
            flags << tr("моды: %1").arg(s.mods.size());
        m_instanceList->addItem(QString("%1 (%2)").arg(s.name, flags.join(", ")));
    }
    if (!m_sources.isEmpty()) {
        m_instanceList->setCurrentRow(0);
    }
    // stash skins on the first source row for the picker
    if (!skins.isEmpty() && !m_sources.isEmpty()) {
        m_sources[0].skins = skins;
        if (m_instanceList->currentRow() == 0) {
            for (const QString& s : skins) {
                auto* item = new QListWidgetItem(QFileInfo(s).fileName(), m_skinsList);
                item->setCheckState(Qt::Checked);
            }
        }
    }
    m_importBtn->setEnabled(!m_sources.isEmpty());
}

static bool copyMissing(const QString& src, const QString& dst)
{
    if (!QFile::exists(src)) {
        return false;
    }
    FS::ensureFilePathExists(dst);
    QFile::remove(dst);
    return QFile::copy(src, dst);
}

static int copyDirFiles(const QString& srcDir, const QString& dstDir, const QStringList& filters)
{
    QDir s(srcDir);
    if (!s.exists()) {
        return 0;
    }
    QDir().mkpath(dstDir);
    int n = 0;
    const auto files = s.entryList(filters, QDir::Files);
    for (const QString& f : files) {
        if (QFile::exists(FS::PathCombine(dstDir, f))) {
            QFile::remove(FS::PathCombine(dstDir, f));
        }
        if (QFile::copy(s.filePath(f), FS::PathCombine(dstDir, f))) {
            n++;
        }
    }
    return n;
}

static std::unique_ptr<nbt::tag_compound> readServersDat(const QString& filename)
{
    try {
        QByteArray input = FS::read(filename);
        std::istringstream in(std::string(input.constData(), input.size()));
        auto pair = nbt::io::read_compound(in);
        if (pair.first != "" || pair.second == nullptr) {
            return nullptr;
        }
        return std::move(pair.second);
    } catch (...) {
        return nullptr;
    }
}

static bool writeServersDat(const QString& filename, nbt::tag_compound* root)
{
    try {
        if (!FS::ensureFilePathExists(filename)) {
            return false;
        }
        std::ostringstream s;
        nbt::io::write_tag("", *root, s);
        QByteArray val(s.str().data(), (int)s.str().size());
        FS::write(filename, val);
        return true;
    } catch (...) {
        return false;
    }
}

bool ITNImportDialog::mergeServersDat(const QString& dstPath, const QString& srcPath)
{
    auto src = readServersDat(srcPath);
    if (!src || !src->has_key("servers")) {
        return false;
    }
    auto dst = readServersDat(dstPath);
    if (!dst) {
        return copyMissing(srcPath, dstPath);
    }
    if (!dst->has_key("servers")) {
        return false;
    }
    auto& dstList = dst->at("servers").as<nbt::tag_list>();
    QSet<QString> ips;
    for (auto& e : dstList) {
        auto& c = e.as<nbt::tag_compound>();
        if (!c.has_key("ip")) {
            continue;
        }
        std::string ip(c["ip"]);
        ips.insert(QString::fromUtf8(ip.c_str()));
    }
    int added = 0;
    auto& srcList = src->at("servers").as<nbt::tag_list>();
    for (auto& e : srcList) {
        auto& c = e.as<nbt::tag_compound>();
        if (!c.has_key("ip") || !c.has_key("name")) {
            continue;
        }
        std::string ip(c["ip"]);
        QString qip = QString::fromUtf8(ip.c_str());
        if (ips.contains(qip)) {
            continue;
        }
        std::string name(c["name"]);
        nbt::tag_compound nc;
        nc.insert("name", name);
        nc.insert("ip", ip);
        if (c.has_key("icon")) {
            try {
                std::string icon(c["icon"]);
                nc.insert("icon", icon);
            } catch (...) {
            }
        }
        dstList.push_back(std::move(nc));
        ips.insert(qip);
        added++;
    }
    if (added == 0) {
        return true;
    }
    return writeServersDat(dstPath, dst.get());
}

bool ITNImportDialog::doImport(const ITNSourceInstance& src, QStringList& log)
{
    QString targetMc = m_inst->gameRoot();
    QDir().mkpath(targetMc);
    int n = 0;
    if (m_optOptions->isChecked() && src.hasOptions) {
        QString dst = FS::PathCombine(targetMc, "options.txt");
        if (QFile::exists(dst)) {
            QFile::remove(dst + ".itn-bak");
            QFile::rename(dst, dst + ".itn-bak");
        }
        if (copyMissing(FS::PathCombine(src.mcDir, "options.txt"), dst)) {
            log << tr("Настройки игры: готово");
            n++;
        }
    }
    if (m_optServers->isChecked() && src.hasServers) {
        QString dst = FS::PathCombine(targetMc, "servers.dat");
        if (!QFile::exists(dst) || mergeServersDat(dst, FS::PathCombine(src.mcDir, "servers.dat"))) {
            log << tr("Серверы: готово");
            n++;
        }
    }
    if (m_optResPacks->isChecked() && src.hasResPacks) {
        int c = copyDirFiles(FS::PathCombine(src.mcDir, "resourcepacks"), FS::PathCombine(targetMc, "resourcepacks"), { "*" });
        c += copyDirFiles(FS::PathCombine(src.mcDir, "texturepacks"), FS::PathCombine(targetMc, "texturepacks"), { "*" });
        log << tr("Ресурспаки: %1").arg(c);
        n++;
    }
    if (m_optShaders->isChecked() && src.hasShaders) {
        int c = copyDirFiles(FS::PathCombine(src.mcDir, "shaderpacks"), FS::PathCombine(targetMc, "shaderpacks"), { "*" });
        log << tr("Шейдеры: %1").arg(c);
        n++;
    }
    if (m_optMods->isChecked()) {
        QStringList picked;
        for (int i = 0; i < m_modsList->count(); ++i) {
            auto* item = m_modsList->item(i);
            if (item->checkState() == Qt::Checked) {
                int row = -1;
                for (int k = 0; k < src.mods.size(); ++k) {
                    if (QFileInfo(src.mods[k]).fileName() == item->text()) {
                        row = k;
                        break;
                    }
                }
                if (row >= 0) {
                    picked.append(src.mods[row]);
                }
            }
        }
        int c = 0;
        QDir().mkpath(FS::PathCombine(targetMc, "mods"));
        for (const QString& m : picked) {
            QString dst = FS::PathCombine(targetMc, "mods", QFileInfo(m).fileName());
            QFile::remove(dst);
            if (QFile::copy(m, dst)) {
                c++;
            }
        }
        log << tr("Моды: %1").arg(c);
        n++;
    }
    if (m_optSkins->isChecked()) {
        QString skinsDir = FS::PathCombine(APPLICATION->dataRoot(), APPLICATION->settings()->get("SkinsDir").toString());
        QDir().mkpath(skinsDir);
        int c = 0;
        for (int i = 0; i < m_skinsList->count(); ++i) {
            auto* item = m_skinsList->item(i);
            if (item->checkState() != Qt::Checked) {
                continue;
            }
            QString srcFile;
            for (const auto& s : src.skins) {
                if (QFileInfo(s).fileName() == item->text()) {
                    srcFile = s;
                    break;
                }
            }
            if (srcFile.isEmpty()) {
                continue;
            }
            QString dst = FS::PathCombine(skinsDir, QFileInfo(srcFile).fileName());
            QFile::remove(dst);
            if (QFile::copy(srcFile, dst)) {
                c++;
            }
        }
        log << tr("Скины: %1 (выбрать можно в аккаунтах)").arg(c);
        n++;
    }
    return n > 0;
}

void ITNImportDialog::runImport()
{
    int row = m_instanceList->currentRow();
    if (row < 0 || row >= m_sources.size()) {
        return;
    }
    QStringList log;
    if (doImport(m_sources[row], log)) {
        QMessageBox::information(this, tr("Импорт"), log.join("\n"));
        emit imported();
    } else {
        QMessageBox::warning(this, tr("Импорт"), tr("Нечего импортировать."));
    }
}
