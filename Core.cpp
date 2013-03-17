/** \file Core.cpp
\brief Define the class for the core
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QMessageBox>
#include <QtPlugin>

#include "Core.h"
#include "ThemesManager.h"

Core::Core(CopyEngineManager *copyEngineList)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->copyEngineList=copyEngineList;
    nextId=0;
    forUpateInformation.setInterval(ULTRACOPIER_TIME_INTERFACE_UPDATE);
    loadInterface();
    //connect(&copyEngineList,	&CopyEngineManager::newCanDoOnlyCopy,				this,	&Core::newCanDoOnlyCopy);
    connect(ThemesManager::themesManager,			&ThemesManager::theThemeNeedBeUnloaded,				this,	&Core::unloadInterface);
    connect(ThemesManager::themesManager,			&ThemesManager::theThemeIsReloaded,				this,	&Core::loadInterface, Qt::QueuedConnection);
    connect(&forUpateInformation,	&QTimer::timeout,						this,	&Core::periodicSynchronization);
}

Core::~Core()
{
    int index=0;
    while(index<copyList.size())
    {
        copyList[index].engine->cancel();
        delete copyList[index].nextConditionalSync;
        delete copyList[index].interface;
        delete copyList[index].engine;
        index++;
    }
}

void Core::newCopyWithoutDestination(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(openNewCopyEngineInstance(Ultracopier::Copy,false,protocolsUsedForTheSources)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return;
    }
    copyList.last().orderId<<orderId;
    copyList.last().engine->newCopy(sources);
    copyList.last().interface->haveExternalOrder();
}

void Core::newCopy(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources,const QString &protocolsUsedForTheDestination,const QString &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+sources.join(";")+", dest: "+destination);
    //search to group the window
    int GroupWindowWhen=OptionEngine::optionEngine->getOptionValue("Ultracopier","GroupWindowWhen").toInt();
    bool haveSameSource=false,haveSameDestination=false;
    if(GroupWindowWhen!=0)
    {
        int index=0;
        while(index<copyList.size())
        {
            if(!copyList.at(index).ignoreMode && copyList.at(index).mode==Ultracopier::Copy)
            {
                if(GroupWindowWhen!=5)
                {
                    if(GroupWindowWhen!=2)
                        haveSameSource=copyList.at(index).engine->haveSameSource(sources);
                    if(GroupWindowWhen!=1)
                        haveSameDestination=copyList.at(index).engine->haveSameDestination(destination);
                }
                if(
                    GroupWindowWhen==5 ||
                    (GroupWindowWhen==1 && haveSameSource) ||
                    (GroupWindowWhen==2 && haveSameDestination) ||
                    (GroupWindowWhen==3 && (haveSameSource && haveSameDestination)) ||
                    (GroupWindowWhen==4 && (haveSameSource || haveSameDestination))
                    )
                {
                    /*protocols are same*/
                    if(copyEngineList->protocolsSupportedByTheCopyEngine(copyList.at(index).engine,protocolsUsedForTheSources,protocolsUsedForTheDestination))
                    {
                        copyList[index].orderId<<orderId;
                        copyList.at(index).engine->newCopy(sources,destination);
                        copyList.at(index).interface->haveExternalOrder();
                        return;
                    }
                }
            }
            index++;
        }
    }
    //else open new windows
    if(openNewCopyEngineInstance(Ultracopier::Copy,false,protocolsUsedForTheSources,protocolsUsedForTheDestination)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return;
    }
    copyList.last().orderId<<orderId;
    copyList.last().engine->newCopy(sources,destination);
    copyList.last().interface->haveExternalOrder();
}

void Core::newMoveWithoutDestination(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources)
{
    if(openNewCopyEngineInstance(Ultracopier::Move,false,protocolsUsedForTheSources)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return;
    }
    copyList.last().orderId<<orderId;
    copyList.last().engine->newMove(sources);
    copyList.last().interface->haveExternalOrder();
}

