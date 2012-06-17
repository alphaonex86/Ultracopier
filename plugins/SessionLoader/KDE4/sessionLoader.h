/** \file sessionLoader.h
\brief Define the session loader
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef SESSION_LOADER_PLUGIN_H
#define SESSION_LOADER_PLUGIN_H

#include <QObject>
#include "Environment.h"
#include "../../../interface/PluginInterface_SessionLoader.h"

/// \brief Define the session loader
class SessionLoader : public PluginInterface_SessionLoader
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_SessionLoader)
public:
	/// \brief to set if it's enabled or not
	void setEnabled(bool);
	/// \brief to get if is enabled
	bool getEnabled();
	/// \brief set the resources for the plugins
	void setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion);
	/// \brief to get the options widget, NULL if not have
	QWidget * options();
public slots:
	/// \brief to reload the translation, because the new language have been loaded
	void newLanguageLoaded();
signals:
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	#endif
};

#endif // SESSION_LOADER_PLUGIN_H
