/** \file pluginLoader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86 */

#include "pluginLoader.h"
#include "PlatformMacro.h"
#include "../../../cpp11addition.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <servprov.h>
#include <exdisp.h>
#include <shlwapi.h>
#include <QProcess>
#include <QStringList>
#include <vector>
#endif
#include <QFile>
#include <QDir>
#include <QCoreApplication>

#ifdef ULTRACOPIER_PLUGIN_DEBUG
    #define NORMAL_EXT "d.dll"
    #define SECOND_EXT ".dll"
#else
    #define NORMAL_EXT ".dll"
    #define SECOND_EXT "d.dll"
#endif
#define CATCHCOPY_DLL_32 "catchcopy32"
#define CATCHCOPY_DLL_64 "catchcopy64"

// 64-bit build detection. The code used to test only _M_X64, which is an
// MSVC-only macro: the MinGW/MXE cross toolchain never defines it (it defines
// __x86_64__/_WIN64), so the 64-bit build silently behaved like a 32-bit one.
// Cover both compilers here so 32/64-bit behaviour is correct under MXE too.
#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(_WIN64)
    #define ULTRACOPIER_BUILD_IS_64BITS
#endif

#ifdef Q_OS_WIN32
/* ---- per-user clipboard-paste interception (no registry / no HKLM / no UAC) ----
 * A low-level keyboard hook lives in the (per-user) Ultracopier process. On Ctrl+V,
 * if the foreground is an Explorer folder window or the Desktop AND the clipboard
 * holds files (CF_HDROP) AND the focus is the file list (not a rename/address/search
 * edit box), it swallows the native paste and asks Ultracopier to do the copy/move
 * (copy vs move from the clipboard "Preferred DropEffect"). Works XP -> 11.
 * This replaces the shell DragDropHandler for Ctrl+V, which Windows never loads from
 * HKCU (machine-wide-only category). */
static HHOOK g_pasteHook=NULL;
static WindowsExplorerLoader* g_pasteSelf=NULL;

// The old mingw 4.9 toolchain (Qt5.6.3 / Windows-XP build) ships a libuuid.a that lacks
// CLSID_ShellWindows and IID_IShellWindows, so linking those SDK symbols fails there
// (newer MXE/Qt6 libuuid has them). Define them locally with unique names so every
// toolchain (XP -> 11, 32/64-bit) links without depending on -luuid carrying them.
static const GUID uc_CLSID_ShellWindows = {0x9ba05972,0xf6a8,0x11cf,{0xa4,0x42,0x00,0xa0,0xc9,0x0a,0x8f,0x39}};
static const IID  uc_IID_IShellWindows  = {0x85cb6900,0x4d95,0x11cf,{0x96,0x0c,0x00,0x80,0xc7,0xf4,0xee,0x85}};

// fast guard run inside the hook: shell folder view (Explorer/Desktop), focus on the list
static bool pasteContextIsShellFolder()
{
    HWND fg=GetForegroundWindow();
    if(fg==NULL)
        return false;
    wchar_t cls[64]; cls[0]=0; GetClassNameW(fg,cls,64);
    const bool isExplorer=(wcscmp(cls,L"CabinetWClass")==0 || wcscmp(cls,L"ExploreWClass")==0);
    const bool isDesktop =(wcscmp(cls,L"Progman")==0 || wcscmp(cls,L"WorkerW")==0);
    if(!isExplorer && !isDesktop)
        return false;
    // keep Ctrl+V a text paste inside rename box / address bar / search box
    GUITHREADINFO gti; gti.cbSize=sizeof(gti);
    if(GetGUIThreadInfo(GetWindowThreadProcessId(fg,NULL),&gti) && gti.hwndFocus!=NULL)
    {
        wchar_t fcls[64]; fcls[0]=0; GetClassNameW(gti.hwndFocus,fcls,64);
        if(wcscmp(fcls,L"Edit")==0 || wcscmp(fcls,L"ComboBox")==0 || wcscmp(fcls,L"ComboBoxEx32")==0
           || wcscmp(fcls,L"RichEdit20W")==0 || wcscmp(fcls,L"RICHEDIT50W")==0)
            return false;
    }
    return true;
}

