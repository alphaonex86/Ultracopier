/** \file session-loader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86 */

#include "sessionLoader.h"

#include <QCoreApplication>

#ifdef Q_OS_WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #error "This plugin will only work under Windows"
#endif

void WindowsSessionLoader::setEnabled(const bool &newValue)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, newValue: "+QString::number(newValue));
    //set value into the variable
    HKEY    ultracopier_regkey;
    //for autostart
    QString runStringApp = "\"" + QCoreApplication::applicationFilePath() + "\"";
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

bool WindowsSessionLoader::getEnabled() const
{
    //return the value into the variable
    HKEY    ultracopier_regkey;
    bool temp=false;
    RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, 0, REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, 0, &ultracopier_regkey, 0);
    DWORD kSize=254;
    if(RegQueryValueEx(ultracopier_regkey,TEXT("ultracopier"),NULL,NULL,(LPBYTE)0,&kSize) == ERROR_SUCCESS)
        temp=true;
    RegCloseKey(ultracopier_regkey);
    return temp;
}

void WindowsSessionLoader::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion)
{
    Q_UNUSED(options);
    Q_UNUSED(writePath);
    Q_UNUSED(pluginPath);
    Q_UNUSED(portableVersion);
}

/// \brief to get the options widget, NULL if not have
QWidget * WindowsSessionLoader::options()
{
    return NULL;
}

/// \brief to reload the translation, because the new language have been loaded
void WindowsSessionLoader::newLanguageLoaded()
{
}
