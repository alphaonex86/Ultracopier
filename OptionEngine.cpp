/** \file OptionEngine.cpp
\brief Define the class of the event dispatcher
\author alpha_one_x86
\version 0.3
\date 2010 */

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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//locate the settings
	#ifdef ULTRACOPIER_VERSION_PORTABLE
		resources=ResourcesManager::getInstance();
		QString settingsFilePath=resources->getWritablePath();
		if(settingsFilePath!="")
			settings = new QSettings(settingsFilePath+"Ultracopier.conf",QSettings::IniFormat);
		else
			settings = NULL;
	#else // ULTRACOPIER_VERSION_PORTABLE
		settings = new QSettings("Ultracopier","Ultracopier");
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
			settings->setValue("test","test");
			if(settings->status()!=QSettings::NoError)
			{
				delete settings;
				settings=NULL;
			}
			else
			{
				settings->remove("test");
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
		resources->disableWritablePath();
		#endif // ULTRACOPIER_VERSION_PORTABLE
		currentBackend=Memory;
	}
	else
		currentBackend=File;
}

/// \brief Destroy the option
OptionEngine::~OptionEngine()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	ResourcesManager::destroyInstanceAtTheLastCall();
}

/// \brief To add option group to options
bool OptionEngine::addOptionGroup(const QString &groupName,const QList<QPair<QString, QVariant> > &KeysList)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start(\""+groupName+"\",[...])");
	//search if previous with the same name exists
	indexGroup=0;
	loop_size=GroupKeysList.size();
	while(indexGroup<loop_size)
	{
		if(GroupKeysList.at(indexGroup).groupName==groupName)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"group already used previously");
			break;
		}
		indexGroup++;
	}
	//else create the entry
	if(indexGroup==loop_size)
	{
		OptionEngineGroup tempEntry;
		tempEntry.groupName=groupName;
		GroupKeysList << tempEntry;
	}
	//here GroupKeysList.at(indexGroup) can be used
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"variable used: GroupKeysList.at("+QString::number(indexGroup)+")");
	//if the backend is file, enter into the group
	if(currentBackend==File)
		settings->beginGroup(groupName);
	//browse all key, and append it to the key
	index=0;
	QList<OptionEngineGroupKey> KeyListTemp;
	loop_size=KeysList.size();
	while(index<loop_size)
	{
		OptionEngineGroupKey theCurrentKey;
		theCurrentKey.variableName=KeysList.at(index).first;
		theCurrentKey.defaultValue=KeysList.at(index).second;
		theCurrentKey.emptyList=false;
		//if memory backend, load the default value into the current value
		if(currentBackend==Memory)
			theCurrentKey.currentValue=theCurrentKey.defaultValue;
		else if(settings->contains(theCurrentKey.variableName))//if file backend, load the default value from the file
			theCurrentKey.currentValue=settings->value(theCurrentKey.variableName);
		else //or if not found load the default value and set into the file
		{
			theCurrentKey.currentValue=theCurrentKey.defaultValue;
			//to switch default value if is unchanged
			//settings->setValue(theCurrentKey.variableName,theCurrentKey.defaultValue);
		}
		if(settings->status()!=QSettings::NoError)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Have writing error, switch to memory only options");
			#ifdef ULTRACOPIER_VERSION_PORTABLE
			resources->disableWritablePath();
			#endif // ULTRACOPIER_VERSION_PORTABLE
			currentBackend=Memory;
		}
		GroupKeysList[indexGroup].KeysList << theCurrentKey;
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, groupName: "+groupName);
	indexGroup=0;
	loop_size=GroupKeysList.size();
	while(indexGroup<loop_size)
	{
		if(GroupKeysList.at(indexGroup).groupName==groupName)
		{
			GroupKeysList.removeAt(indexGroup);
			return true;
		}
		indexGroup++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"value not found, internal bug, groupName: "+groupName);
	return false;
}

