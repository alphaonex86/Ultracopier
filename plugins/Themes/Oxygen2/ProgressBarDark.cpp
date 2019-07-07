#include "ProgressBarDark.h"
#include <QPainter>

ProgressBarDark::ProgressBarDark(QWidget *parent) :
    QProgressBar(parent)
{
    setMinimumHeight(22);
    setMaximumHeight(55);
}

ProgressBarDark::~ProgressBarDark()
{
}

void ProgressBarDark::paintEvent(QPaintEvent *)
{
    if(backgroundLeft.isNull() || backgroundLeft.height()!=height())
    {
        QPixmap background(":/Themes/Oxygen2/resources/progressBarout.png");
        if(background.isNull())
            abort();
        QPixmap bar(":/Themes/Oxygen2/resources/progressBarin.png");
        if(bar.isNull())
            abort();
        if(height()==background.height())
        {
            backgroundLeft=background.copy(0,0,24,55);
            backgroundMiddle=background.copy(24,0,701,55);
            backgroundRight=background.copy(725,0,24,55);
            barLeft=bar.copy(0,0,24,55);
            barMiddle=bar.copy(24,0,701,55);
            barRight=bar.copy(725,0,24,55);
        }
        else
        {
            backgroundLeft=background.copy(0,0,24,55).scaledToHeight(height(),Qt::SmoothTransformation);
            backgroundMiddle=background.copy(24,0,701,55).scaledToHeight(height(),Qt::SmoothTransformation);
            backgroundRight=background.copy(725,0,24,55).scaledToHeight(height(),Qt::SmoothTransformation);
            barLeft=bar.copy(0,0,24,55).scaledToHeight(height(),Qt::SmoothTransformation);
            barMiddle=bar.copy(24,0,701,55).scaledToHeight(height(),Qt::SmoothTransformation);
            barRight=bar.copy(725,0,24,55).scaledToHeight(height(),Qt::SmoothTransformation);
        }
    }
    QPainter paint;
    paint.begin(this);

    if(maximum()<=0)
    {
        paint.drawPixmap(0,0,backgroundLeft.width(),    backgroundLeft.height(),    backgroundLeft);
        paint.drawPixmap(backgroundLeft.width(),        0,
                         width()-backgroundLeft.width()-backgroundRight.width(),    backgroundLeft.height(),backgroundMiddle);
        paint.drawPixmap(width()-backgroundRight.width(),0,                         backgroundRight.width(),    backgroundRight.height(),backgroundRight);
        return;
    }

    int size=width()-barLeft.width()-barRight.width();
    int inpixel=value()*size/maximum();

    paint.drawPixmap(0,0,backgroundLeft.width(),    backgroundLeft.height(),    backgroundLeft);
    paint.drawPixmap(0,0,barLeft.width(),           barLeft.height(),           barLeft);

    paint.drawPixmap(backgroundLeft.width(),        0,
                     width()-backgroundLeft.width()-backgroundRight.width(),    backgroundLeft.height(),backgroundMiddle);
    paint.drawPixmap(barLeft.width(),               0,
                     inpixel,                  barLeft.height(),barMiddle);

    paint.drawPixmap(width()-backgroundRight.width(),0,                         backgroundRight.width(),    backgroundRight.height(),backgroundRight);
    paint.drawPixmap(barLeft.width()+inpixel,          0,                          barRight.width(),           barRight.height(),barRight);
}
