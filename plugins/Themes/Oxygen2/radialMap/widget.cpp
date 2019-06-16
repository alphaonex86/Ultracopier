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

#include "widget.h"

#include "../fileTree.h"
#include "radialMap.h" //constants
#include "map.h"

#include <QUrl>

#include <QApplication>   //sendEvent
#include <QBitmap>        //ctor - finding cursor size
#include <QCursor>        //slotPostMouseEvent()
#include <QTimer>         //member
#include <QWidget>


RadialMap::Widget::Widget(QWidget *parent)
        : QWidget(parent)
        , m_tree(nullptr)
        , m_focus(nullptr)
        , m_map()
        , m_rootSegment(nullptr) //TODO we don't delete it, *shrug*
        , m_toBeDeleted(nullptr)
        , minFontPitch(QFont().pointSize() - 3)
{
    setMinimumSize(0, 0);
    setMaximumSize(1, 1);

    connect(this, &Widget::folderCreated, this, &Widget::sendFakeMouseEvent);
    connect(&m_timer, &QTimer::timeout, this, &Widget::resizeTimeout);
    m_tooltip.setFrameShape(QFrame::StyledPanel);
    m_tooltip.setWindowFlags(Qt::ToolTip | Qt::WindowTransparentForInput);
}

RadialMap::Widget::~Widget()
{
    if(m_rootSegment!=nullptr)
        delete m_rootSegment;
}


QString RadialMap::Widget::path() const
{
    return m_tree->displayPath();
}

QUrl RadialMap::Widget::url(File const * const file) const
{
    return file ? file->url() : m_tree->url();
}

void RadialMap::Widget::invalidate()
{
    if (isValid())
    {
        //**** have to check that only way to invalidate is this function frankly
        //**** otherwise you may get bugs..

        //disable mouse tracking
        setMouseTracking(false);

        // Get this before reseting m_tree below
        QUrl invalidatedUrl(url());

        //ensure this class won't think we have a map still
        m_tree  = nullptr;
        m_focus = nullptr;

        delete m_rootSegment;
        m_rootSegment = nullptr;

        //FIXME move this disablement thing no?
        //      it is confusing in other areas, like the whole createFromCache() thing
        m_map.invalidate();
        update();

        //tell rest of Filelight
        emit invalidated(invalidatedUrl);
    }
}

void
RadialMap::Widget::create(const Folder *tree)
{
    //it is not the responsibility of create() to invalidate first
    //skip invalidation at your own risk

    //FIXME make it the responsibility of create to invalidate first
    setMaximumSize(16777215, 16777215);
    setMinimumSize(150, 100);

    if (tree)
    {
        m_focus = nullptr;
        //generate the filemap image
        m_map.make(tree);

        //this is the inner circle in the center
        if(m_rootSegment!=nullptr)
            delete m_rootSegment;
        m_rootSegment = new Segment(tree, 0, 16*360);

        setMouseTracking(true);
    }

    m_tree = tree;

    //tell rest of Filelight
    emit folderCreated(tree);
}

void
RadialMap::Widget::createFromCache(const Folder *tree)
{
    //no scan was necessary, use cached tree, however we MUST still emit invalidate
    invalidate();
    create(tree);
}

void
RadialMap::Widget::sendFakeMouseEvent() //slot
{
    QMouseEvent me(QEvent::MouseMove, mapFromGlobal(QCursor::pos()), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(this, &me);
    update();
}

void
RadialMap::Widget::resizeTimeout() //slot
{
    // the segments are about to erased!
    // this was a horrid bug, and proves the OO programming should be obeyed always!
    m_focus = nullptr;
    if (m_tree)
        m_map.make(m_tree, true);
    update();
}

void
RadialMap::Widget::refresh(int filth)
{
    //TODO consider a more direct connection

    if (!m_map.isNull())
    {
        switch (filth)
        {
        case 1:
            m_focus=nullptr;
            m_map.make(m_tree, true); //true means refresh only
            break;

        case 2:
            m_map.paint(true); //antialiased painting
            break;

        case 3:
            m_map.colorise(); //FALL THROUGH!
        case 4:
            m_map.paint();

        default:
            break;
        }

        update();
    }
}

RadialMap::Segment::~Segment()
{
    if (isFake())
        delete m_file; //created by us in Builder::build()
}


