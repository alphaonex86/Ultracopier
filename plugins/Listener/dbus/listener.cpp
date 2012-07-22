#include <QtCore>

#include "listener.h"

CatchCopyPlugin::CatchCopyPlugin()
{
	connect(&catchcopy,&Catchcopy::newCopy,	this,&CatchCopyPlugin::newCopy);
	connect(&catchcopy,&Catchcopy::newMove,	this,&CatchCopyPlugin::newMove);
}

void CatchCopyPlugin::listen()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if (!QDBusConnection::sessionBus().isConnected())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
		emit newState(NotListening);
		return;
	}
	if (!QDBusConnection::sessionBus().registerService("info.first-world.catchcopy"))
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QDBusConnection::sessionBus().lastError().message());
		emit newState(NotListening);
		return;
	}
	if(!QDBusConnection::sessionBus().registerObject("/", &catchcopy, QDBusConnection::ExportAllSlots))
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QDBusConnection::sessionBus().lastError().message());
		emit newState(NotListening);
		return;
	}
	emit newState(FullListening);
}

void CatchCopyPlugin::close()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QDBusConnection::sessionBus().unregisterObject("/");
	QDBusConnection::sessionBus().unregisterService("info.first-world.catchcopy");
	emit newState(NotListening);
}

const QString CatchCopyPlugin::errorString()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	return "Unknow error";
}

void CatchCopyPlugin::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion)
{
	Q_UNUSED(options);
	Q_UNUSED(writePath);
	Q_UNUSED(pluginPath);
	Q_UNUSED(portableVersion);
}

/// \brief to get the options widget, NULL if not have
QWidget * CatchCopyPlugin::options()
{
	return NULL;
}

void CatchCopyPlugin::transferFinished(const quint32 &orderId,const bool &withError)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, orderId: "+QString::number(orderId)+", withError: "+QString::number(withError));
}

void CatchCopyPlugin::transferCanceled(const quint32 &orderId)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, orderId: "+QString::number(orderId));
}

/// \brief to reload the translation, because the new language have been loaded
void CatchCopyPlugin::newLanguageLoaded()
{
}

void CatchCopyPlugin::error(QString error)
{
	Q_UNUSED(error);
}

void CatchCopyPlugin::clientName(quint32 client,QString name)
{
	Q_UNUSED(client);
	Q_UNUSED(name);
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,QString("clientName: %1, for the id: %2").arg(name).arg(client));
}
