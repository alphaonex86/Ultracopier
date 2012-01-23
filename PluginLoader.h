/** \file PluginLoader.h
\brief Define the class to load the plugin and lunch it
\author alpha_one_x86
\version 0.3
\date 2010

This class load ALL plugin compatible to listen and catch the copy/move
*/

#ifndef PluginLoader_H
#define PluginLoader_H

#include <QObject>
#include <QList>
#include <QPluginLoader>
#include <QString>
#include <QStringList>

#include "interface/PluginInterface_PluginLoader.h"
#include "PluginsManager.h"
#include "GlobalClass.h"

namespace Ui {
    class PluginLoaderOptions;
}

/// \todo PluginLoader -> put plugin by plugin loading to add plugin no reload all
/// \todo async the plugin call

/** \brief Load the plugin

  It use ResourcesManager(), but it provide more higher abstraction. It parse the plugins information, check it, check the dependancies.

  \see ResourcesManager::ResourcesManager()
  */
class PluginLoader : public QObject, GlobalClass
{
	Q_OBJECT
public:
	explicit PluginLoader(QObject *parent = 0);
	~PluginLoader();
	/** \brief to rended the state */
	void resendState();
	/** \brief should load plugin into file manager if needed */
	void load();
	/** \brief should unload plugin into file manager */
	void unload();
private slots:
	void onePluginAdded(PluginsAvailable plugin);
	void onePluginWillBeRemoved(PluginsAvailable plugin);
	#ifdef ULTRACOPIER_DEBUG
	void debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne);
	#endif // ULTRACOPIER_DEBUG
	void allPluginIsloaded();
	void newState(CatchState state);
private:
	//variable
	struct LocalPlugin
	{
		PluginInterface_PluginLoader * PluginLoaderInterface;
		QPluginLoader * pluginLoader;
		CatchState state;
		QString path;
		bool inWaitOfReply;
		LocalPluginOptions *options;
	};
	QList<LocalPlugin> pluginList;
	bool needEnable;
	//temp variable
	int index,loop_size;
	CatchState last_state;
	bool last_have_plugin,last_inWaitOfReply;
	void sendState(bool force=false);
signals:
	void pluginLoaderReady(CatchState state,bool havePlugin,bool someAreInWaitOfReply);
};

#endif // PluginLoader_H
