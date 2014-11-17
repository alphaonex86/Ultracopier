#include "KeyBind.h"

KeyBind::KeyBind(QWidget *parent) :
    QLineEdit(parent)
{
}

void KeyBind::keyPressEvent(QKeyEvent * event)
{
    emit newKey(event);
}
