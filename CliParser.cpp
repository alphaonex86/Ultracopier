/** \file CliParser.cpp
\brief To group into one class, the CLI parsing
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "CliParser.h"

CliParser::CliParser(QObject *parent) :
    QObject(parent)
{
}

void CliParser::cli(const QStringList &ultracopierArguments,const bool &external)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"ultracopierArguments: "+ultracopierArguments.join(";"));
	if(ultracopierArguments.size()==1)
	{
		if(external)
			QMessageBox::warning(NULL,tr("Warning"),tr("Ultracopier is already running, right click on its system tray icon (near the clock) to use it"));
		// else do nothing, is normal starting without arguements
		return;
	}
	if(ultracopierArguments.size()==2 && ultracopierArguments.last()=="quit" && external)
	{
		QCoreApplication::exit();
		return;
	}
	if(ultracopierArguments.size()>3)
	{
		if(ultracopierArguments[1]=="Copy" || ultracopierArguments[1]=="cp")
		{
			QStringList transferList=ultracopierArguments;
			transferList.removeFirst();
			transferList.removeFirst();
			if(transferList.last()=="?")
			{
				transferList.removeLast();
				emit newCopy(transferList);
			}
			else
			{
				QString destination=transferList.last();
				transferList.removeLast();
				emit newCopy(transferList,destination);
			}
			return;
		}
		if(ultracopierArguments[1]=="Move" || ultracopierArguments[1]=="mv")
		{
			QStringList transferList=ultracopierArguments;
			transferList.removeFirst();
			transferList.removeFirst();
			if(transferList.last()=="?")
			{
				transferList.removeLast();
				emit newMove(transferList);
			}
			else
			{
				QString destination=transferList.last();
				transferList.removeLast();
				emit newMove(transferList,destination);
			}
			return;
		}
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Command line not understand");
		QMessageBox::warning(NULL,tr("Warning"),tr("Command line not understand"));
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Command line not understand");
	QMessageBox::warning(NULL,tr("Warning"),tr("Command line not understand"));
}
