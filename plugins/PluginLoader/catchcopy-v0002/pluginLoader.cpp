/** \file pluginLoader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86 */

#include "pluginLoader.h"
#include "PlatformMacro.h"

#include <QFile>
#include <QDir>
#ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
#include <QCoreApplication>
#endif

#ifdef ULTRACOPIER_PLUGIN_DEBUG
    #define NORMAL_EXT "d.dll"
    #define SECOND_EXT ".dll"
#else
    #define NORMAL_EXT ".dll"
    #define SECOND_EXT "d.dll"
#endif
#define CATCHCOPY_DLL_32 "catchcopy32"
#define CATCHCOPY_DLL_64 "catchcopy64"

PluginLoader::PluginLoader()
{
    //set the startup value into the variable
    dllChecked=false;
    optionsEngine=NULL;
    allDllIsImportant=false;
    Debug=false;
    needBeRegistred=false;
    changeOfArchDetected=false;
    is64Bits=false;
    connect(&optionsWidget,&OptionsWidget::sendAllDllIsImportant,this,&PluginLoader::setAllDllIsImportant);
    connect(&optionsWidget,&OptionsWidget::sendDebug,this,&PluginLoader::setDebug);

#if defined(_M_X64)//64Bits
    is64Bits=true;
#else//32Bits
    char *arch=getenv("windir");
    if(arch!=NULL)
    {
        QDir dir;
        if(dir.exists(QString(arch)+"\\SysWOW64\\"))
            is64Bits=true;
        /// \note commented because it do a crash at the startup, and useless, because is global variable, it should be removed only by the OS
        //delete arch;
    }
#endif
}

PluginLoader::~PluginLoader()
{
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destructor");
        setEnabled(false);
}

void PluginLoader::setEnabled(const bool &needBeRegistred)
{
    if(!checkExistsDll())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("No dll exists"));
        emit newState(Ultracopier::Uncaught);
        if(!needBeRegistred)
            correctlyLoaded.clear();
        return;
    }
    if(this->needBeRegistred==needBeRegistred)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Double event dropped"));
        return;
    }
    this->needBeRegistred=needBeRegistred;
    int index=0;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start, needBeRegistred: %1, allDllIsImportant: %2").arg(needBeRegistred).arg(allDllIsImportant));

    bool oneHaveFound=false;
    index=0;
    while(index<importantDll.size())
    {
        if(QFile::exists(pluginPath+importantDll.at(index)))
        {
            oneHaveFound=true;
            break;
        }
        index++;
    }
    if(!oneHaveFound)
    {
        index=0;
        while(index<secondDll.size())
        {
            if(QFile::exists(pluginPath+secondDll.at(index)))
            {
                oneHaveFound=true;
                break;
            }
            index++;
        }
    }
    if(!oneHaveFound)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("No dll have found"));
        emit newState(Ultracopier::Uncaught);
        if(!needBeRegistred)
            correctlyLoaded.clear();
        return;
    }

    index=0;
    bool importantDll_is_loaded=false,secondDll_is_loaded=false;
    bool importantDll_have_bug=false,secondDll_have_bug=false;
    int importantDll_count=0,secondDll_count=0;
    while(index<importantDll.size())
    {
        if(!RegisterShellExtDll(pluginPath+importantDll.at(index),needBeRegistred,
            !(
                (needBeRegistred)
                ||
                (!needBeRegistred && correctlyLoaded.contains(importantDll.at(index)))
            )
        ))
        {
            if(changeOfArchDetected)
            {
                setEnabled(needBeRegistred);
                return;
            }
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"the important dll have failed: "+importantDll.at(index));
            importantDll_have_bug=true;
        }
        else
        {
            importantDll_is_loaded=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the important dll have been loaded: "+importantDll.at(index));
        }
        importantDll_count++;
        index++;
    }
    index=0;
    while(index<secondDll.size())
    {
        if(!RegisterShellExtDll(pluginPath+secondDll.at(index),needBeRegistred,
            !(
                (needBeRegistred && allDllIsImportant)
                ||
                (!needBeRegistred && correctlyLoaded.contains(secondDll.at(index)))
            )
        ))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"the second dll have failed: "+secondDll.at(index));
            secondDll_have_bug=true;
        }
        else
        {
            secondDll_is_loaded=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the second dll have been loaded: "+secondDll.at(index));
        }
        secondDll_count++;
        index++;
    }

    Ultracopier::CatchState importantDll_state,secondDll_state;
    if(importantDll_count==0)
    {
        if(needBeRegistred)
            importantDll_state=Ultracopier::Caught;
        else
            importantDll_state=Ultracopier::Uncaught;
    }
    else
    {
        if(importantDll_is_loaded)
        {
            if(!importantDll_have_bug)
                importantDll_state=Ultracopier::Caught;
            else
                importantDll_state=Ultracopier::Semiuncaught;
        }
        else
            importantDll_state=Ultracopier::Uncaught;
    }
    if(secondDll_count==0)
        if(needBeRegistred)
            secondDll_state=Ultracopier::Caught;
        else
            secondDll_state=Ultracopier::Uncaught;
    else
    {
        if(secondDll_is_loaded)
        {
            if(!secondDll_have_bug)
                secondDll_state=Ultracopier::Caught;
            else
                secondDll_state=Ultracopier::Semiuncaught;
        }
        else
            secondDll_state=Ultracopier::Uncaught;
    }

    if((importantDll_state==Ultracopier::Uncaught && secondDll_state==Ultracopier::Uncaught) || !needBeRegistred || (importantDll_count==0 && secondDll_count==0))
        emit newState(Ultracopier::Uncaught);
    else if(importantDll_state==Ultracopier::Caught)
        emit newState(Ultracopier::Caught);
    else
        emit newState(Ultracopier::Semiuncaught);

    if(!needBeRegistred)
        correctlyLoaded.clear();
}