static LRESULT CALLBACK pasteKeyboardProc(int code,WPARAM wParam,LPARAM lParam)
{
    if(code==HC_ACTION && (wParam==WM_KEYDOWN || wParam==WM_SYSKEYDOWN))
    {
        const KBDLLHOOKSTRUCT* k=(const KBDLLHOOKSTRUCT*)lParam;
        if((GetAsyncKeyState(VK_CONTROL)&0x8000) && k->vkCode=='V')// Ctrl+V only (Ctrl+X just marks the clipboard)
        {
            if(IsClipboardFormatAvailable(CF_HDROP) && pasteContextIsShellFolder())
            {
                if(g_pasteSelf!=NULL)
                    QMetaObject::invokeMethod(g_pasteSelf,"clipboardPasteDetected",Qt::QueuedConnection);
                return 1;// swallow the native paste; Ultracopier performs it instead
            }
        }
    }
    return CallNextHookEx(g_pasteHook,code,wParam,lParam);
}

// destination folder: Explorer window via IShellWindows, or the user Desktop folder
static std::wstring pasteDestFolder()
{
    HWND fg=GetForegroundWindow();
    if(fg==NULL)
        return std::wstring();
    wchar_t cls[64]; cls[0]=0; GetClassNameW(fg,cls,64);
    if(wcscmp(cls,L"Progman")==0 || wcscmp(cls,L"WorkerW")==0)
    {
        wchar_t p[MAX_PATH]; p[0]=0;
        if(SUCCEEDED(SHGetFolderPathW(NULL,CSIDL_DESKTOPDIRECTORY,NULL,0,p)))
            return std::wstring(p);
        return std::wstring();
    }
    std::wstring res;
    IShellWindows* psw=NULL;
    if(SUCCEEDED(CoCreateInstance(uc_CLSID_ShellWindows,NULL,CLSCTX_ALL,uc_IID_IShellWindows,(void**)&psw)) && psw!=NULL)
    {
        long n=0; psw->get_Count(&n);
        long i=0;
        while(i<n && res.empty())
        {
            VARIANT v; v.vt=VT_I4; v.lVal=i;
            IDispatch* pd=NULL;
            if(SUCCEEDED(psw->Item(v,&pd)) && pd!=NULL)
            {
                IWebBrowserApp* wb=NULL;
                if(SUCCEEDED(pd->QueryInterface(IID_IWebBrowserApp,(void**)&wb)) && wb!=NULL)
                {
                    SHANDLE_PTR hw=0; wb->get_HWND(&hw);
                    if((HWND)hw==fg)
                    {
                        IServiceProvider* sp=NULL;
                        if(SUCCEEDED(wb->QueryInterface(IID_IServiceProvider,(void**)&sp)) && sp!=NULL)
                        {
                            IShellBrowser* sb=NULL;
                            if(SUCCEEDED(sp->QueryService(SID_STopLevelBrowser,IID_IShellBrowser,(void**)&sb)) && sb!=NULL)
                            {
                                IShellView* sv=NULL;
                                if(SUCCEEDED(sb->QueryActiveShellView(&sv)) && sv!=NULL)
                                {
                                    IFolderView* fv=NULL;
                                    if(SUCCEEDED(sv->QueryInterface(IID_IFolderView,(void**)&fv)) && fv!=NULL)
                                    {
                                        IPersistFolder2* pf=NULL;
                                        if(SUCCEEDED(fv->GetFolder(IID_IPersistFolder2,(void**)&pf)) && pf!=NULL)
                                        {
                                            LPITEMIDLIST pidl=NULL;
                                            if(SUCCEEDED(pf->GetCurFolder(&pidl)) && pidl!=NULL)
                                            {
                                                wchar_t p[MAX_PATH]; if(SHGetPathFromIDListW(pidl,p)) res=p;
                                                CoTaskMemFree(pidl);
                                            }
                                            pf->Release();
                                        }
                                        fv->Release();
                                    }
                                    sv->Release();
                                }
                                sb->Release();
                            }
                            sp->Release();
                        }
                    }
                    wb->Release();
                }
                pd->Release();
            }
            i++;
        }
        psw->Release();
    }
    return res;
}

