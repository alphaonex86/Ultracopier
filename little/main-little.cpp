/** \file main.cpp
\brief Define the main() for the point entry
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QApplication>
#include <QtPlugin>
#include "../Variable.h"
#ifndef ULTRACOPIER_LITTLE_RANDOM
#include "../plugins/CopyEngine/Ultracopier-Spec/CopyEngineFactory.h"
#else
#include "../plugins/CopyEngine/Random/CopyEngineFactory.h"
#endif
#include "../plugins/Themes/Oxygen2/ThemesFactory.h"
#include "OptionsEngineLittle.h"
#include "../FacilityEngine.h"
#include <iostream>

Themes * interface=NULL;
CopyEngine * engine=NULL;
uint64_t lastProgression=0;
uint64_t currentProgression=0;
uint64_t totalProgression=0;//store the file byte transfered, used into the remaining time
QTime lastProgressionTime;
QTimer forUpateInformation;///< used to call \see periodicSynchronization()

/** for RemainingTimeAlgo_Traditional **/
//this speed is for instant speed
std::vector<uint64_t> lastSpeedDetected;//stored in bytes
std::vector<double> lastSpeedTime;//stored in ms
//this speed is average speed on more time to calculate the remaining time
std::vector<uint64_t> lastAverageSpeedDetected;//stored in bytes
std::vector<double> lastAverageSpeedTime;//stored in ms

struct RemainingTimeLogarithmicColumn
{
    std::vector<int> lastProgressionSpeed;
    uint64_t totalSize;
    uint64_t transferedSize;
};
/** for RemainingTimeAlgo_Logarithmic **/
std::vector<RemainingTimeLogarithmicColumn> remainingTimeLogarithmicValue;

void connectEngine();
void connectInterfaceAndSync();
void periodicSynchronization();

/// \brief Define the main() for the point entry
int main(int argc, char *argv[])
{
    QApplication ultracopierApplication(argc, argv);
    ultracopierApplication.setApplicationVersion(ULTRACOPIER_VERSION);
    qRegisterMetaType<Ultracopier::CopyMode>("Ultracopier::CopyMode");
    qRegisterMetaType<Ultracopier::ItemOfCopyList>("Ultracopier::ItemOfCopyList");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<Ultracopier::DebugLevel>("Ultracopier::DebugLevel");
    qRegisterMetaType<Ultracopier::EngineActionInProgress>("Ultracopier::EngineActionInProgress");
    qRegisterMetaType<std::vector<Ultracopier::ReturnActionOnCopyList> >("std::vector<Ultracopier::ReturnActionOnCopyList>");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::vector<Ultracopier::ProgressionItem> >("std::vector<Ultracopier::ProgressionItem>");

    FacilityEngine facilityEngine;
    ThemesFactory themesFactory;
    themesFactory.setResources(new OptionsEngineLittle(),std::string(),std::string(),&facilityEngine,true);
    CopyEngineFactory copyEngineFactory;
    copyEngineFactory.setResources(new OptionsEngineLittle(),std::string(),std::string(),&facilityEngine,true);

    interface=static_cast<Themes *>(themesFactory.getInstance());
    engine=static_cast<CopyEngine *>(copyEngineFactory.getInstance());

    connectEngine();
    connectInterfaceAndSync();
    forUpateInformation.setInterval(ULTRACOPIER_TIME_INTERFACE_UPDATE);
    forUpateInformation.start();
    QObject::connect(&forUpateInformation,	&QTimer::timeout,						&periodicSynchronization);

    const int returnVar=ultracopierApplication.exec();

    delete engine;
    return returnVar;
}

void connectEngine()
{
    bool failed=false;
    failed|=!QObject::connect(engine,&CopyEngine::newFolderListing,			interface,&Themes::newFolderListing,Qt::QueuedConnection);//to check to change
    failed|=!QObject::connect(engine,&CopyEngine::actionInProgess,           interface,&Themes::actionInProgess,Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::isInPause,                 interface,&Themes::isInPause,Qt::QueuedConnection);//to check to change
    failed|=!QObject::connect(engine,&CopyEngine::cancelAll,					QCoreApplication::instance(),&QCoreApplication::quit,Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::canBeDeleted,					QCoreApplication::instance(),&QCoreApplication::quit,Qt::QueuedConnection);
    /*failed|=!QObject::connect(engine,&CopyEngine::error,                     interface,&Themes::error,Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::rmPath,                    interface,&Themes::rmPath,Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::mkPath,                    interface,&Themes::mkPath,Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::syncReady,					interface,&Themes::syncReady,Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::doneTime,					interface,&Themes::doneTime,Qt::QueuedConnection);*/
    if(failed)
    {
        std::cerr << "Little version, connectEngine() failed, abort" << std::endl;
        abort();
    }
}

