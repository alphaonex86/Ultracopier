#ifndef INTERNETUPDATER_H
#define INTERNETUPDATER_H

#include "Environment.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#ifdef ULTRACOPIER_INTERNET_SUPPORT

class InternetUpdater : public QObject
{
    Q_OBJECT
public:
    explicit InternetUpdater(QObject *parent = 0);
signals:
    void newUpdate(const std::string &version) const;
private:
    QTimer newUpdateTimer;
    QTimer firstUpdateTimer;
    QNetworkAccessManager qnam;
    QNetworkReply *reply;
private slots:
    void downloadFile();
    void httpFinished();
};

#endif

#endif // INTERNETUPDATER_H
