#include "InternetUpdater.h"
#include "EventDispatcher.h"
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
    firstUpdateTimer.start(1000*60);
}

void InternetUpdater::downloadFile()
{
    if(!OptionEngine::optionEngine->getOptionValue("Ultracopier","checkTheUpdate").toBool())
        return;
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
        QString name="Supercopier";
    #else
        QString name="Ultracopier";
    #endif
    QString ultracopierVersion;
    #ifdef ULTRACOPIER_VERSION_ULTIMATE
    ultracopierVersion=QString("%1 Ultimate/%2").arg(name).arg(ULTRACOPIER_VERSION);
    #else
    ultracopierVersion=QString("%1/%2").arg(name).arg(ULTRACOPIER_VERSION);
    #endif
    #ifdef ULTRACOPIER_VERSION_PORTABLE
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
             ultracopierVersion+=QString(" portable/all-in-one");
        #else
             ultracopierVersion+=QString(" portable");
        #endif
    #else
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
            ultracopierVersion+=QString(" all-in-one");
        #endif
    #endif
    #ifdef ULTRACOPIER_CGMINER
            ultracopierVersion+=QString(" miner");
    #endif
    #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
    ultracopierVersion+=QString(" (OS: %1)").arg(EventDispatcher::GetOSDisplayString());
    #endif
    QNetworkRequest networkRequest(QString("%1?platform=%2").arg(ULTRACOPIER_UPDATER_URL).arg(ULTRACOPIER_PLATFORM_CODE));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,ultracopierVersion);
    reply = qnam.get(networkRequest);
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
