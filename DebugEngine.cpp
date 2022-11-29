/** \file DebugEngine.cpp
\brief Define the class for the debug
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QLocalSocket>
#include <regex>
#include <iostream>

#include "Variable.h"
#include "DebugEngine.h"
#include "ExtraSocket.h"
#include "cpp11addition.h"
#include "FacilityEngine.h"

#ifdef WIN32
#	define __func__ __FUNCTION__
#endif

#ifdef ULTRACOPIER_DEBUGCONSOLE
#undef ULTRACOPIER_DEBUGCONSOLE
#endif
/// \brief The local macro: ULTRACOPIER_DEBUGCONSOLE
#if defined (__FILE__) && defined (__LINE__)
#	define ULTRACOPIER_DEBUGCONSOLE(a,b) addDebugInformation(a,__func__,b,__FILE__,__LINE__)
#else
#	define ULTRACOPIER_DEBUGCONSOLE(a,b) addDebugInformation(a,__func__,b)
#endif

#ifdef ULTRACOPIER_DEBUG

/// \brief initiate the ultracopier event dispatcher and check if no other session is running
DebugEngine::DebugEngine()
{
    //fileNameCleaner=std::regex("\\.\\.?[/\\\\]([^/]+[/\\\\])?");
    quit=false;
    QStringList ultracopierArguments=QCoreApplication::arguments();
    if(ultracopierArguments.size()==2)
        if(ultracopierArguments.last()=="quit")
            quit=true;
    addDebugInformationCallNumber=0;
    //Load the first content
    htmlHeaderContent+="<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">";
    htmlHeaderContent+="<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">";
    htmlHeaderContent+="<head>";
    htmlHeaderContent+="<meta name=\"Language\" content=\"en\" />";
    htmlHeaderContent+="<meta http-equiv=\"content-language\" content=\"english\" />";
    htmlHeaderContent+="<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />";
    htmlHeaderContent+="<style type=\"text/css\">";
    htmlHeaderContent+="body{font-family:\"DejaVu Sans Mono\";font-size:9pt;}";
    htmlHeaderContent+=".Information td{color:#7485df;}";
    htmlHeaderContent+=".Critical td{color:#ff0c00;background-color:#FFFE8C;}";
    htmlHeaderContent+=".Warning td{color:#efa200;}";
    htmlHeaderContent+=".Notice td{color:#999;}";
    htmlHeaderContent+=".time{font-weight:bold;}";
    htmlHeaderContent+=".Note{font-weight:bold;font-size:1.5em}";
    htmlHeaderContent+=".function{font-style:italic;text-decoration:underline}";
    htmlHeaderContent+=".location{padding-right:15px;}";
    htmlHeaderContent+="td {white-space:nowrap;}";
    htmlHeaderContent+="</style>";
    htmlHeaderContent+="<title>";
    htmlHeaderContent+="Ultracopier";
    htmlHeaderContent+=" "+FacilityEngine::version()+" "+ULTRACOPIER_PLATFORM_NAME.toStdString()+", debug report</title>";
    htmlHeaderContent+="</head>";
    htmlHeaderContent+="<body>";
    htmlHeaderContent+="<table>";
    debugHtmlContent=htmlHeaderContent;
    //Load the start time at now
    startTime.start();
    //Load the log file end
    endOfLogFile="</table></body></html>";
    //check if other instance is running, then switch to memory backend
    if(tryConnect())
    {
        ULTRACOPIER_DEBUGCONSOLE(DebugLevel_custom_Notice,"currentBackend: File because other session is runing");
        currentBackend=Memory;
        return;
    }
    //The lock and log file path is not defined
    bool fileNameIsLoaded=false;
    #ifdef ULTRACOPIER_VERSION_PORTABLE
        #ifdef ULTRACOPIER_VERSION_PORTABLEAPPS
            //Load the data folder path
            QDir dir(QCoreApplication::applicationDirPath());
            dir.cdUp();
            dir.cdUp();
            dir.cd("Data");
            logFile.setFileName(dir.absolutePath()+FacilityEngine::separator()+"log.html");
            lockFile.setFileName(dir.absolutePath()+FacilityEngine::separator()+"ultracopier.lock");
            fileNameIsLoaded=true;
        #else
            //Load the ultracopier path
            QDir dir(QCoreApplication::applicationDirPath());
            logFile.setFileName(dir.absolutePath()+FacilityEngine::separator()+"log.html");
            lockFile.setFileName(dir.absolutePath()+FacilityEngine::separator()+"ultracopier.lock");
            fileNameIsLoaded=true;
        #endif
    #else
        #ifdef Q_OS_WIN32
            #define EXTRA_HOME_PATH "\\ultracopier\\"
        #else
            #define EXTRA_HOME_PATH "/.config/Ultracopier/"
        #endif
        //Load the user path only if exists and writable
        QDir dir(QDir::homePath()+EXTRA_HOME_PATH);
        bool errorFound=false;
        //If user's directory not exists create it
        if(!dir.exists())
        {
            //If failed not load the file
            if(!dir.mkpath(dir.absolutePath()))
            {
                errorFound=true;
                puts(qPrintable("Unable to make path: "+dir.absolutePath()+", disable file log"));
            }
        }
        //If no error found set the file name
        if(errorFound==false)
        {
            fileNameIsLoaded=true;
            logFile.setFileName(dir.absolutePath()+QDir::separator()+"log.html");
            lockFile.setFileName(dir.absolutePath()+QDir::separator()+"ultracopier.lock");
        }
        //errorFound=false;
    #endif
    //If the file name is loaded
    if(fileNameIsLoaded)
    {
        //If the previous file is here, then crash previous, ask if the user want to save
        if(lockFile.exists() && logFile.exists() && !quit)
        {
            //Try open the file as read only to propose save it as the user
            //Don't ask it if unable to write, because unable to remove, then alert at all start
            if(removeTheLockFile())
            {
                //Ask to the user
                /* stable version seam not crash, then quit temporally this mesage
                 * QMessageBox::StandardButton reply = QMessageBox::question(NULL,"Save the previous report",
                                                                              #ifdef ULTRACOPIER_MODE_SUPERCOPIER
                                                                               QString("Supercopier")+
                                                                              #else
                                                                                QString("Ultracopier")+
                                                                               #endif
                        " seam have crashed, do you want save the previous report for report it to the forum?",QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
                if(reply==QMessageBox::Yes)
                    saveBugReport();*/
            }
            else
                puts(qPrintable(logFile.fileName()+" unable to open it as read"));
        }
        //Now try to create and open the log file and lock file
        if(!lockFile.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Unbuffered))
        {
            currentBackend=Memory;
            puts(qPrintable(lockFile.fileName()+" unable to open it as write, log into file disabled"));
        }
        else
        {
            if(!logFile.open(QIODevice::ReadWrite|QIODevice::Truncate|QIODevice::Unbuffered))
            {
                currentBackend=Memory;
                puts(qPrintable(logFile.fileName()+" unable to open it as write, log into file disabled"));
                removeTheLockFile();
            }
            else
            {
                logFile.resize(0);
                currentBackend=File;
                logFile.write(debugHtmlContent.data(),static_cast<qint64>(debugHtmlContent.size()));
            }
        }
    }
}

