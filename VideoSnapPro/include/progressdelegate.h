#pragma once
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include "downloaditem.h"

enum Col {
    COL_THUMB    = 0,
    COL_TITLE    = 1,
    COL_PLATFORM = 2,
    COL_FORMAT   = 3,
    COL_STATUS   = 4,
    COL_PROGRESS = 5,
    COL_SPEED    = 6,
    COL_ETA      = 7,
    COL_SIZE     = 8,
    COL_ACTION   = 9,
    COL_COUNT    = 10
};

class ProgressDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ProgressDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt,
               const QModelIndex &idx) const override
    {
        if (idx.column() == COL_THUMB) {
            QPixmap pix = idx.data(Qt::DecorationRole).value<QPixmap>();
            if (!pix.isNull()) {
                QRect r = opt.rect.adjusted(2,2,-2,-2);
                QPixmap scaled = pix.scaled(r.size(), Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation);
                int x = r.x() + (r.width()  - scaled.width())  / 2;
                int y = r.y() + (r.height() - scaled.height()) / 2;
                p->drawPixmap(x, y, scaled);
                return;
            }
        }

        if (idx.column() == COL_PROGRESS) {
            int pct = qBound(0, idx.data(Qt::DisplayRole).toInt(), 100);
            QRect r = opt.rect.adjusted(6, 8, -6, -8);

            // Background track
            p->setPen(Qt::NoPen);
            p->setBrush(QColor(40, 40, 40));
            p->drawRoundedRect(r, 4, 4);

            // Fill
            if (pct > 0) {
                QRect fill = r;
                fill.setWidth(int(r.width() * pct / 100.0));
                QColor bar = (pct == 100) ? QColor(80,200,80) : QColor(212,168,75);
                p->setBrush(bar);
                p->drawRoundedRect(fill, 4, 4);
            }

            // Text
            p->setPen(QColor(220,220,220));
            QFont f = p->font(); f.setPixelSize(11); p->setFont(f);
            p->drawText(r, Qt::AlignCenter,
                        pct == 0 ? "—" : QString("%1%").arg(pct));
            return;
        }

        QStyledItemDelegate::paint(p, opt, idx);
    }

    QSize sizeHint(const QStyleOptionViewItem &opt,
                   const QModelIndex &idx) const override
    {
        if (idx.column() == COL_THUMB)
            return QSize(90, 54);
        return QStyledItemDelegate::sizeHint(opt, idx);
    }
};
