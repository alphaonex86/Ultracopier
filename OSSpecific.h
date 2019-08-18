#ifndef OSSPECIFIC_H
#define OSSPECIFIC_H

#include "Environment.h"

#include <QDialog>

namespace Ui {
class OSSpecific;
}

class OSSpecific : public QDialog
{
    Q_OBJECT

public:
    explicit OSSpecific(QWidget *parent = 0);
    ~OSSpecific();
    bool dontShowAgain();
    QString theme();
private slots:
    void on_pushButton_clicked();
    void updateText();
    void on_radioButtonClassic_toggled(bool checked);
    void on_radioButtonModern_toggled(bool checked);
protected slots:
    void changeEvent(QEvent *e);
private:
    Ui::OSSpecific *ui;
};

#endif // OSSPECIFIC_H
