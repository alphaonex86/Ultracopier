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
	if(ultracopierArguments.size()==1)
	{
		if(external)
			QMessageBox::warning(NULL,tr("Warning"),tr("Ultracopier is already running, right click on its system tray icon (near the clock) to use it"));
		// else do nothing, is normal starting without arguements
	}
	if(ultracopierArguments.size()==2 && ultracopierArguments.last()=="quit" && external)
	{
		QCoreApplication::exit();
	}
}
