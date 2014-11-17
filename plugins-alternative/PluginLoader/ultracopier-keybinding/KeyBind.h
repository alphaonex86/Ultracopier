#ifndef KEYBIND_H
#define KEYBIND_H

#include <QLineEdit>

class KeyBind : public QLineEdit
{
    Q_OBJECT
public:
    explicit KeyBind(QWidget *parent = 0);

signals:
    void newKey(QKeyEvent * event);
public slots:
    void keyPressEvent(QKeyEvent * event);
};

#endif // KEYBIND_H
