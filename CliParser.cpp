/** \file CliParser.cpp
\brief To group into one class, the CLI parsing
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "CliParser.h"
#include "cpp11addition.h"
#include "Core.h"
#ifdef Q_OS_WIN32
#include <windows.h>
#endif
//this is just to support clipboard
#include <QClipboard>
#include <QGuiApplication>
#include <QRegularExpression>

#include <QDebug>

CliParser::CliParser(QObject *parent) :
    QObject(parent)
{
    //this->core=core;
}

/** \brief method to parse the ultracopier arguments
  \param ultracopierArguments the argument list
  \param external true if the arguments come from other instance of ultracopier
*/
void CliParser::cli(const std::vector<std::string> &ultracopierArguments,const bool &external,const bool &onlyCheck)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"ultracopierArguments: "+stringimplode(ultracopierArguments,';'));
    if(ultracopierArguments.size()==1)
    {
        if(external)
        {
            //if(!core->startNewTransferOneUniqueCopyEngine())
            {
                #ifdef Q_OS_WIN32
                QString message(tr("Ultracopier is already running, right click on its system tray icon (near the clock) to use it or just copy and paste"));
                #else
                QString message(tr("Ultracopier is already running, view all notification area icons (near the clock), right click on its system tray icon to use it or just copy and paste"));
                #endif

                QMessageBox::warning(NULL,tr("Warning"),message);
                showSystrayMessage(message.toStdString());
            }
        }
        // else do nothing, is normal starting without arguements
        return;
    }
    else if(ultracopierArguments.size()==2)
    {
        if(ultracopierArguments.back()=="quit")
        {
            if(onlyCheck)
                return;
            QCoreApplication::exit();
            return;
        }
        else if(ultracopierArguments.back()=="uninstall")
        {
            if(onlyCheck)
                return;
            #ifdef Q_OS_WIN32
            HKEY    ultracopier_regkey;
            if(RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, 0, REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, 0, &ultracopier_regkey, 0)==ERROR_SUCCESS)
            {
                RegDeleteValue(ultracopier_regkey, TEXT("ultracopier"));
                RegDeleteValue(ultracopier_regkey, TEXT("catchcopy"));
                RegDeleteValue(ultracopier_regkey, TEXT("catchcopy32.dll"));
                RegDeleteValue(ultracopier_regkey, TEXT("catchcopy32d.dll"));
                RegDeleteValue(ultracopier_regkey, TEXT("catchcopy64.dll"));
                RegDeleteValue(ultracopier_regkey, TEXT("catchcopy64d.dll"));
                RegDeleteValue(ultracopier_regkey, TEXT("catchcopy32all.dll"));
                RegDeleteValue(ultracopier_regkey, TEXT("catchcopy32alld.dll"));
                RegDeleteValue(ultracopier_regkey, TEXT("catchcopy64all.dll"));
                RegDeleteValue(ultracopier_regkey, TEXT("catchcopy64alld.dll"));
                RegCloseKey(ultracopier_regkey);
            }
            #endif
            QCoreApplication::exit();
            return;
        }
        else if(ultracopierArguments.back()=="--help")
        {
            showHelp(false);
            return;
        }
        else if(ultracopierArguments.back()=="--options")
        {
            emit showOptions();
            return;
        }
        else if(stringEndsWith(ultracopierArguments.back(),".urc"))
        {
            tryLoadPlugin(ultracopierArguments.back());
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Command line not understand");
        showHelp();
        return;
    }
    else if(ultracopierArguments.size()==3)
    {
        if(ultracopierArguments.at(1)=="Transfer-list")
        {
            if(onlyCheck)
                return;
            QFile transferFile(QString::fromStdString(ultracopierArguments.back()));
            if(transferFile.open(QIODevice::ReadOnly))
            {
                QString content;
                QByteArray data=transferFile.readLine(64);
                if(data.size()<=0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Problem reading file, or file size is 0");
                    QMessageBox::warning(NULL,tr("Warning"),tr("Problem reading file, or file size is 0"));
                    transferFile.close();
                    return;
                }
                content=QString::fromUtf8(data);
                std::vector<std::string> transferListArguments=stringsplit(content.toStdString(),';');
                transferListArguments[3].erase(std::remove(transferListArguments[3].begin(), transferListArguments[3].end(),'\n'),transferListArguments[3].end());
                if(transferListArguments.at(0)!="Ultracopier" ||
                        transferListArguments.at(1)!="Transfer-list" ||
                        (transferListArguments.at(2)!="Transfer" && transferListArguments.at(2)!="Copy" && transferListArguments.at(2)!="Move")
                        )
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"This file is not supported transfer list");
                    QMessageBox::warning(NULL,tr("Warning"),tr("This file is not supported transfer list"));
                    transferFile.close();
                    return;
                }
                transferFile.close();
                emit newTransferList(transferListArguments.at(3),transferListArguments.at(2),ultracopierArguments.back());
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to open the transfer list file: "+transferFile.errorString().toStdString());
                QMessageBox::warning(NULL,tr("Warning"),tr("Unable to open the transfer list file"));
                return;
            }
            return;
        }
        if(ultracopierArguments.at(1)=="CBmv" || ultracopierArguments.at(1)=="CBcp")
        {
            if(ultracopierArguments.size()==3)
            {
                QClipboard *clipboard = QGuiApplication::clipboard();
                #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                const QStringList &l=clipboard->text().split(QRegularExpression("[\n\r]+"));
                #else
                const QStringList &l=clipboard->text().split(QRegularExpression("[\n\r]+"),Qt::SkipEmptyParts);
                #endif
                /*linux:
                file:///path/test/master.zip
                file:///path/test/New text file
                Windows:
                file://Z:/X
                */
                std::vector<std::string> sourceList;
                int index=0;
                while(index<l.size())
                {
                    sourceList.push_back(l.at(index).toStdString());
                    index++;
                }
                if(ultracopierArguments.back()=="?")
                {
                    if(ultracopierArguments.at(1)=="CBmv")
                        emit newMoveWithoutDestination(sourceList);
                    else
                        emit newCopyWithoutDestination(sourceList);
                }
                else
                {
                    if(ultracopierArguments.at(1)=="CBmv")
                        emit newMove(sourceList,ultracopierArguments.back());
                    else
                        emit newCopy(sourceList,ultracopierArguments.back());
                }
                return;
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Command line not understand");
                showHelp();
                return;
            }
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Command line not understand");
        showHelp();
        return;
    }
    else if(ultracopierArguments.size()>3)
    {
        if(ultracopierArguments.at(1)=="Copy" || ultracopierArguments.at(1)=="cp")
        {
            if(onlyCheck)
                return;
            std::vector<std::string> transferList=ultracopierArguments;
            transferList.erase(transferList.cbegin());//app path
            transferList.erase(transferList.cbegin());//command
            if(transferList.back()=="?")
            {
                transferList.pop_back();
                emit newCopyWithoutDestination(transferList);
            }
            else
            {
                std::string destination=transferList.back();
                transferList.pop_back();
                emit newCopy(transferList,destination);
            }
            return;
        }
        else if(ultracopierArguments.at(1)=="Move" || ultracopierArguments.at(1)=="mv" || ultracopierArguments.at(1)=="CBmv")
        {
            if(onlyCheck)
                return;
            std::vector<std::string> transferList=ultracopierArguments;
            transferList.erase(transferList.cbegin());//app path
            transferList.erase(transferList.cbegin());//command
            if(transferList.back()=="?")
            {
                transferList.pop_back();
                emit newMoveWithoutDestination(transferList);
            }
            else
            {
                std::string destination=transferList.back();
                transferList.pop_back();
                emit newMove(transferList,destination);
            }
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Command line not understand");
        showHelp();
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Command line not understand");
    showHelp();
}

/** \brief show the help
 *\param incorrectArguments if the help is call because the arguments are wrong */
void CliParser::showHelp(const bool &incorrectArguments)
{
    if(incorrectArguments)
        qDebug() << "Incorrect arguments detected";
    qDebug() << tr("The arguments possible are:");
    qDebug() << "--help : "+tr("To display this help");
    qDebug() << "--options : "+tr("To display the options");
    qDebug() << "quit : "+tr("To quit the other instances (if running)");
    qDebug() << "Transfer-list [transfer list file] : "+tr("Open transfer list");
    qDebug() << "cp [source [source2]] [destination] : "+tr("To copy sources to destination, separated by space. If destination is \"?\", ultracopier will ask the user");
    qDebug() << "mv [source [source2]] [destination] : "+tr("To move sources to destination, separated by space. If destination is \"?\", ultracopier will ask the user");

    QString message;
    if(incorrectArguments)
        message+="<b>"+tr("Command not valid")+"</b><br />\n";
    message+=+"<b></b>"+tr("The arguments possible are:")+"\n<ul>";
    message+="<li><b>--help</b> : "+tr("To display this help")+"</li>\n";
    message+="<li><b>--options</b> : "+tr("To display the options")+"</li>\n";
    message+="<li><b>quit</b> : "+tr("To quit the other instances (if running)")+"</li>\n";
    message+="<li><b>Transfer-list [transfer list file]</b> : "+tr("Open transfer list")+"</li>\n";
    message+="<li><b>cp [source [source2]] [destination]</b> : "+tr("To copy sources to destination, separated by space. If destination is \"?\", ultracopier will ask the user")+"</li>\n";
    message+="<li><b>mv [source [source2]] [destination]</b> : "+tr("To move sources to destination, separated by space. If destination is \"?\", ultracopier will ask the user")+"</li>\n";
    message+=+"</ul>";
    if(incorrectArguments)
        QMessageBox::warning(NULL,tr("Warning"),message);
    else
        QMessageBox::information(NULL,tr("Help"),message);
}
