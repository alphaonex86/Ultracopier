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

#include <QApplication>    //make()
#include <QImage>          //make() & paint()
#include <QFont>           //ctor
#include <QFontMetrics>    //ctor
#include <QPainter>
#include <QBrush>

#include "radialMap.h" // defines
#include "../interface.h"
#include "../fileTree.h"
#include "widget.h"
#include <cmath>
#include <qmath.h>

RadialMap::Map::Map()
        : m_signature(nullptr)
        , m_visibleDepth(DEFAULT_RING_DEPTH)
        , m_ringBreadth(MIN_RING_BREADTH)
        , m_innerRadius(0)
        , defaultRingDepth(4)
{

    //FIXME this is all broken. No longer is a maximum depth!
    const int fmh   = QFontMetrics(QFont()).height();
    const int fmhD4 = fmh / 4;
    MAP_2MARGIN = 2 * (fmh - (fmhD4 - LABEL_MAP_SPACER)); //margin is dependent on fitting in labels at top and bottom

    m_minSize=27300;
}

RadialMap::Map::~Map()
{
    delete [] m_signature;
}

void RadialMap::Map::invalidate()
{
    delete [] m_signature;
    m_signature = nullptr;

    m_visibleDepth = defaultRingDepth;
}

void RadialMap::Map::make(const Folder *tree, bool refresh)
{
    if(height()<1)
        return;
    //slow operation so set the wait cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);

    //build a signature of visible components
    {
        //**** REMOVE NEED FOR the +1 with MAX_RING_DEPTH uses
        //**** add some angle bounds checking (possibly in Segment ctor? can I delete in a ctor?)
        //**** this is a mess

        delete [] m_signature;
        m_signature = new std::vector<Segment*>[m_visibleDepth + 1];

        m_root = tree;

        if (!refresh) {
            quint64 varSize=tree->size();
            quint64 varHeight=height();
            quint64 varA=(varSize * 3);
            quint64 varB=(PI * varHeight - MAP_2MARGIN);
            m_minSize = varA / varB;
            findVisibleDepth(tree);
        }

        setRingBreadth();

        // Calculate ring size limits
        m_limits.resize(m_visibleDepth + 1);
        const double size = m_root->size();
        const double pi2B  = M_PI * 4 * m_ringBreadth;
        for (uint depth = 0; depth <= m_visibleDepth; ++depth) {
            m_limits[depth] = uint(size / double(pi2B * (depth + 1))); //min is angle that gives 3px outer diameter for that depth
        }

        build(tree);
    }

    //colour the segments
    colorise();

    m_centerText = tree->humanReadableSize()+"\n"+QObject::tr("%1 files").arg(Themes::simplifiedBigNum(tree->children()));

    //paint the pixmap
    paint();

    QApplication::restoreOverrideCursor();
}

void RadialMap::Map::setRingBreadth()
{
    //FIXME called too many times on creation

    m_ringBreadth = (height() - MAP_2MARGIN) / (2 * m_visibleDepth + 4);
    m_ringBreadth = qBound(MIN_RING_BREADTH, m_ringBreadth, MAX_RING_BREADTH);
}

void RadialMap::Map::findVisibleDepth(const Folder *dir, uint currentDepth)
{

    //**** because I don't use the same minimumSize criteria as in the visual function
    //     this can lead to incorrect visual representation
    //**** BUT, you can't set those limits until you know m_depth!

    //**** also this function doesn't check to see if anything is actually visible
    //     it just assumes that when it reaches a new level everything in it is visible
    //     automatically. This isn't right especially as there might be no files in the
    //     dir provided to this function!

    static uint stopDepth = 0;

    if (dir == m_root) {
        stopDepth = m_visibleDepth;
        m_visibleDepth = 0;
    }

    if (m_visibleDepth < currentDepth) m_visibleDepth = currentDepth;
    if (m_visibleDepth >= stopDepth) return;

    for(const auto& n : dir->folders)
    {
        Folder * folder=n.second;
        if (folder->size() > m_minSize) {
            findVisibleDepth(folder, currentDepth + 1); //if no files greater than min size the depth is still recorded
        }
    }
}

