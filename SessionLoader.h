/** \file SessionLoader.h
\brief Define the class to load the plugin and lunch it
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING

This class load ALL plugin compatible to listen and catch the copy/move
*/

#ifndef SESSIONLOADER_H
#define SESSIONLOADER_H

#include <QObject>
#include <QList>
#include <QPluginLoader>
#include <QString>
#include <QStringList>

#include "interface/PluginInterface_SessionLoader.h"
#include "PluginsManager.h"
#include "GlobalClass.h"

/// \todo SessionLoader -> put plugin by plugin loading to add plugin no reload all
/// \todo async the plugin call

/** \brief manage all SessionLoader plugin */
class SessionLoader : public QObject, GlobalClass
{
	Q_OBJECT
	public:
		explicit SessionLoader(QObject *parent = 0);
		~SessionLoader();
	private slots:
		void onePluginAdded(const PluginsAvailable &plugin);
		void onePluginWillBeRemoved(const PluginsAvailable &plugin);
		void newOptionValue(const QString &groupName,const QString &variableName,const QVariant &value);
		#ifdef ULTRACOPIER_DEBUG
		void debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne);
		#endif // ULTRACOPIER_DEBUG
	private:
		//variable
		struct LocalPlugin
		{
			PluginInterface_SessionLoader * sessionLoaderInterface;
			QPluginLoader * pluginLoader;
			QString path;
			LocalPluginOptions *options;
		};
		QList<LocalPlugin> pluginList;
		bool shouldEnabled;
	signals:
		void previouslyPluginAdded(PluginsAvailable);
};

#endif // SESSIONLOADER_H
