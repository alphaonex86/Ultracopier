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

/** \brief To define the interface between Ultracopier and the plugin loader
 * */
class PluginInterface_PluginLoader : public QObject
{
	Q_OBJECT
	public:
		/// \brief try enable/disable the catching
		virtual void setEnabled(bool) = 0;
		/// \brief to set resources, writePath can be empty if read only mode
		virtual void setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion) = 0;
		/// \brief to get the options widget, NULL if not have
		virtual QWidget * options() = 0;
	public slots:
		/// \brief to reload the translation, because the new language have been loaded
		virtual void newLanguageLoaded() = 0;
	/* signal to implement
	signals:
		void newState(CatchState);*/
};

Q_DECLARE_INTERFACE(PluginInterface_PluginLoader,"first-world.info.ultracopier.PluginInterface.PluginLoader/0.3.0.8");

#endif // PLUGININTERFACE_PLUGINLOADER_H