void Core::newMove(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources,const QString &protocolsUsedForTheDestination,const QString &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+sources.join(";")+", dest: "+destination);
    //search to group the window
    int GroupWindowWhen=OptionEngine::optionEngine->getOptionValue("Ultracopier","GroupWindowWhen").toInt();
    bool haveSameSource=false,haveSameDestination=false;
    if(GroupWindowWhen!=0)
    {
        int index=0;
        while(index<copyList.size())
        {
            if(!copyList.at(index).ignoreMode && copyList.at(index).mode==Ultracopier::Move)
            {
                if(GroupWindowWhen!=5)
                {
                    if(GroupWindowWhen!=2)
                        haveSameSource=copyList.at(index).engine->haveSameSource(sources);
                    if(GroupWindowWhen!=1)
                        haveSameDestination=copyList.at(index).engine->haveSameDestination(destination);
                }
                if(
                    GroupWindowWhen==5 ||
                    (GroupWindowWhen==1 && haveSameSource) ||
                    (GroupWindowWhen==2 && haveSameDestination) ||
                    (GroupWindowWhen==3 && (haveSameSource && haveSameDestination)) ||
                    (GroupWindowWhen==4 && (haveSameSource || haveSameDestination))
                    )
                {
                    /*protocols are same*/
                    if(copyEngineList->protocolsSupportedByTheCopyEngine(copyList.at(index).engine,protocolsUsedForTheSources,protocolsUsedForTheDestination))
                    {
                        copyList[index].orderId<<orderId;
                        copyList.at(index).engine->newMove(sources,destination);
                        copyList.at(index).interface->haveExternalOrder();
                        return;
                    }
                }
            }
            index++;
        }
    }
    //else open new windows
    if(openNewCopyEngineInstance(Ultracopier::Move,false,protocolsUsedForTheSources,protocolsUsedForTheDestination)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return;
    }
    copyList.last().orderId<<orderId;
    copyList.last().engine->newMove(sources,destination);
    copyList.last().interface->haveExternalOrder();
}

/// \brief name to open the right copy engine
void Core::addWindowCopyMove(const Ultracopier::CopyMode &mode,const QString &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+name);
    if(openNewCopyEngineInstance(mode,false,name)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return;
    }
    ActionOnManualOpen ActionOnManualOpen_value=(ActionOnManualOpen)OptionEngine::optionEngine->getOptionValue("Ultracopier","ActionOnManualOpen").toInt();
    if(ActionOnManualOpen_value!=ActionOnManualOpen_Nothing)
    {
        if(ActionOnManualOpen_value==ActionOnManualOpen_Folder)
            copyList.last().engine->userAddFolder(mode);
        else
            copyList.last().engine->userAddFile(mode);
    }
}

/// \brief name to open the right copy engine
void Core::addWindowTransfer(const QString &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start"+name);
    if(openNewCopyEngineInstance(Ultracopier::Copy,true,name)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return;
    }
}

/** new transfer list pased by the CLI */
void Core::newTransferList(QString engine,QString mode,QString file)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("engine: %1, mode: %2, file: %3").arg(engine).arg(mode).arg(file));
    if(mode=="Transfer")
    {
        if(openNewCopyEngineInstance(Ultracopier::Copy,true,engine)==-1)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
            return;
        }
    }
    else if(mode=="Copy")
    {
        if(openNewCopyEngineInstance(Ultracopier::Copy,false,engine)==-1)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
            return;
        }
    }
    else if(mode=="Move")
    {
        if(openNewCopyEngineInstance(Ultracopier::Move,false,engine)==-1)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
            return;
        }
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The argument for the mode is not understand");
        QMessageBox::critical(NULL,tr("Error"),tr("The argument for the mode is not understand"));
        return;
    }
    copyList.last().engine->newTransferList(file);
}

void Core::loadInterface()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //load the extra files to check the themes availability
    if(copyList.size()>0)
    {
        bool error=false;
        index=0;
        loop_size=copyList.size();
        while(index<loop_size)
        {
            copyList[index].interface=ThemesManager::themesManager->getThemesInstance();
            if(copyList[index].interface==NULL)
            {
                copyInstanceCanceledByIndex(index);
                index--;
                error=true;
            }
            else
            {
                if(!copyList.at(index).ignoreMode)
                    copyList.at(index).interface->forceCopyMode(copyList.at(index).mode);
                connectInterfaceAndSync(copyList.count()-1);
                copyList.at(index).engine->syncTransferList();
            }
            index++;
        }
        if(error)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to load the interface, copy aborted");
            QMessageBox::critical(NULL,tr("Error"),tr("Unable to load the interface, copy aborted"));
        }
    }
}