// CF_HDROP file list + the "Preferred DropEffect" (Ctrl+C -> copy, Ctrl+X -> move)
static std::vector<std::wstring> pasteClipboardFiles(bool &isMove)
{
    std::vector<std::wstring> files;
    isMove=false;
    if(!OpenClipboard(NULL))
        return files;
    HANDLE h=GetClipboardData(CF_HDROP);
    if(h!=NULL)
    {
        HDROP drop=(HDROP)h;
        const UINT count=DragQueryFileW(drop,0xFFFFFFFF,NULL,0);
        UINT i=0;
        while(i<count)
        {
            wchar_t f[MAX_PATH]; f[0]=0;
            if(DragQueryFileW(drop,i,f,MAX_PATH))
                files.push_back(std::wstring(f));
            i++;
        }
    }
    const UINT cfPDE=RegisterClipboardFormatW(L"Preferred DropEffect");
    HANDLE he=GetClipboardData(cfPDE);
    if(he!=NULL)
    {
        DWORD* pe=(DWORD*)GlobalLock(he);
        if(pe!=NULL)
        {
            isMove=(((*pe)&DROPEFFECT_MOVE)!=0);
            GlobalUnlock(he);
        }
    }
    CloseClipboard();
    return files;
}
#endif // Q_OS_WIN32

WindowsExplorerLoader::WindowsExplorerLoader()
{
    //set the startup value into the variable
    dllChecked=false;
    optionsEngine=NULL;
    allDllIsImportant=false;
    Debug=false;
    needBeRegistred=false;
    changeOfArchDetected=false;
    is64Bits=false;
    atstartup=false;
    notunload=false;
    catchPaste=true;//intercept Ctrl+V clipboard paste by default (per-user, the case most users use)
    dllChecked=false;
    optionsWidget=new OptionsWidget();
    connect(optionsWidget,&OptionsWidget::sendAllDllIsImportant,this,&WindowsExplorerLoader::setAllDllIsImportant);
    connect(optionsWidget,&OptionsWidget::sendAllUserIsImportant,this,&WindowsExplorerLoader::setAllUserIsImportant);
    connect(optionsWidget,&OptionsWidget::sendDebug,this,&WindowsExplorerLoader::setDebug);
    connect(optionsWidget,&OptionsWidget::sendAtStartup,this,&WindowsExplorerLoader::setAtStartup);
    connect(optionsWidget,&OptionsWidget::sendNotUnload,this,&WindowsExplorerLoader::setNotUnload);
    connect(optionsWidget,&OptionsWidget::sendCatchPaste,this,&WindowsExplorerLoader::setCatchPaste);

#ifdef ULTRACOPIER_BUILD_IS_64BITS//64Bits
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

WindowsExplorerLoader::~WindowsExplorerLoader()
{
    //delete optionsWidget;//attached to the main program, then it's the main program responsive the delete
    removePasteHook();
    if(!notunload)
        setEnabled(false);
}

void WindowsExplorerLoader::setEnabled(const bool &needBeRegistred)
{
#ifdef Q_OS_WIN32
    // the clipboard-paste keyboard hook does not depend on the shell-ext DLL/registration
    if(needBeRegistred)
        installPasteHook();
    else
        removePasteHook();
#endif
    if(!checkExistsDll())
    {
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        if(needBeRegistred)
                emit newState(Ultracopier::Caught);
        else
                emit newState(Ultracopier::Uncaught);
        #else
        emit newState(Ultracopier::Uncaught);
        #endif
        if(!needBeRegistred)
            correctlyLoaded.clear();
        return;
    }
    if(this->needBeRegistred==needBeRegistred)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Double event dropped: %1").arg(needBeRegistred).toStdString());
        if(needBeRegistred)
                emit newState(Ultracopier::Caught);
        else
                emit newState(Ultracopier::Uncaught);
        return;
    }
    this->needBeRegistred=needBeRegistred;
    unsigned int index=0;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, needBeRegistred: %1, allDllIsImportant: %2, allDllIsImportant: %3")
                             .arg(needBeRegistred).arg(allDllIsImportant).arg(allUserIsImportant).toStdString());

    bool oneHaveFound=false;
    index=0;
    while(index<importantDll.size())
    {
        if(QFile::exists(QString::fromStdString(pluginPath+importantDll.at(index))))
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
            if(QFile::exists(QString::fromStdString(pluginPath+secondDll.at(index))))
            {
                oneHaveFound=true;
                break;
            }
            index++;
        }
    }
    if(!oneHaveFound)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"No dll have found");
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
                (!needBeRegistred && correctlyLoaded.find(importantDll.at(index))!=correctlyLoaded.cend())
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
                (!needBeRegistred && correctlyLoaded.find(secondDll.at(index))!=correctlyLoaded.cend())
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

