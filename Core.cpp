/** \file Core.cpp
\brief Define the class for the core
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QMessageBox>
#include <QtPlugin>
#include <cmath>

#include "Core.h"
#include "ThemesManager.h"
#include "cpp11addition.h"

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
    unsigned int index=0;
    while(index<copyList.size())
    {
        copyList[index].engine->cancel();
        delete copyList.at(index).nextConditionalSync;
        delete copyList.at(index).interface;
        delete copyList.at(index).engine;
        index++;
    }
}

void Core::newCopyWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(openNewCopyEngineInstance(Ultracopier::Copy,false,protocolsUsedForTheSources)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return;
    }
    copyList.back().orderId.push_back(orderId);
    copyList.back().engine->newCopy(sources);
    copyList.back().interface->haveExternalOrder();
}

void Core::newTransfer(const Ultracopier::CopyMode &mode,const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources,const std::string &protocolsUsedForTheDestination,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+stringimplode(sources,";")+", dest: "+destination+", mode: "+std::to_string(mode));
    //search to group the window
    int GroupWindowWhen=stringtoint32(OptionEngine::optionEngine->getOptionValue("Ultracopier","GroupWindowWhen"));
    bool haveSameSource=false,haveSameDestination=false;
    if(GroupWindowWhen!=0)
    {
        bool needConfirmation=stringtobool(OptionEngine::optionEngine->getOptionValue("Ultracopier","confirmToGroupWindows"));
        unsigned int index=0;
        while(index<copyList.size())
        {
            bool rightMode=false;
            if(mode==Ultracopier::Copy)
                rightMode=copyList.at(index).mode==Ultracopier::Copy;
            else
                rightMode=copyList.at(index).mode==Ultracopier::Move;
            if(!copyList.at(index).ignoreMode && rightMode && !copyList.at(index).canceled)
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
                        bool confirmed=true;
                        if(needConfirmation)
                        {
                            QMessageBox::StandardButton reply = QMessageBox::question(copyList.at(index).interface,tr("Group window"),tr("Do you want group the transfer with another actual running transfer?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
                            confirmed=(reply==QMessageBox::Yes);
                        }
                        if(confirmed)
                        {
                            copyList[index].orderId.push_back(orderId);
                            if(mode==Ultracopier::Copy)
                                copyList.at(index).engine->newCopy(sources,destination);
                            else
                                copyList.at(index).engine->newMove(sources,destination);
                            copyList.at(index).interface->haveExternalOrder();
                            return;
                        }
                    }
                }
            }
            index++;
        }
    }
    //else open new windows
    if(openNewCopyEngineInstance(mode,false,protocolsUsedForTheSources,protocolsUsedForTheDestination)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a engine instance"));
        return;
    }
    copyList.back().orderId.push_back(orderId);
    if(mode==Ultracopier::Copy)
        copyList.back().engine->newCopy(sources,destination);
    else
        copyList.back().engine->newMove(sources,destination);
    copyList.back().interface->haveExternalOrder();
}

void Core::newCopy(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources,const std::string &protocolsUsedForTheDestination,const std::string &destination)
{
    newTransfer(Ultracopier::Copy,orderId,protocolsUsedForTheSources,sources,protocolsUsedForTheDestination,destination);
}

void Core::newMove(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources,const std::string &protocolsUsedForTheDestination,const std::string &destination)
{
    newTransfer(Ultracopier::Move,orderId,protocolsUsedForTheSources,sources,protocolsUsedForTheDestination,destination);
}

void Core::newMoveWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources)
{
    if(openNewCopyEngineInstance(Ultracopier::Move,false,protocolsUsedForTheSources)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return;
    }
    copyList.back().orderId.push_back(orderId);
    copyList.back().engine->newMove(sources);
    copyList.back().interface->haveExternalOrder();
}

/// \brief name to open the right copy engine
void Core::addWindowCopyMove(const Ultracopier::CopyMode &mode,const std::string &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+name);
    if(openNewCopyEngineInstance(mode,false,name)==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return;
    }
    ActionOnManualOpen ActionOnManualOpen_value=static_cast<ActionOnManualOpen>(stringtoint32(OptionEngine::optionEngine->getOptionValue("Ultracopier","ActionOnManualOpen")));
    if(ActionOnManualOpen_value!=ActionOnManualOpen_Nothing)
    {
        if(ActionOnManualOpen_value==ActionOnManualOpen_Folder)
            copyList.back().engine->userAddFolder(mode);
        else
            copyList.back().engine->userAddFile(mode);
    }
}

/// \brief name to open the right copy engine
void Core::addWindowTransfer(const std::string &name)
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
void Core::newTransferList(std::string engine,std::string mode,std::string file)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"engine: "+engine+", mode: "+mode+", file: "+file);
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The argument for the mode is not valid");
        QMessageBox::critical(NULL,tr("Error"),tr("The argument for the mode is not valid"));
        return;
    }
    copyList.back().engine->newTransferList(file);
}

bool Core::startNewTransferOneUniqueCopyEngine()
{
    if(copyList.size()!=1)
        return false;

    if(openNewCopyEngineInstance(Ultracopier::Copy,true,std::string())==-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to get a copy engine instance");
        QMessageBox::critical(NULL,tr("Error"),tr("Unable to get a copy engine instance"));
        return false;
    }
    return true;
}

void Core::loadInterface()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //load the extra files to check the themes availability
    if(copyList.size()>0)
    {
        bool error=false;
        unsigned int index=0;
        while(index<copyList.size())
        {
            copyList[index].interface=ThemesManager::themesManager->getThemesInstance();
            if(copyList.at(index).interface==NULL)
            {
                copyInstanceCanceledByIndex(index);
                error=true;
            }
            else
            {
                if(!copyList.at(index).ignoreMode)
                    copyList.at(index).interface->forceCopyMode(copyList.at(index).mode);
                connectInterfaceAndSync(static_cast<unsigned int>(copyList.size()-1));
                copyList.at(index).engine->syncTransferList();
                index++;
            }
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
    size_t index=0;
    while(index<copyList.size())
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

unsigned int Core::incrementId()
{
    do
    {
        nextId++;
        if(nextId>2000000)
            nextId=0;
    } while(vectorcontainsAtLeastOne(idList,nextId));
    return nextId;
}

/** open with specific source/destination
\param move Copy or move
\param ignoreMode if need ignore the mode
\param protocolsUsedForTheSources protocols used for sources
\param protocolsUsedForTheDestination protocols used for destination
*/
int Core::openNewCopyEngineInstance(const Ultracopier::CopyMode &mode,const bool &ignoreMode,
    const std::vector<std::string> &protocolsUsedForTheSources,const std::string &protocolsUsedForTheDestination)
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
int Core::openNewCopyEngineInstance(const Ultracopier::CopyMode &mode,const bool &ignoreMode,const std::string &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, mode: "+std::to_string(mode)+", name: "+name);
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
            newItem.numberOfFile=0;
            newItem.numberOfTransferedFile=0;
            newItem.currentProgression=0;
            newItem.totalProgression=0;
            newItem.action=Ultracopier::Idle;
            newItem.lastProgression=0;//store the real byte transfered, used in time remaining calculation
            newItem.isPaused=false;
            newItem.isRunning=false;
            newItem.haveError=false;
            newItem.lastConditionalSync.start();
            newItem.nextConditionalSync=new QTimer();
            newItem.nextConditionalSync->setSingleShot(true);
            newItem.copyEngineIsSync=true;
            newItem.canceled=false;

            switch(stringtoint32(OptionEngine::optionEngine->getOptionValue("Ultracopier","remainingTimeAlgorithm")))
            {
                default:
                case 0:
                    newItem.remainingTimeAlgo=Ultracopier::RemainingTimeAlgo_Traditional;
                break;
                case 1:
                    newItem.remainingTimeAlgo=Ultracopier::RemainingTimeAlgo_Logarithmic;
                    {
                        int index=0;
                        while(index<ULTRACOPIER_MAXREMAININGTIMECOL)
                        {
                            RemainingTimeLogarithmicColumn remainingTimeLogarithmicColumn;
                            remainingTimeLogarithmicColumn.totalSize=0;
                            remainingTimeLogarithmicColumn.transferedSize=0;
                            newItem.remainingTimeLogarithmicValue.push_back(remainingTimeLogarithmicColumn);
                            index++;
                        }
                    }
                break;
            }

            if(!ignoreMode)
            {
                newItem.interface->forceCopyMode(mode);
                newItem.engine->forceMode(mode);
            }
            if(copyList.size()==0)
                forUpateInformation.start();
            copyList.push_back(newItem);
            connectEngine(static_cast<unsigned int>(copyList.size()-1));
            connectInterfaceAndSync(static_cast<unsigned int>(copyList.size()-1));
            return static_cast<int>(newItem.id);
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
        resetSpeedDetected(static_cast<unsigned int>(index));
}

