/** \file LocalPluginOptions.cpp
\brief To bind the options of the plugin, into unique group options
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "LocalPluginOptions.h"

LocalPluginOptions::LocalPluginOptions(const std::string &group)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start(\""+group+"\",[...])");
    groupOptionAdded=false;
    this->group=group;
    connect(OptionEngine::optionEngine,&OptionEngine::resetOptions,this,&OptionInterface::resetOptions);
}

LocalPluginOptions::~LocalPluginOptions()
{
    if(groupOptionAdded)
    {
        if(OptionEngine::optionEngine!=NULL)
            OptionEngine::optionEngine->removeOptionGroup(group);
    }
}

/// \brief To add option group to options
bool LocalPluginOptions::addOptionGroup(const std::vector<std::pair<std::string, std::string> > &KeysList)
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
        return OptionEngine::optionEngine->addOptionGroup(group,KeysList);
    }
}

/// \brief To get option value
std::string LocalPluginOptions::getOptionValue(const std::string &variableName) const
{
    return OptionEngine::optionEngine->getOptionValue(group,variableName);
}

/// \brief To set option value
void LocalPluginOptions::setOptionValue(const std::string &variableName,const std::string &value)
{
    OptionEngine::optionEngine->setOptionValue(group,variableName,value);
}

/*-> disabled because the value will not externaly changed, then useless notification
void LocalPluginOptions::newOptionValue(QString group,QString variable,QVariant value)
{
    if(group==this->group)
        emit newOptionValue(variable,value);
}*/

