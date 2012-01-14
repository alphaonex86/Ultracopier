/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>

#include "factory.h"

Factory::Factory()
{
	optionsEngine=NULL;
	tempWidget=new QWidget();
	ui=new Ui::options();
	ui->setupUi(tempWidget);
}

Factory::~Factory()
{
}

PluginInterface_Themes * Factory::getInstance()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	PluginInterface_Themes * newInterface=new InterfacePlugin(
				optionsEngine->getOptionValue("checkBoxShowSpeed").toBool(),facilityEngine
				);
	connect(newInterface,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	connect(this,SIGNAL(reloadLanguage()),newInterface,SLOT(newLanguageLoaded()));
	return newInterface;
}

void Factory::setResources(OptionInterface * optionsEngine,QString writePath,QString pluginPath,FacilityInterface * facilityEngine,bool portableVersion)
{
	Q_UNUSED(portableVersion);
	Q_UNUSED(writePath);
	Q_UNUSED(pluginPath);
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath: "+pluginPath);
	this->facilityEngine=facilityEngine;
	if(optionsEngine!=NULL)
	{
		this->optionsEngine=optionsEngine;
		//load the options
		QList<QPair<QString, QVariant> > KeysList;
		KeysList.append(qMakePair(QString("checkBoxShowSpeed"),QVariant(true)));
		optionsEngine->addOptionGroup(KeysList);
		connect(optionsEngine,SIGNAL(resetOptions()),this,SLOT(resetOptions()));
	}
	#ifndef __GNUC__
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"__GNUC__ is set");
	#endif
	#ifndef __GNUC__
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"__GNUC__ is set");
	#endif
}

QWidget * Factory::options()
{
	if(optionsEngine!=NULL)
		ui->checkBoxShowSpeed->setChecked(optionsEngine->getOptionValue("checkBoxShowSpeed").toBool());
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"internal error, crash prevented");
	connect(ui->checkBoxShowSpeed,SIGNAL(toggled(bool)),this,SLOT(checkBoxHaveChanged(bool)));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"return the options");
	return tempWidget;
}

void Factory::resetOptions()
{
}

void Factory::checkBoxHaveChanged(bool toggled)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"the checkbox have changed");
	if(optionsEngine!=NULL)
		optionsEngine->setOptionValue("checkBoxShowSpeed",toggled);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::newLanguageLoaded()
{
	ui->retranslateUi(tempWidget);
	emit reloadLanguage();
}

Q_EXPORT_PLUGIN2(interface, Factory);

