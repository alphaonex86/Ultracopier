/** \file OptionInterface.h
\brief Define the class of the option engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef OPTION_INTERFACE_H
#define OPTION_INTERFACE_H

#include <string>
#include <vector>
#include <utility>

#include "../StructEnumDefinition.h"

/** \brief to pass the options to the plugin, the instance is created into Ultracopier from the class LocalPluginOptions()
 * \see LocalPluginOptions()
 * **/
class OptionInterface : public QObject
{
    Q_OBJECT
    public:
        /// \brief To add option group to options
        virtual bool addOptionGroup(const std::vector<std::pair<std::string, std::string> > &KeysList) = 0;
        /*/// \brief To remove option group to options, removed to the load plugin
        virtual bool removeOptionGroup() = 0;*/
        /// \brief To get option value
        virtual std::string getOptionValue(const std::string &variableName) const = 0;
        /// \brief To set option value
        virtual void setOptionValue(const std::string &variableName,const std::string &value) = 0;
    signals:
        //void newOptionValue(std::string,std::string);-> disabled because the value will not externally changed, then useless notification
        void resetOptions() const;
};

#endif // OPTION_INTERFACE_H
