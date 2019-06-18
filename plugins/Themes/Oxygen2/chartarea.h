#ifndef ChartAreaWIDGET_H
#define ChartAreaWIDGET_H

#include <QResizeEvent>
#include <QWidget>

#include "../../../interface/FacilityInterface.h"

namespace ChartArea
{

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(FacilityInterface * facilityEngine,QWidget* = nullptr);
    ~Widget() override;
    void addValue(uint64_t value);
public Q_SLOTS:
    void invalidate();
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
private:
    FacilityInterface * facilityEngine;
    std::vector<uint64_t> m_values;
};
}

#endif
