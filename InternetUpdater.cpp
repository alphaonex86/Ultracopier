#include "InternetUpdater.h"
#include "EventDispatcher.h"
#include "OptionEngine.h"

#ifdef ULTRACOPIER_INTERNET_SUPPORT

#include <QNetworkRequest>
#include <QUrl>

#include "PluginsManager.h"

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
    if(!OptionEngine::optionEngine->getOptionValue(QStringLiteral("Ultracopier"),QStringLiteral("checkTheUpdate")).toBool())
        return;
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
        QString name=QStringLiteral("Supercopier");
    #else
        QString name=QStringLiteral("Ultracopier");
    #endif
    QString ultracopierVersion;
    #ifdef ULTRACOPIER_VERSION_ULTIMATE
    ultracopierVersion=QStringLiteral("%1 Ultimate/%2").arg(name).arg(ULTRACOPIER_VERSION);
    #else
    ultracopierVersion=QStringLiteral("%1/%2").arg(name).arg(ULTRACOPIER_VERSION);
    #endif
    #ifdef ULTRACOPIER_VERSION_PORTABLE
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
             ultracopierVersion+=QStringLiteral(" portable/all-in-one");
        #else
             ultracopierVersion+=QStringLiteral(" portable");
        #endif
    #else
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
            ultracopierVersion+=QStringLiteral(" all-in-one");
        #endif
    #endif
    #ifdef ULTRACOPIER_CGMINER
            ultracopierVersion+=QStringLiteral(" miner");
    #endif
    #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
    ultracopierVersion+=QStringLiteral(" (OS: %1)").arg(EventDispatcher::GetOSDisplayString());
    #endif
    QNetworkRequest networkRequest(QStringLiteral("%1?platform=%2").arg(ULTRACOPIER_UPDATER_URL).arg(ULTRACOPIER_PLATFORM_CODE));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,ultracopierVersion);
    reply = qnam.get(networkRequest);
    connect(reply, &QNetworkReply::finished, this, &InternetUpdater::httpFinished);
}

void InternetUpdater::httpFinished()
{
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!reply->isFinished())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("get the new update failed: not finished"));
        reply->deleteLater();
        return;
    }
    else if (reply->error())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("get the new update failed: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    } else if (!redirectionTarget.isNull()) {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        reply->deleteLater();
        return;
    }
    const QString &newVersion=QString::fromUtf8(reply->readAll());
    if(newVersion.isEmpty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("version string is empty"));
        reply->deleteLater();
        return;
    }
    if(!newVersion.contains(QRegularExpression(QLatin1Literal("^[0-9]+(\\.[0-9]+)+$"))))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("version string don't match: %1").arg(newVersion));
        reply->deleteLater();
        return;
    }
    if(newVersion==ULTRACOPIER_VERSION)
    {
        reply->deleteLater();
        return;
    }
    if(PluginsManager::compareVersion(newVersion,QStringLiteral("<="),ULTRACOPIER_VERSION))
    {
        reply->deleteLater();
        return;
    }
    emit newUpdate(newVersion);
    reply->deleteLater();
}

#endif
