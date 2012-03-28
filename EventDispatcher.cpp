/** \file EventDispatcher.cpp
\brief Define the class of the event dispatcher
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QFSFileEngine>
#include <QCoreApplication>
#include <QMessageBox>

#include "EventDispatcher.h"
#include "ExtraSocket.h"
#include "CompilerInfo.h"

/// \todo group the facility engine

/// \brief Initiate the ultracopier event dispatcher and check if no other session is running
EventDispatcher::EventDispatcher()
{
	connect(&localListener,SIGNAL(cli(QStringList,bool)),&cliParser,SLOT(cli(QStringList,bool)),Qt::QueuedConnection);
	copyMoveEventIdIndex=0;
	backgroundIcon=NULL;
	sessionloader=new SessionLoader(this);
	copyEngineList=new CopyEngineManager(&optionDialog);
	core=new Core(copyEngineList);
	qRegisterMetaType<CatchState>("CatchState");
	qRegisterMetaType<ListeningState>("ListeningState");
	qRegisterMetaType<QList<QUrl> >("QList<QUrl>");
	qRegisterMetaType<QList<ProgressionItem> >("QList<ProgressionItem>");
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(localListener.tryConnect())
	{
		stopIt=true;
		return;
	}
	stopIt=false;
	localListener.listenServer();
	//show the ultracopier information
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,QString("ULTRACOPIER_VERSION: ")+ULTRACOPIER_VERSION);
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,QString("Qt version: %1 (%2)").arg(qVersion()).arg(QT_VERSION));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,QString("ULTRACOPIER_PLATFORM_NAME: ")+ULTRACOPIER_PLATFORM_NAME);
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,QString("Application path: %1 (%2)").arg(QCoreApplication::applicationFilePath()).arg(QCoreApplication::applicationPid()));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,COMPILERINFO);
	//To lunch some initialization after QApplication::exec() to quit eventually
	lunchInitFunction.setInterval(0);
	lunchInitFunction.setSingleShot(true);
	connect(&lunchInitFunction,SIGNAL(timeout()),this,SLOT(initFunction()),Qt::QueuedConnection);
	lunchInitFunction.start();
	//add the options to use
	QList<QPair<QString, QVariant> > KeysList;
	//add the options hidden, will not show in options pannel
	KeysList.clear();
	KeysList.append(qMakePair(QString("Last_version_used"),QVariant("na")));
	KeysList.append(qMakePair(QString("ActionOnManualOpen"),QVariant(1)));
	KeysList.append(qMakePair(QString("GroupWindowWhen"),QVariant(0)));
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
	KeysList.append(qMakePair(QString("List"),QVariant(QStringList() << "Ultracopier-0.3")));
	options->addOptionGroup("CopyEngine",KeysList);

	connect(themes,		SIGNAL(newThemeOptions(QWidget*,bool,bool)),	&optionDialog,	SLOT(newThemeOptions(QWidget*,bool,bool)));
	connect(&cliParser,	SIGNAL(newCopy(QStringList)),			&copyServer,	SLOT(newCopy(QStringList)));
	connect(&cliParser,	SIGNAL(newCopy(QStringList,QString)),		&copyServer,	SLOT(newCopy(QStringList,QString)));
	connect(&cliParser,	SIGNAL(newMove(QStringList)),			&copyServer,	SLOT(newMove(QStringList)));
	connect(&cliParser,	SIGNAL(newMove(QStringList,QString)),		&copyServer,	SLOT(newMove(QStringList,QString)));
}

/// \brief Destroy the ultracopier event dispatcher
EventDispatcher::~EventDispatcher()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(core!=NULL)
		delete core;
	if(copyEngineList!=NULL)
		delete copyEngineList;
	if(sessionloader!=NULL)
		delete sessionloader;
	if(backgroundIcon!=NULL)
		delete backgroundIcon;
}

/// \brief return if need be close
bool EventDispatcher::shouldBeClosed()
{
	return stopIt;
}

/// \brief Quit ultracopier
void EventDispatcher::quit()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Will quit ultracopier");
	//disconnect(QCoreApplication::instance(),SIGNAL(aboutToQuit()),this,SLOT(quit()));
	QCoreApplication::exit();
}

/// \brief Called when event loop is setup
void EventDispatcher::initFunction()
{
	if(core==NULL || copyEngineList==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to initialize correctly the software");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Initialize the variable of event loop");
	//load the systray icon
	if(backgroundIcon==NULL)
	{
		backgroundIcon=new SystrayIcon();
		//connect the slot
		//quit is for this object
//		connect(core,		SIGNAL(newCanDoOnlyCopy(bool)),					backgroundIcon,	SLOT(newCanDoOnlyCopy(bool)));
		connect(backgroundIcon,	SIGNAL(quit()),this,SLOT(quit()));
		//show option is for OptionEngine object
		connect(backgroundIcon,	SIGNAL(showOptions()),						&optionDialog,	SLOT(show()));
		connect(&copyServer,	SIGNAL(listenerReady(ListeningState,bool,bool)),		backgroundIcon,	SLOT(listenerReady(ListeningState,bool,bool)));
		connect(&copyServer,	SIGNAL(pluginLoaderReady(CatchState,bool,bool)),		backgroundIcon,	SLOT(pluginLoaderReady(CatchState,bool,bool)));
		connect(backgroundIcon,	SIGNAL(tryCatchCopy()),						&copyServer,	SLOT(listen()));
		connect(backgroundIcon,	SIGNAL(tryUncatchCopy()),					&copyServer,	SLOT(close()));
		if(options->getOptionValue("CopyListener","CatchCopyAsDefault").toBool())
			copyServer.listen();
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"copyServer.oneListenerIsLoaded(): "+QString::number(copyServer.oneListenerIsLoaded()));
		//backgroundIcon->readyToListen(copyServer.oneListenerIsLoaded());

		connect(backgroundIcon,	SIGNAL(addWindowCopyMove(CopyMode,QString)),				core,		SLOT(addWindowCopyMove(CopyMode,QString)));
		connect(backgroundIcon,	SIGNAL(addWindowTransfer(QString)),					core,		SLOT(addWindowTransfer(QString)));
		connect(&copyServer,	SIGNAL(newCopy(quint32,QStringList,QStringList)),			core,		SLOT(newCopy(quint32,QStringList,QStringList)));
		connect(&copyServer,	SIGNAL(newCopy(quint32,QStringList,QStringList,QString,QString)),	core,		SLOT(newCopy(quint32,QStringList,QStringList,QString,QString)));
		connect(&copyServer,	SIGNAL(newMove(quint32,QStringList,QStringList)),			core,		SLOT(newMove(quint32,QStringList,QStringList)));
		connect(&copyServer,	SIGNAL(newMove(quint32,QStringList,QStringList,QString,QString)),	core,		SLOT(newMove(quint32,QStringList,QStringList,QString,QString)));
		connect(core,		SIGNAL(copyFinished(quint32,bool)),					&copyServer,	SLOT(copyFinished(quint32,bool)));
		connect(core,		SIGNAL(copyCanceled(quint32)),						&copyServer,	SLOT(copyCanceled(quint32)));
		connect(copyEngineList,	SIGNAL(addCopyEngine(QString,bool)),					backgroundIcon,	SLOT(addCopyEngine(QString,bool)));
		connect(copyEngineList,	SIGNAL(removeCopyEngine(QString)),					backgroundIcon,	SLOT(removeCopyEngine(QString)));
		copyEngineList->setIsConnected();
		copyServer.resendState();
	}
	//conntect the last chance signal before quit
	connect(QCoreApplication::instance(),SIGNAL(aboutToQuit()),this,SLOT(quit()));
	//connect the slot for the help dialog
	connect(backgroundIcon,SIGNAL(showHelp()),&theHelp,SLOT(show()));
}

