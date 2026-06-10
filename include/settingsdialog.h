#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSettings>
#include <QStandardPaths>

struct AppSettings {
    QString savePath;
    int     concurrent   = 3;
    QString defaultFormat= "best";
    double  speedLimitMB = 0.0;   // 0 = unlimited
    bool    embedThumb   = true;
    bool    addMetadata  = true;
    bool    autoFetch    = true;   // fetch metadata on paste
    bool    notifyDone   = true;
    bool    minimizeToTray = true;
    QString ytdlpPath    = "yt-dlp";
    QString ffmpegPath   = "ffmpeg";

    void save() const {
        QSettings s;
        s.setValue("savePath",       savePath);
        s.setValue("concurrent",     concurrent);
        s.setValue("defaultFormat",  defaultFormat);
        s.setValue("speedLimitMB",   speedLimitMB);
        s.setValue("embedThumb",     embedThumb);
        s.setValue("addMetadata",    addMetadata);
        s.setValue("autoFetch",      autoFetch);
        s.setValue("notifyDone",     notifyDone);
        s.setValue("minimizeToTray", minimizeToTray);
        s.setValue("ytdlpPath",      ytdlpPath);
        s.setValue("ffmpegPath",     ffmpegPath);
    }

    static AppSettings load() {
        QSettings s;
        AppSettings a;
        a.savePath       = s.value("savePath",
            QStandardPaths::writableLocation(
                QStandardPaths::DownloadLocation)).toString();
        a.concurrent     = s.value("concurrent",   3).toInt();
        a.defaultFormat  = s.value("defaultFormat","best").toString();
        a.speedLimitMB   = s.value("speedLimitMB", 0.0).toDouble();
        a.embedThumb     = s.value("embedThumb",   true).toBool();
        a.addMetadata    = s.value("addMetadata",  true).toBool();
        a.autoFetch      = s.value("autoFetch",    true).toBool();
        a.notifyDone     = s.value("notifyDone",   true).toBool();
        a.minimizeToTray = s.value("minimizeToTray",true).toBool();
        a.ytdlpPath      = s.value("ytdlpPath",   "yt-dlp").toString();
        a.ffmpegPath     = s.value("ffmpegPath",  "ffmpeg").toString();
        return a;
    }
};

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(const AppSettings &current, QWidget *parent = nullptr);
    AppSettings result() const { return m_result; }

private slots:
    void onBrowseSave();
    void onBrowseYtdlp();
    void onBrowseFfmpeg();
    void onAccept();

private:
    void setupUi(const AppSettings &s);

    QLineEdit     *m_savePath    = nullptr;
    QSpinBox      *m_concurrent  = nullptr;
    QComboBox     *m_defFormat   = nullptr;
    QDoubleSpinBox*m_speedLimit  = nullptr;
    QCheckBox     *m_embedThumb  = nullptr;
    QCheckBox     *m_addMeta     = nullptr;
    QCheckBox     *m_autoFetch   = nullptr;
    QCheckBox     *m_notifyDone  = nullptr;
    QCheckBox     *m_minTray     = nullptr;
    QLineEdit     *m_ytdlpPath   = nullptr;
    QLineEdit     *m_ffmpegPath  = nullptr;

    AppSettings    m_result;
};