void connectInterfaceAndSync()
{
    bool failed=false;
    failed|=!QObject::connect(interface,&Themes::pause,                      engine,&CopyEngine::pause);
    failed|=!QObject::connect(interface,&Themes::resume,                     engine,&CopyEngine::resume);
    failed|=!QObject::connect(interface,&Themes::skip,                       engine,&CopyEngine::skip);
    failed|=!QObject::connect(interface,&Themes::newSpeedLimitation,         engine,&CopyEngine::setSpeedLimitation);
    failed|=!QObject::connect(interface,&Themes::userAddFolder,              engine,&CopyEngine::userAddFolder);
    failed|=!QObject::connect(interface,&Themes::userAddFile,                engine,&CopyEngine::userAddFile);
    failed|=!QObject::connect(interface,&Themes::removeItems,                engine,&CopyEngine::removeItems);
    failed|=!QObject::connect(interface,&Themes::moveItemsOnTop,             engine,&CopyEngine::moveItemsOnTop);
    failed|=!QObject::connect(interface,&Themes::moveItemsUp,                engine,&CopyEngine::moveItemsUp);
    failed|=!QObject::connect(interface,&Themes::moveItemsDown,              engine,&CopyEngine::moveItemsDown);
    failed|=!QObject::connect(interface,&Themes::moveItemsOnBottom,          engine,&CopyEngine::moveItemsOnBottom);
    failed|=!QObject::connect(interface,&Themes::exportTransferList,			engine,&CopyEngine::exportTransferList);
    //failed|=!QObject::connect(interface,&Themes::exportErrorIntoTransferList,engine,&CopyEngine::exportErrorIntoTransferList);
    failed|=!QObject::connect(interface,&Themes::importTransferList,			engine,&CopyEngine::importTransferList);

    /*failed|=!QObject::connect(interface,&Themes::newSpeedLimitation,         engine,&CopyEngine::resetSpeedDetectedInterface);
    failed|=!QObject::connect(interface,&Themes::resume,                     engine,&CopyEngine::resetSpeedDetectedInterface);
    failed|=!QObject::connect(interface,&Themes::urlDropped,                 engine,&CopyEngine::urlDropped,Qt::QueuedConnection);*/
    failed|=!QObject::connect(interface,&Themes::cancel,                     engine,&CopyEngine::cancel,Qt::QueuedConnection);

    failed|=!QObject::connect(engine,&CopyEngine::newActionOnList,           interface,&Themes::getActionOnList,	Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::pushFileProgression,		interface,&Themes::setFileProgression,		Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::pushGeneralProgression,	interface,&Themes::setGeneralProgression,		Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::errorToRetry,              interface,&Themes::errorToRetry,		Qt::QueuedConnection);
    failed|=!QObject::connect(engine,&CopyEngine::doneTime,                 interface,&Themes::doneTime,		Qt::QueuedConnection);

    if(failed)
    {
        std::cerr << "Little version, connectEngine() failed, abort" << std::endl;
        abort();
    }
    interface->setSupportSpeedLimitation(engine->supportSpeedLimitation());
    interface->setCopyType(Ultracopier::CopyType::FileAndFolder);
    interface->setTransferListOperation(Ultracopier::TransferListOperation::TransferListOperation_None);
    interface->actionInProgess(Ultracopier::EngineActionInProgress::Idle);
    //interface->isInPause(currentCopyInstance.isPaused);
    interface->isInPause(false);

    interface->setSupportSpeedLimitation(engine->supportSpeedLimitation());
    QWidget *tempWidget=interface->getOptionsEngineWidget();
    if(tempWidget!=NULL)
        interface->getOptionsEngineEnabled(engine->getOptionsEngine(tempWidget));
    //important, to have the modal dialog
    engine->setInterfacePointer(interface);

    //put entry into the interface
    engine->syncTransferList();

    //force the updating, without wait the timer
    periodicSynchronization();
}

