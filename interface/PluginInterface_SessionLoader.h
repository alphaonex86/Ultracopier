/** \file PluginInterface_SessionLoader.h
\brief Define the interface of the plugin of type: session loader
\author alpha_one_x86
\version 0.3
\date 2010 */ 

#ifndef PLUGININTERFACE_SESSIONLOADER_H
#define PLUGININTERFACE_SESSIONLOADER_H

#include <QString>

#include "OptionInterface.h"

#include "../StructEnumDefinition.h"

class PluginInterface_SessionLoader : public QObject
{
	Q_OBJECT
	public:
		virtual void setEnabled(bool) = 0;
		virtual bool getEnabled() = 0;
		virtual void setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion) = 0;
};

Q_DECLARE_INTERFACE(PluginInterface_SessionLoader,"first-world.info.ultracopier.PluginInterface.SessionLoader/0.3.0.1");

#endif // PLUGININTERFACE_SESSIONLOADER_H