void Core::unloadInterface()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    index=0;
    loop_size=copyList.size();
    while(index<loop_size)
    {
        if(copyList.at(index).interface!=NULL)
        {
            //disconnectInterface(index);
            delete copyList.at(index).interface;
            copyList[index].interface=NULL;
            copyList[index].copyEngineIsSync=false;
        }
        index++;
    }
}

int Core::incrementId()
{
    do
    {
        nextId++;
        if(nextId>2000000)
            nextId=0;
    } while(idList.contains(nextId));
    return nextId;
}

/** open with specific source/destination
\param move Copy or move
\param ignoreMode if need ignore the mode
\param protocolsUsedForTheSources protocols used for sources
\param protocolsUsedForTheDestination protocols used for destination
*/
int Core::openNewCopyEngineInstance(const Ultracopier::CopyMode &mode,const bool &ignoreMode,const QStringList &protocolsUsedForTheSources,const QString &protocolsUsedForTheDestination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    CopyEngineManager::returnCopyEngine returnInformations=copyEngineList->getCopyEngine(mode,protocolsUsedForTheSources,protocolsUsedForTheDestination);
    if(returnInformations.engine==NULL)
        return -1;
    return connectCopyEngine(mode,ignoreMode,returnInformations);
}

/** open with specific copy engine
\param move Copy or move
\param ignoreMode if need ignore the mode
\param protocolsUsedForTheSources protocols used for sources
\param protocolsUsedForTheDestination protocols used for destination
*/
int Core::openNewCopyEngineInstance(const Ultracopier::CopyMode &mode,const bool &ignoreMode,const QString &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, mode: "+QString::number(mode)+", name: "+name);
    CopyEngineManager::returnCopyEngine returnInformations=copyEngineList->getCopyEngine(mode,name);
    if(returnInformations.engine==NULL)
        return -1;
    return connectCopyEngine(mode,ignoreMode,returnInformations);
}

/** Connect the copy engine instance provided previously to the management */
int Core::connectCopyEngine(const Ultracopier::CopyMode &mode,bool ignoreMode,const CopyEngineManager::returnCopyEngine &returnInformations)
{
    if(returnInformations.canDoOnlyCopy)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Mode force for unknow reason");
        ignoreMode=false;//force mode if need, normaly not used
    }
    CopyInstance newItem;
    newItem.engine=returnInformations.engine;
    if(newItem.engine!=NULL)
    {
        PluginInterface_Themes *theme=ThemesManager::themesManager->getThemesInstance();
        if(theme!=NULL)
        {
            newItem.id=incrementId();
            newItem.lastProgression=0;
            newItem.interface=theme;
            newItem.ignoreMode=ignoreMode;
            newItem.mode=mode;
            newItem.type=returnInformations.type;
            newItem.transferListOperation=returnInformations.transferListOperation;
            newItem.baseTime=0;
            newItem.numberOfFile=0;
            newItem.numberOfTransferedFile=0;
            newItem.currentProgression=0;
            newItem.totalProgression=0;
            newItem.action=Ultracopier::Idle;
            newItem.lastProgression=0;//store the real byte transfered, used in time remaining calculation
            newItem.isPaused=false;
            newItem.baseTime=0;//stored in ms
            newItem.isRunning=false;
            newItem.haveError=false;
            newItem.lastConditionalSync.start();
            newItem.nextConditionalSync=new QTimer();
            newItem.nextConditionalSync->setSingleShot(true);
            newItem.copyEngineIsSync=true;

            if(!ignoreMode)
            {
                newItem.interface->forceCopyMode(mode);
                newItem.engine->forceMode(mode);
            }
            if(copyList.size()==0)
                forUpateInformation.start();
            copyList << newItem;
            connectEngine(copyList.count()-1);
            connectInterfaceAndSync(copyList.count()-1);
            return newItem.id;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to load the interface, copy aborted");
        delete newItem.engine;
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to load the interface, copy aborted"));
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to load the copy engine, copy aborted");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to load the copy engine, copy aborted"));
    }
    return -1;
}

void Core::resetSpeedDetectedEngine()
{
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
        resetSpeedDetected(index);
}