void periodicSynchronization()
{
    /** ***************** Do time calcul ******************* **/
    //if(!isPaused)
    {
        //calcul the last difference of the transfere
        uint64_t realByteTransfered=engine->realByteTransfered();
        uint64_t diffCopiedSize=0;
        if(realByteTransfered>=lastProgression)
            diffCopiedSize=realByteTransfered-lastProgression;
        lastProgression=realByteTransfered;

        // algo 1:
        // ((double)currentProgression)/totalProgression -> example: done 80% -> 0.8
        // baseTime+runningTime -> example: done into 80s, remaining time: 80/0.8-80=80*(1/0.8-1)=20s
        // algo 2 (not used):
        // remaining byte/current speed

        //remaining time: (total byte - lastProgression)/byte per ms since the start
        /*if(totalProgression==0 || currentProgression==0)
            interface->remainingTime(-1);
        else if((totalProgression-currentProgression)>1024)
            interface->remainingTime(transferAddedTime*((double)totalProgression/(double)currentProgression-1)/1000);*/

        //do the speed calculation
        if(lastProgressionTime.isNull())
            lastProgressionTime.start();
        else
        {
            //if((action==Ultracopier::Copying || action==Ultracopier::CopyingAndListing))
            {
                lastSpeedTime.push_back(lastProgressionTime.elapsed());
                lastSpeedDetected.push_back(diffCopiedSize);
                lastAverageSpeedTime.push_back(lastProgressionTime.elapsed());
                lastAverageSpeedDetected.push_back(diffCopiedSize);
                while(lastSpeedTime.size()>ULTRACOPIER_MAXVALUESPEEDSTORED)
                    lastSpeedTime.erase(lastSpeedTime.cbegin());
                while(lastSpeedDetected.size()>ULTRACOPIER_MAXVALUESPEEDSTORED)
                    lastSpeedDetected.erase(lastSpeedDetected.cbegin());
                while(lastAverageSpeedTime.size()>ULTRACOPIER_MAXVALUESPEEDSTOREDTOREMAININGTIME)
                    lastAverageSpeedTime.erase(lastAverageSpeedTime.cbegin());
                while(lastAverageSpeedDetected.size()>ULTRACOPIER_MAXVALUESPEEDSTOREDTOREMAININGTIME)
                    lastAverageSpeedDetected.erase(lastAverageSpeedDetected.cbegin());
                double totTime=0,totAverageTime=0;
                double totSpeed=0,totAverageSpeed=0;

                //current speed
                unsigned int index_sub_loop=0;
                while(index_sub_loop<lastSpeedDetected.size())
                {
                    totTime+=lastSpeedTime.at(index_sub_loop);
                    totSpeed+=lastSpeedDetected.at(index_sub_loop);
                    index_sub_loop++;
                }
                totTime/=1000;

                //speed to calculate the remaining time
                index_sub_loop=0;
                while(index_sub_loop<lastAverageSpeedDetected.size())
                {
                    totAverageTime+=lastAverageSpeedTime.at(index_sub_loop);
                    totAverageSpeed+=lastAverageSpeedDetected.at(index_sub_loop);
                    index_sub_loop++;
                }
                totAverageTime/=1000;

                if(totTime>0)
                    if(lastAverageSpeedDetected.size()>=ULTRACOPIER_MINVALUESPEED)
                        interface->detectedSpeed(totSpeed/totTime);

                if(totAverageTime>0)
                    if(lastAverageSpeedDetected.size()>=ULTRACOPIER_MINVALUESPEEDTOREMAININGTIME)
                    {
                        /*if(remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Traditional)
                        {
                            if(totSpeed>0)
                            {
                                //remaining time: (total byte - lastProgression)/byte per ms since the start
                                if(totalProgression==0 || currentProgression==0)
                                    interface->remainingTime(-1);
                                else if((totalProgression-currentProgression)>1024)
                                    interface->remainingTime((totalProgression-currentProgression)/(totAverageSpeed/totAverageTime));
                            }
                            else
                                interface->remainingTime(-1);
                        }
                        else if(remainingTimeAlgo==Ultracopier::RemainingTimeAlgo_Logarithmic)*/
                        {
                            int remainingTimeValue=0;
                            //calculate for each file class
                            index_sub_loop=0;
                            while(index_sub_loop<remainingTimeLogarithmicValue.size())
                            {
                                const RemainingTimeLogarithmicColumn &remainingTimeLogarithmicColumn=remainingTimeLogarithmicValue.at(index_sub_loop);
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
                                        if(totalProgression==0 || currentProgression==0)
                                            remainingTimeValue+=1;
                                        else if((totalProgression-currentProgression)>1024)
                                            remainingTimeValue+=remainingSize/totAverageSpeed;
                                    }
                                    else
                                        remainingTimeValue+=1;
                                }
                                index_sub_loop++;
                            }
                            interface->remainingTime(remainingTimeValue);
                        }
                        /*else
                        {}//error case*/
                    }
            }
            lastProgressionTime.restart();
        }
    }
}
