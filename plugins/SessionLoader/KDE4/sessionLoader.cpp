/** \file session-loader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>
#include <QFile>
#include <QDir>

#include "sessionLoader.h"
void TestPlugin::setEnabled(bool newValue)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, newValue: "+QString::number(newValue));
	QFile link(QDir::homePath()+"/.kde4/Autostart/ultracopier.sh");
	if(!newValue)
	{
		if(link.exists() && !link.remove())
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to remove from the startup: "+link.errorString());
	}
	else
	{
		if(link.open(QIODevice::WriteOnly))
		{
			link.write(QString("#!/bin/bash\n").toLocal8Bit());
			link.write(QString(QCoreApplication::applicationFilePath()).toLocal8Bit());
			link.close();
			if(!link.setPermissions(QFile::ExeOwner|QFile::WriteOwner|QFile::ReadOwner))
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to set permissions: "+link.errorString());
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to open in writing the file: "+link.errorString());
	}
}

bool TestPlugin::getEnabled()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, return this value: "+QString::number(QFile::exists(QDir::homePath()+"/.kde4/Autostart/ultracopier.sh")));
	//return the value into the variable
	return QFile::exists(QDir::homePath()+"/.kde4/Autostart/ultracopier.sh");
}

Q_EXPORT_PLUGIN2(sessionLoader, TestPlugin);
