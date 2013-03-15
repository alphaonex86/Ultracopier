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

#include "interface/OptionInterface.h"

#include "Environment.h"
#include "Singleton.h"
#include "ResourcesManager.h"

/** \brief To store the options

  That's allow to have mutualised way to store the options. Then the plugins just keep Ultracopier manage it, the portable version will store on the disk near the application, and the normal version will keep at the normal location.
  That's allow to have cache and buffer to not slow down Ultracopier when it's doing heavy copy/move.
*/
class OptionEngine : public QObject, public Singleton<OptionEngine>
{
    Q_OBJECT
    //class OptionEngine : public OptionInterface, public QDialog, public Singleton<OptionEngine>
    friend class Singleton<OptionEngine>;
    public:
        /// \brief To add option group to options
        bool addOptionGroup(const QString &groupName,const QList<QPair<QString, QVariant> > &KeysList);
        /// \brief To remove option group to options, remove the widget need be do into the calling object
        bool removeOptionGroup(const QString &groupName);
        /// \brief To get option value
        QVariant getOptionValue(const QString &groupName,const QString &variableName) const;
        /// \brief To set option value
        void setOptionValue(const QString &groupName,const QString &variableName,const QVariant &value);
        /// \brief To invalid option value
        //void setInvalidOptionValue(const QString &groupName,const QString &variableName);
        /// \brief get query reset options
        void queryResetOptions();
    private:
        /// \brief Initiate the option, load from backend
        OptionEngine();
        /// \brief Destroy the option
        ~OptionEngine();

        /// \brief OptionEngineGroupKey then: Group -> Key
        struct OptionEngineGroupKey
        {
            QVariant defaultValue;
            QVariant currentValue;
            bool emptyList;
        };

        /// \brief store the option group list
        QHash<QString,QHash<QString,OptionEngineGroupKey> > GroupKeysList;
        QStringList unmanagedTabName;
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
        #ifdef ULTRACOPIER_VERSION_PORTABLE
        ResourcesManager *resources;
        #endif // ULTRACOPIER_VERSION_PORTABLE
        //the reset of right value of widget need be do into the calling object
        void internal_resetToDefaultValue();
        //temp variable
        int loop_size,loop_sub_size,indexGroup,indexGroupKey,index;
    signals:
        void newOptionValue(const QString&,const QString&,const QVariant&);
        void resetOptions();
};

#endif // OPTION_ENGINE_H