bool WindowsExplorerLoader::checkExistsDll()
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
        importantDll.push_back(CATCHCOPY_DLL_64);
        secondDll.push_back(CATCHCOPY_DLL_32);
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"32Bits is important");
        importantDll.push_back(CATCHCOPY_DLL_32);
        secondDll.push_back(CATCHCOPY_DLL_64);
    }

    unsigned int index=0;
    while(index<importantDll.size())
    {
        if(!QFile::exists(QString::fromStdString(pluginPath+importantDll.at(index)+NORMAL_EXT)))
        {
            if(!QFile::exists(QString::fromStdString(pluginPath+importantDll.at(index)+SECOND_EXT)))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"file not found, drop to the list: "+
                                          pluginPath+importantDll.at(index)+NORMAL_EXT+
                                         " and "+
                                         pluginPath+importantDll.at(index)+SECOND_EXT
                                         );
                importantDll.erase(importantDll.cbegin()+index);
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
        if(!QFile::exists(QString::fromStdString(pluginPath+secondDll.at(index)+NORMAL_EXT)))
        {
            if(!QFile::exists(QString::fromStdString(pluginPath+secondDll.at(index)+SECOND_EXT)))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,
                                         "file not found, drop to the list: "+pluginPath+secondDll.at(index)+NORMAL_EXT+
                                         " and "+pluginPath+secondDll.at(index)+SECOND_EXT
                                         );
                secondDll.erase(secondDll.cbegin()+index);
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

void WindowsExplorerLoader::setResources(OptionInterface * options, const std::string &writePath, const std::string &pluginPath, const bool &portableVersion)
{
    Q_UNUSED(options);
    Q_UNUSED(writePath);
    Q_UNUSED(pluginPath);
    Q_UNUSED(portableVersion);
    #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    this->pluginPath=QCoreApplication::applicationDirPath().toStdString()+"/";
    #else
    this->pluginPath=pluginPath;
    #endif
    this->optionsEngine=options;
    if(optionsEngine!=NULL)
    {
        std::vector<std::pair<std::string, std::string> > KeysList;
        KeysList.push_back(std::pair<std::string, std::string>("allDllIsImportant","false"));
        KeysList.push_back(std::pair<std::string, std::string>("allUserIsImportant","false"));
        KeysList.push_back(std::pair<std::string, std::string>("Debug","false"));
        KeysList.push_back(std::pair<std::string, std::string>("atstartup","false"));
        KeysList.push_back(std::pair<std::string, std::string>("notunload","false"));
        KeysList.push_back(std::pair<std::string, std::string>("catchPaste","true"));
        optionsEngine->addOptionGroup(KeysList);
        allDllIsImportant=stringtobool(optionsEngine->getOptionValue("allDllIsImportant"));
        allUserIsImportant=stringtobool(optionsEngine->getOptionValue("allUserIsImportant"));
        Debug=stringtobool(optionsEngine->getOptionValue("Debug"));
        atstartup=stringtobool(optionsEngine->getOptionValue("atstartup"));
        notunload=stringtobool(optionsEngine->getOptionValue("notunload"));
        catchPaste=stringtobool(optionsEngine->getOptionValue("catchPaste"));
        optionsWidget->setAllDllIsImportant(allDllIsImportant);
        optionsWidget->setDebug(Debug);
        optionsWidget->setAllUserIsImportant(allUserIsImportant);
        optionsWidget->setAtStartup(atstartup);
        optionsWidget->setNotUnload(notunload);
        optionsWidget->setCatchPaste(catchPaste);
    }
}

bool WindowsExplorerLoader::RegisterShellExtDll(std::string dllPath, const bool &bRegister, const bool &quiet)
{
    if(!bRegister && notunload)
        return true;
    if(allUserIsImportant)
        stringreplaceOne(dllPath,".dll","all.dll");
    if(Debug)
    {
        std::string message;
        if(bRegister)
            message+="Try load the dll: %1, and "+dllPath;
        else
            message+="Try unload the dll: %1, and "+dllPath;
        if(quiet)
            message+="don't open the UAC";
        else
            message+="open the UAC if needed";
        QMessageBox::information(NULL,"Debug",QString::fromStdString(message));
    }
    if(bRegister && correctlyLoaded.find(dllPath)!=correctlyLoaded.cend())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Try dual load: "+dllPath);
        return false;
    }
    // register
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, bRegister: "+std::to_string(bRegister));
    //set value into the variable
    {
        #ifdef Q_OS_WIN32
        const QString p=QString::fromStdString(dllPath);
        QFileInfo info(p);
        QString filename=info.fileName();
        QString QtdllPath=QString::fromStdString(dllPath);
        QtdllPath.replace( "/", "\\" );
        QString runStringApp = "regsvr32 /s \""+QtdllPath+"\"";
        // QString::utf16() is NUL-terminated and not length-limited; the previous
        // fixed wchar_t[255] buffers were filled with toWCharArray() which neither
        // NUL-terminates nor bounds-checks, so the registry value name/data was
        // corrupted (and a long dll path overflowed the stack buffer).
        const wchar_t *windowsString=reinterpret_cast<const wchar_t*>(runStringApp.utf16());
        const wchar_t *windowsStringKey=reinterpret_cast<const wchar_t*>(filename.utf16());
        const DWORD windowsStringBytes=(DWORD)((runStringApp.length()+1)*sizeof(wchar_t));//include the NUL for REG_SZ
        {
            HKEY ultracopier_regkey;
            if(RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, 0, REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, 0, &ultracopier_regkey, 0)==ERROR_SUCCESS)
            {
                if(atstartup)
                {
                    RegDeleteValue(ultracopier_regkey, TEXT("catchcopy"));
                    DWORD kSize=254;
                    if(RegQueryValueEx(ultracopier_regkey,windowsStringKey,NULL,NULL,(LPBYTE)0,&kSize)!=ERROR_SUCCESS)
                    {

                        if(RegSetValueEx(ultracopier_regkey, windowsStringKey, 0, REG_SZ, (const BYTE*)windowsString, windowsStringBytes)!=ERROR_SUCCESS)
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"bRegister: "+std::to_string(bRegister)+" failed");
                    }
                }
                else
                    RegDeleteValue(ultracopier_regkey,windowsStringKey);
                RegCloseKey(ultracopier_regkey);
            }
        }
        {
            HKEY    ultracopier_regkey;
            if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, 0, REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, 0, &ultracopier_regkey, 0)==ERROR_SUCCESS)
            {
                if(atstartup)
                {
                    RegDeleteValue(ultracopier_regkey, TEXT("catchcopy"));
                    DWORD kSize=254;
                    if(RegQueryValueEx(ultracopier_regkey,windowsStringKey,NULL,NULL,(LPBYTE)0,&kSize)!=ERROR_SUCCESS)
                    {
                        if(RegSetValueEx(ultracopier_regkey,windowsStringKey,0,REG_SZ,(const BYTE*)windowsString, windowsStringBytes)!=ERROR_SUCCESS)
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"bRegister: "+std::to_string(bRegister)+" failed");
                    }
                }
                else
                    RegDeleteValue(ultracopier_regkey,windowsStringKey);
                RegCloseKey(ultracopier_regkey);
            }
        }
        #endif
    }
    ////////////////////////////// First way to load //////////////////////////////
    QStringList arguments;
    if(!Debug)
        arguments.append("/s");
    if(!bRegister)
        arguments.append("/u");
    arguments.append(QString::fromStdString(dllPath));
    QString regsvrExe=QStringLiteral("regsvr32.exe");
