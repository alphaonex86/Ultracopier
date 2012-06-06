/** \file pluginLoader.h
\brief Define the plugin loader
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef PLUGIN_LOADER_TEST_H
#define PLUGIN_LOADER_TEST_H

#include <QObject>
#include <QtCore>
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
	bool allDllIsImportant;
	bool changeOfArchDetected,is64Bits;
signals:
	void newState(CatchState);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	#endif
private slots:
	void setAllDllIsImportant(bool allDllIsImportant);
};

#endif // PLUGIN_LOADER_TEST_H
