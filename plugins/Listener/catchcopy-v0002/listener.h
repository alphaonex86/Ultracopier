/** \file listener.h
\brief Define the server compatible with Ultracopier interface
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef SERVER_H
#define SERVER_H

#include <QString>
#include <QtPlugin>

#include "Environment.h"
#include "../../../interface/PluginInterface_Listener.h"
#include "catchcopy-api-0002/ServerCatchcopy.h"

/// \brief Define the server compatible with Ultracopier interface
class CatchCopyPlugin : public PluginInterface_Listener
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.Listener/0.4.0.0" FILE "plugin.json")
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
	void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion);
	/// \brief to get the options widget, NULL if not have
	QWidget * options();
public slots:
	/// \brief say to the client that's the copy/move is finished
	void transferFinished(const quint32 &orderId,const bool &withError);
	/// \brief say to the client that's the copy/move is finished
	void transferCanceled(const quint32 &orderId);
	/// \brief to reload the translation, because the new language have been loaded
	void newLanguageLoaded();
private:
	ServerCatchcopy server;
private slots:
	void error(QString error);
	void clientName(quint32 client,QString name);
};

#endif // SERVER_H
