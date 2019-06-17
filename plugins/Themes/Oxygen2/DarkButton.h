#ifndef DarkButton_H
#define DarkButton_H

#include <QPushButton>

class DarkButton : public QPushButton
{
public:
    DarkButton(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
private:
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap backgroundPushedLeft,backgroundPushedMiddle,backgroundPushedRight;
};

#endif // PROGRESSBARDARK_H
