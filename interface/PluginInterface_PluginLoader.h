/** \file PluginInterface_PluginLoader.h
\brief Define the interface of the plugin of type: plugin loader
\author alpha_one_x86
\version 0.3
\date 2010 */ 

#ifndef PLUGININTERFACE_PLUGINLOADER_H
#define PLUGININTERFACE_PLUGINLOADER_H

#include <QString>

#include "OptionInterface.h"

#include "../StructEnumDefinition.h"

class PluginInterface_PluginLoader : public QObject
{
	Q_OBJECT
	public:
		virtual void setEnabled(bool) = 0;
		//to set resources, writePath can be empty if read only mode
		virtual void setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion) = 0;
	/* signal to implement
	signals:
		void newState(CatchState);*/
};

Q_DECLARE_INTERFACE(PluginInterface_PluginLoader,"first-world.info.ultracopier.PluginInterface.PluginLoader/0.3.0.1");

#endif // PLUGININTERFACE_PLUGINLOADER_H
