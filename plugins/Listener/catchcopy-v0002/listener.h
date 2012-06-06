/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef SERVER_H
#define SERVER_H

#include <QObject>

#include "Environment.h"
#include "../../../interface/PluginInterface_Listener.h"
#include "catchcopy-api-0002/ServerCatchcopy.h"

/// \brief Define the server compatible with Ultracopier interface
class CatchCopyPlugin : public PluginInterface_Listener
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_Listener)
public:
	CatchCopyPlugin();
	/// \brief try listen the copy/move
	void listen();
	/// \brief stop listen to copy/move
	void close();
	/// \brief return the error strong
	const QString errorString();
	/// \brief set resources for this plugins
	void setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion);
	/// \brief to get the options widget, NULL if not have
	QWidget * options();
public slots:
	/// \brief say to the client that's the copy/move is finished
	void transferFinished(quint32 orderId,bool withError);
	/// \brief say to the client that's the copy/move is finished
	void transferCanceled(quint32 orderId);
	/// \brief to reload the translation, because the new language have been loaded
	void newLanguageLoaded();
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
	/// \brief new state
	void newState(ListeningState state);
	/// \brief new copy is incoming
	void newCopy(quint32 orderId,QStringList sources);
	/// \brief new copy is incoming, with destination
	void newCopy(quint32 orderId,QStringList sources,QString destination);
	/// \brief new move is incoming
	void newMove(quint32 orderId,QStringList sources);
	/// \brief new move is incoming, with destination
	void newMove(quint32 orderId,QStringList sources,QString destination);
};

#endif // SERVER_H
