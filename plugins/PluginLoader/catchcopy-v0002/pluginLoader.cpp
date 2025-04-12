/** \file pluginLoader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86 */

#include "pluginLoader.h"
#include "PlatformMacro.h"
#include "../../../cpp11addition.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#endif
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
    dllChecked=false;
    optionsWidget=new OptionsWidget();
    connect(optionsWidget,&OptionsWidget::sendAllDllIsImportant,this,&WindowsExplorerLoader::setAllDllIsImportant);
    connect(optionsWidget,&OptionsWidget::sendAllUserIsImportant,this,&WindowsExplorerLoader::setAllUserIsImportant);
    connect(optionsWidget,&OptionsWidget::sendDebug,this,&WindowsExplorerLoader::setDebug);
    connect(optionsWidget,&OptionsWidget::sendAtStartup,this,&WindowsExplorerLoader::setAtStartup);
    connect(optionsWidget,&OptionsWidget::sendNotUnload,this,&WindowsExplorerLoader::setNotUnload);

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

WindowsExplorerLoader::~WindowsExplorerLoader()
{
    //delete optionsWidget;//attached to the main program, then it's the main program responsive the delete
    if(!notunload)
        setEnabled(false);
}

void WindowsExplorerLoader::setEnabled(const bool &needBeRegistred)
{
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
        optionsEngine->addOptionGroup(KeysList);
        allDllIsImportant=stringtobool(optionsEngine->getOptionValue("allDllIsImportant"));
        allUserIsImportant=stringtobool(optionsEngine->getOptionValue("allUserIsImportant"));
        Debug=stringtobool(optionsEngine->getOptionValue("Debug"));
        atstartup=stringtobool(optionsEngine->getOptionValue("atstartup"));
        notunload=stringtobool(optionsEngine->getOptionValue("notunload"));
        optionsWidget->setAllDllIsImportant(allDllIsImportant);
        optionsWidget->setDebug(Debug);
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
        wchar_t windowsString[255];
        runStringApp.toWCharArray(windowsString);
        wchar_t windowsStringKey[255];
        filename.toWCharArray(windowsStringKey);
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

                        if(RegSetValueEx(ultracopier_regkey, windowsStringKey, 0, REG_SZ, (BYTE*)windowsString, runStringApp.length()*2)!=ERROR_SUCCESS)
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
                        if(RegSetValueEx(ultracopier_regkey,windowsStringKey,0,REG_SZ,(BYTE*)windowsString, runStringApp.length()*2)!=ERROR_SUCCESS)
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: regsvr32 "+arguments.join(" ").toStdString());
    int result;
    #ifdef Q_OS_WIN32
    QProcess process;
    process.start("regsvr32",arguments);
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
    #if ! defined(_M_X64)
    if(result==999 && !changeOfArchDetected)//code of wrong arch for the dll
    {
        changeOfArchDetected=true;
        std::vector<std::string> temp;
        temp = importantDll;
        secondDll = importantDll;
        importantDll = temp;
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
            wchar_t arrayArg[65535];
            int size_lenght=arguments.join(" ").toWCharArray(arrayArg);
            //size_lenght*sizeof(wchar_t)
            wcscpy(arrayArg+size_lenght*sizeof(wchar_t),TEXT("\0"));
            SHELLEXECUTEINFO sei;
            memset(&sei, 0, sizeof(sei));
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_UNICODE;
            sei.lpVerb = TEXT("runas");
            sei.lpFile = TEXT("regsvr32.exe");
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"in win32 mode: arrayArg: "+QString::fromWCharArray(arrayArg,size_lenght).toStdString());
            sei.lpParameters = arrayArg;
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
