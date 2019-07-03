/** \file CopyEngineManager.cpp
\brief Define the copy engine manager
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QMessageBox>

#include "CopyEngineManager.h"
#include "LanguagesManager.h"
#include "cpp11addition.h"

#ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
#include "plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.h"
#endif

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
    std::vector<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_CopyEngine);
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
    std::string pluginPath=plugin.path+PluginsManager::getResolvedPluginName("copyEngine");
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    /*more IO
    if(!QFile(pluginPath).exists())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The plugin binary is missing: "+pluginPath);
        return;
    }*/
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+pluginPath);
    //search into loaded session
    unsigned int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).pluginPath==pluginPath)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Engine already found!");
            return;
        }
        index++;
    }
    CopyEnginePlugin newItem;
    newItem.pluginPath=pluginPath;
    newItem.path=plugin.path;
    newItem.name=plugin.name;
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    newItem.pointer=new QPluginLoader(QString::fromStdString(newItem.pluginPath));
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"static copy engine not found");
        return;
    }
    #else
    QObject *pluginObject = newItem.pointer->instance();
    if(pluginObject==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to load the plugin: "+newItem.pointer->errorString().toStdString());
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to load the plugin for "+newItem.pluginPath);
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Plugin already found");
            newItem.pointer->unload();
            return;
        }
        index++;
    }
    if(newItem.factory==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to cast the plugin: "+newItem.pointer->errorString().toStdString());
        newItem.pointer->unload();
        return;
    }
    #endif
    #else
    newItem.factory=new CopyEngineFactory();
    #endif

    #ifdef ULTRACOPIER_DEBUG
    connect(newItem.factory,&PluginInterface_CopyEngineFactory::debugInformation,this,&CopyEngineManager::debugInformation,Qt::QueuedConnection);
    #endif // ULTRACOPIER_DEBUG
    newItem.options=new LocalPluginOptions("CopyEngine-"+newItem.name);
    newItem.factory->setResources(newItem.options,plugin.writablePath,plugin.path,&FacilityEngine::facilityEngine,ULTRACOPIER_VERSION_PORTABLE_BOOL);
    connect(OptionEngine::optionEngine,&OptionEngine::resetOptions,newItem.factory,&PluginInterface_CopyEngineFactory::resetOptions);
    newItem.optionsWidget=newItem.factory->options();
    newItem.supportedProtocolsForTheSource=newItem.factory->supportedProtocolsForTheSource();
    newItem.supportedProtocolsForTheDestination=newItem.factory->supportedProtocolsForTheDestination();
    newItem.canDoOnlyCopy=newItem.factory->canDoOnlyCopy();
    newItem.type=newItem.factory->getCopyType();
    newItem.transferListOperation=newItem.factory->getTransferListOperation();
    optionDialog->addPluginOptionWidget(PluginType_CopyEngine,newItem.name,newItem.optionsWidget);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"plugin: "+newItem.name+" loaded, send options");
    //emit newCopyEngineOptions(plugin.path,newItem.name,newItem.optionsWidget);
    pluginList.push_back(newItem);
    connect(LanguagesManager::languagesManager,&LanguagesManager::newLanguageLoaded,newItem.factory,&PluginInterface_CopyEngineFactory::newLanguageLoaded);
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        allPluginIsloaded();
    if(isConnected)
        emit addCopyEngine(newItem.name,newItem.canDoOnlyCopy);
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void CopyEngineManager::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    unsigned int index=0;
    while(index<pluginList.size())
    {
        if(pluginList.at(index).path==plugin.path)
        {
            if(pluginList.at(index).intances.size()<=0)
            {
                emit removeCopyEngine(pluginList.at(index).name);
                pluginList.erase(pluginList.begin()+index);
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
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

CopyEngineManager::returnCopyEngine CopyEngineManager::getCopyEngine(const Ultracopier::CopyMode &mode,
    const std::vector<std::string> &protocolsUsedForTheSources,const std::string &protocolsUsedForTheDestination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, pluginList.size(): "+std::to_string(pluginList.size())+", mode: "+std::to_string((int)mode)+", and particular protocol");
    returnCopyEngine temp;
    unsigned int index=0;
    bool isTheGoodEngine=false;
    while(index<pluginList.size())
    {
        const CopyEnginePlugin &copyEnginePlugin=pluginList.at(index);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"pluginList.at("+std::to_string(index)+").name: "+copyEnginePlugin.name);
        isTheGoodEngine=false;
        if(mode!=Ultracopier::Move || !copyEnginePlugin.canDoOnlyCopy)
        {
            if(protocolsUsedForTheSources.size()==0)
                isTheGoodEngine=true;
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"copyEnginePlugin.supportedProtocolsForTheDestination: "+stringimplode(copyEnginePlugin.supportedProtocolsForTheDestination,";"));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"protocolsUsedForTheDestination: "+protocolsUsedForTheDestination);
                if(protocolsUsedForTheDestination.empty() || vectorcontainsAtLeastOne(copyEnginePlugin.supportedProtocolsForTheDestination,protocolsUsedForTheDestination))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"copyEnginePlugin.supportedProtocolsForTheSource: "+stringimplode(copyEnginePlugin.supportedProtocolsForTheSource,";"));
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"protocolsUsedForTheSources.at(indexProto): "+stringimplode(protocolsUsedForTheSources,";"));
                    isTheGoodEngine=true;
                    unsigned int indexProto=0;
                    while(indexProto<protocolsUsedForTheSources.size())
                    {
                        if(!vectorcontainsAtLeastOne(copyEnginePlugin.supportedProtocolsForTheSource,protocolsUsedForTheSources.at(indexProto)))
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
            pluginList[index].intances.push_back(pluginList.at(index).factory->getInstance());
            temp.engine=pluginList.at(index).intances.back();
            temp.canDoOnlyCopy=pluginList.at(index).canDoOnlyCopy;
            temp.havePause=pluginList.at(index).factory->havePause();
            temp.type=pluginList.at(index).type;
            temp.transferListOperation=pluginList.at(index).transferListOperation;
            return temp;
        }
        index++;
    }
    if(mode==Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Cannot find any copy engine with move support");
        QMessageBox::critical(NULL,tr("Warning"),tr("Cannot find any copy engine with move support"));
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Cannot find any compatible engine!");
        QMessageBox::critical(NULL,tr("Warning"),tr("Cannot find any compatible engine!"));
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"protocolsUsedForTheSources: "+stringimplode(protocolsUsedForTheSources,";")+", protocolsUsedForTheDestination: "+protocolsUsedForTheDestination);

    temp.engine=NULL;
    temp.type=Ultracopier::File;
    temp.canDoOnlyCopy=true;
    return temp;
}

