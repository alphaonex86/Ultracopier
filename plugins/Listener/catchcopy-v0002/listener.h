/** \file server.h
\brief Define the server to listen catchcopy in protocol v0002
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include "Environment.h"
#include "../../../interface/PluginInterface_Listener.h"
#include "catchcopy-api-0002/ServerCatchcopy.h"

class CatchCopyPlugin : public PluginInterface_Listen
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_Listen)
public:
	CatchCopyPlugin();
	void listen();
	void close();
	const QString errorString();
	void setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion);
public slots:
	void copyFinished(quint32 orderId,bool withError);
	void copyCanceled(quint32 orderId);
private:
	ServerCatchcopy server;
private slots:
	void error(QString error);
	void clientName(quint32 client,QString name);
signals:
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	#endif
	void newState(ListeningState state);
	void newCopy(quint32 orderId,QStringList sources);
	void newCopy(quint32 orderId,QStringList sources,QString destination);
	void newMove(quint32 orderId,QStringList sources);
	void newMove(quint32 orderId,QStringList sources,QString destination);
};

#endif // SERVER_H
