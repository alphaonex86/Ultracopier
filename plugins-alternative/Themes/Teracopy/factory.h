/** \file factory.h
\brief Define the factory
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef FACTORY_H
#define FACTORY_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QCloseEvent>

#include "interface/PluginInterface_Themes.h"
#include "interface.h"
#include "Environment.h"

class Factory : public PluginInterface_ThemesFactory
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_ThemesFactory)
public:
	PluginInterface_Themes * getInstance();
	void setResources(OptionInterface *,QString writePath,QString pluginPath,FacilityInterface * facilityEngine);
	QWidget * options();
public slots:
	void resetOptions();
	void newLanguageLoaded();
signals:
	void reloadLanguage();
private:
	FacilityInterface * facilityEngine;
};

#endif // FACTORY_H