CopyEngineManager::returnCopyEngine CopyEngineManager::getCopyEngine(const Ultracopier::CopyMode &mode,const std::string &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, pluginList.size(): "+std::to_string(pluginList.size())+", with mode: "+std::to_string((int)mode)+", and name: "+name);
    returnCopyEngine temp;
    unsigned int index=0;
    while(index<pluginList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Check matching: "+pluginList.at(index).name);
        if(pluginList.at(index).name==name || name.empty())
        {
            if(mode==Ultracopier::Move && pluginList.at(index).canDoOnlyCopy)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"This copy engine does not support move: pluginList.at(index).canDoOnlyCopy: "+std::to_string(pluginList.at(index).canDoOnlyCopy));
                QMessageBox::critical(NULL,tr("Warning"),tr("This copy engine does not support move"));
                temp.engine=NULL;
                return temp;
            }
            pluginList[index].intances.push_back(pluginList.at(index).factory->getInstance());
            temp.engine=pluginList.at(index).intances.back();
            temp.canDoOnlyCopy=pluginList.at(index).canDoOnlyCopy;
            temp.type=pluginList.at(index).type;
            temp.transferListOperation=pluginList.at(index).transferListOperation;
            return temp;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Cannot find any engine with this name: "+name);
    QMessageBox::critical(NULL,tr("Warning"),tr("Cannot find any engine with this name: %1").arg(QString::fromStdString(name)));
    temp.engine=NULL;
    temp.type=Ultracopier::File;
    temp.canDoOnlyCopy=true;
    return temp;
}