/// \brief Destroy the ultracopier event dispatcher
DebugEngine::~DebugEngine()
{
    if(currentBackend==File)
    {
        removeTheLockFile();
        //Finalize the log file
        logFile.write(endOfLogFile.data(),static_cast<qint64>(endOfLogFile.size()));
        logFile.close();
    }
}

/// \brief ask to the user where save the bug report
void DebugEngine::saveBugReport()
{
    bool errorFound;
    do
    {
        errorFound=false;
        //Ask where it which save it
        QString fileName = QFileDialog::getSaveFileName(NULL,"Save file","ultracopier-bug-report.log.html","Log file (*.log.html)");
        if(fileName!="")
        {
            if(QFile::exists(fileName))
            {
                if(!QFile::remove(fileName))
                {
                    errorFound=true;
                    puts(qPrintable(fileName+" unable remove it"));
                    QMessageBox::critical(NULL,"Error","Unable to save the bug report");
                }
            }
            if(!errorFound)
            {
                if(currentBackend==Memory)
                {
                    QFile dest(fileName);
                    if(dest.open(QIODevice::WriteOnly|QIODevice::Truncate))
                    {
                        if(dest.write(htmlHeaderContent.data(),htmlHeaderContent.size())!=(qint64)htmlHeaderContent.size())
                        {
                            errorFound=true;
                            puts(qPrintable(fileName+" unable to open it as write: "+logFile.errorString()));
                            QMessageBox::critical(NULL,"Error","Unable to save the bug report"+logFile.errorString());
                        }
                        else
                        {
                            QString s=DebugModel::debugModel->toString();
                            if(dest.write(s.toStdString().c_str(),s.size())!=(qint64)s.size())
                            {
                                errorFound=true;
                                puts(qPrintable(fileName+" unable to open it as write: "+logFile.errorString()));
                                QMessageBox::critical(NULL,"Error","Unable to save the bug report"+logFile.errorString());
                            }
                        }
                        dest.close();
                    }
                    else
                    {
                        errorFound=true;
                        puts(qPrintable(fileName+" unable to open it as write: "+logFile.errorString()));
                        QMessageBox::critical(NULL,"Error","Unable to save the bug report"+logFile.errorString());
                    }
                }
                else
                {
                    //Open the destination as write
                    if(!logFile.copy(fileName))
                    {
                        errorFound=true;
                        puts(qPrintable(fileName+" unable to open it as write: "+logFile.errorString()));
                        QMessageBox::critical(NULL,"Error","Unable to save the bug report"+logFile.errorString());
                    }
                    /* https://doc.qt.io/qt-5/qfile.html#copy
                     * This file is closed before it is copied. */
                    if(!logFile.isOpen())
                    {
                        if(!logFile.open(QIODevice::ReadWrite|QIODevice::Unbuffered|QIODevice::Append))
                        {
                            currentBackend=Memory;
                            puts(qPrintable(logFile.fileName()+" unable to reopen it as write, log into file disabled"));
                            removeTheLockFile();
                        }
                        else
                            currentBackend=File;
                    }
                    else
                        std::cerr << "Error, strange, this case should not be exists, see Qt doc" << std::endl;
                }
            }
        }
    } while(errorFound!=false);
}