void Core::resetSpeedDetectedInterface()
{
    int index=indexCopySenderInterface();
    if(index!=-1)
        resetSpeedDetected(index);
}

void Core::resetSpeedDetected(const int &index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start on %1").arg(index));
    copyList[index].runningTime.restart();
    copyList[index].lastSpeedDetected.clear();
    copyList[index].lastSpeedTime.clear();
}

void Core::actionInProgess(const Ultracopier::EngineActionInProgress &action)
{
    index=indexCopySenderCopyEngine();
    if(index!=-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("action: %1, from %2").arg(action).arg(index));
        //drop here the duplicate action
        if(copyList[index].action==action)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("The copy engine have send 2x the same EngineActionInProgress"));
            return;
        }
        //update time runing for time remaning caculation
        if(action==Ultracopier::Copying || action==Ultracopier::CopyingAndListing)
        {
            if(!copyList.at(index).isRunning)
            {
                copyList[index].isRunning=true;
                copyList[index].runningTime.restart();
            }
        }
        else
        {
            if(copyList.at(index).isRunning)
            {
                copyList[index].isRunning=false;
                copyList[index].baseTime+=copyList[index].runningTime.elapsed();
            }
        }
        //do sync
        periodicSynchronizationWithIndex(index);
        copyList[index].action=action;
        if(copyList.at(index).interface!=NULL)
            copyList.at(index).interface->actionInProgess(action);
        if(action==Ultracopier::Idle)
        {
            index_sub_loop=0;
            loop_size=copyList.at(index).orderId.size();
            while(index_sub_loop<loop_size)
            {
                emit copyCanceled(copyList.at(index).orderId.at(index_sub_loop));
                index_sub_loop++;
            }
            copyList[index].orderId.clear();
            resetSpeedDetected(index);
        }
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the interface sender");
}

void Core::newFolderListing(const QString &path)
{
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
    {
        copyList[index].folderListing=path;
        copyList.at(index).interface->newFolderListing(path);
    }
}

void Core::isInPause(const bool &isPaused)
{
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
    {
        if(!isPaused)
            resetSpeedDetected(index);
        copyList[index].isPaused=isPaused;
        copyList.at(index).interface->isInPause(isPaused);
    }
}

/// \brief get the right copy instance (copy engine + interface), by signal emited from copy engine
int Core::indexCopySenderCopyEngine()
{
    QObject * senderObject=sender();
    if(senderObject==NULL)
    {
        //QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Qt sender() NULL");
        return -1;
    }
    index=0;
    loop_size=copyList.size();
    while(index<loop_size)
    {
        if(copyList.at(index).engine==senderObject)
            return index;
        index++;
    }
    //QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Sender not located in the list");
    return -1;
}

/// \brief get the right copy instance (copy engine + interface), by signal emited from interface
int Core::indexCopySenderInterface()
{
    QObject * senderObject=sender();
    if(senderObject==NULL)
    {
        //QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Qt sender() NULL");
        return -1;
    }
    index=0;
    loop_size=copyList.size();
    while(index<loop_size)
    {
        if(copyList.at(index).interface==senderObject)
            return index;
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to locate QObject * sender");
    PluginInterface_Themes * interface = qobject_cast<PluginInterface_Themes *>(senderObject);
    if(interface==NULL)
    {
        //QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Qt sender themes NULL");
        return -1;
    }
    index=0;
    while(index<loop_size)
    {
        if(copyList.at(index).interface==interface)
            return index;
        index++;
    }
    //QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Sender not located in the list");
    return -1;
}

void Core::connectEngine(const int &index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start with index: %1: %2").arg(index).arg((quint64)sender()));
    //disconnectEngine(index);

    CopyInstance& currentCopyInstance=copyList[index];
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::newFolderListing,			this,&Core::newFolderListing,Qt::QueuedConnection))//to check to change
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the engine can not work correctly: %1: %2 for newFolderListing()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::actionInProgess,	this,&Core::actionInProgess,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the engine can not work correctly: %1: %2 for actionInProgess()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::isInPause,				this,&Core::isInPause,Qt::QueuedConnection))//to check to change
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the engine can not work correctly: %1: %2 for isInPause()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::cancelAll,					this,&Core::copyInstanceCanceledByEngine,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the engine can not work correctly: %1: %2 for cancelAll()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::error,	this,&Core::error,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the engine can not work correctly: %1: %2 for error()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::rmPath,				this,&Core::rmPath,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the engine can not work correctly: %1: %2 for rmPath()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::mkPath,				this,&Core::mkPath,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the engine can not work correctly: %1: %2 for mkPath()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::syncReady,					this,&Core::syncReady,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the engine can not work correctly: %1: %2 for syncReady()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::canBeDeleted,					this,&Core::deleteCopyEngine,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the engine can not work correctly: %1: %2 for syncReady()").arg(index).arg((quint64)sender()));
}

