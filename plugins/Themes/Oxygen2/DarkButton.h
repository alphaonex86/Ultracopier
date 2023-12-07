#ifndef DarkButton_H
#define DarkButton_H

#include <QPushButton>

class DarkButton : public QPushButton
{
public:
    DarkButton(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *) override;
protected:
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    #endif
private:
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap backgroundPushedLeft,backgroundPushedMiddle,backgroundPushedRight;
    QPixmap overLeft,overMiddle,overRight;
    bool over;
    bool enabled;
};

#endif // PROGRESSBARDARK_H
