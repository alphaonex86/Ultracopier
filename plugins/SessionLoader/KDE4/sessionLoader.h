/** \file session-loader.h
\brief Define the session plugin loader test
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef SESSION_LOADER_PLUGIN_H
#define SESSION_LOADER_PLUGIN_H

#include <QObject>
#include "Environment.h"
#include "interface/PluginInterface_SessionLoader.h"

class SessionLoaderPlugin : public PluginInterface_SessionLoader
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_SessionLoader)
public:
	void setEnabled(bool);
	bool getEnabled();
	void setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion);
signals:
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	#endif
};

#endif // SESSION_LOADER_PLUGIN_H
