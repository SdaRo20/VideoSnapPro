#include "thememanager.h"

QString ThemeManager::styleSheet() const
{
    bool dark = (m_theme == Dark);

    QString bg       = dark ? "#0f0f0f" : "#f5f5f5";
    QString surface  = dark ? "#181818" : "#ffffff";
    QString surface2 = dark ? "#1e1e1e" : "#ebebeb";
    QString surface3 = dark ? "#242424" : "#e0e0e0";
    QString border   = dark ? "#2a2a2a" : "#d2d2d2";
    QString border2  = dark ? "#3a3a3a" : "#bbbbbb";
    QString txt      = dark ? "#e0e0e0" : "#1e1e1e";
    QString txtMuted = dark ? "#777777" : "#888888";
    QString accent   = "#d4a84b";
    QString accentHov= "#e0b95c";
    QString accentPrs= "#c09030";
    QString danger   = dark ? "#2a1010" : "#fff0f0";
    QString dangerBrd= dark ? "#551111" : "#ffcccc";
    QString dangerTxt= "#e04444";

    return QString(R"(
QMainWindow, QWidget, QDialog {
    background: %1; color: %7;
}
QGroupBox {
    border: 1px solid %5; border-radius: 8px;
    margin-top: 8px; font-size: 11px; color: %8;
    padding: 10px 6px 8px 6px;
}
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
QTextEdit, QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background: %2; border: 1px solid %5; border-radius: 6px;
    color: %7; padding: 4px 8px;
    selection-background-color: %11; selection-color: #000;
}
QTextEdit:focus, QLineEdit:focus, QSpinBox:focus { border-color: %6; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox::down-arrow { width: 10px; }
QComboBox QAbstractItemView {
    background: %3; border: 1px solid %5;
    selection-background-color: %11; selection-color: #000;
}
QSpinBox::up-button, QSpinBox::down-button { width: 16px; }
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width: 16px; }
QPushButton {
    background: %3; border: 1px solid %6;
    border-radius: 6px; color: %7;
    padding: 5px 14px; font-size: 12px;
}
QPushButton:hover  { background: %4; border-color: %6; color: %7; }
QPushButton:pressed { background: %2; }
QPushButton#btnStart {
    background: %11; border-color: %11;
    color: #000; font-weight: bold;
}
QPushButton#btnStart:hover   { background: %12; }
QPushButton#btnStart:pressed { background: %13; }
QPushButton#btnAbort {
    background: %14; border-color: %15; color: %16;
}
QPushButton#btnAbort:hover { background: %14; border-color: %16; }
QPushButton#btnTheme {
    background: transparent; border: 1px solid %6;
    border-radius: 6px; padding: 4px 10px; font-size: 13px;
}
QTableWidget {
    background: %1; alternate-background-color: %2;
    border: 1px solid %5; border-radius: 8px;
    gridline-color: %5; color: %7; font-size: 12px;
}
QHeaderView::section {
    background: %2; border: none;
    border-bottom: 1px solid %5; border-right: 1px solid %5;
    color: %8; font-size: 11px; font-weight: bold; padding: 6px 8px;
}
QTableWidget::item:selected { background: %3; color: %7; }
QStatusBar {
    color: %8; font-size: 11px; border-top: 1px solid %5;
    background: %2;
}
QScrollBar:vertical { background: %2; width: 8px; border: none; }
QScrollBar::handle:vertical {
    background: %4; border-radius: 4px; min-height: 24px;
}
QScrollBar::handle:vertical:hover { background: %6; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: %2; height: 8px; border: none; }
QScrollBar::handle:horizontal {
    background: %4; border-radius: 4px; min-width: 24px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QTabWidget::pane { border: 1px solid %5; border-radius: 6px; background: %2; }
QTabBar::tab {
    background: %3; border: 1px solid %5;
    border-bottom: none; border-radius: 6px 6px 0 0;
    padding: 6px 16px; color: %8; font-size: 12px;
}
QTabBar::tab:selected { background: %2; color: %7; }
QTabBar::tab:hover { background: %4; }
QSlider::groove:horizontal {
    background: %4; height: 4px; border-radius: 2px;
}
QSlider::handle:horizontal {
    background: %11; width: 14px; height: 14px;
    border-radius: 7px; margin: -5px 0;
}
QSlider::sub-page:horizontal { background: %11; border-radius: 2px; }
QCheckBox { color: %7; font-size: 12px; }
QCheckBox::indicator {
    width: 16px; height: 16px;
    border: 1px solid %6; border-radius: 4px; background: %2;
}
QCheckBox::indicator:checked { background: %11; border-color: %11; }
QLabel { color: %7; }
QMenuBar { background: %2; color: %7; border-bottom: 1px solid %5; }
QMenuBar::item:selected { background: %3; }
QMenu { background: %2; border: 1px solid %5; color: %7; }
QMenu::item:selected { background: %11; color: #000; }
QSplitter::handle { background: %5; }
QToolTip { background: %3; border: 1px solid %6; color: %7; padding: 4px; }
QProgressBar {
    background: %3; border: none; border-radius: 3px;
    text-align: center; color: %7; font-size: 11px;
}
QProgressBar::chunk { background: %11; border-radius: 3px; }
)")
    .arg(bg)        // 1
    .arg(surface)   // 2
    .arg(surface2)  // 3
    .arg(surface3)  // 4
    .arg(border)    // 5
    .arg(border2)   // 6
    .arg(txt)       // 7
    .arg(txtMuted)  // 8
    .arg("")        // 9 unused
    .arg("")        // 10 unused
    .arg(accent)    // 11
    .arg(accentHov) // 12
    .arg(accentPrs) // 13
    .arg(danger)    // 14
    .arg(dangerBrd) // 15
    .arg(dangerTxt);// 16
}