/// \brief Internal function to remove the lock file
bool DebugEngine::removeTheLockFile()
{
    //close the file and remove it
    lockFile.close();
    if(!lockFile.remove())
    {
        puts(qPrintable(lockFile.fileName()+" unable to remove it, crash report at the next start"));
        return false;
    }
    else
        return true;
}

void DebugEngine::addDebugInformationStatic(const Ultracopier::DebugLevel &level,const std::string& function,const std::string& text,const std::string& file,const int& ligne,const std::string& location)
{
    if(DebugEngine::debugEngine==NULL)
    {
        std::cerr << "After close: " << function << file << ligne;
        return;
    }
    DebugLevel_custom tempLevel=DebugLevel_custom_Information;
    switch(level)
    {
        case Ultracopier::DebugLevel_Information:
            tempLevel=DebugLevel_custom_Information;
        break;
        case Ultracopier::DebugLevel_Critical:
            tempLevel=DebugLevel_custom_Critical;
        break;
        case Ultracopier::DebugLevel_Warning:
            tempLevel=DebugLevel_custom_Warning;
        break;
        case Ultracopier::DebugLevel_Notice:
            tempLevel=DebugLevel_custom_Notice;
        break;
        default:
            tempLevel=DebugLevel_custom_Notice;
    }
    DebugEngine::debugEngine->addDebugInformation(tempLevel,function,text,file,ligne,location);
}

void DebugEngine::addDebugNote(const std::string& text)
{
    if(DebugEngine::debugEngine==NULL)
        return;
    DebugEngine::debugEngine->addDebugInformation(DebugLevel_custom_UserNote,"",text,"",-1,"Core");
}

std::string DebugEngine::debugInformationToHtml(const unsigned int &t,const DebugLevel_custom &level,const std::string& f,const std::string& text,std::string file,const int& ligne,const std::string& location)
{
    //Load the time from start
    std::string addDebugInformation_htmlFormat;
    switch(level)
    {
        case DebugLevel_custom_Information:
            addDebugInformation_htmlFormat="<tr class=\"Information\"><td class=\"time\">";
        break;
        case DebugLevel_custom_Critical:
            addDebugInformation_htmlFormat="<tr class=\"Critical\"><td class=\"time\">";
        break;
        case DebugLevel_custom_Warning:
            addDebugInformation_htmlFormat="<tr class=\"Warning\"><td class=\"time\">";
        break;
        case DebugLevel_custom_Notice:
        {
            addDebugInformation_htmlFormat="<tr class=\"Notice\"><td class=\"time\">";
        }
        break;
        case DebugLevel_custom_UserNote:
            addDebugInformation_htmlFormat="<tr class=\"Note\"><td class=\"time\">";
        break;
    }
    std::string addDebugInformation_fileString=file;
    std::string addDebugInformation_lignestring=std::to_string(ligne);
    std::string addDebugInformation_time=std::to_string(t);
    if(addDebugInformation_time.size()<8)
        addDebugInformation_time=std::string(8-addDebugInformation_time.size(),' ')+addDebugInformation_time;
    if(ligne!=-1)
        addDebugInformation_fileString+=":"+addDebugInformation_lignestring;
    addDebugInformation_htmlFormat+=addDebugInformation_time+"</td><td>"+addDebugInformation_fileString+"</td><td class=\"function\">"+f+"()</td><td class=\"location\">"+location+"</td><td>"+htmlEntities(text)+"</td></tr>\n";
    return addDebugInformation_htmlFormat;
}

