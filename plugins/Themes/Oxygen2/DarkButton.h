#ifndef DarkButton_H
#define DarkButton_H

#include <QPushButton>

class DarkButton : public QPushButton
{
public:
    DarkButton(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
protected:
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
private:
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap backgroundPushedLeft,backgroundPushedMiddle,backgroundPushedRight;
    QPixmap overLeft,overMiddle,overRight;
    bool over;
    bool enabled;
};

#endif // PROGRESSBARDARK_H
