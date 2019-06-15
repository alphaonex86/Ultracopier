#include "chartarea.h"

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QPointF>
#include <iostream>

ChartArea::Widget::Widget(QWidget *parent)
        : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::MinimumExpanding);
    setMinimumHeight(20);
    setMinimumWidth(100);
}

ChartArea::Widget::~Widget()
{
}

void ChartArea::Widget::invalidate()
{
}

void ChartArea::Widget::resizeEvent(QResizeEvent*)
{
}

void ChartArea::Widget::addValue(uint64_t value)
{
    m_values.push_back(value);
    while(m_values.size()>64)
        m_values.erase(m_values.begin());
    update();
}

void ChartArea::Widget::paintEvent(QPaintEvent*)
{
    QPainter painter;
    painter.begin(this);
    painter.drawRect(0,0,width()-1,height()-1);
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform,true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing,true);

    while(m_values.size()<64)
        m_values.insert(m_values.begin(),0);
    std::vector<uint64_t> values=m_values;
    QVector<QPointF> points;
    {
        uint64_t max=0;
        unsigned int index=0;
        while(index<values.size())
        {
            if(max<values.at(index))
                max=values.at(index);
            index++;
        }
        if(max<=0)
        {
            points << QPointF(0, height()-1);
            points << QPointF(width()/2, height()-1);
            points << QPointF(width()-1, height()-1);
        }
        else
        {
            index=0;
            while(index<values.size())
            {
                int w=width()-1;
                int nw=w*index/(m_values.size()-1);
                points << QPointF(nw, height()-values.at(index)*height()/max);
                index++;
            }
        }
    }

    painter.setPen(Qt::NoPen);
    QLinearGradient gradient(0,height()*0.5,0,height());
    gradient.setColorAt(0, QColor(160,240,160,150));
    //gradient.setColorAt(0.2, QColor(100,220,100,200));
    gradient.setColorAt(1, QColor(160,240,160,0));
    painter.setBrush(gradient);
    points.push_front(QPointF(0, height()-1));
    points.push_back(QPointF(width()-1, height()-1));
    painter.drawPolygon(QPolygonF(points));

    if(width()*height()>250000)
        painter.setPen(QPen(QColor(160,240,160), 3));
    else
        painter.setPen(QPen(QColor(160,240,160), 2));
    painter.drawPolyline(QPolygonF(points));


    // todo: bounding rect + center flag
    if(height()>30)
    {
        QFont font = painter.font();
        int heightTemp=height()/5;
        if(heightTemp<14)
            heightTemp=14;
        font.setPixelSize(heightTemp);
        painter.setFont(font);

        painter.setPen(QPen(QColor(140,140,140), 3));
        painter.drawText(0,0,width(),height(),Qt::AlignHCenter | Qt::AlignBottom,tr("%1B/s").arg(m_values.back()));
    }
    painter.end();
}
