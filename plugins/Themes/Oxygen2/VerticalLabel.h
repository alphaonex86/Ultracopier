#ifndef VERTICALLABELCUSTOM_H
#define VERTICALLABELCUSTOM_H

#include <QLabel>

class VerticalLabel : public QLabel
{
public:
    explicit VerticalLabel(QWidget *parent=0);
    explicit VerticalLabel(const QString &text, QWidget *parent=0);
    ~VerticalLabel() override;
    void setColor(QColor color);
protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
private:
    QColor color;
};

#endif // VERTICALLABELCUSTOM_H
