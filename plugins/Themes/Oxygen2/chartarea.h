#ifndef ChartAreaWIDGET_H
#define ChartAreaWIDGET_H

#include <QResizeEvent>
#include <QWidget>

namespace ChartArea
{

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget* = nullptr);
    ~Widget() override;
    void addValue(uint64_t value);
public Q_SLOTS:
    void invalidate();
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
private:
    std::vector<uint64_t> m_values;
};
}

#endif