#ifdef ULTRACOPIER_DEBUG
void CopyEngineManager::debugInformation(const Ultracopier::DebugLevel &level,const std::string& fonction,const std::string& text,const std::string& file,const int& ligne)
{
    DebugEngine::addDebugInformationStatic(level,fonction,text,file,ligne,"Copy Engine plugin");
}
#endif // ULTRACOPIER_DEBUG

/// \brief To notify when new value into a group have changed
void CopyEngineManager::newOptionValue(const std::string &groupName,const std::string &variableName,const std::string &value)
{
    if(groupName=="CopyEngine" && variableName=="List")
    {
        Q_UNUSED(value)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"start(\""+groupName+"\",\""+variableName+"\",\""+value+"\")");
        allPluginIsloaded();
    }
}

void CopyEngineManager::setIsConnected()
{
    /* ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
      prevent bug, I don't know why, in one case it bug here
      */
    isConnected=true;
    unsigned int index=0;
    while(index<pluginList.size())
    {
        emit addCopyEngine(pluginList.at(index).name,pluginList.at(index).canDoOnlyCopy);
        index++;
    }
}

void CopyEngineManager::allPluginIsloaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    std::vector<std::string> actualList;
    unsigned int index=0;
    while(index<pluginList.size())
    {
        actualList.push_back(pluginList.at(index).name);
        index++;
    }
    std::vector<std::string> preferedList=stringsplit(OptionEngine::optionEngine->getOptionValue("CopyEngine","List"),';');
    vectorRemoveDuplicatesForSmallList(preferedList);
    vectorRemoveDuplicatesForSmallList(actualList);
    index=0;
    while(index<preferedList.size())
    {
        if(!vectorcontainsAtLeastOne(actualList,preferedList.at(index)))
        {
            preferedList.erase(preferedList.cbegin()+index);
            index--;
        }
        index++;
    }
    index=0;
    while(index<actualList.size())
    {
        if(!vectorcontainsAtLeastOne(preferedList,actualList.at(index)))
            preferedList.push_back( actualList.at(index));
        index++;
    }
    OptionEngine::optionEngine->setOptionValue("CopyEngine","List",stringimplode(preferedList,';'));
    std::vector<CopyEnginePlugin> newPluginList;
    index=0;
    while(index<preferedList.size())
    {
        unsigned int pluginListIndex=0;
        while(pluginListIndex<pluginList.size())
        {
            if(preferedList.at(index)==pluginList.at(pluginListIndex).name)
            {
                newPluginList.push_back(pluginList.at(pluginListIndex));
                break;
            }
            pluginListIndex++;
        }
        index++;
    }
    pluginList=newPluginList;
}

bool CopyEngineManager::protocolsSupportedByTheCopyEngine(PluginInterface_CopyEngine * engine,const std::vector<std::string> &protocolsUsedForTheSources,const std::string &protocolsUsedForTheDestination)
{
    unsigned int index=0;
    while(index<pluginList.size())
    {
        const CopyEnginePlugin &copyEnginePlugin=pluginList.at(index);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"pluginList.at("+std::to_string(index)+").name: "+copyEnginePlugin.name);
        if(vectorcontainsAtLeastOne(copyEnginePlugin.intances,engine))
        {
            if(!vectorcontainsAtLeastOne(copyEnginePlugin.supportedProtocolsForTheDestination,protocolsUsedForTheDestination))
                return false;
            unsigned int indexProto=0;
            while(indexProto<protocolsUsedForTheSources.size())
            {
                if(!vectorcontainsAtLeastOne(copyEnginePlugin.supportedProtocolsForTheSource,protocolsUsedForTheSources.at(indexProto)))
                    return false;
                indexProto++;
            }
            return true;
        }
    }
    return false;
}
