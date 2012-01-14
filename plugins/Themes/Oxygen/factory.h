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
#include "ui_options.h"
#include "interface.h"
#include "Environment.h"

namespace Ui {
	class options;
}

class Factory : public PluginInterface_ThemesFactory
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_ThemesFactory)
public:
	Factory();
	~Factory();
	PluginInterface_Themes * getInstance();
	void setResources(OptionInterface * optionsEngine,QString writePath,QString pluginPath,FacilityInterface * facilityEngine,bool portableVersion);
	QWidget * options();
private slots:
	void checkBoxHaveChanged(bool toggled);
	void resetOptions();
	void newLanguageLoaded();
private:
	OptionInterface * optionsEngine;
	Ui::options *ui;
	QWidget *tempWidget;
	FacilityInterface * facilityEngine;
signals:
	void reloadLanguage();
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	#endif
};

#endif // FACTORY_H
