/** \file CopyEngineManager.cpp
\brief Define the copy engine manager
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QDebug>
#include <QMessageBox>

#include "CopyEngineManager.h"

CopyEngineManager::CopyEngineManager(OptionDialog *optionDialog)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->optionDialog=optionDialog;
	//setup the ui layout
	plugins->lockPluginListEdition();
	QList<PluginsAvailable> list=plugins->getPluginsByCategory(PluginType_CopyEngine);
	foreach(PluginsAvailable currentPlugin,list)
		onePluginAdded(currentPlugin);
	connect(plugins,SIGNAL(onePluginAdded(PluginsAvailable)),		this,SLOT(onePluginAdded(PluginsAvailable)));
	connect(plugins,SIGNAL(onePluginWillBeRemoved(PluginsAvailable)),	this,SLOT(onePluginWillBeRemoved(PluginsAvailable)),Qt::DirectConnection);
	connect(plugins,SIGNAL(pluginListingIsfinish()),			this,SLOT(allPluginIsloaded()));
	plugins->unlockPluginListEdition();
	//load the options
	isConnected=false;
	connect(languages,	SIGNAL(newLanguageLoaded(QString)),			&facilityEngine,SLOT(retranslate()));
}

void CopyEngineManager::onePluginAdded(PluginsAvailable plugin)
{
	if(plugin.category!=PluginType_CopyEngine)
		return;
	//setFileName
	QString pluginPath=plugin.path+PluginsManager::getResolvedPluginName("copyEngine");
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+pluginPath);
	//search into loaded session
	int index=0;
	while(index<pluginList.size())
	{
		if(pluginList.at(index).pluginPath==pluginPath)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,QString("Engine already found!"));
			return;
		}
		index++;
	}
	CopyEnginePlugin newItem;
	newItem.pluginPath=pluginPath;
	newItem.path=plugin.path;
	newItem.name=plugin.name;
	newItem.pointer=new QPluginLoader(newItem.pluginPath);
	QObject *pluginObject = newItem.pointer->instance();
	if(pluginObject==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("unable to load the plugin: %1").arg(newItem.pointer->errorString()));
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("unable to load the plugin for %1").arg(newItem.pluginPath));
		newItem.pointer->unload();
		return;
	}
	newItem.factory = qobject_cast<PluginInterface_CopyEngineFactory *>(pluginObject);
	//check if found
	index=0;
	while(index<pluginList.size())
	{
		if(pluginList.at(index).factory==newItem.factory)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Plugin already found"));
			newItem.pointer->unload();
			return;
		}
		index++;
	}
	if(newItem.factory==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("unable to cast the plugin: %1").arg(newItem.pointer->errorString()));
		newItem.pointer->unload();
		return;
	}
	#ifdef ULTRACOPIER_DEBUG
	qRegisterMetaType<DebugLevel>("DebugLevel");
	connect(newItem.factory,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SLOT(debugInformation(DebugLevel,QString,QString,QString,int)),Qt::QueuedConnection);
	#endif // ULTRACOPIER_DEBUG
	newItem.options=new LocalPluginOptions("CopyEngine-"+newItem.name);
	newItem.factory->setResources(newItem.options,plugin.writablePath,plugin.path,&facilityEngine,ULTRACOPIER_VERSION_PORTABLE_BOOL);
	newItem.optionsWidget=newItem.factory->options();
	newItem.supportedProtocolsForTheSource=newItem.factory->supportedProtocolsForTheSource();
	newItem.supportedProtocolsForTheDestination=newItem.factory->supportedProtocolsForTheDestination();
	newItem.canDoOnlyCopy=newItem.factory->canDoOnlyCopy();
	newItem.type=newItem.factory->getCopyType();
	optionDialog->addCopyEngineWidget(newItem.name,newItem.optionsWidget);
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"plugin: "+newItem.name+" loaded, send options");
	//emit newCopyEngineOptions(plugin.path,newItem.name,newItem.optionsWidget);
	pluginList << newItem;
	connect(languages,SIGNAL(newLanguageLoaded(QString)),newItem.factory,SLOT(newLanguageLoaded()));
	if(plugins->allPluginHaveBeenLoaded())
		allPluginIsloaded();
	if(isConnected)
		emit addCopyEngine(newItem.name,newItem.canDoOnlyCopy);
}

void CopyEngineManager::onePluginWillBeRemoved(PluginsAvailable plugin)
{
	int index=0;
	while(index<pluginList.size())
	{
		if(pluginList.at(index).path==plugin.path)
		{
			if(pluginList.at(index).intances.size()<=0)
			{
				emit removeCopyEngine(pluginList.at(index).name);
				pluginList.removeAt(index);
				allPluginIsloaded();
				return;
			}
		}
		index++;
	}
}

void CopyEngineManager::onePluginWillBeUnloaded(PluginsAvailable plugin)
{
	if(plugin.category!=PluginType_CopyEngine)
		return;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<pluginList.size())
	{
		if(pluginList.at(index).path==plugin.path)
		{
			optionDialog->removeCopyEngineWidget(pluginList.at(index).name);
			delete pluginList.at(index).options;
			delete pluginList.at(index).factory;
			pluginList.at(index).pointer->unload();
			return;
		}
		index++;
	}
}

CopyEngineManager::returnCopyEngine CopyEngineManager::getCopyEngine(CopyMode mode,QStringList protocolsUsedForTheSources,QString protocolsUsedForTheDestination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start, pluginList.size(): %1, mode: %2, and particular protocol").arg(pluginList.size()).arg((int)mode));
	returnCopyEngine temp;
	int index=0;
	bool isTheGoodEngine=false;
	while(index<pluginList.size())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("pluginList.at(%1).name: %2").arg(index).arg(pluginList.at(index).name));
		isTheGoodEngine=false;
		if(mode!=Move || !pluginList.at(index).canDoOnlyCopy)
		{
			if(protocolsUsedForTheSources.size()==0)
				isTheGoodEngine=true;
			else
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("pluginList.at(index).supportedProtocolsForTheDestination: %1").arg(pluginList.at(index).supportedProtocolsForTheDestination.join(";")));
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("protocolsUsedForTheDestination: %1").arg(protocolsUsedForTheDestination));
				if(pluginList.at(index).supportedProtocolsForTheDestination.contains(protocolsUsedForTheDestination))
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("pluginList.at(index).supportedProtocolsForTheSource: %1").arg(pluginList.at(index).supportedProtocolsForTheSource.join(";")));
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("protocolsUsedForTheSources.at(indexProto): %1").arg(protocolsUsedForTheSources.join(";")));
					isTheGoodEngine=true;
					int indexProto=0;
					while(indexProto<protocolsUsedForTheSources.size())
					{
						if(!pluginList.at(index).supportedProtocolsForTheSource.contains(protocolsUsedForTheSources.at(indexProto)))
						{
							isTheGoodEngine=false;
							break;
						}
						indexProto++;
					}
				}
			}
		}
		if(isTheGoodEngine)
		{
			pluginList[index].intances<<pluginList[index].factory->getInstance();
			temp.engine=pluginList[index].intances.last();
			temp.canDoOnlyCopy=pluginList.at(index).canDoOnlyCopy;
			temp.type=pluginList.at(index).type;
			return temp;
		}
		index++;
	}
	if(mode==Move)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Cannot find any copy engine with motions support");
		QMessageBox::critical(NULL,tr("Warning"),tr("Cannot find any copy engine with motions support"));
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Cannot find any compatible engine!");
		QMessageBox::critical(NULL,tr("Warning"),tr("Cannot find any compatible engine!"));
	}
	temp.engine=NULL;
	temp.type=File;
	temp.canDoOnlyCopy=true;
	return temp;
}

CopyEngineManager::returnCopyEngine CopyEngineManager::getCopyEngine(CopyMode mode,QString name)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start, pluginList.size(): %1, with mode: %2, and name: %3").arg(pluginList.size()).arg((int)mode).arg(name));
	returnCopyEngine temp;
	int index=0;
	while(index<pluginList.size())
	{
		if(pluginList.at(index).name==name)
		{
			if(mode==Move && pluginList.at(index).canDoOnlyCopy)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"This copy engine does not support motions: pluginList.at(index).canDoOnlyCopy: "+QString::number(pluginList.at(index).canDoOnlyCopy));
				QMessageBox::critical(NULL,tr("Warning"),tr("This copy engine does not support motions"));
				temp.engine=NULL;
				return temp;
			}
			pluginList[index].intances<<pluginList[index].factory->getInstance();
			temp.engine=pluginList[index].intances.last();
			temp.canDoOnlyCopy=pluginList.at(index).canDoOnlyCopy;
			temp.type=pluginList.at(index).type;
			return temp;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Cannot find any engine with this name");
	QMessageBox::critical(NULL,tr("Warning"),tr("Cannot find any engine with this name"));
	temp.engine=NULL;
	temp.type=File;
	temp.canDoOnlyCopy=true;
	return temp;
}

#ifdef ULTRACOPIER_DEBUG
void CopyEngineManager::debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
	DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Copy Engine plugin");
}
#endif // ULTRACOPIER_DEBUG

/// \brief To notify when new value into a group have changed
void CopyEngineManager::newOptionValue(QString groupName,QString variableName,QVariant value)
{
	if(groupName=="CopyEngine" && variableName=="List")
	{
		Q_UNUSED(value)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"start(\""+groupName+"\",\""+variableName+"\",\""+value.toString()+"\")");
		allPluginIsloaded();
	}
}

void CopyEngineManager::setIsConnected()
{
	/* ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	  prevent bug, I don't know why, in one case it bug here
	  */
	isConnected=true;
	int index=0;
	while(index<pluginList.size())
	{
		emit addCopyEngine(pluginList.at(index).name,pluginList.at(index).canDoOnlyCopy);
		index++;
	}
}