void Core::connectInterfaceAndSync(const int &index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start with index: %1: %2").arg(index).arg((quint64)sender()));
    //disconnectInterface(index);

    CopyInstance& currentCopyInstance=copyList[index];
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::pause,					currentCopyInstance.engine,&PluginInterface_CopyEngine::pause))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for pause()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::resume,					currentCopyInstance.engine,&PluginInterface_CopyEngine::resume))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for resume()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::skip,				currentCopyInstance.engine,&PluginInterface_CopyEngine::skip))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for skip()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::newSpeedLimitation,		currentCopyInstance.engine,&PluginInterface_CopyEngine::setSpeedLimitation))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for newSpeedLimitation()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::userAddFolder,			currentCopyInstance.engine,&PluginInterface_CopyEngine::userAddFolder))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for userAddFolder()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::userAddFile,			currentCopyInstance.engine,&PluginInterface_CopyEngine::userAddFile))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for userAddFile()").arg(index).arg((quint64)sender()));

    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::removeItems,			currentCopyInstance.engine,&PluginInterface_CopyEngine::removeItems))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for removeItems()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::moveItemsOnTop,		currentCopyInstance.engine,&PluginInterface_CopyEngine::moveItemsOnTop))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for moveItemsOnTop()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::moveItemsUp,			currentCopyInstance.engine,&PluginInterface_CopyEngine::moveItemsUp))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for moveItemsUp()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::moveItemsDown,		currentCopyInstance.engine,&PluginInterface_CopyEngine::moveItemsDown))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for moveItemsDown()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::moveItemsOnBottom,		currentCopyInstance.engine,&PluginInterface_CopyEngine::moveItemsOnBottom))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for moveItemsOnBottom()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::exportTransferList,			currentCopyInstance.engine,&PluginInterface_CopyEngine::exportTransferList))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for exportTransferList()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::importTransferList,			currentCopyInstance.engine,&PluginInterface_CopyEngine::importTransferList))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for importTransferList()").arg(index).arg((quint64)sender()));

    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::newSpeedLimitation,		this,&Core::resetSpeedDetectedInterface))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for newSpeedLimitation()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::resume,					this,&Core::resetSpeedDetectedInterface))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for resume()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::cancel,					this,&Core::copyInstanceCanceledByInterface,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for cancel()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::urlDropped,			this,&Core::urlDropped,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for urlDropped()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::newActionOnList,this,&Core::getActionOnList,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for newActionOnList()").arg(index).arg((quint64)sender()));

    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::pushFileProgression,		currentCopyInstance.interface,&PluginInterface_Themes::setFileProgression,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for pushFileProgression()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::pushGeneralProgression,		currentCopyInstance.interface,&PluginInterface_Themes::setGeneralProgression,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for pushGeneralProgression()").arg(index).arg((quint64)sender()));
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::pushGeneralProgression,		this,&Core::pushGeneralProgression,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("error at connect, the interface can not work correctly: %1: %2 for pushGeneralProgression() for this").arg(index).arg((quint64)sender()));

    currentCopyInstance.interface->setSupportSpeedLimitation(currentCopyInstance.engine->supportSpeedLimitation());
    currentCopyInstance.interface->setCopyType(currentCopyInstance.type);
    currentCopyInstance.interface->setTransferListOperation(currentCopyInstance.transferListOperation);
    currentCopyInstance.interface->actionInProgess(currentCopyInstance.action);
    currentCopyInstance.interface->isInPause(currentCopyInstance.isPaused);
    if(currentCopyInstance.haveError)
        currentCopyInstance.interface->errorDetected();
    QWidget *tempWidget=currentCopyInstance.interface->getOptionsEngineWidget();
    if(tempWidget!=NULL)
        currentCopyInstance.interface->getOptionsEngineEnabled(currentCopyInstance.engine->getOptionsEngine(tempWidget));
    //important, to have the modal dialog
    currentCopyInstance.engine->setInterfacePointer(currentCopyInstance.interface);

    //put entry into the interface
    currentCopyInstance.engine->syncTransferList();

    //force the updating, without wait the timer
    periodicSynchronizationWithIndex(index);
}

