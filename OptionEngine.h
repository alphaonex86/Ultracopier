/** \file OptionEngine.h
\brief Define the class of the option engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef OPTION_ENGINE_H
#define OPTION_ENGINE_H

#include <QDialog>
#include <QList>
#include <QSettings>
#include <QFormLayout>
#include <QLayout>
#include <QStringList>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QAbstractButton>
#include <QTimer>
#include <QWidget>
#include <QHash>
#include <unordered_map>

#include "interface/OptionInterface.h"

#include "Environment.h"
#include "ResourcesManager.h"

/** \brief To store the options

  That's allow to have mutualised way to store the options. Then the plugins just keep Ultracopier manage it, the portable version will store on the disk near the application, and the normal version will keep at the normal location.
  That's allow to have cache and buffer to not slow down Ultracopier when it's doing heavy copy/move.
*/
class OptionEngine : public QObject
{
    Q_OBJECT
    //class OptionEngine : public OptionInterface, public QDialog, public Singleton<OptionEngine>
    public:
        /// \brief Initiate the option, load from backend
        OptionEngine();
        /// \brief Destroy the option
        ~OptionEngine();
        /// \brief To add option group to options
        bool addOptionGroup(const std::string &groupName,const std::vector<std::pair<std::string, std::string> > &KeysList);
        /// \brief To remove option group to options, remove the widget need be do into the calling object
        bool removeOptionGroup(const std::string &groupName);
        /// \brief To get option value
        std::string getOptionValue(const std::string &groupName,const std::string &variableName) const;
        /// \brief To set option value
        void setOptionValue(const std::string &groupName,const std::string &variableName,const std::string &value);
        /// \brief To invalid option value
        //void setInvalidOptionValue(const QString &groupName,const QString &variableName);
        /// \brief get query reset options
        void queryResetOptions();
    private:
        /// \brief OptionEngineGroupKey then: Group -> Key
        struct OptionEngineGroupKey
        {
            std::string defaultValue;
            std::string currentValue;
            bool emptyList;
        };

        /// \brief store the option group list
        std::unordered_map<std::string,std::unordered_map<std::string,OptionEngineGroupKey> > GroupKeysList;
        std::vector<std::string> unmanagedTabName;
        /// \brief Enumeration of backend
        enum Backend
        {
            Memory,		//Do intensive usage of memory, used only if the file backend is not available
            File		//Store all directly into file
        };
        /// \brief The current backend
        Backend currentBackend;
        /// \brief To store QSettings for the backend
        QSettings *settings;
        //the reset of right value of widget need be do into the calling object
        void internal_resetToDefaultValue();
    signals:
        void newOptionValue(const std::string&,const std::string&,const std::string&) const;
        void resetOptions() const;
    public:
        static OptionEngine *optionEngine;
};

#endif // OPTION_ENGINE_H
