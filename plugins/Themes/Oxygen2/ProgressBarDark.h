#ifndef PROGRESSBARDARK_H
#define PROGRESSBARDARK_H

#include <QProgressBar>

class ProgressBarDark : public QProgressBar
{
public:
    ProgressBarDark(QWidget *parent = nullptr);
    ~ProgressBarDark();
    void paintEvent(QPaintEvent *) override;
private:
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap barLeft,barMiddle,barRight;
};

#endif // PROGRESSBARDARK_H