void Core::periodicSynchronization()
{
    index_sub_loop=0;
    loop_size=copyList.size();
    while(index_sub_loop<loop_size)
    {
        if(copyList.at(index_sub_loop).action==Ultracopier::Copying || copyList.at(index_sub_loop).action==Ultracopier::CopyingAndListing)
            periodicSynchronizationWithIndex(index_sub_loop);
        index_sub_loop++;
    }
}

void Core::periodicSynchronizationWithIndex(const int &index)
{
    CopyInstance& currentCopyInstance=copyList[index];
    if(currentCopyInstance.engine==NULL || currentCopyInstance.interface==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"some thread is null");
        return;
    }

    /** ***************** Do time calcul ******************* **/
    if(!currentCopyInstance.isPaused)
    {
        //calcul the last difference of the transfere
        realByteTransfered=currentCopyInstance.engine->realByteTransfered();
        diffCopiedSize=0;
        if(realByteTransfered>=currentCopyInstance.lastProgression)
            diffCopiedSize=realByteTransfered-currentCopyInstance.lastProgression;
        currentCopyInstance.lastProgression=realByteTransfered;

        // algo 1:
        // ((double)currentProgression)/totalProgression -> example: done 80% -> 0.8
        // baseTime+runningTime -> example: done into 80s, remaining time: 80/0.8-80=80*(1/0.8-1)=20s
        // algo 2 (not used):
        // remaining byte/current speed

        transferAddedTime=currentCopyInstance.baseTime+currentCopyInstance.runningTime.elapsed();

        //remaining time: (total byte - lastProgression)/byte per ms since the start
        if(currentCopyInstance.totalProgression==0 || currentCopyInstance.currentProgression==0)
            currentCopyInstance.interface->remainingTime(-1);
        else
            currentCopyInstance.interface->remainingTime(transferAddedTime*((double)currentCopyInstance.totalProgression/(double)currentCopyInstance.currentProgression-1)/1000);

        //do the speed calculation
        if(lastProgressionTime.isNull())
            lastProgressionTime.start();
        else
        {
            if((currentCopyInstance.action==Ultracopier::Copying || currentCopyInstance.action==Ultracopier::CopyingAndListing))
            {
                currentCopyInstance.lastSpeedTime << lastProgressionTime.elapsed();
                currentCopyInstance.lastSpeedDetected << diffCopiedSize;
                while(currentCopyInstance.lastSpeedDetected.size()>ULTRACOPIER_MAXVALUESPEEDSTORED)
                {
                    currentCopyInstance.lastSpeedTime.removeFirst();
                    currentCopyInstance.lastSpeedDetected.removeFirst();
                }
                totTime=0;
                totSpeed=0;
                index_sub_loop=0;
                loop_size=currentCopyInstance.lastSpeedDetected.size();
                while(index_sub_loop<loop_size)
                {
                    totTime+=currentCopyInstance.lastSpeedTime.at(index_sub_loop);
                    totSpeed+=currentCopyInstance.lastSpeedDetected.at(index_sub_loop);
                    index_sub_loop++;
                }
                totTime/=1000;
                currentCopyInstance.interface->detectedSpeed(totSpeed/totTime);
            }
            lastProgressionTime.restart();
        }
    }
}

/// \brief the copy engine have canceled the transfer
void Core::copyInstanceCanceledByEngine()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
        copyInstanceCanceledByIndex(index);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the copy engine sender");
}

/// \brief the interface have canceled the transfer
void Core::copyInstanceCanceledByInterface()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    int index=indexCopySenderInterface();
    if(index!=-1)
        copyInstanceCanceledByIndex(index);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the copy engine sender");
}

