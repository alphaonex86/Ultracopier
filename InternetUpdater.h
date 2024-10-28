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
    ~InternetUpdater();
    void checkUpdate();
signals:
    void newUpdate(const std::string &version) const;
    void errorUpdate(const std::string &errorString) const;
    void noNewUpdate() const;
private:
    QTimer newUpdateTimer;
    QTimer firstUpdateTimer;
    QNetworkAccessManager *qnam;//destroy to close connection
    QNetworkReply *reply;

    void downloadFileInternal(const bool force=false);
private slots:
    void downloadFile();
    void httpFinished();
};

#endif

#endif // INTERNETUPDATER_H
