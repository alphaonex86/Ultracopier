/** \file main.cpp
\brief Define the main() for the point entry
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QApplication>
#include <QtPlugin>

#include "Environment.h"
#include "EventDispatcher.h"
#include "LanguagesManager.h"
#include "ThemesManager.h"
#include "DebugEngine.h"
#include "ResourcesManager.h"
#include "OptionEngine.h"
#include "PluginsManager.h"

#ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    Q_IMPORT_PLUGIN(CopyEngineFactory)
    Q_IMPORT_PLUGIN(ThemesFactory)
    Q_IMPORT_PLUGIN(Listener)
    #ifdef Q_OS_WIN32
        Q_IMPORT_PLUGIN(WindowsExplorerLoader)
        #if !defined(ULTRACOPIER_VERSION_PORTABLE)
        Q_IMPORT_PLUGIN(WindowsSessionLoader)
        #endif
    #endif
#endif

#ifdef ULTRACOPIER_DEBUG
DebugModel *DebugModel::debugModel=NULL;
DebugEngine *DebugEngine::debugEngine=NULL;
#endif
ResourcesManager *ResourcesManager::resourcesManager=NULL;
OptionEngine *OptionEngine::optionEngine=NULL;
PluginsManager *PluginsManager::pluginsManager=NULL;
LanguagesManager *LanguagesManager::languagesManager=NULL;
ThemesManager *ThemesManager::themesManager=NULL;

void registerTheOptions()
{
    OptionEngine::optionEngine=new OptionEngine();

    //register the var
    //add the options to use
    QList<QPair<QString, QVariant> > KeysList;
    //add the options hidden, will not show in options pannel
    KeysList.clear();
    KeysList.append(qMakePair(QStringLiteral("Last_version_used"),QVariant("na")));
    #ifdef ULTRACOPIER_VERSION_ULTIMATE
    KeysList.append(qMakePair(QStringLiteral("key"),QStringLiteral("")));
    #endif
    KeysList.append(qMakePair(QStringLiteral("ActionOnManualOpen"),QVariant(1)));
    KeysList.append(qMakePair(QStringLiteral("GroupWindowWhen"),QVariant(0)));
    KeysList.append(qMakePair(QStringLiteral("displayOSSpecific"),QVariant(true)));
    KeysList.append(qMakePair(QStringLiteral("confirmToGroupWindows"),QVariant(true)));
    KeysList.append(qMakePair(QStringLiteral("giveGPUTime"),QVariant(true)));
    KeysList.append(qMakePair(QStringLiteral("remainingTimeAlgorithm"),QVariant(0)));
    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
    KeysList.append(qMakePair(QStringLiteral("checkTheUpdate"),QVariant(true)));
    #else
    KeysList.append(qMakePair(QStringLiteral("checkTheUpdate"),QVariant(false)));
    #endif
    #endif
    OptionEngine::optionEngine->addOptionGroup("Ultracopier",KeysList);

    KeysList.clear();
    KeysList.append(qMakePair(QStringLiteral("List"),QVariant(QStringList() << QStringLiteral("Ultracopier"))));
    OptionEngine::optionEngine->addOptionGroup(QStringLiteral("CopyEngine"),KeysList);

    //load the GUI option
    QString defaultLogFile="";
    if(ResourcesManager::resourcesManager->getWritablePath()!="")
        defaultLogFile=ResourcesManager::resourcesManager->getWritablePath()+"ultracopier-files.log";
    KeysList.clear();
    KeysList.append(qMakePair(QStringLiteral("enabled"),QVariant(false)));
    KeysList.append(qMakePair(QStringLiteral("file"),QVariant(defaultLogFile)));
    KeysList.append(qMakePair(QStringLiteral("transfer"),QVariant(true)));
    KeysList.append(qMakePair(QStringLiteral("error"),QVariant(true)));
    KeysList.append(qMakePair(QStringLiteral("folder"),QVariant(true)));
    KeysList.append(qMakePair(QStringLiteral("sync"),QVariant(true)));
    KeysList.append(qMakePair(QStringLiteral("transfer_format"),QVariant("[%time%] %source% (%size%) %destination%")));
    KeysList.append(qMakePair(QStringLiteral("error_format"),QVariant("[%time%] %path%, %error%")));
    KeysList.append(qMakePair(QStringLiteral("folder_format"),QVariant("[%time%] %operation% %path%")));
    OptionEngine::optionEngine->addOptionGroup(QStringLiteral("Write_log"),KeysList);

    KeysList.clear();
    KeysList.append(qMakePair(QStringLiteral("CatchCopyAsDefault"),QVariant(true)));
    OptionEngine::optionEngine->addOptionGroup(QStringLiteral("CopyListener"),KeysList);

    KeysList.clear();
    KeysList.append(qMakePair(QStringLiteral("LoadAtSessionStarting"),QVariant(true)));
    OptionEngine::optionEngine->addOptionGroup(QStringLiteral("SessionLoader"),KeysList);
}

/// \brief Define the main() for the point entry
int main(int argc, char *argv[])
{
    int returnCode;
    QApplication ultracopierApplication(argc, argv);
    ultracopierApplication.setApplicationVersion(ULTRACOPIER_VERSION);
    ultracopierApplication.setQuitOnLastWindowClosed(false);
    qRegisterMetaType<PluginsAvailable>("PluginsAvailable");
    qRegisterMetaType<Ultracopier::DebugLevel>("Ultracopier::DebugLevel");
    qRegisterMetaType<Ultracopier::CopyMode>("Ultracopier::CopyMode");
    qRegisterMetaType<Ultracopier::ItemOfCopyList>("Ultracopier::ItemOfCopyList");

    #ifdef ULTRACOPIER_DEBUG
    DebugModel::debugModel=new DebugModel();
    DebugEngine::debugEngine=new DebugEngine();
    #endif
    ResourcesManager::resourcesManager=new ResourcesManager();
    registerTheOptions();

    PluginsManager::pluginsManager=new PluginsManager();
    LanguagesManager::languagesManager=new LanguagesManager();
    ThemesManager::themesManager=new ThemesManager();

    //the main code, event loop of Qt and event dispatcher of ultracopier
    {
        EventDispatcher backgroundRunningInstance;
        if(backgroundRunningInstance.shouldBeClosed())
            returnCode=0;
        else
            returnCode=ultracopierApplication.exec();
    }

    delete ThemesManager::themesManager;
    ThemesManager::themesManager=NULL;
    delete LanguagesManager::languagesManager;
    LanguagesManager::languagesManager=NULL;
    delete PluginsManager::pluginsManager;
    PluginsManager::pluginsManager=NULL;
    delete OptionEngine::optionEngine;
    OptionEngine::optionEngine=NULL;
    delete ResourcesManager::resourcesManager;
    ResourcesManager::resourcesManager=NULL;
    #ifdef ULTRACOPIER_DEBUG
    delete DebugEngine::debugEngine;
    DebugEngine::debugEngine=NULL;
    delete DebugModel::debugModel;
    DebugModel::debugModel=NULL;
    #endif

    return returnCode;
}

