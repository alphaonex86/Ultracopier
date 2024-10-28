/** \file EventDispatcher.cpp
\brief Define the class of the event dispatcher
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QCoreApplication>
#include <QMessageBox>
#include <QWidget>
#include <QStorageInfo>
#include <iostream>

#include "EventDispatcher.h"
#include "ExtraSocket.h"
#include "CompilerInfo.h"
#include "ThemesManager.h"
#include "cpp11addition.h"
#include "ProductKey.h"

#ifdef Q_OS_UNIX
    #include <unistd.h>
    #include <sys/types.h>
#endif
#ifdef Q_OS_WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <tchar.h>
    #include <stdio.h>
    #include <strsafe.h>
#endif
#ifdef Q_OS_MAC
#include <QStringList>
#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#endif

/// \brief Initiate the ultracopier event dispatcher and check if no other session is running
EventDispatcher::EventDispatcher()
{
    qRegisterMetaType<QList<Ultracopier::ReturnActionOnCopyList> >("QList<Ultracopier::ReturnActionOnCopyList>");
    qRegisterMetaType<QList<Ultracopier::ProgressionItem> >("QList<Ultracopier::ProgressionItem>");
    qRegisterMetaType<Ultracopier::EngineActionInProgress>("Ultracopier::EngineActionInProgress");
    qRegisterMetaType<QList<QUrl> >("QList<QUrl>");
    qRegisterMetaType<Ultracopier::ItemOfCopyList>("Ultracopier::ItemOfCopyList");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<std::vector<Ultracopier::ProgressionItem> >("std::vector<Ultracopier::ProgressionItem>");
    qRegisterMetaType<std::vector<Ultracopier::ReturnActionOnCopyList> >("std::vector<Ultracopier::ReturnActionOnCopyList>");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<QLocalSocket::LocalSocketError >("QLocalSocket::LocalSocketError");

    copyServer=new CopyListener(&optionDialog);
    if(!connect(&localListener, &LocalListener::cli,                    &cliParser,     &CliParser::cli,Qt::QueuedConnection))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(ThemesManager::themesManager,         &ThemesManager::newThemeOptions,	&optionDialog,	&OptionDialog::newThemeOptions))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(&cliParser,     &CliParser::newCopyWithoutDestination,	copyServer,     &CopyListener::copyWithoutDestination))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(&cliParser,     &CliParser::newCopy,					copyServer,     &CopyListener::copy))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(&cliParser,     &CliParser::newMoveWithoutDestination,	copyServer,     &CopyListener::moveWithoutDestination))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(&cliParser,     &CliParser::newMove,					copyServer,     &CopyListener::move))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(copyServer,     &CopyListener::newClientList,			&optionDialog,  &OptionDialog::newClientList))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
    if(!connect(&cliParser,     &CliParser::tryLoadPlugin,				PluginsManager::pluginsManager,        &PluginsManager::tryLoadPlugin))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
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
    #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Windows version: "+GetOSDisplayString());
    #endif
    #ifdef __STDC_VERSION__
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"__STDC_VERSION__: "+std::to_string(__STDC_VERSION__));
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,std::string("ULTRACOPIER_VERSION: ")+FacilityEngine::version());
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,std::string("Qt version: ")+qVersion()+" "+std::to_string(QT_VERSION));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,std::string("ULTRACOPIER_PLATFORM_NAME: ")+ULTRACOPIER_PLATFORM_NAME.toStdString());
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Application path: "+QCoreApplication::applicationFilePath().toStdString()+" "+std::to_string(QCoreApplication::applicationPid()));
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,COMPILERINFO);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Local socket: "+ExtraSocket::pathSocket(ULTRACOPIER_SOCKETNAME));
    #if defined(ULTRACOPIER_DEBUG) && defined(ULTRACOPIER_PLUGIN_ALL_IN_ONE)
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Version as all in one");
            QObjectList objectList=QPluginLoader::staticInstances();
            int index=0;
            while(index<objectList.size())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"static plugin: "+objectList.at(index)->metaObject()->className().toStdString());
                index++;
            }
        #else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Version as all in one, direct");
        #endif
    #endif

    {
        const QList<QStorageInfo> mountedVolumesList=QStorageInfo::mountedVolumes();
        int index=0;
        while(index<mountedVolumesList.size())
        {
            #ifdef Q_OS_WIN32
            const int typeint=GetDriveTypeA(mountedVolumesList.at(index).rootPath().toStdString().c_str());
            std::string type("Unknown type ("+std::to_string(typeint)+")");
            switch(typeint)
            {
            case 0: type="Cannot be determined";break;
            case 1: type="Invalid root path/Not available";break;
            case 2: type="Removable";break;
            case 3: type="Fixed";break;
            case 4: type="Network";break;
            case 5: type="CD-ROM";break;
            case 6: type="RAMDISK";break;
            default:break;
            }
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"mountSysPoint: "+mountedVolumesList.at(index).rootPath().toStdString()+" "+type);
            #else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"mountSysPoint: "+mountedVolumesList.at(index).rootPath().toStdString());
            #endif
            index++;
        }
        if(mountedVolumesList.isEmpty())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"mountSysPoint is empty");
    }

    //To lunch some initialization after QApplication::exec() to quit eventually
    lunchInitFunction.setInterval(0);
    lunchInitFunction.setSingleShot(true);
    connect(&lunchInitFunction,&QTimer::timeout,this,&EventDispatcher::initFunction,Qt::QueuedConnection);
    lunchInitFunction.start();
    if(OptionEngine::optionEngine->getOptionValue("Ultracopier","Last_version_used")!="na" && OptionEngine::optionEngine->getOptionValue("Ultracopier","Last_version_used")!=FacilityEngine::version())
    {
        //then ultracopier have been updated
    }
    OptionEngine::optionEngine->setOptionValue("Ultracopier","Last_version_used",FacilityEngine::version());
    unsigned int a=stringtouint32(OptionEngine::optionEngine->getOptionValue("Ultracopier","ActionOnManualOpen"));
    if(a>2)
        OptionEngine::optionEngine->setOptionValue("Ultracopier","ActionOnManualOpen","1");
    a=stringtouint32(OptionEngine::optionEngine->getOptionValue("Ultracopier","GroupWindowWhen"));
    if(a>5)
        OptionEngine::optionEngine->setOptionValue("Ultracopier","GroupWindowWhen","0");

    connect(&cliParser,	&CliParser::newTransferList,core,	&Core::newTransferList);
}

/// \brief Destroy the ultracopier event dispatcher
EventDispatcher::~EventDispatcher()
{
    if(core!=NULL)
    {
        delete core;
        core=NULL;
    }
    if(copyEngineList!=NULL)
    {
        delete copyEngineList;
        copyEngineList=NULL;
    }
    #ifndef ULTRACOPIER_VERSION_PORTABLE
    if(sessionloader!=NULL)
    {
        delete sessionloader;
        sessionloader=NULL;
    }
    #endif
    if(backgroundIcon!=NULL)
    {
        delete backgroundIcon;
        backgroundIcon=NULL;
    }
    if(copyServer!=NULL)
    {
        delete copyServer;
        copyServer=NULL;
    }
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
    disconnect(QCoreApplication::instance(),SIGNAL(aboutToQuit()),this,SLOT(quit()));
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
    if(!connect(copyServer,	&CopyListener::newCopyWithoutDestination,	core,		&Core::newCopyWithoutDestination,Qt::DirectConnection))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(copyServer,	&CopyListener::newCopy,						core,		&Core::newCopy,Qt::DirectConnection))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(copyServer,	&CopyListener::newMoveWithoutDestination,	core,		&Core::newMoveWithoutDestination,Qt::DirectConnection))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(copyServer,	&CopyListener::newMove,						core,		&Core::newMove,Qt::DirectConnection))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(core,		&Core::copyFinished,						copyServer,	&CopyListener::copyFinished,Qt::DirectConnection))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    if(!connect(core,		&Core::copyCanceled,						copyServer,	&CopyListener::copyCanceled,Qt::DirectConnection))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
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
        if(!connect(backgroundIcon,	&SystrayIcon::quit,this,&EventDispatcher::quit))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        //show option is for OptionEngine object
        if(!connect(backgroundIcon,	&SystrayIcon::showOptions,					&optionDialog,	&OptionDialog::show,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(backgroundIcon,	&SystrayIcon::showProductKey,					ProductKey::productKey,	&ProductKey::show,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(core,	&Core::askProductKey,					ProductKey::productKey,	&ProductKey::show,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(ProductKey::productKey,	&ProductKey::changeToUltimate,backgroundIcon,	&SystrayIcon::changeToUltimate,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(ProductKey::productKey,	&ProductKey::changeToUltimate,core,	&Core::changeToUltimate,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(&cliParser,     &CliParser::showOptions,					&optionDialog,	&OptionDialog::show,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(copyServer,	&CopyListener::listenerReady,					backgroundIcon,	&SystrayIcon::listenerReady,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(copyServer,	&CopyListener::pluginLoaderReady,				backgroundIcon,	&SystrayIcon::pluginLoaderReady,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(backgroundIcon,	&SystrayIcon::tryCatchCopy,					copyServer,	&CopyListener::listen,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(backgroundIcon,	&SystrayIcon::tryUncatchCopy,				copyServer,	&CopyListener::close,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(stringtobool(OptionEngine::optionEngine->getOptionValue("CopyListener","CatchCopyAsDefault")))
            copyServer->listen();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"copyServer.oneListenerIsLoaded(): "+std::to_string(copyServer->oneListenerIsLoaded()));
        //backgroundIcon->readyToListen(copyServer.oneListenerIsLoaded());

        #ifdef ULTRACOPIER_DEBUG
        if(!connect(backgroundIcon,	&SystrayIcon::saveBugReport,                    DebugEngine::debugEngine,		&DebugEngine::saveBugReport,Qt::QueuedConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        #endif
        if(!connect(backgroundIcon,	&SystrayIcon::addWindowCopyMove,				core,		&Core::addWindowCopyMove,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(backgroundIcon,	&SystrayIcon::addWindowTransfer,				core,		&Core::addWindowTransfer,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(copyEngineList,	&CopyEngineManager::addCopyEngine,				backgroundIcon,	&SystrayIcon::addCopyEngine,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(copyEngineList,	&CopyEngineManager::removeCopyEngine,			backgroundIcon,	&SystrayIcon::removeCopyEngine,Qt::DirectConnection))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        #ifdef ULTRACOPIER_INTERNET_SUPPORT
        if(!connect(&internetUpdater,&InternetUpdater::newUpdate,                   backgroundIcon, &SystrayIcon::newUpdate))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(&theHelp,&HelpDialog::checkUpdate,                              &internetUpdater,&InternetUpdater::checkUpdate))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(&internetUpdater,&InternetUpdater::newUpdate,                   &theHelp,&HelpDialog::newUpdate))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(&internetUpdater,&InternetUpdater::noNewUpdate,                 &theHelp,&HelpDialog::noNewUpdate))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        if(!connect(&internetUpdater,&InternetUpdater::errorUpdate,                 &theHelp,&HelpDialog::errorUpdate))
        {
            std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
            abort();
        }
        #endif
        copyEngineList->setIsConnected();
        copyServer->resendState();

        connect(&cliParser,     &CliParser::showSystrayMessage,                    backgroundIcon,&SystrayIcon::showSystrayMessage,Qt::QueuedConnection);
    }
    //connect the slot for the help dialog
    if(!connect(backgroundIcon,&SystrayIcon::showHelp,&theHelp,&HelpDialog::show))
    {
        std::cerr << "connect error at " << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
        abort();
    }
    #ifdef ULTRACOPIER_DEBUG
    DebugModel::debugModel->setupTheTimer();
    #endif
}

#ifdef Q_OS_WIN32
std::string EventDispatcher::GetOSDisplayString()
{
   QString Os;
   OSVERSIONINFOEX osvi;
   SYSTEM_INFO si;
   BOOL bOsVersionInfoEx;

   ZeroMemory(&si, sizeof(SYSTEM_INFO));
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);

   if(bOsVersionInfoEx == 0)
        return "Os detection blocked";
    GetNativeSystemInfo(&si);

   if(VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && osvi.dwMajorVersion>4)
   {
        if(osvi.dwMajorVersion==10)
        {
            switch(osvi.dwMinorVersion)
            {
            case 0:
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+=QStringLiteral("Windows 10/11 ");
                else Os+=QStringLiteral("Windows Server 2016 ");
                break;
            default:
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+=QStringLiteral("Windows (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
                else Os+=QStringLiteral("Windows Server (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
                break;
            }
        }
      else if(osvi.dwMajorVersion==6)
      {
          switch(osvi.dwMinorVersion)
          {
            case 0:
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+=QStringLiteral("Windows Vista ");
                else Os+=QStringLiteral("Windows Server 2008 ");
            break;
            case 1:
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+=QStringLiteral("Windows 7 ");
                else Os+=QStringLiteral("Windows Server 2008 R2 ");
            break;
            case 2:
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+=QStringLiteral("Windows 8 ");
                else Os+=QStringLiteral("Windows Server 2012 ");
            break;
            case 3:
                if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+=QStringLiteral("Windows 9.1 ");
                else Os+=QStringLiteral("Windows Server 2012 R2 ");
            break;
            default:
                 if(osvi.wProductType==VER_NT_WORKSTATION)
                    Os+=QStringLiteral("Windows (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
                 else Os+=QStringLiteral("Windows Server (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
            break;
          }
      }
      else if(osvi.dwMajorVersion==5)
      {
            switch(osvi.dwMinorVersion)
            {
                case 0:
                    Os+=QStringLiteral("Windows 2000 ");
                    if(osvi.wProductType==VER_NT_WORKSTATION)
                       Os+=QStringLiteral("Professional");
                    else
                    {
                       if(osvi.wSuiteMask & VER_SUITE_DATACENTER)
                          Os+=QStringLiteral("Datacenter Server");
                       else if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                          Os+=QStringLiteral("Advanced Server");
                       else Os+=QStringLiteral("Server");
                    }
                break;
                case 1:
                    Os+=QStringLiteral("Windows XP ");
                    if(osvi.wSuiteMask & VER_SUITE_PERSONAL)
                       Os+=QStringLiteral("Home Edition");
                    else Os+=QStringLiteral("Professional");
                break;
                case 2:
                    if(GetSystemMetrics(SM_SERVERR2))
                        Os+=QStringLiteral("Windows Server 2003 R2, ");
                    else if(osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
                        Os+=QStringLiteral("Windows Storage Server 2003");
                    else if(osvi.wSuiteMask & VER_SUITE_WH_SERVER )
                        Os+=QStringLiteral("Windows Home Server");
                    else if(osvi.wProductType==VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
                        Os+=QStringLiteral("Windows XP Professional x64 Edition");
                    else Os+=QStringLiteral("Windows Server 2003, ");
                    // Test for the server type.
                    if(osvi.wProductType!=VER_NT_WORKSTATION )
                    {
                        if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64)
                        {
                            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                                Os+=QStringLiteral("Datacenter Edition for Itanium-based Systems");
                            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                                Os+=QStringLiteral("Enterprise Edition for Itanium-based Systems");
                        }
                        else if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
                        {
                            if(osvi.wSuiteMask & VER_SUITE_DATACENTER)
                                Os+=QStringLiteral("Datacenter x64 Edition");
                            else if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                                Os+=QStringLiteral("Enterprise x64 Edition");
                            else Os+=QStringLiteral("Standard x64 Edition");
                        }
                        else
                        {
                            if(osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER)
                                Os+=QStringLiteral("Compute Cluster Edition");
                            else if( osvi.wSuiteMask & VER_SUITE_DATACENTER)
                                Os+=QStringLiteral("Datacenter Edition");
                            else if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                                Os+=QStringLiteral("Enterprise Edition");
                            else if(osvi.wSuiteMask & VER_SUITE_BLADE)
                                Os+=QStringLiteral("Web Edition");
                            else Os+=QStringLiteral("Standard Edition");
                        }
                    }
                break;
            }
        }
        else
        {
            if(osvi.wProductType==VER_NT_WORKSTATION)
                Os+=QStringLiteral("Windows (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
            else Os+=QStringLiteral("Windows Server (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
        }

        // Include service pack (if any) and build number.
        QString QszCSDVersion=QString::fromUtf16((ushort*)osvi.szCSDVersion);
        if(!QszCSDVersion.isEmpty())
            Os+=QStringLiteral(" %1").arg(QszCSDVersion);
        Os+=QStringLiteral(" (build %1)").arg(osvi.dwBuildNumber);
        if(osvi.dwMajorVersion >= 6)
        {
            if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
                Os+=QStringLiteral(", 64-bit");
            else if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL)
                Os+=QStringLiteral(", 32-bit");
        }
    }
    else
    {
       if(osvi.wProductType==VER_NT_WORKSTATION)
           Os+=QStringLiteral("Windows (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
       else Os+=QStringLiteral("Windows Server (dwMajorVersion: %1, dwMinorVersion: %2)").arg(osvi.dwMinorVersion).arg(osvi.dwMinorVersion);
    }
    return Os.toStdString();
}
#endif

#ifdef Q_OS_MAC
std::string EventDispatcher::GetOSDisplayString()
{
        QStringList key;
    QStringList string;
    QFile xmlFile(QStringLiteral("/System/Library/CoreServices/SystemVersion.plist"));
    if(xmlFile.open(QIODevice::ReadOnly))
    {
        QString content=xmlFile.readAll();
        xmlFile.close();
        QString errorStr;
        int errorLine;
        int errorColumn;
        QDomDocument domDocument;
        if (!domDocument.setContent(content, false, &errorStr,&errorLine,&errorColumn))
            return "Mac OS X";
        else
        {
            QDomElement root = domDocument.documentElement();
            if(root.tagName()!=QStringLiteral("plist"))
                return "Mac OS X";
            else
            {
                if(root.isElement())
                {
                    QDomElement SubChild=root.firstChildElement(QStringLiteral("dict"));
                    while(!SubChild.isNull())
                    {
                        if(SubChild.isElement())
                        {
                            QDomElement SubChild2=SubChild.firstChildElement(QStringLiteral("key"));
                            while(!SubChild2.isNull())
                            {
                                if(SubChild2.isElement())
                                    key << SubChild2.text();
                                else
                                    return "Mac OS X";
                                SubChild2 = SubChild2.nextSiblingElement(QStringLiteral("key"));
                            }
                            SubChild2=SubChild.firstChildElement(QStringLiteral("string"));
                            while(!SubChild2.isNull())
                            {
                                if(SubChild2.isElement())
                                    string << SubChild2.text();
                                else
                                    return "Mac OS X";
                                SubChild2 = SubChild2.nextSiblingElement(QStringLiteral("string"));
                            }
                        }
                        else
                            return "Mac OS X";
                        SubChild = SubChild.nextSiblingElement(QStringLiteral("property"));
                    }
                }
                else
                    return "Mac OS X";
            }
        }
    }
    if(key.size()!=string.size())
        return "Mac OS X";
    int index=0;
    while(index<key.size())
    {
        if(key.at(index)==QStringLiteral("ProductVersion"))
            return "Mac OS X "+string.at(index).toStdString();
        index++;
    }
    return "Mac OS X";
}
#endif
