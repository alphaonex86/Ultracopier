#ifndef PRODUCTKEY_H
#define PRODUCTKEY_H

#include <QDialog>

namespace Ui {
class ProductKey;
}

class ProductKey : public QDialog
{
    Q_OBJECT

public:
    explicit ProductKey(QWidget *parent = 0);
    ~ProductKey();
    bool isUltimate() const;
    static ProductKey *productKey;
    bool parseKey();
private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::ProductKey *ui;
    bool ultimate;
};

#endif // PRODUCTKEY_H