void Core::resetSpeedDetectedInterface()
{
    int index=indexCopySenderInterface();
    if(index!=-1)
        resetSpeedDetected(static_cast<unsigned int>(index));
}

void Core::resetSpeedDetected(const unsigned int &bindex)
{
    const size_t &index=bindex;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start on "+std::to_string(index));
    switch(copyList.at(index).remainingTimeAlgo)
    {
        case Ultracopier::RemainingTimeAlgo_Logarithmic:
        {
            size_t sub_index=0;
            while(sub_index<ULTRACOPIER_MAXREMAININGTIMECOL)
            {
                copyList[index].remainingTimeLogarithmicValue[sub_index].lastProgressionSpeed.clear();
                copyList[index].remainingTimeLogarithmicValue[sub_index].totalSize=0;
                copyList[index].remainingTimeLogarithmicValue[sub_index].transferedSize=0;
                sub_index++;
            }
        }
        break;
        default:
        case Ultracopier::RemainingTimeAlgo_Traditional:
            copyList[index].lastSpeedDetected.clear();
            copyList[index].lastSpeedTime.clear();
            copyList[index].lastAverageSpeedDetected.clear();
            copyList[index].lastAverageSpeedTime.clear();
        break;
    }
}

void Core::doneTime(const std::vector<std::pair<uint64_t,uint32_t> > &timeList)
{
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
    {
        CopyInstance &copyInstance=copyList[index];
        switch(copyInstance.remainingTimeAlgo)
        {
            case Ultracopier::RemainingTimeAlgo_Logarithmic:
            if(copyInstance.remainingTimeLogarithmicValue.size()<ULTRACOPIER_MAXREMAININGTIMECOL)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"bug, copyInstance.remainingTimeLogarithmicValue.size() "+std::to_string(copyInstance.remainingTimeLogarithmicValue.size())+" <ULTRACOPIER_MAXREMAININGTIMECOL");
            else
            {
                unsigned int sub_index=0;
                while(sub_index<timeList.size())
                {
                    const std::pair<uint64_t,uint32_t> &timeUnit=timeList.at(sub_index);
                    const uint8_t &col=fileCatNumber(timeUnit.first);
                    RemainingTimeLogarithmicColumn &remainingTimeLogarithmicColumn=copyInstance.remainingTimeLogarithmicValue[col];
                    if(copyInstance.remainingTimeLogarithmicValue.size()<=col)
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"bug, copyInstance.remainingTimeLogarithmicValue.size() "+std::to_string(copyInstance.remainingTimeLogarithmicValue.size())+" < col %2"+std::to_string(col));
                        break;
                    }
                    else
                    {
                        if(timeUnit.second>0)
                        {
                            remainingTimeLogarithmicColumn.lastProgressionSpeed.push_back(static_cast<unsigned int>(timeUnit.first/timeUnit.second));
                            if(remainingTimeLogarithmicColumn.lastProgressionSpeed.size()>ULTRACOPIER_MAXVALUESPEEDSTORED)
                                remainingTimeLogarithmicColumn.lastProgressionSpeed.pop_back();
                        }
                    }
                    sub_index++;
                }
            }
            break;
            default:
            case Ultracopier::RemainingTimeAlgo_Traditional:
            break;
        }
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the interface sender");
}

