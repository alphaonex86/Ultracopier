#include "listener.h"

Listener::Listener()
{
    connect(&catchcopy,&Catchcopy::newCopy,	this,&Listener::newCopy);
    connect(&catchcopy,&Catchcopy::newMove,	this,&Listener::newMove);
}

void Listener::listen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if (!QDBusConnection::sessionBus().isConnected())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        emit newState(Ultracopier::NotListening);
        return;
    }
    if (!QDBusConnection::sessionBus().registerService("info.first-world.catchcopy"))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QDBusConnection::sessionBus().lastError().message());
        emit newState(Ultracopier::NotListening);
        return;
    }
    if(!QDBusConnection::sessionBus().registerObject("/", &catchcopy, QDBusConnection::ExportAllSlots))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QDBusConnection::sessionBus().lastError().message());
        emit newState(Ultracopier::NotListening);
        return;
    }
    emit newState(Ultracopier::FullListening);
}

void Listener::close()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QDBusConnection::sessionBus().unregisterObject("/");
    QDBusConnection::sessionBus().unregisterService("info.first-world.catchcopy");
    emit newState(Ultracopier::NotListening);
}

const QString Listener::errorString() const
{
    return "Unknow error";
}

void Listener::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion)
{
    Q_UNUSED(options);
    Q_UNUSED(writePath);
    Q_UNUSED(pluginPath);
    Q_UNUSED(portableVersion);
}

/// \brief to get the options widget, NULL if not have
QWidget * Listener::options()
{
    return NULL;
}

void Listener::transferFinished(const quint32 &orderId,const bool &withError)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, orderId: "+QString::number(orderId)+", withError: "+QString::number(withError));
}

void Listener::transferCanceled(const quint32 &orderId)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, orderId: "+QString::number(orderId));
}

/// \brief to reload the translation, because the new language have been loaded
void Listener::newLanguageLoaded()
{
}

void Listener::error(QString error)
{
    Q_UNUSED(error);
}

void Listener::clientName(quint32 client,QString name)
{
    Q_UNUSED(client);
    Q_UNUSED(name);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("clientName: %1, for the id: %2").arg(name).arg(client));
}
