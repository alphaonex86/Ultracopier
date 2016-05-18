/** \file pluginLoader.h
\brief Define the plugin loader
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGIN_LOADER_TEST_H
#define PLUGIN_LOADER_TEST_H

#include <QObject>
#include <QMessageBox>

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
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.PluginLoader/1.0.0.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_PluginLoader)
public:
    WindowsExplorerLoader();
    ~WindowsExplorerLoader();
    /// \brief try enable/disable the catching
    void setEnabled(const bool &needBeRegistred);
    /// \brief to set resources, writePath can be empty if read only mode
    void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion);
    /// \brief to get the options widget, NULL if not have
    QWidget * options();
public slots:
    /// \brief to reload the translation, because the new language have been loaded
    void newLanguageLoaded();
private:
    QString pluginPath;
    QStringList importantDll,secondDll;
    QSet<QString> correctlyLoaded;
    bool RegisterShellExtDll(const QString &dllPath, const bool &bRegister,const bool &quiet);
    bool checkExistsDll();
    bool dllChecked;
    bool needBeRegistred;
    OptionInterface * optionsEngine;
    OptionsWidget *optionsWidget;
    bool allDllIsImportant,Debug;
    bool changeOfArchDetected,is64Bits;
private slots:
    void setAllDllIsImportant(bool allDllIsImportant);
    void setDebug(bool Debug);
};

#endif // PLUGIN_LOADER_TEST_H