void Core::actionInProgess(const Ultracopier::EngineActionInProgress &action)
{
    int index=indexCopySenderCopyEngine();
    if(index!=-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action: "+std::to_string(action)+", from "+std::to_string(index));
        //drop here the duplicate action
        if(copyList.at(index).action==action)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"The copy engine have send 2x the same EngineActionInProgress");
            return;
        }
        //update time runing for time remaning caculation
        if(action==Ultracopier::Copying || action==Ultracopier::CopyingAndListing)
        {
            if(!copyList.at(index).isRunning)
                copyList[index].isRunning=true;
        }
        else
        {
            if(copyList.at(index).isRunning)
                copyList[index].isRunning=false;
        }
        //do sync
        periodicSynchronizationWithIndex(index);
        copyList[index].action=action;
        if(copyList.at(index).interface!=NULL)
            copyList.at(index).interface->actionInProgess(action);
        if(action==Ultracopier::Idle)
        {
            unsigned int index_sub_loop=0;
            while(index_sub_loop<copyList.at(index).orderId.size())
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

void Core::newFolderListing(const std::string &path)
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
    const QObject * senderObject=sender();
    if(senderObject==NULL)
    {
        //QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Qt sender() NULL");
        return -1;
    }
    unsigned int index=0;
    while(index<copyList.size())
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
    unsigned int index=0;
    while(index<copyList.size())
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
    while(index<copyList.size())
    {
        if(copyList.at(index).interface==interface)
            return index;
        index++;
    }
    //QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Sender not located in the list");
    return -1;
}

