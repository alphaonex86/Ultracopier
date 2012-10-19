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
#include <windows.h>
#include <tlhelp32.h>

#include "../../../interface/PluginInterface_PluginLoader.h"
#include "Environment.h"
#include "OptionsWidget.h"

/// \brief \brief Define the plugin loader
class PluginLoader : public PluginInterface_PluginLoader
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.PluginLoader/0.4.0.0" FILE "plugin.json")
	Q_INTERFACES(PluginInterface_PluginLoader)
public:
        PluginLoader();
        ~PluginLoader();
	/// \brief try enable/disable the catching
	void setEnabled(bool);
	/// \brief to set resources, writePath can be empty if read only mode
	void setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion);
	/// \brief to get the options widget, NULL if not have
	QWidget * options();
public slots:
	/// \brief to reload the translation, because the new language have been loaded
	void newLanguageLoaded();
private:
	QString pluginPath;
	QStringList importantDll,secondDll;
	QSet<QString> correctlyLoaded;
	bool RegisterShellExtDll(QString dllPath, bool bRegister,bool quiet);
	bool checkExistsDll();
	bool dllChecked;
	bool needBeRegistred;
	bool WINAPI DLLEjecteurW(DWORD dwPid,PWSTR szDLLPath);
	void HardUnloadDLL(QString myDllName);
	OptionInterface * optionsEngine;
	OptionsWidget optionsWidget;
	bool allDllIsImportant,Debug;
	bool changeOfArchDetected,is64Bits;
private slots:
	void setAllDllIsImportant(bool allDllIsImportant);
	void setDebug(bool Debug);
};

#endif // PLUGIN_LOADER_TEST_H
