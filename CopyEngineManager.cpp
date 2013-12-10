/** \file CopyEngineManager.cpp
\brief Define the copy engine manager
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QMessageBox>

#include "CopyEngineManager.h"
#include "LanguagesManager.h"

CopyEngineManager::CopyEngineManager(OptionDialog *optionDialog)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->optionDialog=optionDialog;
    //setup the ui layout
    PluginsManager::pluginsManager->lockPluginListEdition();
    connect(this,&CopyEngineManager::previouslyPluginAdded,			this,&CopyEngineManager::onePluginAdded,Qt::QueuedConnection);
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginAdded,                this,&CopyEngineManager::onePluginAdded,Qt::QueuedConnection);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginWillBeRemoved,		this,&CopyEngineManager::onePluginWillBeRemoved,Qt::DirectConnection);
    #endif
    connect(PluginsManager::pluginsManager,&PluginsManager::pluginListingIsfinish,			this,&CopyEngineManager::allPluginIsloaded,Qt::QueuedConnection);
    QList<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_CopyEngine);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager->unlockPluginListEdition();
    //load the options
    isConnected=false;
}

void CopyEngineManager::onePluginAdded(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_CopyEngine)
        return;
    //setFileName
    QString pluginPath=plugin.path+PluginsManager::getResolvedPluginName(QStringLiteral("copyEngine"));
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    /*more IO
    if(!QFile(pluginPath).exists())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("The plugin binary is missing: ")+pluginPath);
        return;
    }*/
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start: ")+pluginPath);
    //search into loaded session
    int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).pluginPath==pluginPath)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QStringLiteral("Engine already found!"));
            return;
        }
        index++;
    }
    CopyEnginePlugin newItem;
    newItem.pluginPath=pluginPath;
    newItem.path=plugin.path;
    newItem.name=plugin.name;
    newItem.pointer=new QPluginLoader(newItem.pluginPath);
    #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    QObjectList objectList=QPluginLoader::staticInstances();
    index=0;
    QObject *pluginObject;
    while(index<objectList.size())
    {
        pluginObject=objectList.at(index);
        newItem.factory = qobject_cast<PluginInterface_CopyEngineFactory *>(pluginObject);
        if(newItem.factory!=NULL)
            break;
        index++;
    }
    if(index==objectList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("static copy engine not found"));
        return;
    }
    #else
    QObject *pluginObject = newItem.pointer->instance();
    if(pluginObject==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to load the plugin: %1").arg(newItem.pointer->errorString()));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to load the plugin for %1").arg(newItem.pluginPath));
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Plugin already found"));
            newItem.pointer->unload();
            return;
        }
        index++;
    }
    if(newItem.factory==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to cast the plugin: %1").arg(newItem.pointer->errorString()));
        newItem.pointer->unload();
        return;
    }
    #endif
    #ifdef ULTRACOPIER_DEBUG
    connect(newItem.factory,&PluginInterface_CopyEngineFactory::debugInformation,this,&CopyEngineManager::debugInformation,Qt::QueuedConnection);
    #endif // ULTRACOPIER_DEBUG
    newItem.options=new LocalPluginOptions(QStringLiteral("CopyEngine-")+newItem.name);
    newItem.factory->setResources(newItem.options,plugin.writablePath,plugin.path,&FacilityEngine::facilityEngine,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    newItem.optionsWidget=newItem.factory->options();
    newItem.supportedProtocolsForTheSource=newItem.factory->supportedProtocolsForTheSource();
    newItem.supportedProtocolsForTheDestination=newItem.factory->supportedProtocolsForTheDestination();
    newItem.canDoOnlyCopy=newItem.factory->canDoOnlyCopy();
    newItem.type=newItem.factory->getCopyType();
    newItem.transferListOperation=newItem.factory->getTransferListOperation();
    optionDialog->addPluginOptionWidget(PluginType_CopyEngine,newItem.name,newItem.optionsWidget);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("plugin: ")+newItem.name+QStringLiteral(" loaded, send options"));
    //emit newCopyEngineOptions(plugin.path,newItem.name,newItem.optionsWidget);
    pluginList << newItem;
    connect(LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,newItem.factory,&PluginInterface_CopyEngineFactory::newLanguageLoaded);
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        allPluginIsloaded();
    if(isConnected)
        emit addCopyEngine(newItem.name,newItem.canDoOnlyCopy);
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void CopyEngineManager::onePluginWillBeRemoved(const PluginsAvailable &plugin)
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

void CopyEngineManager::onePluginWillBeUnloaded(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_CopyEngine)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).path==plugin.path)
        {
            delete pluginList.at(index).options;
            delete pluginList.at(index).factory;
            pluginList.at(index).pointer->unload();
            return;
        }
        index++;
    }
}
#endif

