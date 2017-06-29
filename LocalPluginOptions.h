/** \file LocalPluginOptions.h
\brief To bind the options of the plugin, into unique group options
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef LOCALPLUGINOPTIONS_H
#define LOCALPLUGINOPTIONS_H

#include <QObject>

#include "interface/OptionInterface.h"
#include "OptionEngine.h"

/** \brief To store the options

  That's allow to have mutualised way to store the options. Then the plugins just keep Ultracopier manage it, the portable version will store on the disk near the application, and the normal version will keep at the normal location.

  \see OptionEngine::OptionEngine()
*/
class LocalPluginOptions : public OptionInterface
{
    Q_OBJECT
public:
    explicit LocalPluginOptions(const std::string &group);
    ~LocalPluginOptions();
    /// \brief To add option group to options
    bool addOptionGroup(const std::vector<std::pair<std::string, std::string> > &KeysList);
    /*/// \brief To remove option group to options, removed to the load plugin
    bool removeOptionGroup();*/
    /// \brief To get option value
    std::string getOptionValue(const std::string &variableName) const;
    /// \brief To set option value
    void setOptionValue(const std::string &variableName,const std::string &value);
protected:
    //for the options
    QString group;
    bool groupOptionAdded;
/*public slots:-> disabled because the value will not externaly changed, then useless notification
    void newOptionValue(QString group,QString variable,QVariant value);*/
};

#endif // LOCALPLUGINOPTIONS_H
