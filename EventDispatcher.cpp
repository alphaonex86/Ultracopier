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
#include "ThemesManager.h"

#ifdef Q_OS_UNIX
    #include <unistd.h>
    #include <sys/types.h>
#endif
#ifdef Q_OS_WIN32
    #include <windows.h>
    #include <tchar.h>
    #include <stdio.h>
    #include <strsafe.h>
    typedef void (WINAPI *PGNSI) (LPSYSTEM_INFO);
    typedef BOOL (WINAPI *PGPI) (DWORD, DWORD, DWORD, DWORD, PDWORD);
#endif

/// \brief Initiate the ultracopier event dispatcher and check if no other session is running
EventDispatcher::EventDispatcher()
{
    qRegisterMetaType<QList<Ultracopier::ReturnActionOnCopyList> >("QList<Ultracopier::ReturnActionOnCopyList>");
    qRegisterMetaType<QList<Ultracopier::ProgressionItem> >("QList<Ultracopier::ProgressionItem>");
    qRegisterMetaType<Ultracopier::EngineActionInProgress>("Ultracopier::EngineActionInProgress");
    qRegisterMetaType<QList<QUrl> >("QList<QUrl>");
    qRegisterMetaType<Ultracopier::ItemOfCopyList>("Ultracopier::ItemOfCopyList");

    copyServer=new CopyListener(&optionDialog);
    connect(&localListener, &LocalListener::cli,                    &cliParser,     &CliParser::cli,Qt::QueuedConnection);
    connect(ThemesManager::themesManager,         &ThemesManager::newThemeOptions,		&optionDialog,	&OptionDialog::newThemeOptions);
    connect(&cliParser,     &CliParser::newCopyWithoutDestination,	copyServer,     &CopyListener::copyWithoutDestination);
    connect(&cliParser,     &CliParser::newCopy,					copyServer,     &CopyListener::copy);
    connect(&cliParser,     &CliParser::newMoveWithoutDestination,	copyServer,     &CopyListener::moveWithoutDestination);
    connect(&cliParser,     &CliParser::newMove,					copyServer,     &CopyListener::move);
    connect(copyServer,     &CopyListener::newClientList,			&optionDialog,  &OptionDialog::newClientList);
    #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
    connect(&cliParser,     &CliParser::tryLoadPlugin,				PluginsManager::pluginsManager,        &PluginsManager::tryLoadPlugin);
    #endif
    copyMoveEventIdIndex=0;
    backgroundIcon=NULL;
    stopIt=false;
    #ifndef ULTRACOPIER_VERSION_PORTABLE
    sessionloader=new SessionLoader(&optionDialog);
    #endif
    copyEngineList=new CopyEngineManager(&optionDialog);
    core=new Core(copyEngineList);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //show the ultracopier information
    #ifdef Q_OS_WIN32
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("Windows version: %1").arg(GetOSDisplayString()));
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("ULTRACOPIER_VERSION: ")+ULTRACOPIER_VERSION);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("Qt version: %1 (%2)").arg(qVersion()).arg(QT_VERSION));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("ULTRACOPIER_PLATFORM_NAME: ")+ULTRACOPIER_PLATFORM_NAME);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("Application path: %1 (%2)").arg(QCoreApplication::applicationFilePath()).arg(QCoreApplication::applicationPid()));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,COMPILERINFO);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("Local socket: ")+ExtraSocket::pathSocket(ULTRACOPIER_SOCKETNAME));
    #if defined(ULTRACOPIER_DEBUG) && defined(ULTRACOPIER_PLUGIN_ALL_IN_ONE)
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("Version as all in one"));
    QObjectList objectList=QPluginLoader::staticInstances();
    int index=0;
    while(index<objectList.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("static plugin: %1").arg(objectList.at(index)->metaObject()->className()));
        index++;
    }
    #endif
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
    KeysList.append(qMakePair(QString("confirmToGroupWindows"),QVariant(true)));
    OptionEngine::optionEngine->addOptionGroup("Ultracopier",KeysList);
    if(OptionEngine::optionEngine->getOptionValue("Ultracopier","Last_version_used")!=QVariant("na") && OptionEngine::optionEngine->getOptionValue("Ultracopier","Last_version_used")!=QVariant(ULTRACOPIER_VERSION))
    {
        //then ultracopier have been updated
    }
    OptionEngine::optionEngine->setOptionValue("Ultracopier","Last_version_used",QVariant(ULTRACOPIER_VERSION));
    int a=OptionEngine::optionEngine->getOptionValue("Ultracopier","ActionOnManualOpen").toInt();
    if(a<0 || a>2)
        OptionEngine::optionEngine->setOptionValue("Ultracopier","ActionOnManualOpen",QVariant(1));
    a=OptionEngine::optionEngine->getOptionValue("Ultracopier","GroupWindowWhen").toInt();
    if(a<0 || a>5)
        OptionEngine::optionEngine->setOptionValue("Ultracopier","GroupWindowWhen",QVariant(0));

    KeysList.clear();
    KeysList.append(qMakePair(QString("List"),QVariant(QStringList() << "Ultracopier")));
    OptionEngine::optionEngine->addOptionGroup("CopyEngine",KeysList);

    connect(&cliParser,	&CliParser::newTransferList,core,	&Core::newTransferList);
}