//**** segments currently overlap at edges (i.e. end of first is start of next)
bool RadialMap::Map::build(const Folder * const dir, const uint depth, uint a_start, const uint a_end)
{
    //first iteration: dir == m_root

    if (dir->children() == 0) //we do fileCount rather than size to avoid chance of divide by zero later
        return false;

    uint64_t hiddenSize = 0;
    uint hiddenFileCount = 0;

    for(const auto& n : dir->folders)
    {
        Folder * folder=n.second;
        if (folder->size() < m_limits[depth] * 6) { // limit is half a degree? we want at least 3 degrees
            hiddenSize += folder->size();
            hiddenFileCount += folder->children(); //need to add one to count the dir as well
            ++hiddenFileCount;
            continue;
        }
        unsigned int a_len = (unsigned int)(5760 * ((double)folder->size() / (double)m_root->size()));
        Segment *s = new Segment(folder, a_start, a_len);
        m_signature[depth].push_back(s);
        if (depth != m_visibleDepth) {
            //recurse
            s->m_hasHiddenChildren = build(folder, depth + 1, a_start, a_start + a_len);
        } else {
            s->m_hasHiddenChildren = true;
        }
        a_start += a_len; //**** should we add 1?
    }
    for (File *file : dir->onlyFiles) {
        if (file->size() < m_limits[depth] * 6) { // limit is half a degree? we want at least 3 degrees
            hiddenSize += file->size();
            ++hiddenFileCount;
            continue;
        }
        unsigned int a_len = (unsigned int)(5760 * ((double)file->size() / (double)m_root->size()));
        Segment *s = new Segment(file, a_start, a_len);
        m_signature[depth].push_back(s);
        a_start += a_len; //**** should we add 1?
    }

    if (hiddenFileCount == dir->children()) {
        return true;
    }

    if (depth == 0 && hiddenSize >= m_limits[depth] && hiddenFileCount > 0) {
        //append a segment for unrepresented space - a "fake" segment
        const QString s = QObject::tr("%n file(s), with an average size of %1","file",hiddenFileCount)
                .arg(QString::fromStdString(File::facilityEngine->sizeToString(hiddenSize/hiddenFileCount)));


        (m_signature + depth)->push_back(new Segment(new File(s.toUtf8().constData(), hiddenSize), a_start, a_end - a_start, true));
    }

    return false;
}

bool RadialMap::Map::resize(const QRect &rect)
{
    //there's a MAP_2MARGIN border

    const int mw=width();
    const int mh=height();
    const int cw=rect.width();
    const int ch=rect.height();

    if (cw < mw || ch < mh || (cw > mw && ch > mh))
    {
        uint size = ((cw < ch) ? cw : ch) - MAP_2MARGIN;

        //this also causes uneven sizes to always resize when resizing but map is small in that dimension
        //size -= size % 2; //even sizes mean less staggered non-antialiased resizing

        {
            const uint minSize = MIN_RING_BREADTH * 2 * (m_visibleDepth + 2);

            if (size < minSize)
                size = minSize;

            //this QRect is used by paint()
            m_rect.setRect(0,0,size,size);
        }
        m_pixmap = QPixmap(m_rect.size());

        //resize the pixmap
        size += MAP_2MARGIN;

        if (m_signature != nullptr)
        {
            setRingBreadth();
            paint();
        }

        return true;
    }

    return false;
}

void RadialMap::Map::colorise()
{
    if (!m_signature || m_signature->empty()) {
        //std::cerr << "no signature yet" << std::endl;
        return;
    }

    QColor cp, cb;
    double darkness = 1;
    double contrast = (double)94 / (double)100;
    int h, s1, s2, v1, v2;

    for (uint i = 0; i <= m_visibleDepth; ++i, darkness += 0.04) {
        for (Segment *segment : m_signature[i]) {
            h  = int(segment->start() / 16);
            s1 = 160;
            v1 = (int)(255.0 / darkness); //doing this more often than once seems daft!

            v2 = v1 - int(contrast * v1);
            s2 = s1 + int(contrast * (255 - s1));

            if (s1 < 80) s1 = 80; //can fall too low and makes contrast between the files hard to discern

            if (segment->isFake()) { //multi-file
                cb.setHsv(h, s2, (v2 < 90) ? 90 : v2); //too dark if < 100
                cp.setHsv(h, 17, v1);
            } else if (!segment->file()->isFolder()) { //file
                cb.setHsv(h, 17, v1);
                cp.setHsv(h, 17, v2);
            } else { //folder
                cb.setHsv(h, s1, v1); //v was 225
                cp.setHsv(h, s2, v2); //v was 225 - delta
            }

            segment->setPalette(cp, cb);
        }
    }
}

