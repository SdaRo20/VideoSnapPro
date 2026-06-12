#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>
#include <QPushButton>

class ConverterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConverterDialog(QWidget *parent = nullptr);

private slots:
    void onBrowseInput();
    void onBrowseOutput();
    void onConvert();
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus);

private:
    void setupUi();
    QLineEdit   *m_input    = nullptr;
    QLineEdit   *m_output   = nullptr;
    QComboBox   *m_format   = nullptr;
    QComboBox   *m_quality  = nullptr;
    QProgressBar*m_progress = nullptr;
    QLabel      *m_status   = nullptr;
    QPushButton *m_convertBtn = nullptr;
    QProcess    *m_proc     = nullptr;
};
