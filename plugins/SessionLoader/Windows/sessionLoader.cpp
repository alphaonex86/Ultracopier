/** \file session-loader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86
\version 0.3
\date 2010 */

#if defined (Q_OS_WIN32)
#include <windows.h>
#else
#error "Not under windows, plugin will not work"
#endif

#include "sessionLoader.h"

void SessionLoader::setEnabled(bool newValue)
{
	ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, newValue: "+QString::number(newValue));
	//set value into the variable
	HKEY    ultracopier_regkey;
	//for autostart
	QString runStringApp = "\"" + QApplication::applicationFilePath() + "\"";
	runStringApp.replace( "/", "\\" );
	wchar_t windowsString[255];
	runStringApp.toWCharArray(windowsString);
	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, 0, REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, 0, &ultracopier_regkey, 0);
        if(newValue)
		RegSetValueEx(ultracopier_regkey, TEXT("ultracopier"), 0, REG_SZ, (BYTE*)windowsString, runStringApp.length()*2);
	else
		RegDeleteValue(ultracopier_regkey, TEXT("ultracopier"));
        RegCloseKey(ultracopier_regkey);
}

bool SessionLoader::getEnabled()
{
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
	//return the value into the variable
	HKEY    ultracopier_regkey;
	bool temp=false;
	RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, 0, REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, 0, &ultracopier_regkey, 0);
	DWORD kSize=254;
	if(RegQueryValueEx(ultracopier_regkey,TEXT("ultracopier"),NULL,NULL,(LPBYTE)0,&kSize) == ERROR_SUCCESS)
		temp=true;
	RegCloseKey(ultracopier_regkey);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"return this value: "+QString::number(temp));
        return temp;
}

void SessionLoader::setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion)
{
	Q_UNUSED(options);
	Q_UNUSED(writePath);
	Q_UNUSED(pluginPath);
	Q_UNUSED(portableVersion);
}

/// \brief to get the options widget, NULL if not have
QWidget * SessionLoader::options()
{
	return NULL;
}

/// \brief to reload the translation, because the new language have been loaded
void SessionLoader::newLanguageLoaded()
{
}
