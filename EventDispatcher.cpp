/** \file EventDispatcher.cpp
\brief Define the class of the event dispatcher
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QCoreApplication>
#include <QMessageBox>
#include <QWidget>

#include "EventDispatcher.h"
#include "ExtraSocket.h"
#include "CompilerInfo.h"
#include "OSSpecific.h"

#ifdef Q_OS_UNIX
    #include <unistd.h>
    #include <sys/types.h>
#else
    #include <windows.h>
#endif

/// \brief Initiate the ultracopier event dispatcher and check if no other session is running
EventDispatcher::EventDispatcher()
{
    qRegisterMetaType<QList<Ultracopier::ReturnActionOnCopyList> >("QList<Ultracopier::ReturnActionOnCopyList>");
    qRegisterMetaType<QList<Ultracopier::ProgressionItem> >("QList<Ultracopier::ProgressionItem>");
    qRegisterMetaType<Ultracopier::EngineActionInProgress>("Ultracopier::EngineActionInProgress");

    copyServer=new CopyListener(&optionDialog);
    connect(&localListener,&LocalListener::cli,&cliParser,&CliParser::cli,Qt::QueuedConnection);
    connect(themes,		&ThemesManager::newThemeOptions,			&optionDialog,	&OptionDialog::newThemeOptions);
    connect(&cliParser,	&CliParser::newCopyWithoutDestination,			copyServer,	&CopyListener::copyWithoutDestination);
    connect(&cliParser,	&CliParser::newCopy,					copyServer,	&CopyListener::copy);
    connect(&cliParser,	&CliParser::newMoveWithoutDestination,			copyServer,	&CopyListener::moveWithoutDestination);
    connect(&cliParser,	&CliParser::newMove,					copyServer,	&CopyListener::move);
    copyMoveEventIdIndex=0;
    backgroundIcon=NULL;
    stopIt=false;
    sessionloader=new SessionLoader(&optionDialog);
    copyEngineList=new CopyEngineManager(&optionDialog);
    core=new Core(copyEngineList);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //show the ultracopier information
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("ULTRACOPIER_VERSION: ")+ULTRACOPIER_VERSION);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("Qt version: %1 (%2)").arg(qVersion()).arg(QT_VERSION));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("ULTRACOPIER_PLATFORM_NAME: ")+ULTRACOPIER_PLATFORM_NAME);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("Application path: %1 (%2)").arg(QCoreApplication::applicationFilePath()).arg(QCoreApplication::applicationPid()));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,COMPILERINFO);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("Local socket: ")+ExtraSocket::pathSocket(ULTRACOPIER_SOCKETNAME));
    //To lunch some initialization after QApplication::exec() to quit eventually
    lunchInitFunction.setInterval(0);
    lunchInitFunction.setSingleShot(true);
    connect(&lunchInitFunction,&QTimer::timeout,this,&EventDispatcher::initFunction,Qt::QueuedConnection);
    lunchInitFunction.start();
    //add the options to use
    QList<QPair<QString, QVariant> > KeysList;
    //add the options hidden, will not show in options pannel
    KeysList.clear();
    KeysList.append(qMakePair(QString("Last_version_used"),QVariant("na")));
    KeysList.append(qMakePair(QString("ActionOnManualOpen"),QVariant(1)));
    KeysList.append(qMakePair(QString("GroupWindowWhen"),QVariant(0)));
    KeysList.append(qMakePair(QString("displayOSSpecific"),QVariant(true)));
    options->addOptionGroup("Ultracopier",KeysList);
    if(options->getOptionValue("Ultracopier","Last_version_used")!=QVariant("na") && options->getOptionValue("Ultracopier","Last_version_used")!=QVariant(ULTRACOPIER_VERSION))
    {
        //then ultracopier have been updated
    }
    options->setOptionValue("Ultracopier","Last_version_used",QVariant(ULTRACOPIER_VERSION));
    int a=options->getOptionValue("Ultracopier","ActionOnManualOpen").toInt();
    if(a<0 || a>2)
        options->setOptionValue("Ultracopier","ActionOnManualOpen",QVariant(1));
    a=options->getOptionValue("Ultracopier","GroupWindowWhen").toInt();
    if(a<0 || a>5)
        options->setOptionValue("Ultracopier","GroupWindowWhen",QVariant(0));

    KeysList.clear();
    KeysList.append(qMakePair(QString("List"),QVariant(QStringList() << "Ultracopier")));
    options->addOptionGroup("CopyEngine",KeysList);

    connect(&cliParser,	&CliParser::newTransferList,core,	&Core::newTransferList);
}

/// \brief Destroy the ultracopier event dispatcher
EventDispatcher::~EventDispatcher()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(core!=NULL)
        delete core;
    if(copyEngineList!=NULL)
        delete copyEngineList;
    if(sessionloader!=NULL)
        delete sessionloader;
    if(backgroundIcon!=NULL)
        delete backgroundIcon;
    delete copyServer;
}

/// \brief return if need be close
bool EventDispatcher::shouldBeClosed()
{
    return stopIt;
}

/// \brief Quit ultracopier
void EventDispatcher::quit()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Will quit ultracopier");
    //disconnect(QCoreApplication::instance(),SIGNAL(aboutToQuit()),this,SLOT(quit()));
    QCoreApplication::exit();
}

/// \brief Called when event loop is setup
void EventDispatcher::initFunction()
{
    if(core==NULL || copyEngineList==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Unable to initialize correctly the software");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Initialize the variable of event loop");
    connect(copyServer,	&CopyListener::newCopyWithoutDestination,			core,		&Core::newCopyWithoutDestination);
    connect(copyServer,	&CopyListener::newCopy,						core,		&Core::newCopy);
    connect(copyServer,	&CopyListener::newMoveWithoutDestination,			core,		&Core::newMoveWithoutDestination);
    connect(copyServer,	&CopyListener::newMove,						core,		&Core::newMove);
    connect(core,		&Core::copyFinished,						copyServer,	&CopyListener::copyFinished);
    connect(core,		&Core::copyCanceled,						copyServer,	&CopyListener::copyCanceled);
    if(localListener.tryConnect())
    {
        stopIt=true;
        //why before removed???
        QCoreApplication::quit();
        return;
    }
    localListener.listenServer();
    //load the systray icon
    if(backgroundIcon==NULL)
    {
        backgroundIcon=new SystrayIcon();
        //connect the slot
        //quit is for this object
//		connect(core,		&Core::newCanDoOnlyCopy,					backgroundIcon,	&SystrayIcon::newCanDoOnlyCopy);
        connect(backgroundIcon,	&SystrayIcon::quit,this,&EventDispatcher::quit);
        //show option is for OptionEngine object
        connect(backgroundIcon,	&SystrayIcon::showOptions,					&optionDialog,	&OptionDialog::show);
        connect(copyServer,	&CopyListener::listenerReady,					backgroundIcon,	&SystrayIcon::listenerReady);
        connect(copyServer,	&CopyListener::pluginLoaderReady,				backgroundIcon,	&SystrayIcon::pluginLoaderReady);
        connect(backgroundIcon,	&SystrayIcon::tryCatchCopy,					copyServer,	&CopyListener::listen);
        connect(backgroundIcon,	&SystrayIcon::tryUncatchCopy,					copyServer,	&CopyListener::close);
        if(options->getOptionValue("CopyListener","CatchCopyAsDefault").toBool())
            copyServer->listen();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"copyServer.oneListenerIsLoaded(): "+QString::number(copyServer->oneListenerIsLoaded()));
        //backgroundIcon->readyToListen(copyServer.oneListenerIsLoaded());

        connect(backgroundIcon,	&SystrayIcon::addWindowCopyMove,				core,		&Core::addWindowCopyMove);
        connect(backgroundIcon,	&SystrayIcon::addWindowTransfer,				core,		&Core::addWindowTransfer);
        connect(copyEngineList,	&CopyEngineManager::addCopyEngine,				backgroundIcon,	&SystrayIcon::addCopyEngine);
        connect(copyEngineList,	&CopyEngineManager::removeCopyEngine,				backgroundIcon,	&SystrayIcon::removeCopyEngine);
        copyEngineList->setIsConnected();
        copyServer->resendState();
    }
    //conntect the last chance signal before quit
    connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,this,&EventDispatcher::quit);
    //connect the slot for the help dialog
    connect(backgroundIcon,&SystrayIcon::showHelp,&theHelp,&HelpDialog::show);

    if(options->getOptionValue("Ultracopier","displayOSSpecific").toBool())
    {
        OSSpecific oSSpecific;
        oSSpecific.exec();
        if(oSSpecific.dontShowAgain())
            options->setOptionValue("Ultracopier","displayOSSpecific",QVariant(false));
    }
}