/// \brief For add message info, this function is thread safe
void DebugEngine::addDebugInformation(const DebugLevel_custom &level,const std::string& function,const std::string& text,std::string file,const int& ligne,const std::string& location)
{
    if(DebugEngine::debugEngine==NULL)
    {
        std::cerr << "After close: " << function << file << ligne;
        return;
    }
    //Remove the compiler extra patch generated
    //file=file.remove(fileNameCleaner);don't clean, too many performance heart
    std::string addDebugInformation_lignestring=std::to_string(ligne);
    bool important=true;
    switch(level)
    {
        case DebugLevel_custom_Notice:
            important=false;
        break;
        default:
        break;
    }
    std::string addDebugInformation_time=std::to_string(startTime.elapsed());
    if(addDebugInformation_time.size()<8)
        addDebugInformation_time=std::string(8-addDebugInformation_time.size(),' ')+addDebugInformation_time;

    std::string addDebugInformation_htmlFormat=DebugEngine::debugInformationToHtml(startTime.elapsed(),level,function,text,file,ligne,location);
    //To prevent access of string in multi-thread
    {
        //Show the text in console
        std::string addDebugInformation_textFormat;

        addDebugInformation_textFormat = "("+addDebugInformation_time+") ";
        if(!file.empty() && ligne!=-1)
            addDebugInformation_textFormat += file+":"+addDebugInformation_lignestring+":";
        addDebugInformation_textFormat += function+"(), (location: "+location+"): "+text;
        QMutexLocker lock_mutex(&mutex);
        if(currentBackend==File)
        {
            if(logFile.size()<ULTRACOPIER_DEBUG_MAX_ALL_SIZE*1024*1024 || (important && logFile.size()<ULTRACOPIER_DEBUG_MAX_IMPORTANT_SIZE*1024*1024))
            {
                std::cout << addDebugInformation_textFormat << std::endl;
                if(logFile.write(addDebugInformation_htmlFormat.data(),static_cast<qint64>(addDebugInformation_htmlFormat.size()))!=(qint64)addDebugInformation_htmlFormat.size())
                {
                    if(!logFile.isOpen())
                        std::cerr << "Error to write (not open): " << logFile.errorString().toStdString() << std::endl;
                    else
                        std::cerr << "Error to write: " << logFile.errorString().toStdString() << std::endl;
                }
            }
        }
        else
        {
            if(debugHtmlContent.size()<ULTRACOPIER_DEBUG_MAX_ALL_SIZE*1024*1024 || (important && debugHtmlContent.size()<ULTRACOPIER_DEBUG_MAX_IMPORTANT_SIZE*1024*1024))
            {
                std::cout << addDebugInformation_textFormat << std::endl;
                debugHtmlContent+=addDebugInformation_htmlFormat;
            }
        }
        //Send the new line
        if(addDebugInformationCallNumber<ULTRACOPIER_DEBUG_MAX_GUI_LINE)
        {
            addDebugInformationCallNumber++;
            DebugModel::debugModel->addDebugInformation(startTime.elapsed(),level,function,text,file,static_cast<unsigned int>(ligne),location);
        }
    }
}

/// \brief Get the html text info for re-show it
std::string DebugEngine::getTheDebugHtml()
{
    if(currentBackend==File)
    {
        logFile.seek(0);
        if(!logFile.isOpen())
            ULTRACOPIER_DEBUGCONSOLE(DebugLevel_custom_Warning,"The log file is not open");
        const QByteArray &data=logFile.readAll();
        return std::string(data.constData(),static_cast<size_t>(data.size()))+endOfLogFile;
    }
    else
        return debugHtmlContent+endOfLogFile;
}

/// \brief Get the html end
std::string DebugEngine::getTheDebugEnd()
{
    return endOfLogFile;
}

/// \brief Drop the html entities
std::string DebugEngine::htmlEntities(const std::string &text)
{
    std::string newText(text);
    stringreplaceAll(newText,"&","&amp;");
    /*stringreplaceAll(newText,"\"","&quot;");
    stringreplaceAll(newText,"\\","&#039;");*/
    stringreplaceAll(newText,"<","&lt;");
    stringreplaceAll(newText,">","&gt;");
    return newText;
}

/// \brief return the current backend
DebugEngine::Backend DebugEngine::getCurrentBackend()
{
    return currentBackend;
}

bool DebugEngine::tryConnect()
{
    QLocalSocket localSocket;
    localSocket.connectToServer(QString::fromStdString(ExtraSocket::pathSocket(ULTRACOPIER_SOCKETNAME)),QIODevice::WriteOnly|QIODevice::Unbuffered);
    if(localSocket.waitForConnected(1000))
    {
        localSocket.disconnectFromServer();
        return true;
    }
    else
        return false;
}

#endif
