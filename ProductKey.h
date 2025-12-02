#ifndef PRODUCTKEY_H
#define PRODUCTKEY_H

#include <QDialog>

#include "Variable.h"
#ifdef ULTRACOPIER_INTERNET_SUPPORT
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#endif

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
    bool isIllegal() const;
    static ProductKey *productKey;
    bool parseKey(QString orgkey=QString());
private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
private:
    Ui::ProductKey *ui;
    bool ultimate;
    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    QString lastKey;
    QString bannedList;
    QTimer newUpdateTimer;
    QTimer firstUpdateTimer;
    QNetworkAccessManager *qnam;//destroy to close connection
    QNetworkReply *reply;
    #endif
signals:
    void changeToUltimate();
    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    void bannedKey();
private slots:
    void checkUpdate();
    void downloadFile();
    void httpFinished();
    void downloadFileInternal();
    #endif
};

#endif // PRODUCTKEY_H
