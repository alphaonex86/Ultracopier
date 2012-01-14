/** \file factory.h
\brief Define the factory
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef FACTORY_H
#define FACTORY_H

#include <QObject>
#include <QWidget>

#include "interface.h"
#include "interface/PluginInterface_Themes.h"

class Factory : public PluginInterface_ThemesFactory
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_ThemesFactory)
	public:
		PluginInterface_Themes * getInstance();
		//to set resources, writePath can be empty if read only mode
		void setResources(OptionInterface *,QString writePath,QString pluginPath,FacilityInterface * facilityEngine);
		QWidget * options();
	signals:
		void reloadLanguage();
	public slots:
		void resetOptions();
		void newLanguageLoaded();
};

#endif // FACTORY_H
