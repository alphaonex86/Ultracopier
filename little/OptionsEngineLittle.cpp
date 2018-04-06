#include "OptionsEngineLittle.h"

OptionsEngineLittle::OptionsEngineLittle()
{
}

/// \brief To add option group to options
bool OptionsEngineLittle::addOptionGroup(const std::vector<std::pair<std::string, std::string> > &KeysList)
{
    unsigned index=0;
    while(index<KeysList.size())
    {
        const std::pair<std::string, std::string> &pair=KeysList.at(index);
        options[pair.first]=pair.second;
        index++;
    }
    return true;
}

/// \brief To get option value
std::string OptionsEngineLittle::getOptionValue(const std::string &variableName) const
{
    if(options.find(variableName)==options.cend())
        return std::string();
    return options.at(variableName);
}

/// \brief To set option value
void OptionsEngineLittle::setOptionValue(const std::string &variableName,const std::string &value)
{
    if(options.find(variableName)==options.cend())
        return;
    options[variableName]=value;
}

