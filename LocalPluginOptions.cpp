/** \file LocalPluginOptions.cpp
\brief To bind the options of the plugin, into unique group options
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "LocalPluginOptions.h"

LocalPluginOptions::LocalPluginOptions(const QString &group)
{
	ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start(\""+group+"\",[...])");
	groupOptionAdded=false;
	this->group=group;
	options=OptionEngine::getInstance();
	connect(options,&OptionEngine::resetOptions,this,&OptionInterface::resetOptions);
}

LocalPluginOptions::~LocalPluginOptions()
{
	ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start(\""+group+"\",[...])");
	if(groupOptionAdded)
		options->removeOptionGroup(group);
	else
		ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"option not used");
	OptionEngine::destroyInstanceAtTheLastCall();
}

/// \brief To add option group to options
bool LocalPluginOptions::addOptionGroup(const QList<QPair<QString, QVariant> > &KeysList)
{
	ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start(\""+group+"\",[...])");
	if(groupOptionAdded)
	{
		ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Group already added!");
		return false;
	}
	else
	{
		groupOptionAdded=true;
		return options->addOptionGroup(group,KeysList);
	}
}

/// \brief To get option value
QVariant LocalPluginOptions::getOptionValue(const QString &variableName)
{
	return options->getOptionValue(group,variableName);
}

/// \brief To set option value
void LocalPluginOptions::setOptionValue(const QString &variableName,const QVariant &value)
{
	options->setOptionValue(group,variableName,value);
}

/*-> disabled because the value will not externaly changed, then useless notification
void LocalPluginOptions::newOptionValue(QString group,QString variable,QVariant value)
{
	if(group==this->group)
		emit newOptionValue(variable,value);
}*/