void RadialMap::Map::paint(bool antialias)
{
    QPainter paint;
    QRect rect = m_rect;

    rect.adjust(5, 5, -5, -5);
    m_pixmap.fill(Qt::transparent);

    //m_rect.moveRight(1); // Uncommenting this breaks repainting when recreating map from cache


    //**** best option you can think of is to make the circles slightly less perfect,
    //  ** i.e. slightly eliptic when resizing inbetween

    if (m_pixmap.isNull())
        return;

    if (!paint.begin(&m_pixmap)) {
        //qWarning() << "Filelight::RadialMap Failed to initialize painting, returning...";
        return;
    }

    if (antialias) {
        paint.translate(0.7, 0.7);
        paint.setRenderHint(QPainter::Antialiasing);
    }

    int step = m_ringBreadth;
    int excess = -1;

    //do intelligent distribution of excess to prevent nasty resizing
    if (m_ringBreadth != MAX_RING_BREADTH && m_ringBreadth != MIN_RING_BREADTH) {
        excess = rect.width() % m_ringBreadth;
        ++step;
    }

    for (int x = m_visibleDepth; x >= 0; --x)
    {
        int width = rect.width() / 2;
        //clever geometric trick to find largest angle that will give biggest arrow head
        uint a_max = int(acos((double)width / double((width + 5))) * (180*16 / M_PI));

        for (Segment *segment : m_signature[x]) {
            //draw the pie segments, most of this code is concerned with drawing the little
            //arrows on the ends of segments when they have hidden files

            paint.setPen(segment->pen());

            if (segment->hasHiddenChildren())
            {
                //draw arrow head to indicate undisplayed files/directories
                QPolygon pts(3);
                QPoint pos, cpos = rect.center();
                uint a[3] = { segment->start(), segment->length(), 0 };

                a[2] = a[0] + (a[1] / 2); //assign to halfway between
                if (a[1] > a_max)
                {
                    a[1] = a_max;
                    a[0] = a[2] - a_max / 2;
                }

                a[1] += a[0];

                for (int i = 0, radius = width; i < 3; ++i)
                {
                    double ra = M_PI/(180*16) * a[i], sinra, cosra;

                    if (i == 2)
                        radius += 5;
                    sinra = qSin(ra);
                    cosra = qCos(ra);
                    pos.rx() = cpos.x() + static_cast<int>(cosra * radius);
                    pos.ry() = cpos.y() - static_cast<int>(sinra * radius);
                    pts.setPoint(i, pos);
                }

                paint.setBrush(segment->pen());
                paint.drawPolygon(pts);
            }

            paint.setPen(QColor(120,120,120));
            paint.setBrush(segment->brush());
            paint.drawPie(rect, segment->start(), segment->length());

            if (segment->hasHiddenChildren())
            {
                //**** code is bloated!
                paint.save();
                QPen pen = paint.pen();
                int width = 2;
                pen.setWidth(width);
                paint.setPen(pen);
                QRect rect2 = rect;
                width /= 2;
                rect2.adjust(width, width, -width, -width);
                paint.drawArc(rect2, segment->start(), segment->length());
                paint.restore();
            }
        }

        if (excess >= 0) { //excess allows us to resize more smoothly (still crud tho)
            if (excess < 2) //only decrease rect by more if even number of excesses left
                --step;
            excess -= 2;
        }

        rect.adjust(step, step, -step, -step);
    }

    //  if(excess > 0) rect.addCoords(excess, excess, 0, 0); //ugly

    paint.setPen(QColor(120,120,120));
    paint.setBrush(QColor(255,255,255));
    paint.drawEllipse(rect);
    if(width()>200)
    {
        paint.setPen(QColor(0,0,0));
        paint.drawText(rect, Qt::AlignCenter, m_centerText);
    }

    m_innerRadius = rect.width() / 2; //rect.width should be multiple of 2

    paint.end();
}

