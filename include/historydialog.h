#pragma once
#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QList>
#include "downloaditem.h"

struct HistoryEntry {
    QString   title;
    QString   url;
    QString   format;
    QString   outputFile;
    QString   platform;
    QDateTime finishedAt;
    bool      success;
};

class HistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HistoryDialog(QWidget *parent = nullptr);

    static void addEntry(const HistoryEntry &e);
    static QList<HistoryEntry> loadHistory();
    static void saveHistory(const QList<HistoryEntry> &list);
    static QString historyFilePath();

private slots:
    void onClearHistory();
    void onOpenFile();
    void onCopyUrl();
    void onSearchChanged(const QString &text);

private:
    void load();
    void populateTable(const QList<HistoryEntry> &list);
    QTableWidget *m_table = nullptr;
    QLineEdit    *m_searchBox = nullptr;
    QList<HistoryEntry> m_allEntries;
};
