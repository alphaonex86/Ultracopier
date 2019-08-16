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

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QVector>

#include "../fileTree.h"
#include "radialMap.h"
#include "widget.h"
#include <math.h>
#include <qmath.h>

namespace RadialMap
{
class Label
{
public:
    Label(const RadialMap::Segment *s, int l) : segment(s), level(l), angle(segment->start() + (segment->length() / 2)) { }

    bool tooClose(const int otherAngle) const {
        return (angle > otherAngle - LABEL_ANGLE_MARGIN && angle < otherAngle + LABEL_ANGLE_MARGIN);
    }

    const RadialMap::Segment *segment;
    const unsigned int level;
    const int angle;

    int targetX, targetY, middleX, startY, startX;
    int textX, textY, tw, th;

    QString qs;
};

void RadialMap::Widget::paintExplodedLabels(QPainter &paint) const
{
    //we are a friend of RadialMap::Map

    QVector<Label*> list;
    unsigned int startLevel = 0;


    //1. Create list of labels  sorted in the order they will be rendered

    if (m_focus && m_focus->file() != m_tree) { //separate behavior for selected vs unselected segments
        //don't bother with files
        if (m_focus && m_focus->file() && !m_focus->file()->isFolder()) {
            return;
        }

        //find the range of levels we will be potentially drawing labels for
        //startLevel is the level above whatever m_focus is in
        for (const Folder *p = (const Folder*)m_focus->file(); p != m_tree; ++startLevel) {
            p = p->parent();
        }

        //range=2 means 2 levels to draw labels for

        const uint start = m_focus->start();
        const uint end = m_focus->end();  //boundary angles
        const uint minAngle = int(m_focus->length() * LABEL_MIN_ANGLE_FACTOR);


        //**** Levels should be on a scale starting with 0
        //**** range is a useless parameter
        //**** keep a topblock var which is the lowestLevel OR startLevel for indentation purposes
        for (unsigned int i = startLevel; i <= m_map.m_visibleDepth; ++i) {
            for (const Segment *segment : m_map.m_signature[i]) {
                if (segment->start() >= start && segment->end() <= end) {
                    if (segment->length() > minAngle) {
                        list.append(new Label(segment, i));
                    }
                }
            }
        }
    } else {
        for (Segment *segment : *m_map.m_signature) {
            if (segment->length() > 288) {
                list.append(new Label(segment, 0));

            }
        }
    }

    std::sort(list.begin(), list.end(), [](Label *item1, Label *item2) {
        //you add 1440 to work round the fact that later you want the circle split vertically
        //and as it is you start at 3 o' clock. It's to do with rightPrevY, stops annoying bug

        int angle1 = (item1)->angle + 1440;
        int angle2 = (item2)->angle + 1440;

        // Also sort by level
        if (angle1 == angle2) {
                return (item1->level > item2->level);
        }

        if (angle1 > 5760) angle1 -= 5760;
        if (angle2 > 5760) angle2 -= 5760;

        return (angle1 < angle2);

    });

    //2. Check to see if any adjacent labels are too close together
    //   if so, remove it (the least significant labels, since we sort by level too).

    int pos = 0;
    while (pos < list.size() - 1) {
        if (list[pos]->tooClose(list[pos+1]->angle)) {
            delete list.takeAt(pos+1);
        } else {
            ++pos;
        }
    }

    //used in next two steps
    bool varySizes;
    //**** should perhaps use doubles
    int  *sizes = new int [ m_map.m_visibleDepth + 1 ]; //**** make sizes an array of floats I think instead (or doubles)

    // If the minimum is larger than the default it fucks up further down
    if (paint.font().pointSize() < 0 ||
        paint.font().pointSize() < minFontPitch) {
        QFont font = paint.font();
        font.setPointSize(minFontPitch);
        paint.setFont(font);
    }

    QVector<Label*>::iterator it;

    do {
        //3. Calculate font sizes

        {
            //determine current range of levels to draw for
            uint range = 0;

            for (Label *label : list) {
                range = qMax(range, label->level);

                //**** better way would just be to assign if nothing is range
            }

            range -= startLevel; //range 0 means 1 level of labels

            varySizes = range != 0;

            if (varySizes) {
                //create an array of font sizes for various levels
                //will exceed normal font pitch automatically if necessary, but not minPitch
                //**** this needs to be checked lots

                //**** what if this is negative (min size gtr than default size)
                uint step = (paint.font().pointSize() - minFontPitch) / range;
                if (step == 0) {
                    step = 1;
                }

                for (uint x = range + startLevel, y = minFontPitch; x >= startLevel; y += step, --x) {
                    sizes[x] = y;
                }
            }
        }

        //4. determine label co-ordinates


        const int preSpacer = int(m_map.m_ringBreadth * 0.5) + m_map.m_innerRadius;
        const int fullStrutLength = (m_map.width() - m_map.MAP_2MARGIN) / 2 + LABEL_MAP_SPACER; //full length of a strut from map center

        int prevLeftY  = 0;
        int prevRightY = height();

        QFont font;

        for (it = list.begin(); it != list.end(); ++it) {
            Label *label = *it;
            //** bear in mind that text is drawn with QPoint param as BOTTOM left corner of text box
            QString string = label->segment->file()->displayName();
            if (varySizes) {
                font.setPointSize(sizes[label->level]);
            }
            QFontMetrics fontMetrics(font);
            const int minTextWidth = fontMetrics.horizontalAdvance(QString::fromLatin1("M...")) + LABEL_TEXT_HMARGIN; // Fully elided string

            const int fontHeight  = fontMetrics.height() + LABEL_TEXT_VMARGIN; //used to ensure label texts don't overlap
            const int lineSpacing = fontHeight / 4;

            const bool rightSide = (label->angle < 1440 || label->angle > 4320);

            double sinra, cosra;
            const double ra = M_PI/2880 * label->angle; //convert to radians
            sinra = qSin(ra);
            cosra = qCos(ra);

            const int spacer = preSpacer + m_map.m_ringBreadth * label->level;

            const int centerX = m_map.width()  / 2 + m_offset.x();  //centre relative to canvas
            const int centerY = m_map.height() / 2 + m_offset.y();
            int targetX = centerX + cosra * spacer;
            int targetY = centerY - sinra * spacer;
            int startX = targetX + cosra * (fullStrutLength - spacer + m_map.m_ringBreadth / 2);
            int startY = targetY - sinra * (fullStrutLength - spacer);

            if (rightSide) { //righthand side, going upwards
                if (startY > prevRightY /*- fmh*/) { //then it is too low, needs to be drawn higher
                    startY = prevRightY /*- fmh*/;
                }
            } else {//lefthand side, going downwards
                if (startY < prevLeftY/* + fmh*/) { //then we're too high, need to be drawn lower
                    startY = prevLeftY /*+ fmh*/;
                }
            }

            int middleX = targetX - (startY - targetY) / tan(ra);
            int textY = startY + lineSpacing;

            int textX;
            const int textWidth = fontMetrics.horizontalAdvance(string) + LABEL_TEXT_HMARGIN;
            if (rightSide) {
                if (startX + minTextWidth > width() || textY < fontHeight || middleX < targetX) {
                    //skip this strut
                    //**** don't duplicate this code
                    list.erase(it); //will delete the label and set it to list.current() which _should_ be the next ptr
                    break;
                }

                prevRightY = textY - fontHeight - lineSpacing; //must be after above's "continue"

                if (m_offset.x() + m_map.width() + textWidth < width()) {
                    startX = m_offset.x() + m_map.width();
                } else {
                    startX = qMax(width() - textWidth, startX);
                    string = fontMetrics.elidedText(string, Qt::ElideMiddle, width() - startX);
                }

                textX = startX + LABEL_TEXT_HMARGIN;
            } else { // left side
                if (startX - minTextWidth < 0 || textY > height() || middleX > targetX) {
                    //skip this strut
                    list.erase(it); //will delete the label and set it to list.current() which _should_ be the next ptr
                    break;
                }

                prevLeftY = textY + fontHeight - lineSpacing;

                if (m_offset.x() - textWidth > 0) {
                    startX = m_offset.x();
                    textX = startX - textWidth - LABEL_TEXT_HMARGIN;
                } else {
                    textX = 0;
                    string = fontMetrics.elidedText(string, Qt::ElideMiddle, startX);
                    startX = fontMetrics.horizontalAdvance(string) + LABEL_TEXT_HMARGIN;
                }
            }

            label->targetX = targetX;
            label->targetY = targetY;
            label->middleX = middleX;
            label->startY = startY;
            label->startX = startX;
            label->textX = textX;
            label->textY = textY;
            label->qs = string;
        }

        //if an element is deleted at this stage, we need to do this whole
        //iteration again, thus the following loop
        //**** in rare case that deleted label was last label in top level
        //     and last in labelList too, this will not work as expected (not critical)

    } while (it != list.end());


    //5. Render labels

    QFont font;
    for (Label *label : list) {
        if (varySizes) {
            //**** how much overhead in making new QFont each time?
            //     (implicate sharing remember)
            font.setPointSize(sizes[label->level]);
            paint.setFont(font);
        }

        paint.setPen(QPen(QColor(0,0,0),2));
        paint.drawLine(label->targetX, label->targetY, label->middleX, label->startY);
        paint.drawLine(label->middleX, label->startY, label->startX, label->startY);

        paint.setPen(QPen(QColor(255,255,255),1));
        paint.drawLine(label->targetX, label->targetY, label->middleX, label->startY);
        paint.drawLine(label->middleX, label->startY, label->startX, label->startY);

        paint.drawText(label->textX, label->textY, label->qs);
    }

    qDeleteAll(list);
    delete [] sizes;
}
}

