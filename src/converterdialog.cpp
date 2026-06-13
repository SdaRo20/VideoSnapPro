#include "converterdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QLabel>
#include <QGroupBox>

ConverterDialog::ConverterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Audio / Video Converter");
    setMinimumWidth(520);
    setupUi();
}

void ConverterDialog::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);

    auto *grp  = new QGroupBox("Convert a file");
    auto *form = new QFormLayout(grp);
    form->setSpacing(8);

    // Input file
    m_input = new QLineEdit;
    m_input->setPlaceholderText("Select source file…");
    auto *inputRow = new QHBoxLayout;
    inputRow->addWidget(m_input);
    auto *inBtn = new QPushButton("Browse…");
    inBtn->setFixedWidth(80);
    connect(inBtn, &QPushButton::clicked, this, &ConverterDialog::onBrowseInput);
    inputRow->addWidget(inBtn);
    form->addRow("Input file:", inputRow);

    // Output format
    m_format = new QComboBox;
    m_format->addItems({"MP3","AAC","FLAC","OGG","WAV","M4A","MP4","MKV","WEBM"});
    form->addRow("Output format:", m_format);

    // Quality
    m_quality = new QComboBox;
    m_quality->addItems({"Best (slow)","High","Medium","Small (fast)"});
    form->addRow("Quality:", m_quality);

    // Output file
    m_output = new QLineEdit;
    m_output->setPlaceholderText("Auto-generated from input…");
    auto *outRow = new QHBoxLayout;
    outRow->addWidget(m_output);
    auto *outBtn = new QPushButton("Browse…");
    outBtn->setFixedWidth(80);
    connect(outBtn, &QPushButton::clicked, this, &ConverterDialog::onBrowseOutput);
    outRow->addWidget(outBtn);
    form->addRow("Output file:", outRow);

    root->addWidget(grp);

    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    root->addWidget(m_progress);

    m_status = new QLabel("Ready");
    m_status->setStyleSheet("color: #888; font-size: 11px;");
    root->addWidget(m_status);

    m_convertBtn = new QPushButton("Convert");
    m_convertBtn->setObjectName("btnStart");
    m_convertBtn->setFixedHeight(36);
    connect(m_convertBtn, &QPushButton::clicked, this, &ConverterDialog::onConvert);
    root->addWidget(m_convertBtn);

    // Auto-fill output when format changes
    connect(m_format, &QComboBox::currentTextChanged, this, [this](const QString &fmt) {
        if (!m_input->text().isEmpty()) {
            QFileInfo fi(m_input->text());
            m_output->setText(fi.dir().absolutePath() + "/" +
                              fi.completeBaseName() + "." + fmt.toLower());
        }
    });
}

void ConverterDialog::onBrowseInput()
{
    QString f = QFileDialog::getOpenFileName(this, "Select file", "",
        "Media Files (*.mp4 *.mkv *.webm *.avi *.mp3 *.m4a *.aac *.flac *.ogg *.wav);;All (*)");
    if (f.isEmpty()) return;
    m_input->setText(f);

    // Auto-fill output
    QFileInfo fi(f);
    QString ext = m_format->currentText().toLower();
    m_output->setText(fi.dir().absolutePath() + "/" +
                      fi.completeBaseName() + "." + ext);
}

void ConverterDialog::onBrowseOutput()
{
    QString ext = m_format->currentText().toLower();
    QString f = QFileDialog::getSaveFileName(this, "Save as", m_output->text(),
        QString("*.%1").arg(ext));
    if (!f.isEmpty()) m_output->setText(f);
}

void ConverterDialog::onConvert()
{
    if (m_input->text().isEmpty() || m_output->text().isEmpty()) {
        m_status->setText("Please select input and output files.");
        return;
    }

    m_convertBtn->setEnabled(false);
    m_progress->setValue(0);
    m_status->setText("Converting…");

    // Build ffmpeg args
    QStringList args;
    args << "-y" << "-i" << m_input->text();

    QString fmt = m_format->currentText().toLower();
    int q = m_quality->currentIndex(); // 0=best, 3=small

    // Audio quality
    if (fmt == "mp3") {
        int aq = q == 0 ? 0 : q == 1 ? 2 : q == 2 ? 5 : 7;
        args << "-codec:a" << "libmp3lame" << "-q:a" << QString::number(aq);
    } else if (fmt == "aac" || fmt == "m4a") {
        int br = q == 0 ? 320 : q == 1 ? 256 : q == 2 ? 192 : 128;
        args << "-codec:a" << "aac" << "-b:a" << QString("%1k").arg(br);
    } else if (fmt == "flac") {
        args << "-codec:a" << "flac";
    } else if (fmt == "ogg") {
        args << "-codec:a" << "libvorbis" << "-q:a" << QString::number(4 - q);
    } else if (fmt == "wav") {
        args << "-codec:a" << "pcm_s16le";
    } else {
        // Video passthrough or re-encode
        if (q > 0)
            args << "-crf" << QString::number(18 + q * 5);
    }

    args << "-progress" << "pipe:1" << "-nostats";
    args << m_output->text();

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyRead, this, &ConverterDialog::onProcessOutput);
    connect(m_proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ConverterDialog::onProcessFinished);

    m_proc->start("ffmpeg", args);
    if (!m_proc->waitForStarted(5000)) {
        m_status->setText("ffmpeg not found — install it and add to PATH");
        m_convertBtn->setEnabled(true);
    }
}

void ConverterDialog::onProcessOutput()
{
    while (m_proc->canReadLine()) {
        QString line = QString::fromUtf8(m_proc->readLine()).trimmed();
        if (line.startsWith("out_time_ms=")) {
            // Estimate progress from time (rough)
            long long ms = line.mid(12).toLongLong() / 1000;
            m_status->setText(QString("Converting… %1:%2")
                .arg(ms/60000).arg((ms%60000)/1000, 2, 10, QChar('0')));
        }
    }
}

void ConverterDialog::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
    m_convertBtn->setEnabled(true);
    if (exitCode == 0) {
        m_progress->setValue(100);
        m_status->setText("Done! Saved to: " + m_output->text());
    } else {
        m_status->setText("Conversion failed — check input file and ffmpeg installation");
    }
    m_proc->deleteLater();
    m_proc = nullptr;
}
