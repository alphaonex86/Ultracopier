/** \file CliParser.cpp
\brief To group into one class, the CLI parsing
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "CliParser.h"

#include <QDebug>

CliParser::CliParser(QObject *parent) :
    QObject(parent)
{
}

/** \brief method to parse the ultracopier arguments
  \param ultracopierArguments the argument list
  \param external true if the arguments come from other instance of ultracopier
*/
void CliParser::cli(const QStringList &ultracopierArguments,const bool &external,const bool &onlyCheck)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("ultracopierArguments: ")+ultracopierArguments.join(QStringLiteral(";")));
    if(ultracopierArguments.size()==1)
    {
        if(external)
            QMessageBox::warning(NULL,tr("Warning"),tr("Ultracopier is already running, right click on its system tray icon (near the clock) to use it"));
        // else do nothing, is normal starting without arguements
        return;
    }
    else if(ultracopierArguments.size()==2)
    {
        if(ultracopierArguments.last()==QStringLiteral("quit"))
        {
            if(onlyCheck)
                return;
            QCoreApplication::exit();
            return;
        }
        else if(ultracopierArguments.last()==QStringLiteral("--help"))
        {
            showHelp(false);
            return;
        }
        else if(ultracopierArguments.last()==QStringLiteral("--options"))
        {
            emit showOptions();
            return;
        }
        else if(ultracopierArguments.last().endsWith(QStringLiteral(".urc")))
        {
            tryLoadPlugin(ultracopierArguments.last());
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Command line not understand");
        showHelp();
        return;
    }
    else if(ultracopierArguments.size()==3)
    {
        if(ultracopierArguments.at(1)==QStringLiteral("Transfer-list"))
        {
            if(onlyCheck)
                return;
            QFile transferFile(ultracopierArguments.last());
            if(transferFile.open(QIODevice::ReadOnly))
            {
                QString content;
                QByteArray data=transferFile.readLine(64);
                if(data.size()<=0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Problem reading file, or file size is 0"));
                    QMessageBox::warning(NULL,tr("Warning"),tr("Problem reading file, or file size is 0"));
                    transferFile.close();
                    return;
                }
                content=QString::fromUtf8(data);
                content.remove('\n');
                QStringList transferListArguments=content.split(';');
                if(transferListArguments.size()!=4 || transferListArguments.at(0)!=QStringLiteral("Ultracopier-0.3") || transferListArguments.at(1)!=QStringLiteral("Transfer-list"))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("This file is not supported transfer list"));
                    QMessageBox::warning(NULL,tr("Warning"),tr("This file is not supported transfer list"));
                    transferFile.close();
                    return;
                }
                transferFile.close();
                emit newTransferList(transferListArguments.at(3),transferListArguments.at(2),ultracopierArguments.last());
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Unable to open the transfer list file: %1").arg(transferFile.errorString()));
                QMessageBox::warning(NULL,tr("Warning"),tr("Unable to open the transfer list file"));
                return;
            }
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Command line not understand");
        showHelp();
        return;
    }
    else if(ultracopierArguments.size()>3)
    {
        if(ultracopierArguments.at(1)==QStringLiteral("Copy") || ultracopierArguments.at(1)==QStringLiteral("cp"))
        {
            if(onlyCheck)
                return;
            QStringList transferList=ultracopierArguments;
            transferList.removeFirst();
            transferList.removeFirst();
            if(transferList.last()=="?")
            {
                transferList.removeLast();
                emit newCopyWithoutDestination(transferList);
            }
            else
            {
                QString destination=transferList.last();
                transferList.removeLast();
                emit newCopy(transferList,destination);
            }
            return;
        }
        else if(ultracopierArguments.at(1)==QStringLiteral("Move") || ultracopierArguments.at(1)==QStringLiteral("mv"))
        {
            if(onlyCheck)
                return;
            QStringList transferList=ultracopierArguments;
            transferList.removeFirst();
            transferList.removeFirst();
            if(transferList.last()=="?")
            {
                transferList.removeLast();
                emit newMoveWithoutDestination(transferList);
            }
            else
            {
                QString destination=transferList.last();
                transferList.removeLast();
                emit newMove(transferList,destination);
            }
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Command line not understand"));
        showHelp();
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Command line not understand"));
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