#if defined(Q_OS_WIN32) && !defined(ULTRACOPIER_BUILD_IS_64BITS)
    // 32-bit build on 64-bit Windows: register the 64-bit dll with the *64-bit*
    // regsvr32 so it lands in the native HKCU\Software\Classes view that the
    // 64-bit Explorer actually reads. A plain "regsvr32" is WOW64-redirected to
    // the 32-bit SysWOW64\regsvr32.exe, which cannot load a 64-bit dll and writes
    // to the WOW6432Node view that 64-bit Explorer ignores. The 64-bit regsvr32 is
    // reachable from a 32-bit process only via the virtual %windir%\Sysnative
    // alias. This stays per-user (HKCU), so no elevation/UAC is needed.
    if(is64Bits && dllPath.find("catchcopy64")!=std::string::npos)
    {
        const char *windir=getenv("windir");
        if(windir!=NULL)
        {
            const QString sysnative=QString::fromLocal8Bit(windir)+QStringLiteral("\\Sysnative\\regsvr32.exe");
            if(QFile::exists(sysnative))
                regsvrExe=sysnative;
        }
    }
#endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+regsvrExe.toStdString()+" "+arguments.join(" ").toStdString());
    int result;
    #ifdef Q_OS_WIN32
    QProcess process;
    process.start(regsvrExe,arguments);
    if(!process.waitForStarted())
        result=985;
    else if(!process.waitForFinished())
        result=984;
    else
    {
        result=process.exitCode();
        QString out=QString::fromLocal8Bit(process.readAllStandardOutput());
        QString outError=QString::fromLocal8Bit(process.readAllStandardError());
        if(!out.isEmpty())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"regsvr32 output: "+out.toStdString());
        if(!outError.isEmpty())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"regsvr32 error output: "+outError.toStdString());
    }
    #else
    result=0;
    #endif
    bool ok=false;
    if(result==0)
    {
        if(bRegister)
            correctlyLoaded.insert(dllPath);
        ok=true;
    }
    #ifndef ULTRACOPIER_BUILD_IS_64BITS
    if(result==999 && !changeOfArchDetected)//code of wrong arch for the dll
    {
        changeOfArchDetected=true;
        // real swap: the old code did temp=important; second=important; important=temp;
        // which overwrote both lists with importantDll and lost the other-arch dll.
        std::swap(importantDll,secondDll);
        return false;
    }
    #endif
    if(result==5)
    {
        if(!quiet || (!bRegister && correctlyLoaded.find(dllPath)!=correctlyLoaded.cend()))
        {
            arguments.last()=QStringLiteral("\"%1\"").arg(arguments.last());
            ////////////////////////////// Last way to load //////////////////////////////
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"try it in win32");
            // try with regsvr32, win32 because for admin dialog

            #ifdef Q_OS_WIN32
            // NUL-terminated via utf16(). The previous code did
            //   wcscpy(arrayArg + size_lenght*sizeof(wchar_t), L"")
            // which advances a wchar_t* by size_lenght*sizeof(wchar_t) ELEMENTS
            // (double scaling): the terminator landed far past the string end,
            // leaving uninitialised garbage in the arguments handed to the elevated
            // regsvr32, so the UAC (runas) registration silently failed.
            const QString argString=arguments.join(" ");
            const wchar_t *params=reinterpret_cast<const wchar_t*>(argString.utf16());
            SHELLEXECUTEINFO sei;
            memset(&sei, 0, sizeof(sei));
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_UNICODE;
            sei.lpVerb = TEXT("runas");
            // same regsvr32 (64-bit via Sysnative when needed) as the QProcess path
            sei.lpFile = reinterpret_cast<const wchar_t*>(regsvrExe.utf16());
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"in win32 mode: arrayArg: "+argString.toStdString());
            sei.lpParameters = params;
            sei.nShow = SW_SHOW;
            ok=ShellExecuteEx(&sei);
            #else
            ok=true;
            #endif
            if(ok && bRegister)
                correctlyLoaded.insert(dllPath);
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"not try because need be quiet: "+dllPath);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"regsvr32 terminated with: "+std::to_string(result));
    if(!bRegister)
        correctlyLoaded.erase(dllPath);
    return ok;
}