bool PluginLoader::checkExistsDll()
{
    if(dllChecked)
    {
        if(importantDll.size()>0 || secondDll.size()>0)
            return true;
        else
            return false;
    }
    dllChecked=true;

    if(is64Bits)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"64Bits is important");
        importantDll << CATCHCOPY_DLL_64;
        secondDll << CATCHCOPY_DLL_32;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"32Bits is important");
        importantDll << CATCHCOPY_DLL_32;
        secondDll << CATCHCOPY_DLL_64;
    }

    int index=0;
    while(index<importantDll.size())
    {
        if(!QFile::exists(pluginPath+importantDll.at(index)+NORMAL_EXT))
        {
            if(!QFile::exists(pluginPath+importantDll.at(index)+SECOND_EXT))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"file not found, drop to the list: "+importantDll.at(index));
                importantDll.removeAt(index);
                index--;
            }
            else
                importantDll[index]+=SECOND_EXT;
        }
        else
            importantDll[index]+=NORMAL_EXT;
        index++;
    }
    index=0;
    while(index<secondDll.size())
    {
        if(!QFile::exists(pluginPath+secondDll.at(index)+NORMAL_EXT))
        {
            if(!QFile::exists(pluginPath+secondDll.at(index)+SECOND_EXT))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"file not found, drop to the list: "+secondDll.at(index));
                secondDll.removeAt(index);
                index--;
            }
            else
                secondDll[index]+=SECOND_EXT;
        }
        else
            secondDll[index]+=NORMAL_EXT;
        index++;
    }
    if(importantDll.size()>0 || secondDll.size()>0)
        return true;
    else
        return false;
}

void PluginLoader::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion)
{
    Q_UNUSED(options);
    Q_UNUSED(writePath);
    Q_UNUSED(pluginPath);
    Q_UNUSED(portableVersion);
    #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    this->pluginPath=QCoreApplication::applicationDirPath()+"/";
    #else
    this->pluginPath=pluginPath;
    #endif
    this->optionsEngine=options;
    if(optionsEngine!=NULL)
    {
        QList<QPair<QString, QVariant> > KeysList;
        KeysList.append(qMakePair(QString("allDllIsImportant"),QVariant(false)));
        KeysList.append(qMakePair(QString("Debug"),QVariant(false)));
        optionsEngine->addOptionGroup(KeysList);
        allDllIsImportant=optionsEngine->getOptionValue("allDllIsImportant").toBool();
        Debug=optionsEngine->getOptionValue("Debug").toBool();
        optionsWidget.setAllDllIsImportant(allDllIsImportant);
        optionsWidget.setDebug(Debug);
    }
}

