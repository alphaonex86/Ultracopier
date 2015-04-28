/** \file OptionEngine.cpp
\brief Define the class of the event dispatcher
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QFileInfo>
#include <QDir>
#include <QPluginLoader>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QMessageBox>

#include "OptionEngine.h"

/// \todo async the options write

/// \brief Initiate the option, load from backend
OptionEngine::OptionEngine()
{
    //locate the settings
    #ifdef ULTRACOPIER_VERSION_PORTABLE
        QString settingsFilePath=ResourcesManager::resourcesManager->getWritablePath();
        if(settingsFilePath!="")
            settings = new QSettings(settingsFilePath+QStringLiteral("Ultracopier.conf"),QSettings::IniFormat);
        else
            settings = NULL;
    #else // ULTRACOPIER_VERSION_PORTABLE
        settings = new QSettings(QStringLiteral("Ultracopier"),QStringLiteral("Ultracopier"));
    #endif // ULTRACOPIER_VERSION_PORTABLE
    if(settings!=NULL)
    {
        //do some write test
        if(settings->status()!=QSettings::NoError)
        {
            delete settings;
            settings=NULL;
        }
        else if(!settings->isWritable())
        {
            delete settings;
            settings=NULL;
        }
        else
        {
            settings->setValue(QStringLiteral("test"),QStringLiteral("test"));
            if(settings->status()!=QSettings::NoError)
            {
                delete settings;
                settings=NULL;
            }
            else
            {
                settings->remove(QStringLiteral("test"));
                if(settings->status()!=QSettings::NoError)
                {
                    delete settings;
                    settings=NULL;
                }
            }
        }
    }
    //set the backend
    if(settings==NULL)
    {
        #ifdef ULTRACOPIER_VERSION_PORTABLE
        ResourcesManager::resourcesManager->disableWritablePath();
        #endif // ULTRACOPIER_VERSION_PORTABLE
        currentBackend=Memory;
    }
    else
        currentBackend=File;
}

/// \brief Destroy the option
OptionEngine::~OptionEngine()
{
}

/// \brief To add option group to options
bool OptionEngine::addOptionGroup(const QString &groupName,const QList<QPair<QString, QVariant> > &KeysList)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start(\"")+groupName+QStringLiteral("\",[...])"));
    //search if previous with the same name exists
    if(GroupKeysList.contains(groupName))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("group already used previously"));
        return false;
    }
    //if the backend is file, enter into the group
    if(currentBackend==File)
        settings->beginGroup(groupName);
    //browse all key, and append it to the key
    int index=0;
    //QList<OptionEngineGroupKey> KeyListTemp;
    const int &loop_size=KeysList.size();
    while(index<loop_size)
    {
        OptionEngineGroupKey theCurrentKey;
        theCurrentKey.defaultValue=KeysList.at(index).second;
        //if memory backend, load the default value into the current value
        if(currentBackend==Memory)
            theCurrentKey.currentValue=theCurrentKey.defaultValue;
        else
        {
            if(settings->contains(KeysList.at(index).first))//if file backend, load the default value from the file
            {
                theCurrentKey.currentValue=settings->value(KeysList.at(index).first);
                #ifdef ULTRACOPIER_DEBUG
                if(theCurrentKey.currentValue!=theCurrentKey.defaultValue)
                {
                    #ifdef ULTRACOPIER_VERSION_ULTIMATE
                    if(groupName=="Ultracopier" && KeysList.at(index).first=="key")
                    {
                    }
                    else
                    #endif
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("The current key: %1, group: %2, have value: %3").arg(groupName).arg(KeysList.at(index).first).arg(theCurrentKey.currentValue.toString()));
                }
                #endif
            }
            else //or if not found load the default value and set into the file
            {
                theCurrentKey.currentValue=theCurrentKey.defaultValue;
                //to switch default value if is unchanged
                //settings->setValue(KeysList.at(index).first,theCurrentKey.defaultValue);
            }
            if(settings->status()!=QSettings::NoError)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Have writing error, switch to memory only options");
                #ifdef ULTRACOPIER_VERSION_PORTABLE
                ResourcesManager::resourcesManager->disableWritablePath();
                #endif // ULTRACOPIER_VERSION_PORTABLE
                currentBackend=Memory;
            }
        }
        GroupKeysList[groupName][KeysList.at(index).first]=theCurrentKey;
        index++;
    }
    //if the backend is file, leave into the group
    if(currentBackend==File)
        settings->endGroup();
    return true;
}

/// \brief To remove option group to options, remove the widget need be do into the calling object
bool OptionEngine::removeOptionGroup(const QString &groupName)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, groupName: ")+groupName);
    if(GroupKeysList.remove(groupName)!=1)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("value not found, internal bug, groupName: ")+groupName);
    return false;
}

/// \brief To get option value
QVariant OptionEngine::getOptionValue(const QString &groupName,const QString &variableName) const
{
    if(GroupKeysList.contains(groupName))
    {
        if(GroupKeysList.value(groupName).contains(variableName))
            return GroupKeysList.value(groupName).value(variableName).currentValue;
        QMessageBox::critical(NULL,"Internal error",tr("The variable was not found: %1 %2").arg(groupName).arg(variableName));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"value not found, internal bug, groupName: "+groupName+", variableName: "+variableName);
        return QVariant();
    }
    QMessageBox::critical(NULL,"Internal error",tr("The variable was not found: %1 %2").arg(groupName).arg(variableName));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("The variable was not found: %1 %2").arg(groupName).arg(variableName));
    //return default value
    return QVariant();
}

/// \brief To set option value
void OptionEngine::setOptionValue(const QString &groupName,const QString &variableName,const QVariant &value)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("groupName: \"")+groupName+QStringLiteral("\", variableName: \"")+variableName+QStringLiteral("\", value: \"")+value.toString()+QStringLiteral("\""));

    if(GroupKeysList.contains(groupName))
    {
        if(GroupKeysList.value(groupName).contains(variableName))
        {
            //prevent re-write the same value into the variable
            if(GroupKeysList.value(groupName).value(variableName).currentValue==value)
                return;
            //write ONLY the new value
            GroupKeysList[groupName][variableName].currentValue=value;
            if(currentBackend==File)
            {
                settings->beginGroup(groupName);
                settings->setValue(variableName,value);
                settings->endGroup();
                if(settings->status()!=QSettings::NoError)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Have writing error, switch to memory only options"));
                    #ifdef ULTRACOPIER_VERSION_PORTABLE
                    ResourcesManager::resourcesManager->disableWritablePath();
                    #endif // ULTRACOPIER_VERSION_PORTABLE
                    currentBackend=Memory;
                }
            }
            emit newOptionValue(groupName,variableName,value);
            return;
        }
        QMessageBox::critical(NULL,"Internal error",tr("The variable was not found: %1 %2").arg(groupName).arg(variableName));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"value not found, internal bug, groupName: "+groupName+", variableName: "+variableName);
        return;
    }
    QMessageBox::critical(NULL,"Internal error",tr("The variable was not found: %1 %2").arg(groupName).arg(variableName));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("The variable was not found: %1 %2").arg(groupName).arg(variableName));
}

//the reset of right value of widget need be do into the calling object
void OptionEngine::internal_resetToDefaultValue()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));

    QHash<QString,QHash<QString,OptionEngineGroupKey> >::const_iterator i = GroupKeysList.constBegin();
    QHash<QString,QHash<QString,OptionEngineGroupKey> >::const_iterator i_end = GroupKeysList.constEnd();
    QHash<QString,OptionEngineGroupKey>::const_iterator j;
    QHash<QString,OptionEngineGroupKey>::const_iterator j_end;
    while (i != i_end)
    {
        j = i.value().constBegin();
        j_end = i.value().constEnd();
        while (j != j_end)
        {
            if(j.value().currentValue!=j.value().defaultValue)
            {
                GroupKeysList[i.key()][j.key()].currentValue=j.value().defaultValue;
                emit newOptionValue(i.key(),j.key(),j.value().currentValue);
            }
            ++j;
        }
        ++i;
    }
}

void OptionEngine::queryResetOptions()
{
    emit resetOptions();
}