/// \brief to get the options widget, NULL if not have
QWidget * WindowsExplorerLoader::options()
{
    return optionsWidget;
}

void WindowsExplorerLoader::newLanguageLoaded()
{
    optionsWidget->retranslate();
}

void WindowsExplorerLoader::setAllDllIsImportant(bool allDllIsImportant)
{
    this->allDllIsImportant=allDllIsImportant;
    optionsEngine->setOptionValue("allDllIsImportant",std::to_string(allDllIsImportant));
}

void WindowsExplorerLoader::setAllUserIsImportant(bool allUserIsImportant)
{
    this->allUserIsImportant=allUserIsImportant;
    optionsEngine->setOptionValue("allUserIsImportant",std::to_string(allUserIsImportant));
}

void WindowsExplorerLoader::setDebug(bool Debug)
{
    this->Debug=Debug;
    optionsEngine->setOptionValue("Debug",std::to_string(Debug));
}

void WindowsExplorerLoader::setAtStartup(bool val)
{
    this->atstartup=val;
    optionsEngine->setOptionValue("atstartup",std::to_string(val));
    bool invert=!needBeRegistred;
    setEnabled(invert);
    setEnabled(!invert);
}

void WindowsExplorerLoader::setNotUnload(bool val)
{
    this->notunload=val;
    optionsEngine->setOptionValue("notunload",std::to_string(val));
}

