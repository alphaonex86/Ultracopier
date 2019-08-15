/** \file pluginLoader.h
\brief Define the plugin loader
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGIN_LOADER_TEST_H
#define PLUGIN_LOADER_TEST_H

#include <QObject>
#include <QMessageBox>
#include <unordered_set>

#include <QString>
#include <QStringList>
#include <QProcess>
#include <QSet>
#ifdef Q_OS_WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <tlhelp32.h>
#endif

#include "../../../interface/PluginInterface_PluginLoader.h"
#include "Environment.h"
#include "OptionsWidget.h"

/// \brief \brief Define the plugin loader
class WindowsExplorerLoader : public PluginInterface_PluginLoader
{
    Q_OBJECT
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.PluginLoader/1.2.4.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_PluginLoader)
    #endif
public:
    WindowsExplorerLoader();
    ~WindowsExplorerLoader();
    /// \brief try enable/disable the catching
    void setEnabled(const bool &needBeRegistred) override;
    /// \brief to set resources, writePath can be empty if read only mode
    void setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,const bool &portableVersion) override;
    /// \brief to get the options widget, NULL if not have
    QWidget * options() override;
public slots:
    /// \brief to reload the translation, because the new language have been loaded
    void newLanguageLoaded() override;
private:
    std::string pluginPath;
    std::vector<std::string> importantDll,secondDll;
    std::unordered_set<std::string> correctlyLoaded;
    bool RegisterShellExtDll(std::string dllPath, const bool &bRegister, const bool &quiet);
    bool checkExistsDll();
    bool dllChecked;
    bool needBeRegistred;
    OptionInterface * optionsEngine;
    OptionsWidget *optionsWidget;
    bool allDllIsImportant,allUserIsImportant,Debug;
    bool changeOfArchDetected,is64Bits;
private slots:
    void setAllDllIsImportant(bool allDllIsImportant);
    void setAllUserIsImportant(bool allDllIsImportant);
    void setDebug(bool Debug);
};

#endif // PLUGIN_LOADER_TEST_H
