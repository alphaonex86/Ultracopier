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

#ifndef RADIALMAP_H
#define RADIALMAP_H

#include <QColor>

class File;

namespace RadialMap
{
class Segment //all angles are in 16ths of degrees
{
public:
    Segment(const File *f, uint s, uint l, bool isFake = false)
            : m_angleStart(s)
            , m_angleSegment(l)
            , m_file(f)
            , m_hasHiddenChildren(false)
            , m_fake(isFake) {}
    ~Segment();

    uint          start() const {
        return m_angleStart;
    }
    uint         length() const {
        return m_angleSegment;
    }
    uint            end() const {
        return m_angleStart + m_angleSegment;
    }
    const File    *file() const {
        return m_file;
    }
    const QColor&   pen() const {
        return m_pen;
    }
    const QColor& brush() const {
        return m_brush;
    }

    bool isFake() const {
        return m_fake;
    }
    bool hasHiddenChildren() const {
        return m_hasHiddenChildren;
    }

    bool intersects(uint a) const {
        return ((a >= start()) && (a < end()));
    }

    friend class Map;
    friend class Builder;

private:
    void setPalette(const QColor &p, const QColor &b) {
        m_pen = p;
        m_brush = b;
    }

    const uint m_angleStart, m_angleSegment;
    const File* const m_file;
    QColor m_pen, m_brush;
    bool m_hasHiddenChildren;
    const bool m_fake;
};
}


#ifndef PI
#define PI 3.141592653589793
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#define MIN_RING_BREADTH 7
#define MAX_RING_BREADTH 60
#define DEFAULT_RING_DEPTH 4 //first level = 0
#define MIN_RING_DEPTH 0

#define LABEL_MAP_SPACER 7
#define LABEL_TEXT_HMARGIN 5
#define LABEL_TEXT_VMARGIN 0
#define LABEL_ANGLE_MARGIN 32
#define LABEL_MIN_ANGLE_FACTOR 0.05
#define LABEL_MAX_CHARS 30

#endif
