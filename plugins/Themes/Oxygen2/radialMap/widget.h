/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#ifndef WIDGET_H
#define WIDGET_H

#include <QUrl>

#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWidget>
#include <QTimer>

#include "map.h"

class Folder;
class File;
namespace KIO {
class Job;
}

namespace RadialMap
{
class Segment;

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget* = nullptr);
    ~Widget() override;
    QString path() const;
    QUrl url(File const * const = nullptr) const;

    bool isValid() const {
        return m_tree != nullptr;
    }

    friend class Label; //FIXME badness

public Q_SLOTS:
    void create(const Folder*);
    void invalidate();
    void resizeTimeout();
    void refresh(int);

private Q_SLOTS:
    void sendFakeMouseEvent();
    void createFromCache(const Folder*);

Q_SIGNALS:
    void activated(const QUrl&);
    void invalidated(const QUrl&);
    void folderCreated(const Folder*);
    void mouseHover(const QString&);
    void giveMeTreeFor(const QUrl&);

protected:
    void changeEvent(QEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void enterEvent(QEvent*) override;
    void leaveEvent(QEvent*) override;

protected:
    const Segment *segmentAt(QPoint&) const; //FIXME const reference for a library others can use
    const Segment *rootSegment() const {
        return m_rootSegment;    ///never == 0
    }
    const Segment *focusSegment() const {
        return m_focus;    ///0 == nothing in focus
    }

private:
    void paintExplodedLabels(QPainter&) const;

    const Folder *m_tree;
    const Segment   *m_focus;
    QPoint           m_offset;
    QTimer           m_timer;
    Map              m_map;
    Segment          *m_rootSegment;
    const Segment    *m_toBeDeleted;
    QLabel           m_tooltip;
    int minFontPitch;
};
}

#endif
