/** \file session-loader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>
#include <QMessageBox>

#include "sessionLoader.h"

TestPlugin::TestPlugin()
{
	//set the startup value into the variable
	internalValue=false;
}

void TestPlugin::setEnabled(bool newValue)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, newValue: "+QString::number(newValue));
	//set value into the variable
	internalValue=newValue;
}

bool TestPlugin::getEnabled()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, return this value: "+QString::number(internalValue));
	//return the value into the variable
	return internalValue;
}

Q_EXPORT_PLUGIN2(sessionLoader, TestPlugin);
