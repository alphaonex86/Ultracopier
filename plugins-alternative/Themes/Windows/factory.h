/** \file factory.h
\brief Define the factory, to create instance of the interface
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef FACTORY_H
#define FACTORY_H

#include <QObject>
#include <QWidget>

#include "interface.h"
#include "../../../interface/PluginInterface_Themes.h"

/// \brief Define the factory, to create instance of the interface
class Factory : public PluginInterface_ThemesFactory
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.ThemesFactory/0.4.0.0" FILE "plugin.json")
	Q_INTERFACES(PluginInterface_ThemesFactory)
	public:
		/// \brief to return the instance of the copy engine
		PluginInterface_Themes * getInstance();
		/// \brief set the resources, to store options, to have facilityInterface
		void setResources(OptionInterface * optionsEngine,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityEngine,const bool &portableVersion);
		/// \brief to get the default options widget
		QWidget * options();
		/// \brief to get a resource icon
		QIcon getIcon(const QString &fileName);
	signals:
		void reloadLanguage();
	public slots:
		void resetOptions();
		void newLanguageLoaded();
	private:
		FacilityInterface * facilityEngine;
};

#endif // FACTORY_H
