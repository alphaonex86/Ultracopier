/** \file session-loader.h
\brief Define the session plugin loader test
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef PLUGIN_LOADER_TEST_H
#define PLUGIN_LOADER_TEST_H

#include <QObject>
#include "interface/PluginInterface_PluginLoader.h"
#include "Environment.h"

class TestPlugin : public PluginInterface_PluginLoader
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_PluginLoader)
public:
	TestPlugin();
	void setEnabled(bool);
	void setResources(QString writePath,QString pluginPath);
private:
	bool internalValue;
};

#endif // PLUGIN_LOADER_TEST_H