CopyEngineManager::returnCopyEngine CopyEngineManager::getCopyEngine(const Ultracopier::CopyMode &mode,const QStringList &protocolsUsedForTheSources,const QString &protocolsUsedForTheDestination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, pluginList.size(): %1, mode: %2, and particular protocol").arg(pluginList.size()).arg((int)mode));
    returnCopyEngine temp;
    int index=0;
    bool isTheGoodEngine=false;
    while(index<pluginList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("pluginList.at(%1).name: %2").arg(index).arg(pluginList.at(index).name));
        isTheGoodEngine=false;
        if(mode!=Ultracopier::Move || !pluginList.at(index).canDoOnlyCopy)
        {
            if(protocolsUsedForTheSources.size()==0)
                isTheGoodEngine=true;
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("pluginList.at(index).supportedProtocolsForTheDestination: %1").arg(pluginList.at(index).supportedProtocolsForTheDestination.join(";")));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("protocolsUsedForTheDestination: %1").arg(protocolsUsedForTheDestination));
                if(protocolsUsedForTheDestination.isEmpty() || pluginList.at(index).supportedProtocolsForTheDestination.contains(protocolsUsedForTheDestination))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("pluginList.at(index).supportedProtocolsForTheSource: %1").arg(pluginList.at(index).supportedProtocolsForTheSource.join(";")));
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("protocolsUsedForTheSources.at(indexProto): %1").arg(protocolsUsedForTheSources.join(";")));
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
            temp.transferListOperation=pluginList.at(index).transferListOperation;
            return temp;
        }
        index++;
    }
    if(mode==Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Cannot find any copy engine with motions support");
        QMessageBox::critical(NULL,tr("Warning"),tr("Cannot find any copy engine with motions support"));
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Cannot find any compatible engine!");
        QMessageBox::critical(NULL,tr("Warning"),tr("Cannot find any compatible engine!"));
    }
    temp.engine=NULL;
    temp.type=Ultracopier::File;
    temp.canDoOnlyCopy=true;
    return temp;
}

CopyEngineManager::returnCopyEngine CopyEngineManager::getCopyEngine(const Ultracopier::CopyMode &mode,const QString &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, pluginList.size(): %1, with mode: %2, and name: %3").arg(pluginList.size()).arg((int)mode).arg(name));
    returnCopyEngine temp;
    int index=0;
    while(index<pluginList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("Check matching: %1").arg(pluginList.at(index).name));
        if(pluginList.at(index).name==name)
        {
            if(mode==Ultracopier::Move && pluginList.at(index).canDoOnlyCopy)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"This copy engine does not support motions: pluginList.at(index).canDoOnlyCopy: "+QString::number(pluginList.at(index).canDoOnlyCopy));
                QMessageBox::critical(NULL,tr("Warning"),tr("This copy engine does not support motions"));
                temp.engine=NULL;
                return temp;
            }
            pluginList[index].intances<<pluginList[index].factory->getInstance();
            temp.engine=pluginList[index].intances.last();
            temp.canDoOnlyCopy=pluginList.at(index).canDoOnlyCopy;
            temp.type=pluginList.at(index).type;
            temp.transferListOperation=pluginList.at(index).transferListOperation;
            return temp;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Cannot find any engine with this name: %1").arg(name));
    QMessageBox::critical(NULL,tr("Warning"),tr("Cannot find any engine with this name: %1").arg(name));
    temp.engine=NULL;
    temp.type=Ultracopier::File;
    temp.canDoOnlyCopy=true;
    return temp;
}

#ifdef ULTRACOPIER_DEBUG
void CopyEngineManager::debugInformation(const Ultracopier::DebugLevel &level,const QString& fonction,const QString& text,const QString& file,const int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,QStringLiteral("Copy Engine plugin"));
}
#endif // ULTRACOPIER_DEBUG

/// \brief To notify when new value into a group have changed
void CopyEngineManager::newOptionValue(const QString &groupName,const QString &variableName,const QVariant &value)
{
    if(groupName==QStringLiteral("CopyEngine") && variableName==QStringLiteral("List"))
    {
        Q_UNUSED(value)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"start(\""+groupName+"\",\""+variableName+"\",\""+value.toString()+"\")");
        allPluginIsloaded();
    }
}

void CopyEngineManager::setIsConnected()
{
    /* ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    QStringList actualList;
    int index=0;
    while(index<pluginList.size())
    {
        actualList << pluginList.at(index).name;
        index++;
    }
    QStringList preferedList=OptionEngine::optionEngine->getOptionValue(QStringLiteral("CopyEngine"),QStringLiteral("List")).toStringList();
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
    OptionEngine::optionEngine->setOptionValue(QStringLiteral("CopyEngine"),QStringLiteral("List"),preferedList);
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

bool CopyEngineManager::protocolsSupportedByTheCopyEngine(PluginInterface_CopyEngine * engine,const QStringList &protocolsUsedForTheSources,const QString &protocolsUsedForTheDestination)
{
    int index=0;
    while(index<pluginList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("pluginList.at(%1).name: %2").arg(index).arg(pluginList.at(index).name));
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