void WindowsExplorerLoader::installPasteHook()
{
#ifdef Q_OS_WIN32
    if(!catchPaste || g_pasteHook!=NULL)
        return;
    g_pasteSelf=this;
    // module containing the hook proc (the plugin DLL, or the exe in all-in-one builds)
    HMODULE hmod=NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&pasteKeyboardProc,&hmod);
    g_pasteHook=SetWindowsHookExW(WH_KEYBOARD_LL,pasteKeyboardProc,hmod,0);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,
        g_pasteHook!=NULL?std::string("clipboard paste hook installed"):std::string("clipboard paste hook install FAILED"));
#endif
}

void WindowsExplorerLoader::removePasteHook()
{
#ifdef Q_OS_WIN32
    if(g_pasteHook!=NULL)
    {
        UnhookWindowsHookEx(g_pasteHook);
        g_pasteHook=NULL;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"clipboard paste hook removed");
    }
    g_pasteSelf=NULL;
#endif
}

void WindowsExplorerLoader::clipboardPasteDetected()
{
#ifdef Q_OS_WIN32
    const std::wstring dest=pasteDestFolder();
    bool isMove=false;
    const std::vector<std::wstring> files=pasteClipboardFiles(isMove);
    if(dest.empty() || files.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"clipboard paste: empty dest/clipboard, ignored");
        return;
    }
    std::vector<std::string> sources;
    for(size_t i=0;i<files.size();i++)
        sources.push_back(QString::fromWCharArray(files.at(i).c_str()).toStdString());
    const std::string destination=QString::fromWCharArray(dest.c_str()).toStdString();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,
        std::string(isMove?"clipboard mv ":"clipboard cp ")+std::to_string(files.size())
        +" file(s) -> "+destination);
    // Deliver IN-PROCESS: this hook already runs inside the running (per-user) Ultracopier,
    // so emit straight to CopyListener::copy/move (via PluginLoaderCore) instead of spawning a
    // second ultracopier.exe that would only forward back over the single-instance QLocalServer.
    // That cold process start was the whole Ctrl+V -> window latency.
    if(isMove)
        emit newMove(sources,destination);
    else
        emit newCopy(sources,destination);
#endif
}

void WindowsExplorerLoader::setCatchPaste(bool val)
{
    this->catchPaste=val;
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("catchPaste",std::to_string(val));
#ifdef Q_OS_WIN32
    if(val && needBeRegistred)
        installPasteHook();
    else
        removePasteHook();
#endif
}