/// \brief the transfer have been canceled
void Core::copyInstanceCanceledByIndex(const int &index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, remove with the index: "+QString::number(index));
    //disconnectEngine(index);
    //disconnectInterface(index);
    CopyInstance& currentCopyInstance=copyList[index];
    currentCopyInstance.engine->cancel();
    delete currentCopyInstance.nextConditionalSync;
    delete currentCopyInstance.interface;
    index_sub_loop=0;
    loop_size=currentCopyInstance.orderId.size();
    while(index_sub_loop<loop_size)
    {
        emit copyCanceled(currentCopyInstance.orderId.at(index_sub_loop));
        index_sub_loop++;
    }
    currentCopyInstance.orderId.clear();
    copyList.removeAt(index);
    if(copyList.size()==0)
        forUpateInformation.stop();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"copyList.size(): "+QString::number(copyList.size()));
}

/// \brief only when the copy engine say it's ready to delete them self, it call this
void Core::deleteCopyEngine()
{
    QObject * senderObject=sender();
    if(senderObject==NULL)
    {
        //QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Qt sender() NULL");
        return;
    }
    PluginInterface_CopyEngine * copyEngine = static_cast<PluginInterface_CopyEngine *>(senderObject);
    if(copyEngine==NULL)
    {
        //QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Qt sender() NULL");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, delete the copy engine");
    delete copyEngine;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop, delete the copy engine");
}

//error occurred
void Core::error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error)
{
    log.error(path,size,mtime,error);
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
    {
        copyList[index].haveError=true;
        copyList.at(index).interface->errorDetected();
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the copy engine sender");
}

//for the extra logging
void Core::rmPath(const QString &path)
{
    log.rmPath(path);
}

void Core::mkPath(const QString &path)
{
    log.mkPath(path);
}

/// \brief to rsync after a new interface connection
void Core::syncReady()
{
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
        copyList[index].copyEngineIsSync=true;
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the copy engine sender");
}

void Core::getActionOnList(const QList<Ultracopier::ReturnActionOnCopyList> &actionList)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //log to thje file
    if(log.logTransfer())
    {
        int index=0;
        int size=actionList.size();
        while(index<size)
        {
            const Ultracopier::ReturnActionOnCopyList &returnAction=actionList.at(index);
            switch(returnAction.type)
            {
                case Ultracopier::PreOperation:
                    log.newTransferStart(returnAction.addAction);
                break;
                case Ultracopier::RemoveItem:
                    if(returnAction.userAction.moveAt==0)
                        log.newTransferStop(returnAction.addAction);
                    else
                        log.transferSkip(returnAction.addAction);
                break;
                default:
                break;
            }
            index++;
        }
    }
    //send the the interface
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
    {
        if(copyList[index].copyEngineIsSync)
            copyList[index].interface->getActionOnList(actionList);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the copy engine sender");
}

void Core::pushGeneralProgression(const quint64 &current,const quint64 &total)
{
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
    {
        copyList[index].currentProgression=current;
        copyList[index].totalProgression=total;
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the copy engine sender");
}

/// \brief used to drag and drop files
void Core::urlDropped(const QList<QUrl> &urls)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    int index=indexCopySenderInterface();
    if(index!=-1)
    {
        QStringList sources;
        int index_loop=0;
        while(index_loop<urls.size())
        {
            if(!urls.at(index_loop).isEmpty())
                sources << urls.at(index_loop).toLocalFile();
            index_loop++;
        }
        if(sources.size()==0)
            return;
        else
        {
            if(copyList.at(index).ignoreMode)
            {
                QMessageBox::StandardButton reply=QMessageBox::question(copyList.at(index).interface,tr("Transfer mode"),tr("Do you want do as a copy? Else if you reply no, it will be moved."),QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,QMessageBox::Cancel);
                if(reply==QMessageBox::Yes)
                    copyList.at(index).engine->newCopy(sources);
                if(reply==QMessageBox::No)
                    copyList.at(index).engine->newMove(sources);
            }
            else
            {
                if(copyList.at(index).mode==Ultracopier::Copy)
                    copyList.at(index).engine->newCopy(sources);
                else
                    copyList.at(index).engine->newMove(sources);
            }
        }
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the copy engine sender");
}
