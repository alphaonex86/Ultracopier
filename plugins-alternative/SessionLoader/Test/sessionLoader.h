/** \file session-loader.h
\brief Define the session plugin loader test
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef SESSION_LOADER_TEST_H
#define SESSION_LOADER_TEST_H

#include <QObject>
#include "Environment.h"
#include "interface/PluginInterface_SessionLoader.h"

class TestPlugin : public PluginInterface_SessionLoader
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_SessionLoader)
public:
	TestPlugin();
	void setEnabled(bool);
	bool getEnabled();
private:
	bool internalValue;
};

#endif // SESSION_LOADER_TEST_H
