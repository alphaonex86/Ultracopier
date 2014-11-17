#ifndef OptionsWidget_H
#define OptionsWidget_H

#include <QWidget>
#include "KeyBind.h"

namespace Ui {
class OptionsWidget;
}

class OptionsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OptionsWidget(QWidget *parent = 0);
    ~OptionsWidget();
    void setKeyBind(const QKeySequence &keySequence);
    void retranslate();
private:
    Ui::OptionsWidget *ui;
    KeyBind *keyBind;
private slots:
    void newKey(QKeyEvent * event);
signals:
    void sendKeyBind(const QKeySequence &keySequence);
    void newKeyBind(QKeyEvent * event);
};

#endif // OptionsWidget_H
