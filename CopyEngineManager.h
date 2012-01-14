/** \file CopyEngineManager.h
\brief Define the copy engine manager
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef COPYENGINEMANAGER_H
#define COPYENGINEMANAGER_H

#include <QObject>
#include <QPluginLoader>
#include <QList>
#include <QWidget>
#include <QString>

#include "Environment.h"
#include "GlobalClass.h"
#include "LocalPluginOptions.h"
#include "OptionDialog.h"
#include "interface/PluginInterface_CopyEngine.h"

namespace Ui {
	class CopyEngineOptions;
}

class CopyEngineManager : public QObject, public GlobalClass
{
	Q_OBJECT
public:
	struct returnCopyEngine
	{
		PluginInterface_CopyEngine * engine;
		bool canDoOnlyCopy;
		CopyType type;
	};
	explicit CopyEngineManager(OptionDialog *optionDialog);
	returnCopyEngine getCopyEngine(CopyMode mode,QStringList protocolsUsedForTheSources,QString protocolsUsedForTheDestination);
	returnCopyEngine getCopyEngine(CopyMode mode,QString name);
	//bool currentEngineCanDoOnlyCopy(QStringList protocolsUsedForTheSources,QString protocolsUsedForTheDestination="");
	//CopyType currentEngineGetCopyType(QStringList protocolsUsedForTheSources,QString protocolsUsedForTheDestination="");
	void setIsConnected();
private slots:
	void onePluginAdded(PluginsAvailable plugin);
	void onePluginWillBeRemoved(PluginsAvailable plugin);
	void onePluginWillBeUnloaded(PluginsAvailable plugin);
	#ifdef ULTRACOPIER_DEBUG
	void debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne);
	#endif // ULTRACOPIER_DEBUG
	/// \brief To notify when new value into a group have changed
	void newOptionValue(QString groupName,QString variableName,QVariant value);
	void allPluginIsloaded();
private:
	/// \brief the option interface
	struct CopyEnginePlugin
	{
		QString path;
		QString name;
		QString pluginPath;
		QStringList supportedProtocolsForTheSource;
		QStringList supportedProtocolsForTheDestination;
		QPluginLoader * pointer;
		PluginInterface_CopyEngineFactory * factory;
		QList<PluginInterface_CopyEngine *> intances;
		bool canDoOnlyCopy;
		CopyType type;
		LocalPluginOptions *options;
		QWidget *optionsWidget;
	};
	QList<CopyEnginePlugin> pluginList;
	OptionDialog *optionDialog;
	bool isConnected;
signals:
	//void newCopyEngineOptions(QString,QString,QWidget *);
	void addCopyEngine(QString name,bool canDoOnlyCopy);
	void removeCopyEngine(QString name);
};

#endif // COPYENGINEMANAGER_H