void Core::connectEngine(const unsigned int &index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start with index: "+std::to_string(index)+": "+std::to_string((uint64_t)sender()));
    //disconnectEngine(index);

    CopyInstance& currentCopyInstance=copyList[index];
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::newFolderListing,			this,&Core::newFolderListing,Qt::QueuedConnection))//to check to change
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for newFolderListing()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::actionInProgess,	this,&Core::actionInProgess,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for actionInProgess()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::isInPause,				this,&Core::isInPause,Qt::QueuedConnection))//to check to change
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for isInPause()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::cancelAll,					this,&Core::copyInstanceCanceledByEngine,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for cancelAll()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::error,	this,&Core::error,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for error()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::rmPath,				this,&Core::rmPath,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for rmPath()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::mkPath,				this,&Core::mkPath,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for mkPath()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::syncReady,					this,&Core::syncReady,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for syncReady()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::canBeDeleted,					this,&Core::deleteCopyEngine,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for syncReady()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::doneTime,					this,&Core::doneTime,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the engine can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for doneTime()");
}

void Core::connectInterfaceAndSync(const unsigned int &index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start with index: "+std::to_string(index)+": "+std::to_string((uint64_t)sender()));
    //disconnectInterface(index);

    CopyInstance& currentCopyInstance=copyList[index];
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::pause,					currentCopyInstance.engine,&PluginInterface_CopyEngine::pause))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for pause()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::resume,					currentCopyInstance.engine,&PluginInterface_CopyEngine::resume))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for resume()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::skip,				currentCopyInstance.engine,&PluginInterface_CopyEngine::skip))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for skip()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::newSpeedLimitation,		currentCopyInstance.engine,&PluginInterface_CopyEngine::setSpeedLimitation))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for newSpeedLimitation()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::userAddFolder,			currentCopyInstance.engine,&PluginInterface_CopyEngine::userAddFolder))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for userAddFolder()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::userAddFile,			currentCopyInstance.engine,&PluginInterface_CopyEngine::userAddFile))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for userAddFile()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::removeItems,			currentCopyInstance.engine,&PluginInterface_CopyEngine::removeItems))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for removeItems()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::moveItemsOnTop,		currentCopyInstance.engine,&PluginInterface_CopyEngine::moveItemsOnTop))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for moveItemsOnTop()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::moveItemsUp,			currentCopyInstance.engine,&PluginInterface_CopyEngine::moveItemsUp))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for moveItemsUp()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::moveItemsDown,		currentCopyInstance.engine,&PluginInterface_CopyEngine::moveItemsDown))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for moveItemsDown()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::moveItemsOnBottom,		currentCopyInstance.engine,&PluginInterface_CopyEngine::moveItemsOnBottom))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for moveItemsOnBottom()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::exportTransferList,			currentCopyInstance.engine,&PluginInterface_CopyEngine::exportTransferList))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for exportTransferList()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::exportErrorIntoTransferList,			currentCopyInstance.engine,&PluginInterface_CopyEngine::exportErrorIntoTransferList))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for exportErrorIntoTransferList()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::importTransferList,			currentCopyInstance.engine,&PluginInterface_CopyEngine::importTransferList))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for importTransferList()");

    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::newSpeedLimitation,		this,&Core::resetSpeedDetectedInterface))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for newSpeedLimitation()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::resume,					this,&Core::resetSpeedDetectedInterface))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for resume()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::cancel,					this,&Core::copyInstanceCanceledByInterface,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for cancel()");
    if(!connect(currentCopyInstance.interface,&PluginInterface_Themes::urlDropped,			this,&Core::urlDropped,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for urlDropped()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::newActionOnList,this,&Core::getActionOnList,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for newActionOnList()");

    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::pushFileProgression,		currentCopyInstance.interface,&PluginInterface_Themes::setFileProgression,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for pushFileProgression()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::pushGeneralProgression,		currentCopyInstance.interface,&PluginInterface_Themes::setGeneralProgression,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for pushGeneralProgression()");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::pushGeneralProgression,		this,&Core::pushGeneralProgression,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for pushGeneralProgression() for this");
    if(!connect(currentCopyInstance.engine,&PluginInterface_CopyEngine::errorToRetry,		currentCopyInstance.interface,&PluginInterface_Themes::errorToRetry,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"error at connect, the interface can not work correctly: "+std::to_string(index)+": "+std::to_string((uint64_t)sender())+" for errorToRetry() for this");

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
    unsigned int index_sub_loop=0;
    while(index_sub_loop<copyList.size())
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
        quint64 diffCopiedSize=0;
        if(realByteTransfered>=currentCopyInstance.lastProgression)
            diffCopiedSize=realByteTransfered-currentCopyInstance.lastProgression;
        currentCopyInstance.lastProgression=realByteTransfered;

        // algo 1:
        // ((double)currentProgression)/totalProgression -> example: done 80% -> 0.8
        // baseTime+runningTime -> example: done into 80s, remaining time: 80/0.8-80=80*(1/0.8-1)=20s
        // algo 2 (not used):
        // remaining byte/current speed

        //remaining time: (total byte - lastProgression)/byte per ms since the start
        /*if(currentCopyInstance.totalProgression==0 || currentCopyInstance.currentProgression==0)
            currentCopyInstance.interface->remainingTime(-1);
        else if((currentCopyInstance.totalProgression-currentCopyInstance.currentProgression)>1024)
            currentCopyInstance.interface->remainingTime(transferAddedTime*((double)currentCopyInstance.totalProgression/(double)currentCopyInstance.currentProgression-1)/1000);*/

        //do the speed calculation
        if(lastProgressionTime.isNull())
            lastProgressionTime.start();
        else
        {
            if((currentCopyInstance.action==Ultracopier::Copying || currentCopyInstance.action==Ultracopier::CopyingAndListing))
            {
                currentCopyInstance.lastSpeedTime.push_back(lastProgressionTime.elapsed());
                currentCopyInstance.lastSpeedDetected.push_back(diffCopiedSize);
                currentCopyInstance.lastAverageSpeedTime.push_back(lastProgressionTime.elapsed());
                currentCopyInstance.lastAverageSpeedDetected.push_back(diffCopiedSize);
                while(currentCopyInstance.lastSpeedTime.size()>ULTRACOPIER_MAXVALUESPEEDSTORED)
                    currentCopyInstance.lastSpeedTime.erase(currentCopyInstance.lastSpeedTime.cbegin());
                while(currentCopyInstance.lastSpeedDetected.size()>ULTRACOPIER_MAXVALUESPEEDSTORED)
                    currentCopyInstance.lastSpeedDetected.erase(currentCopyInstance.lastSpeedDetected.cbegin());
                while(currentCopyInstance.lastAverageSpeedTime.size()>ULTRACOPIER_MAXVALUESPEEDSTOREDTOREMAININGTIME)
                    currentCopyInstance.lastAverageSpeedTime.erase(currentCopyInstance.lastAverageSpeedTime.cbegin());
                while(currentCopyInstance.lastAverageSpeedDetected.size()>ULTRACOPIER_MAXVALUESPEEDSTOREDTOREMAININGTIME)
                    currentCopyInstance.lastAverageSpeedDetected.erase(currentCopyInstance.lastAverageSpeedDetected.cbegin());
                double totTime=0,totAverageTime=0;
                double totSpeed=0,totAverageSpeed=0;

                //current speed
                unsigned int index_sub_loop=0;
                while(index_sub_loop<currentCopyInstance.lastSpeedDetected.size())
                {
                    totTime+=currentCopyInstance.lastSpeedTime.at(index_sub_loop);
                    totSpeed+=currentCopyInstance.lastSpeedDetected.at(index_sub_loop);
                    index_sub_loop++;
                }
                totTime/=1000;

                //speed to calculate the remaining time
                index_sub_loop=0;
                while(index_sub_loop<currentCopyInstance.lastAverageSpeedDetected.size())
                {
                    totAverageTime+=currentCopyInstance.lastAverageSpeedTime.at(index_sub_loop);
                    totAverageSpeed+=currentCopyInstance.lastAverageSpeedDetected.at(index_sub_loop);
                    index_sub_loop++;
                }
                totAverageTime/=1000;

                if(totTime>0)
                    if(currentCopyInstance.lastAverageSpeedDetected.size()>=ULTRACOPIER_MINVALUESPEED)
                        currentCopyInstance.interface->detectedSpeed(totSpeed/totTime);

                if(totAverageTime>0)
                    if(currentCopyInstance.lastAverageSpeedDetected.size()>=ULTRACOPIER_MINVALUESPEEDTOREMAININGTIME)
                    {
                        if(currentCopyInstance.remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Traditional)
                        {
                            if(totSpeed>0)
                            {
                                //remaining time: (total byte - lastProgression)/byte per ms since the start
                                if(currentCopyInstance.totalProgression==0 || currentCopyInstance.currentProgression==0)
                                    currentCopyInstance.interface->remainingTime(-1);
                                else if((currentCopyInstance.totalProgression-currentCopyInstance.currentProgression)>1024)
                                    currentCopyInstance.interface->remainingTime((currentCopyInstance.totalProgression-currentCopyInstance.currentProgression)/(totAverageSpeed/totAverageTime));
                            }
                            else
                                currentCopyInstance.interface->remainingTime(-1);
                        }
                        else if(currentCopyInstance.remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Logarithmic)
                        {
                            int remainingTimeValue=0;
                            //calculate for each file class
                            index_sub_loop=0;
                            while(index_sub_loop<currentCopyInstance.remainingTimeLogarithmicValue.size())
                            {
                                const RemainingTimeLogarithmicColumn &remainingTimeLogarithmicColumn=currentCopyInstance.remainingTimeLogarithmicValue.at(index_sub_loop);
                                //normal detect
                                const quint64 &remainingSize=remainingTimeLogarithmicColumn.totalSize-remainingTimeLogarithmicColumn.transferedSize;
                                if(remainingTimeLogarithmicColumn.lastProgressionSpeed.size()>=ULTRACOPIER_MINVALUESPEED)
                                {
                                    int average_speed=0;
                                    unsigned int temp_loop_index=0;
                                    while(temp_loop_index<remainingTimeLogarithmicColumn.lastProgressionSpeed.size())
                                    {
                                        average_speed+=remainingTimeLogarithmicColumn.lastProgressionSpeed.at(temp_loop_index);
                                        temp_loop_index++;
                                    }
                                    average_speed/=remainingTimeLogarithmicColumn.lastProgressionSpeed.size();
                                    remainingTimeValue+=remainingSize/average_speed;
                                }
                                //fallback
                                else
                                {
                                    if(totSpeed>0)
                                    {
                                        //remaining time: (total byte - lastProgression)/byte per ms since the start
                                        if(currentCopyInstance.totalProgression==0 || currentCopyInstance.currentProgression==0)
                                            remainingTimeValue+=1;
                                        else if((currentCopyInstance.totalProgression-currentCopyInstance.currentProgression)>1024)
                                            remainingTimeValue+=remainingSize/totAverageSpeed;
                                    }
                                    else
                                        remainingTimeValue+=1;
                                }
                                index_sub_loop++;
                            }
                            currentCopyInstance.interface->remainingTime(remainingTimeValue);
                        }
                        else
                        {}//error case
                    }
            }
            lastProgressionTime.restart();
        }
    }
}