/// \brief Destroy the ultracopier event dispatcher
EventDispatcher::~EventDispatcher()
{
    if(core!=NULL)
        delete core;
    if(copyEngineList!=NULL)
        delete copyEngineList;
    #ifndef ULTRACOPIER_VERSION_PORTABLE
    if(sessionloader!=NULL)
        delete sessionloader;
    #endif
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
    connect(copyServer,	&CopyListener::newCopyWithoutDestination,	core,		&Core::newCopyWithoutDestination,Qt::DirectConnection);
    connect(copyServer,	&CopyListener::newCopy,						core,		&Core::newCopy,Qt::DirectConnection);
    connect(copyServer,	&CopyListener::newMoveWithoutDestination,	core,		&Core::newMoveWithoutDestination,Qt::DirectConnection);
    connect(copyServer,	&CopyListener::newMove,						core,		&Core::newMove,Qt::DirectConnection);
    connect(core,		&Core::copyFinished,						copyServer,	&CopyListener::copyFinished,Qt::DirectConnection);
    connect(core,		&Core::copyCanceled,						copyServer,	&CopyListener::copyCanceled,Qt::DirectConnection);
    if(localListener.tryConnect())
    {
        stopIt=true;
        QCoreApplication::exit(1);//by 1, return process is in progress
        return;
    }
    localListener.listenServer();
    //load the systray icon
    if(backgroundIcon==NULL)
    {
        backgroundIcon=new SystrayIcon();
        //connect the slot
        //quit is for this object
//		connect(core,		&Core::newCanDoOnlyCopy,					backgroundIcon,	&SystrayIcon::newCanDoOnlyCopy,Qt::DirectConnection);
        connect(backgroundIcon,	&SystrayIcon::quit,this,&EventDispatcher::quit);
        //show option is for OptionEngine object
        connect(backgroundIcon,	&SystrayIcon::showOptions,					&optionDialog,	&OptionDialog::show,Qt::DirectConnection);
        connect(copyServer,	&CopyListener::listenerReady,					backgroundIcon,	&SystrayIcon::listenerReady,Qt::DirectConnection);
        connect(copyServer,	&CopyListener::pluginLoaderReady,				backgroundIcon,	&SystrayIcon::pluginLoaderReady,Qt::DirectConnection);
        connect(backgroundIcon,	&SystrayIcon::tryCatchCopy,					copyServer,	&CopyListener::listen,Qt::DirectConnection);
        connect(backgroundIcon,	&SystrayIcon::tryUncatchCopy,					copyServer,	&CopyListener::close,Qt::DirectConnection);
        if(OptionEngine::optionEngine->getOptionValue("CopyListener","CatchCopyAsDefault").toBool())
            copyServer->listen();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"copyServer.oneListenerIsLoaded(): "+QString::number(copyServer->oneListenerIsLoaded()));
        //backgroundIcon->readyToListen(copyServer.oneListenerIsLoaded());

        connect(backgroundIcon,	&SystrayIcon::addWindowCopyMove,				core,		&Core::addWindowCopyMove,Qt::DirectConnection);
        connect(backgroundIcon,	&SystrayIcon::addWindowTransfer,				core,		&Core::addWindowTransfer,Qt::DirectConnection);
        connect(copyEngineList,	&CopyEngineManager::addCopyEngine,				backgroundIcon,	&SystrayIcon::addCopyEngine,Qt::DirectConnection);
        connect(copyEngineList,	&CopyEngineManager::removeCopyEngine,				backgroundIcon,	&SystrayIcon::removeCopyEngine,Qt::DirectConnection);
        copyEngineList->setIsConnected();
        copyServer->resendState();
    }
    //conntect the last chance signal before quit
    connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,this,&EventDispatcher::quit,Qt::DirectConnection);
    //connect the slot for the help dialog
    connect(backgroundIcon,&SystrayIcon::showHelp,&theHelp,&HelpDialog::show);
    #ifdef ULTRACOPIER_DEBUG
    DebugModel::debugModel->setupTheTimer();
    #endif
}