/// \brief To get option value
QVariant OptionEngine::getOptionValue(const QString &groupName,const QString &variableName)
{
	indexGroup=0;
	loop_size=GroupKeysList.size();
	while(indexGroup<loop_size)
	{
		if(GroupKeysList.at(indexGroup).groupName==groupName)
		{
			//search if previous with the same name exists
			indexGroupKey=0;
			loop_sub_size=GroupKeysList.at(indexGroup).KeysList.size();
			while(indexGroupKey<loop_sub_size)
			{
				if(GroupKeysList.at(indexGroup).KeysList.at(indexGroupKey).variableName==variableName)
					return GroupKeysList.at(indexGroup).KeysList.at(indexGroupKey).currentValue;
				indexGroupKey++;
			}
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"value not found, internal bug, groupName: "+groupName+", variableName: "+variableName);
			return QVariant();
		}
		indexGroup++;
	}
	QMessageBox::critical(NULL,"Internal error","Get the option value but not found");
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"group \""+groupName+"\" not found, internal bug!");
	//return default value
	return QVariant();
}

/// \brief To set option value
void OptionEngine::setOptionValue(const QString &groupName,const QString &variableName,const QVariant &value)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"groupName: \""+groupName+"\", variableName: \""+variableName+"\", value: \""+value.toString()+"\"");
	indexGroup=0;
	loop_size=GroupKeysList.size();
	while(indexGroup<loop_size)
	{
		if(GroupKeysList.at(indexGroup).groupName==groupName)
		{
			//search if previous with the same name exists
			indexGroupKey=0;
			loop_sub_size=GroupKeysList.at(indexGroup).KeysList.size();
			while(indexGroupKey<loop_sub_size)
			{
				if(GroupKeysList.at(indexGroup).KeysList.at(indexGroupKey).variableName==variableName)
				{
					if(GroupKeysList.at(indexGroup).KeysList.at(indexGroupKey).currentValue!=value) //protection to prevent same value writing
					{
						GroupKeysList[indexGroup].KeysList[indexGroupKey].currentValue=value;
						if(currentBackend==File)
						{
							settings->beginGroup(groupName);
							settings->setValue(variableName,value);
							settings->endGroup();
							if(settings->status()!=QSettings::NoError)
							{
								ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Have writing error, switch to memory only options");
								#ifdef ULTRACOPIER_VERSION_PORTABLE
								resources->disableWritablePath();
								#endif // ULTRACOPIER_VERSION_PORTABLE
								currentBackend=Memory;
							}
						}
						emit newOptionValue(groupName,variableName,value);
					}
					return;
				}
				indexGroupKey++;
			}
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"value not found, internal bug, groupName: \""+groupName+"\", variableName: \""+variableName+"\", value: \""+value.toString()+"\"");
			return;
		}
		indexGroup++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"group \""+groupName+"\" not found, internal bug, groupName: \""+groupName+"\", variableName: \""+variableName+"\", value: \""+value.toString()+"\"");
}

//the reset of right value of widget need be do into the calling object
void OptionEngine::internal_resetToDefaultValue()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int indexGroup=0;
	while(indexGroup<GroupKeysList.size())
	{
		//search if previous with the same name exists
		int indexGroupKey=0;
		while(indexGroupKey<GroupKeysList.at(indexGroup).KeysList.size())
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"option check: "+GroupKeysList.at(indexGroup).KeysList.at(indexGroupKey).variableName);
			if(GroupKeysList.at(indexGroup).KeysList.at(indexGroupKey).currentValue!=GroupKeysList.at(indexGroup).KeysList.at(indexGroupKey).defaultValue)
				GroupKeysList[indexGroup].KeysList[indexGroupKey].currentValue=GroupKeysList[indexGroup].KeysList[indexGroupKey].defaultValue;
			indexGroupKey++;
		}
		indexGroup++;
	}
}

void OptionEngine::queryResetOptions()
{
	emit resetOptions();
}
