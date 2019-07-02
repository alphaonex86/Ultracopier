#include "VerticalLabel.h"
#include <QPainter>
#include <QApplication>

VerticalLabel::VerticalLabel(QWidget *parent)
    : QLabel(parent)
{
    color=QApplication::palette().text().color();
}

VerticalLabel::VerticalLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
}

VerticalLabel::~VerticalLabel()
{
}

void VerticalLabel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setPen(color);
    painter.setBrush(Qt::Dense1Pattern);
    painter.rotate(90);
    painter.drawText(0,0, text());
}

void VerticalLabel::setColor(QColor color)
{
    this->color=color;
}

QSize VerticalLabel::minimumSizeHint() const
{
    QSize s = QLabel::minimumSizeHint();
    return QSize(s.height(), s.width());
}

QSize VerticalLabel::sizeHint() const
{
    QSize s = QLabel::sizeHint();
    return QSize(s.height(), s.width());
}