void CopyEngineManager::allPluginIsloaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QStringList actualList;
	int index=0;
	while(index<pluginList.size())
	{
		actualList << pluginList.at(index).name;
		index++;
	}
	QStringList preferedList=options->getOptionValue("CopyEngine","List").toStringList();
	preferedList.removeDuplicates();
	actualList.removeDuplicates();
	index=0;
	while(index<preferedList.size())
	{
		if(!actualList.contains(preferedList.at(index)))
		{
			preferedList.removeAt(index);
			index--;
		}
		index++;
	}
	index=0;
	while(index<actualList.size())
	{
		if(!preferedList.contains(actualList.at(index)))
			preferedList << actualList.at(index);
		index++;
	}
	options->setOptionValue("CopyEngine","List",preferedList);
	QList<CopyEnginePlugin> newPluginList;
	index=0;
	while(index<preferedList.size())
	{
		int pluginListIndex=0;
		while(pluginListIndex<pluginList.size())
		{
			if(preferedList.at(index)==pluginList.at(pluginListIndex).name)
			{
				newPluginList << pluginList.at(pluginListIndex);
				break;
			}
			pluginListIndex++;
		}
		index++;
	}
	pluginList=newPluginList;
}

bool CopyEngineManager::protocolsSupportedByTheCopyEngine(PluginInterface_CopyEngine * engine,QStringList protocolsUsedForTheSources,QString protocolsUsedForTheDestination)
{
	int index=0;
	while(index<pluginList.size())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("pluginList.at(%1).name: %2").arg(index).arg(pluginList.at(index).name));
		if(pluginList.at(index).intances.contains(engine))
		{
			if(!pluginList.at(index).supportedProtocolsForTheDestination.contains(protocolsUsedForTheDestination))
				return false;
			int indexProto=0;
			while(indexProto<protocolsUsedForTheSources.size())
			{
				if(!pluginList.at(index).supportedProtocolsForTheSource.contains(protocolsUsedForTheSources.at(indexProto)))
					return false;
				indexProto++;
			}
			return true;
		}
	}
	return false;
}
