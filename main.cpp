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

DebugModel *DebugModel::debugModel=NULL;
DebugEngine *DebugEngine::debugEngine=NULL;
ResourcesManager *ResourcesManager::resourcesManager=NULL;
OptionEngine *OptionEngine::optionEngine=NULL;
PluginsManager *PluginsManager::pluginsManager=NULL;
LanguagesManager *LanguagesManager::languagesManager=NULL;
ThemesManager *ThemesManager::themesManager=NULL;

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
    //the main code, event loop of Qt and event dispatcher of ultracopier
    {
        DebugModel::debugModel=new DebugModel();
        DebugEngine::debugEngine=new DebugEngine();
        ResourcesManager::resourcesManager=new ResourcesManager();
        OptionEngine::optionEngine=new OptionEngine();
        PluginsManager::pluginsManager=new PluginsManager();
        LanguagesManager::languagesManager=new LanguagesManager();
        ThemesManager::themesManager=new ThemesManager();

        EventDispatcher backgroundRunningInstance;
        if(backgroundRunningInstance.shouldBeClosed())
            returnCode=0;
        else
            returnCode=ultracopierApplication.exec();

        delete ThemesManager::themesManager;
        delete LanguagesManager::languagesManager;
        delete PluginsManager::pluginsManager;
        delete OptionEngine::optionEngine;
        delete ResourcesManager::resourcesManager;
        delete DebugEngine::debugEngine;
        delete DebugModel::debugModel;
    }
    return returnCode;
}