uint8_t Core::fileCatNumber(uint64_t size)
{
    //all is in base 10 to understand more easily
    //drop the big value
    if(size>ULTRACOPIER_REMAININGTIME_BIGFILEMEGABYTEBASE10*1000*1000)
        size=ULTRACOPIER_REMAININGTIME_BIGFILEMEGABYTEBASE10*1000*1000;
    size=size/100;//to group all the too small file into the value 0
    return log10(size);
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
void Core::copyInstanceCanceledByIndex(const unsigned int &index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, remove with the index: "+std::to_string(index));
    //disconnectEngine(index);
    //disconnectInterface(index);
    copyList[index].canceled=true;
    CopyInstance& currentCopyInstance=copyList[index];
    currentCopyInstance.engine->cancel();
    delete currentCopyInstance.nextConditionalSync;
    delete currentCopyInstance.interface;
    unsigned int index_sub_loop=0;
    while(index_sub_loop<currentCopyInstance.orderId.size())
    {
        emit copyCanceled(currentCopyInstance.orderId.at(index_sub_loop));
        index_sub_loop++;
    }
    currentCopyInstance.orderId.clear();
    copyList.erase(copyList.cbegin()+index);
    if(copyList.size()==0)
        forUpateInformation.stop();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"copyList.size(): "+std::to_string(copyList.size()));
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
void Core::error(const std::string &path,const uint64_t &size,const uint64_t &mtime,const std::string &error)
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
void Core::rmPath(const std::string &path)
{
    log.rmPath(path);
}

void Core::mkPath(const std::string &path)
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

void Core::getActionOnList(const std::vector<Ultracopier::ReturnActionOnCopyList> &actionList)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //send the the interface
    const int &index=indexCopySenderCopyEngine();
    if(index!=-1)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start2");
        if(copyList.at(index).copyEngineIsSync)
            copyList.at(index).interface->getActionOnList(actionList);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start3");
        //log to the file and compute the remaining time
        if(log.logTransfer() || copyList.at(index).remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Logarithmic)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start4");
            unsigned int sub_index=0;
            if(log.logTransfer() && copyList.at(index).remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Logarithmic)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start5");
                while(sub_index<actionList.size())
                {
                    const Ultracopier::ReturnActionOnCopyList &returnAction=actionList.at(sub_index);
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
                            if(copyList.at(index).remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Logarithmic)
                            {
                                const quint8 &col=fileCatNumber(returnAction.addAction.size);
                                copyList[index].remainingTimeLogarithmicValue[col].transferedSize+=returnAction.addAction.size;
                            }
                        break;
                        case Ultracopier::AddingItem:
                            if(copyList.at(index).remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Logarithmic)
                            {
                                const quint8 &col=fileCatNumber(returnAction.addAction.size);
                                copyList[index].remainingTimeLogarithmicValue[col].totalSize+=returnAction.addAction.size;
                            }
                        break;
                        default:
                        break;
                    }
                    sub_index++;
                }
            }
            else if(log.logTransfer())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start6");
                while(sub_index<actionList.size())
                {
                    const Ultracopier::ReturnActionOnCopyList &returnAction=actionList.at(sub_index);
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
                            if(copyList.at(index).remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Logarithmic)
                            {
                                const quint8 &col=fileCatNumber(returnAction.addAction.size);
                                copyList[index].remainingTimeLogarithmicValue[col].transferedSize+=returnAction.addAction.size;
                            }
                        break;
                        default:
                        break;
                    }
                    sub_index++;
                }
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start7");
                while(sub_index<actionList.size())
                {
                    const Ultracopier::ReturnActionOnCopyList &returnAction=actionList.at(sub_index);
                    switch(returnAction.type)
                    {
                        case Ultracopier::RemoveItem:
                            if(copyList.at(index).remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Logarithmic)
                            {
                                const quint8 &col=fileCatNumber(returnAction.addAction.size);
                                copyList[index].remainingTimeLogarithmicValue[col].transferedSize+=returnAction.addAction.size;
                            }
                        break;
                        case Ultracopier::AddingItem:
                            if(copyList.at(index).remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Logarithmic)
                            {
                                const quint8 &col=fileCatNumber(returnAction.addAction.size);
                                copyList[index].remainingTimeLogarithmicValue[col].totalSize+=returnAction.addAction.size;
                            }
                        break;
                        default:
                        break;
                    }
                    sub_index++;
                }
            }
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start8");
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start9");
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to locate the copy engine sender");
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start end");
}

void Core::pushGeneralProgression(const uint64_t &current,const uint64_t &total)
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
void Core::urlDropped(const std::vector<std::string> &urls)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    int bindex=indexCopySenderInterface();
    if(bindex!=-1)
    {
        const unsigned int &index=static_cast<unsigned int>(bindex);
        std::vector<std::string> sources;
        unsigned int index_loop=0;
        while(index_loop<urls.size())
        {
            if(!urls.at(index_loop).empty())
                sources.push_back(urls.at(index_loop));
            index_loop++;
        }
        if(sources.size()==0)
            return;
        else
        {
            if(copyList.at(index).ignoreMode)
            {
                QMessageBox::StandardButton reply=QMessageBox::question(copyList.at(index).interface,tr("Transfer mode"),
                           tr("Do you want to copy? If no, it will be moved."),QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,QMessageBox::Cancel);
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