bool PluginLoader::RegisterShellExtDll(const QString &dllPath, const bool &bRegister,const bool &quiet)
{
    if(Debug)
    {
        QString message;
        if(bRegister)
            message+=QString("Try load the dll: %1, and ").arg(dllPath);
        else
            message+=QString("Try unload the dll: %1, and ").arg(dllPath);
        if(quiet)
            message+=QString("don't open the UAC");
        else
            message+=QString("open the UAC if needed");
        QMessageBox::information(NULL,"Debug",message);
    }
    if(bRegister && correctlyLoaded.contains(dllPath))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Try dual load: %1").arg(dllPath));
        return false;
    }
    ////////////////////////////// First way to load //////////////////////////////

    wchar_t arrayArg[65535];
    int size_lenght;
    // first try - load dll and register it manually.
    HRESULT hResult = S_OK;
    // if failed - try by loading extension manually (would fail on vista when running as user)
    hResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if(SUCCEEDED(hResult))
    {
        HRESULT (STDAPICALLTYPE *pfn)(void);
        size_lenght=dllPath.toWCharArray(arrayArg);
        HINSTANCE hMod = LoadLibrary(arrayArg);	// load the dll
        if(hMod == NULL)
            hResult = HRESULT_FROM_WIN32(GetLastError());
        if(SUCCEEDED(hResult) && !hMod)
            hResult = E_FAIL;
        if(SUCCEEDED(hResult))
        {
            (FARPROC&)pfn = GetProcAddress(hMod, (bRegister ? "DllRegisterServer" : "DllUnregisterServer"));
            if(pfn == NULL)
                hResult = E_FAIL;
            if(SUCCEEDED(hResult))
                hResult = (*pfn)();

            CoFreeLibrary(hMod);
        }
        CoUninitialize();
        if(SUCCEEDED(hResult) && SCODE_CODE(hResult) != ERROR_ACCESS_DENIED)
    {
        if(bRegister)
            correctlyLoaded << dllPath;
        return true;
    }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("fail by LoadLibrary: %1, error code: %2").arg(dllPath).arg((quint32)hResult));
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("fail by CoInitializeEx: %1, error code: %2").arg(dllPath).arg((quint32)hResult));

    ////////////////////////////// Second way to load //////////////////////////////
    QStringList arguments;
    if(!Debug)
        arguments.append("/s");
    if(!bRegister)
        arguments.append("/u");
    arguments.append(dllPath);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: regsvr32 "+arguments.join(" "));
    int result=QProcess::execute("regsvr32",arguments);
    bool ok=false;
    if(result==0)
    {
        if(bRegister)
            correctlyLoaded << dllPath;
        ok=true;
    }
    #if ! defined(_M_X64)
    if(result==999 && !changeOfArchDetected)//code of wrong arch for the dll
    {
        changeOfArchDetected=true;
        QStringList temp;
        temp = importantDll;
        secondDll = importantDll;
        importantDll = temp;
        return false;
    }
    #endif
    if(result==5)
    {
        if(!quiet || (!bRegister && correctlyLoaded.contains(dllPath)))
        {
            arguments.last()=QString("\"%1\"").arg(arguments.last());
            ////////////////////////////// Last way to load //////////////////////////////
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"try it in win32");
            // try with regsvr32, win32 because for admin dialog

            size_lenght=arguments.join(" ").toWCharArray(arrayArg);
            //size_lenght*sizeof(wchar_t)
            wcscpy(arrayArg+size_lenght*sizeof(wchar_t),TEXT("\0"));
            SHELLEXECUTEINFO sei;
            memset(&sei, 0, sizeof(sei));
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_UNICODE;
            sei.lpVerb = TEXT("runas");
            sei.lpFile = TEXT("regsvr32.exe");
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"in win32 mode: arrayArg: "+QString::fromWCharArray(arrayArg,size_lenght));
            sei.lpParameters = arrayArg;
            sei.nShow = SW_SHOW;
            ok=ShellExecuteEx(&sei);
            if(ok && bRegister)
                correctlyLoaded << dllPath;
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"not try because need be quiet: "+dllPath);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("regsvr32 terminated with: %1").arg(result));
    if(!bRegister)
        correctlyLoaded.remove(dllPath);
    if(!bRegister)
        HardUnloadDLL(dllPath);
    return ok;
}

