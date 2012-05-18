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
	PluginInterface_Themes * newInterface=new Themes(
				optionsEngine->getOptionValue("checkBoxShowSpeed").toBool(),facilityEngine,optionsEngine->getOptionValue("moreButtonPushed").toBool()
				);
	connect(newInterface,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	connect(this,SIGNAL(reloadLanguage()),newInterface,SLOT(newLanguageLoaded()));
	return newInterface;
}

void Factory::setResources(OptionInterface * optionsEngine,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityEngine,bool portableVersion)
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
		KeysList.append(qMakePair(QString("moreButtonPushed"),QVariant(false)));
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
	{
		ui->checkBoxShowSpeed->setChecked(optionsEngine->getOptionValue("checkBoxShowSpeed").toBool());
		ui->checkBoxStartWithMoreButtonPushed->setChecked(optionsEngine->getOptionValue("moreButtonPushed").toBool());
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"internal error, crash prevented");
	connect(ui->checkBoxShowSpeed,SIGNAL(toggled(bool)),this,SLOT(checkBoxShowSpeedHaveChanged(bool)));
	connect(ui->checkBoxStartWithMoreButtonPushed,SIGNAL(toggled(bool)),this,SLOT(checkBoxStartWithMoreButtonPushedHaveChanged(bool)));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"return the options");
	return tempWidget;
}

QIcon Factory::getIcon(const QString &fileName)
{
	if(fileName=="SystemTrayIcon/exit.png")
	{
		QIcon tempIcon=QIcon::fromTheme("application-exit");
		if(!tempIcon.isNull())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("use substitution ionc for: %1").arg(fileName));
			return tempIcon;
		}
	}
	if(fileName=="SystemTrayIcon/add.png")
	{
		QIcon tempIcon=QIcon::fromTheme("list-add");
		if(!tempIcon.isNull())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("use substitution ionc for: %1").arg(fileName));
			return tempIcon;
		}
	}
	if(fileName=="SystemTrayIcon/informations.png")
	{
		QIcon tempIcon=QIcon::fromTheme("help-about");
		if(!tempIcon.isNull())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("use substitution ionc for: %1").arg(fileName));
			return tempIcon;
		}
	}
	if(fileName=="SystemTrayIcon/options.png")
	{
		QIcon tempIcon=QIcon::fromTheme("applications-system");
		if(!tempIcon.isNull())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("use substitution ionc for: %1").arg(fileName));
			return tempIcon;
		}
	}
	return QIcon(":/resources/"+fileName);
}

void Factory::resetOptions()
{
	ui->checkBoxShowSpeed->setChecked(true);
	ui->checkBoxStartWithMoreButtonPushed->setChecked(false);
}

void Factory::checkBoxShowSpeedHaveChanged(bool toggled)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"the checkbox have changed");
	if(optionsEngine!=NULL)
		optionsEngine->setOptionValue("checkBoxShowSpeed",toggled);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::checkBoxStartWithMoreButtonPushedHaveChanged(bool toggled)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"the checkbox have changed");
	if(optionsEngine!=NULL)
		optionsEngine->setOptionValue("moreButtonPushed",toggled);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::newLanguageLoaded()
{
	ui->retranslateUi(tempWidget);
	emit reloadLanguage();
}

Q_EXPORT_PLUGIN2(interface, Factory);

