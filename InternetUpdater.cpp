#include "InternetUpdater.h"
#include "OptionEngine.h"

#ifdef ULTRACOPIER_INTERNET_SUPPORT

#include <QNetworkRequest>
#include <QUrl>

InternetUpdater::InternetUpdater(QObject *parent) :
    QObject(parent)
{
    connect(&newUpdateTimer,&QTimer::timeout,this,&InternetUpdater::downloadFile);
    connect(&firstUpdateTimer,&QTimer::timeout,this,&InternetUpdater::downloadFile);
    newUpdateTimer.start(1000*3600);
    firstUpdateTimer.setSingleShot(true);
    firstUpdateTimer.start(1000*360);
}

void InternetUpdater::downloadFile()
{
    if(!OptionEngine::optionEngine->getOptionValue("Ultracopier","checkTheUpdate").toBool())
        return;
    reply = qnam.get(QNetworkRequest(QString("%1?platform=%2").arg(ULTRACOPIER_UPDATER_URL).arg(ULTRACOPIER_PLATFORM_CODE)));
    connect(reply, &QNetworkReply::finished, this, &InternetUpdater::httpFinished);
}

void InternetUpdater::httpFinished()
{
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (reply->error())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("get the new update failed: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    } else if (!redirectionTarget.isNull()) {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        reply->deleteLater();
        return;
    }
    QString newVersion=QString::fromUtf8(reply->readAll());
    if(newVersion==ULTRACOPIER_VERSION)
        return;
    emit newUpdate(newVersion);
    reply->deleteLater();
}

#endif
