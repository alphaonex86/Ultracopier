#include "DarkButton.h"
#include <QPainter>

DarkButton::DarkButton(QWidget *parent) :
    QPushButton(parent)
{
    setMinimumHeight(36);
    setMaximumHeight(36);
    setStyleSheet("border:none;color:#afb;");
}

void DarkButton::paintEvent(QPaintEvent * event)
{
    if(backgroundLeft.isNull() || backgroundLeft.height()!=height())
    {
        QPixmap background(":/darkButton.png");
        QPixmap backgroundPushed(":/darkButtonPushed.png");
        if(height()==background.height())
        {
            backgroundLeft=background.copy(0,0,10,36);
            backgroundMiddle=background.copy(10,0,46,36);
            backgroundRight=background.copy(56,0,10,36);
            backgroundPushedLeft=backgroundPushed.copy(0,0,10,36);
            backgroundPushedMiddle=backgroundPushed.copy(10,0,46,36);
            backgroundPushedRight=backgroundPushed.copy(56,0,10,36);
        }
        else
        {
            backgroundLeft=background.copy(0,0,10,36).scaledToHeight(height(),Qt::SmoothTransformation);
            backgroundMiddle=background.copy(10,0,46,36).scaledToHeight(height(),Qt::SmoothTransformation);
            backgroundRight=background.copy(56,0,10,36).scaledToHeight(height(),Qt::SmoothTransformation);
            backgroundPushedLeft=backgroundPushed.copy(0,0,10,36).scaledToHeight(height(),Qt::SmoothTransformation);
            backgroundPushedMiddle=backgroundPushed.copy(10,0,46,36).scaledToHeight(height(),Qt::SmoothTransformation);
            backgroundPushedRight=backgroundPushed.copy(56,0,10,36).scaledToHeight(height(),Qt::SmoothTransformation);
        }
    }
    QPainter paint;
    paint.begin(this);

    if(isDown())
    {
        paint.drawPixmap(0,0,backgroundPushedLeft.width(),    backgroundPushedLeft.height(),    backgroundPushedLeft);
        paint.drawPixmap(backgroundPushedLeft.width(),        0,
                         width()-backgroundPushedLeft.width()-backgroundPushedRight.width(),    backgroundPushedLeft.height(),backgroundPushedMiddle);
        paint.drawPixmap(width()-backgroundPushedRight.width(),0,                         backgroundPushedRight.width(),    backgroundPushedRight.height(),backgroundPushedRight);
    }
    else
    {
        paint.drawPixmap(0,0,backgroundLeft.width(),    backgroundLeft.height(),    backgroundLeft);
        paint.drawPixmap(backgroundLeft.width(),        0,
                         width()-backgroundLeft.width()-backgroundRight.width(),    backgroundLeft.height(),backgroundMiddle);
        paint.drawPixmap(width()-backgroundRight.width(),0,                         backgroundRight.width(),    backgroundRight.height(),backgroundRight);
    }
    QPushButton::paintEvent(event);
}