bool WINAPI PluginLoader::DLLEjecteurW(DWORD dwPid,PWSTR szDLLPath)
{
    /* Search address of module */
    MODULEENTRY32W meModule;
    meModule.dwSize = sizeof(meModule);
    HANDLE hSnapshot = NULL;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
    if(hSnapshot == NULL)
        return false;

    /* Search the right modules of the process */
    Module32FirstW(hSnapshot, &meModule);
    do{
        if((lstrcmpiW(meModule.szModule,szDLLPath) == 0) || (lstrcmpiW(meModule.szExePath,szDLLPath) == 0))break;
    }while(Module32NextW(hSnapshot, &meModule));

    /* Get handle of the process */
    HANDLE hProcess;

    hProcess = OpenProcess(PROCESS_ALL_ACCESS, false,dwPid);
    if(hProcess == NULL)
    {
        CloseHandle(hSnapshot);
        return false;
    }

    LPTHREAD_START_ROUTINE lpthThreadFunction;
    /* Get addresse of FreeLibrary in kernel32.dll */
    lpthThreadFunction = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "FreeLibrary");
    if(lpthThreadFunction == NULL)
    {
        CloseHandle(hProcess);
        CloseHandle(hSnapshot);
        return false;
    }

    /* Creation the remote thread */
    DWORD dwThreadID = 0;
    HANDLE hThread = NULL;
    hThread = CreateRemoteThread(hProcess, NULL, 0, lpthThreadFunction,meModule.modBaseAddr, 0, &dwThreadID);
    if(hThread == NULL)
    {
        CloseHandle(hSnapshot);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread,INFINITE);

    CloseHandle(hProcess);
    CloseHandle(hThread);

    return true;
}

void PluginLoader::HardUnloadDLL(const QString &myDllName)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+myDllName);
    HANDLE hSnapShot1;
    PROCESSENTRY32 uProcess;
    HANDLE hSnapShot2;
    MODULEENTRY32 me32;
    QString DllLoaded = "";
    QString DllLoadedName = "";
    QString DllLoadedPath = "";
    bool bResult;
    bool r;

    hSnapShot1 = CreateToolhelp32Snapshot(TH32CS_SNAPALL,0);

    uProcess.dwSize = (DWORD) sizeof(PROCESSENTRY32);

    r = Process32First(hSnapShot1, &uProcess);

    while ( r )
    {
        r = Process32Next(hSnapShot1, &uProcess);
        QString myProcessName;
        myProcessName=QString::fromWCharArray(uProcess.szExeFile);
        if (uProcess.th32ProcessID < 99999)
        {
            hSnapShot2 = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, uProcess.th32ProcessID );
            if (hSnapShot2 != INVALID_HANDLE_VALUE)
            {
                me32.dwSize = sizeof(me32);
                bResult = Module32First( hSnapShot2, &me32 );
                while( bResult )
                {
                        DllLoaded=QString::fromWCharArray(me32.szExePath);
                        DllLoadedName=QString::fromWCharArray(me32.szModule);
                        if (DllLoaded == myDllName)
                        {
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"The path: "+DllLoaded);
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,myProcessName+" ("+QString::number(uProcess.th32ProcessID)+")");
                            DLLEjecteurW(uProcess.th32ProcessID,me32.szExePath);
                        }
                        bResult = Module32Next( hSnapShot2, &me32 );
                }
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"(int)hSnapShot2 != -1 for "+myProcessName+" ("+QString::number(uProcess.th32ProcessID)+")");
            if(hSnapShot2)
                CloseHandle(hSnapShot2);
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"uProcess.th32ProcessID > 99999 for "+myProcessName+" ("+QString::number(uProcess.th32ProcessID)+")");
    }
    CloseHandle(hSnapShot1);
}

/// \brief to get the options widget, NULL if not have
QWidget * PluginLoader::options()
{
    return &optionsWidget;
}

void PluginLoader::newLanguageLoaded()
{
    optionsWidget.retranslate();
}

void PluginLoader::setAllDllIsImportant(bool allDllIsImportant)
{
    this->allDllIsImportant=allDllIsImportant;
    optionsEngine->setOptionValue("allDllIsImportant",allDllIsImportant);
}

void PluginLoader::setDebug(bool Debug)
{
    this->Debug=Debug;
    optionsEngine->setOptionValue("Debug",Debug);
}