#ifdef Q_OS_WIN32
QString EventDispatcher::GetOSDisplayString()
{
   QString Os;
   OSVERSIONINFOEX osvi;
   SYSTEM_INFO si;
   PGNSI pGNSI;
   PGPI pGPI;
   BOOL bOsVersionInfoEx;
   DWORD dwType;

   ZeroMemory(&si, sizeof(SYSTEM_INFO));
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);

   if(bOsVersionInfoEx == NULL)
        return "Os detection blocked";

   // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

   pGNSI = (PGNSI) GetProcAddress(
      GetModuleHandle(TEXT("kernel32.dll")),
      "GetNativeSystemInfo");
   if(NULL != pGNSI)
      pGNSI(&si);
   else GetSystemInfo(&si);

   if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId &&
        osvi.dwMajorVersion > 4 )
   {
      if ( osvi.dwMajorVersion == 6 )
      {
         if( osvi.dwMinorVersion == 0 )
         {
            if( osvi.wProductType == VER_NT_WORKSTATION )
                Os+="Windows Vista ";
            else Os+="Windows Server 2008 " ;
         }

         if ( osvi.dwMinorVersion == 1 )
         {
            if( osvi.wProductType == VER_NT_WORKSTATION )
                Os+="Windows 7 ";
            else Os+="Windows Server 2008 R2 ";
         }

         pGPI = (PGPI) GetProcAddress(
            GetModuleHandle(TEXT("kernel32.dll")),
            "GetProductInfo");

         pGPI( osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

         switch( dwType )
         {
            case PRODUCT_ULTIMATE:
               Os+="Ultimate Edition";
               break;
            case PRODUCT_PROFESSIONAL:
               Os+="Professional";
               break;
            case PRODUCT_HOME_PREMIUM:
               Os+="Home Premium Edition";
               break;
            case PRODUCT_HOME_BASIC:
               Os+="Home Basic Edition";
               break;
            case PRODUCT_ENTERPRISE:
               Os+="Enterprise Edition";
               break;
            case PRODUCT_BUSINESS:
               Os+="Business Edition";
               break;
            case PRODUCT_STARTER:
               Os+="Starter Edition";
               break;
            case PRODUCT_CLUSTER_SERVER:
               Os+="Cluster Server Edition";
               break;
            case PRODUCT_DATACENTER_SERVER:
               Os+="Datacenter Edition";
               break;
            case PRODUCT_DATACENTER_SERVER_CORE:
               Os+="Datacenter Edition (core installation)";
               break;
            case PRODUCT_ENTERPRISE_SERVER:
               Os+="Enterprise Edition";
               break;
            case PRODUCT_ENTERPRISE_SERVER_CORE:
               Os+="Enterprise Edition (core installation)";
               break;
            case PRODUCT_ENTERPRISE_SERVER_IA64:
               Os+="Enterprise Edition for Itanium-based Systems";
               break;
            case PRODUCT_SMALLBUSINESS_SERVER:
               Os+="Small Business Server";
               break;
            case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
               Os+="Small Business Server Premium Edition";
               break;
            case PRODUCT_STANDARD_SERVER:
               Os+="Standard Edition";
               break;
            case PRODUCT_STANDARD_SERVER_CORE:
               Os+="Standard Edition (core installation)";
               break;
            case PRODUCT_WEB_SERVER:
               Os+="Web Server Edition";
               break;
         }
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
      {
         if( GetSystemMetrics(SM_SERVERR2) )
            Os+= "Windows Server 2003 R2, ";
         else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
            Os+= "Windows Storage Server 2003";
         else if ( osvi.wSuiteMask & VER_SUITE_WH_SERVER )
            Os+= "Windows Home Server";
         else if( osvi.wProductType == VER_NT_WORKSTATION &&
                  si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
         {
            Os+= "Windows XP Professional x64 Edition";
         }
         else Os+="Windows Server 2003, ";

         // Test for the server type.
         if ( osvi.wProductType != VER_NT_WORKSTATION )
         {
            if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
            {
                if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                   Os+= "Datacenter Edition for Itanium-based Systems";
                else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                   Os+= "Enterprise Edition for Itanium-based Systems";
            }

            else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
            {
                if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                   Os+= "Datacenter x64 Edition";
                else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                   Os+= "Enterprise x64 Edition";
                else Os+= "Standard x64 Edition";
            }

            else
            {
                if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
                   Os+= "Compute Cluster Edition";
                else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                   Os+= "Datacenter Edition";
                else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                   Os+= "Enterprise Edition";
                else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
                   Os+= "Web Edition";
                else Os+= "Standard Edition";
            }
         }
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
      {
         Os+="Windows XP ";
         if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
            Os+= "Home Edition";
         else Os+= "Professional";
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
      {
         Os+="Windows 2000 ";

         if ( osvi.wProductType == VER_NT_WORKSTATION )
         {
            Os+= "Professional";
         }
         else
         {
            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
               Os+= "Datacenter Server";
            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
               Os+= "Advanced Server";
            else Os+= "Server";
         }
      }

       // Include service pack (if any) and build number.

        QString QszCSDVersion=QString::fromUtf16((ushort*)osvi.szCSDVersion);
      if( !QszCSDVersion.isEmpty() )
          Os+=QString(" %1").arg(QszCSDVersion);
        Os+=QString(" (build %1)").arg(osvi.dwBuildNumber);
      if ( osvi.dwMajorVersion >= 6 )
      {
         if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
            Os+= ", 64-bit";
         else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
            Os+=", 32-bit";
      }

      return Os;
   }

   else
      return "This sample does not support this version of Windows.\n";
}
#endif
