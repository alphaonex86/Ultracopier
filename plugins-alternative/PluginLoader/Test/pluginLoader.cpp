/** \file session-loader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>
#include <QMessageBox>

#include "pluginLoader.h"

TestPlugin::TestPlugin()
{
	//set the startup value into the variable
	internalValue=false;
}

void TestPlugin::setEnabled(bool newValue)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, newValue: "+QString::number(newValue));
	//set value into the variable
	if(newValue)
		emit newState(Caught);
	else
		emit newState(Uncaught);
}

void TestPlugin::setResources(QString writePath,QString pluginPath)
{
	Q_UNUSED(writePath);
	Q_UNUSED(pluginPath);
}

Q_EXPORT_PLUGIN2(pluginLoader, TestPlugin);
