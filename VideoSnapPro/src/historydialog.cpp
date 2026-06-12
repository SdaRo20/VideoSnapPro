#include "historydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QMenu>
#include <QLabel>

QString HistoryDialog::historyFilePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/history.json";
}

QList<HistoryEntry> HistoryDialog::loadHistory()
{
    QList<HistoryEntry> list;
    QFile f(historyFilePath());
    if (!f.open(QIODevice::ReadOnly)) return list;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return list;
    for (auto v : doc.array()) {
        QJsonObject o = v.toObject();
        HistoryEntry e;
        e.title      = o["title"].toString();
        e.url        = o["url"].toString();
        e.format     = o["format"].toString();
        e.outputFile = o["outputFile"].toString();
        e.platform   = o["platform"].toString();
        e.finishedAt = QDateTime::fromString(o["finishedAt"].toString(), Qt::ISODate);
        e.success    = o["success"].toBool();
        list.prepend(e); // newest first
    }
    return list;
}

void HistoryDialog::saveHistory(const QList<HistoryEntry> &list)
{
    QJsonArray arr;
    // Keep last 500
    int start = qMax(0, list.size() - 500);
    for (int i = start; i < list.size(); ++i) {
        const auto &e = list[i];
        QJsonObject o;
        o["title"]      = e.title;
        o["url"]        = e.url;
        o["format"]     = e.format;
        o["outputFile"] = e.outputFile;
        o["platform"]   = e.platform;
        o["finishedAt"] = e.finishedAt.toString(Qt::ISODate);
        o["success"]    = e.success;
        arr.append(o);
    }
    QFile f(historyFilePath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(arr).toJson());
}

void HistoryDialog::addEntry(const HistoryEntry &e)
{
    auto list = loadHistory();
    list.append(e);
    saveHistory(list);
}

HistoryDialog::HistoryDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Download History");
    setMinimumSize(780, 480);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(10);

    auto *hdr = new QHBoxLayout;
    auto *title = new QLabel("Download History");
    title->setStyleSheet("font-size: 15px; font-weight: bold;");
    hdr->addWidget(title);
    hdr->addStretch();

    auto *clearBtn = new QPushButton("Clear History");
    clearBtn->setObjectName("btnAbort");
    connect(clearBtn, &QPushButton::clicked, this, &HistoryDialog::onClearHistory);
    hdr->addWidget(clearBtn);
    root->addLayout(hdr);

    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels({"Title","Platform","Format","Date","Status","File"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_table, &QTableWidget::customContextMenuRequested, this,
        [this](const QPoint &pos) {
            QMenu menu;
            menu.addAction("Open file", this, &HistoryDialog::onOpenFile);
            menu.addAction("Copy URL",  this, &HistoryDialog::onCopyUrl);
            menu.exec(m_table->viewport()->mapToGlobal(pos));
        });

    root->addWidget(m_table);

    auto *hint = new QLabel("Right-click a row to open the file or copy the URL");
    hint->setStyleSheet("color: #666; font-size: 11px;");
    root->addWidget(hint);

    load();
}

void HistoryDialog::load()
{
    m_table->setRowCount(0);
    auto list = loadHistory();
    for (const auto &e : list) {
        int r = m_table->rowCount();
        m_table->insertRow(r);

        auto *t = new QTableWidgetItem(e.title);
        t->setToolTip(e.url);
        m_table->setItem(r, 0, t);
        m_table->setItem(r, 1, new QTableWidgetItem(e.platform));
        m_table->setItem(r, 2, new QTableWidgetItem(e.format));
        m_table->setItem(r, 3, new QTableWidgetItem(
            e.finishedAt.toString("yyyy-MM-dd hh:mm")));

        auto *st = new QTableWidgetItem(e.success ? "Done" : "Error");
        st->setForeground(e.success ? QColor(80,200,80) : QColor(220,60,60));
        m_table->setItem(r, 4, st);

        auto *fi = new QTableWidgetItem(e.outputFile);
        fi->setToolTip(e.outputFile);
        m_table->setItem(r, 5, fi);
    }
}

void HistoryDialog::onClearHistory()
{
    if (QMessageBox::question(this, "Clear History",
        "Delete all history entries?") != QMessageBox::Yes) return;
    saveHistory({});
    m_table->setRowCount(0);
}

void HistoryDialog::onOpenFile()
{
    int r = m_table->currentRow();
    if (r < 0) return;
    QString path = m_table->item(r, 5)->text();
    if (!path.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void HistoryDialog::onCopyUrl()
{
    int r = m_table->currentRow();
    if (r < 0) return;
    QString url = m_table->item(r, 0)->toolTip();
    if (!url.isEmpty())
        QApplication::clipboard()->setText(url);
}
