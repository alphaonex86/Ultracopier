/** \file CopyEngineManager.h
\brief Define the copy engine manager
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

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
#include "FacilityEngine.h"

namespace Ui {
	class CopyEngineOptions;
}

/** \brief Manage copy engine plugins and their instance */
class CopyEngineManager : public QObject, public GlobalClass
{
	Q_OBJECT
public:
	/** \brief internal structure to return one copy engine instance */
	struct returnCopyEngine
	{
		PluginInterface_CopyEngine * engine;	///< The copy engine instance
		bool canDoOnlyCopy;			///< true if can do only the copy (not move)
		CopyType type;				///< Kind of copy what it can do
		TransferListOperation transferListOperation;
	};
	explicit CopyEngineManager(OptionDialog *optionDialog);
	/** \brief return copy engine instance when know the sources and destinations
	  \param mode the mode (copy/move)
	  \param protocolsUsedForTheSources list of sources used
	  \param protocolsUsedForTheDestination list of destination used
	  \see getCopyEngine()
	  */
	returnCopyEngine getCopyEngine(const CopyMode &mode,const QStringList &protocolsUsedForTheSources,const QString &protocolsUsedForTheDestination);
	/** \brief return copy engine instance with specific engine
	  \param mode the mode (copy/move)
	  \param name name of the engine needed
	  \see getCopyEngine()
	  */
	returnCopyEngine getCopyEngine(const CopyMode &mode,const QString &name);
	//bool currentEngineCanDoOnlyCopy(QStringList protocolsUsedForTheSources,QString protocolsUsedForTheDestination="");
	//CopyType currentEngineGetCopyType(QStringList protocolsUsedForTheSources,QString protocolsUsedForTheDestination="");
	/** \brief to send all signal because all object is connected on it */
	void setIsConnected();
	/** \brief check if the protocols given is supported by the copy engine
	  \see Core::newCopy()
	  \see Core::newMove()
	  */
	bool protocolsSupportedByTheCopyEngine(PluginInterface_CopyEngine * engine,const QStringList &protocolsUsedForTheSources,const QString &protocolsUsedForTheDestination);
private slots:
	void onePluginAdded(const PluginsAvailable &plugin);
	void onePluginWillBeRemoved(const PluginsAvailable &plugin);
	void onePluginWillBeUnloaded(const PluginsAvailable &plugin);
	#ifdef ULTRACOPIER_DEBUG
	void debugInformation(DebugLevel level,const QString& fonction,const QString& text,const QString& file,const int& ligne);
	#endif // ULTRACOPIER_DEBUG
	/// \brief To notify when new value into a group have changed
	void newOptionValue(const QString &groupName,const QString &variableName,const QVariant &value);
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
		TransferListOperation transferListOperation;
		LocalPluginOptions *options;
		QWidget *optionsWidget;
	};
	QList<CopyEnginePlugin> pluginList;
	OptionDialog *optionDialog;
	bool isConnected;
	FacilityEngine facilityEngine;
signals:
	//void newCopyEngineOptions(QString,QString,QWidget *);
	void addCopyEngine(QString name,bool canDoOnlyCopy);
	void removeCopyEngine(QString name);
	void previouslyPluginAdded(PluginsAvailable);
};

#endif // COPYENGINEMANAGER_H
