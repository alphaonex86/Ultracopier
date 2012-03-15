/** \file Core.h
\brief Define the class definition for core, the Copy of each copy/move window
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QStringList>
#include <QString>
#include <QList>
#include <QTimer>
#include <QTime>
#include <QFile>
#include <QUrl>

#include "Environment.h"
#include "StructEnumDefinition.h"
#include "CopyEngineManager.h"
#include "GlobalClass.h"
#include "LogThread.h"
#include "interface/PluginInterface_CopyEngine.h"
#include "interface/PluginInterface_Themes.h"

/** \brief Define the class definition for core, the Copy of each copy/move window

This class provide a core for dispatch the event of signal/slot, it checks too if not other instance is running */
class Core : public QObject, public GlobalClass
{
	Q_OBJECT
	public:
		/// \brief Initate the core of one copy or move window, dispatch the event specific at this window
		Core(CopyEngineManager *copyEngineList);
	private:
		CopyEngineManager *copyEngineList;
		struct CopyInstance
		{
			int id;
			PluginInterface_CopyEngine * engine;
			PluginInterface_Themes * interface;
			bool ignoreMode;
			CopyMode mode;
			quint64 numberOfFile;
			quint64 numberOfTransferedFile;
			quint64 sizeToCopy;
			EngineActionInProgress action;
			quint64 lastProgression;//store the real byte transfered, used in time remaining calculation
			QList<quint64> lastSpeedDetected;//stored in bytes
			QList<double> lastSpeedTime;//stored in ms
			QList<ItemOfCopyList> transferItemList;//full info of started item
			QList<quint64> progressionList;
			QList<quint32> orderId;//external order send via listener plugin
			QString folderListing;
			QString collisionAction;
			QString errorAction;
			bool isPaused;
			quint64 baseTime;//stored in ms
			QTime runningTime;
			bool isRunning;
			CopyType type;
			TransferListOperation transferListOperation;
			bool haveError;
			QTime lastConditionalSync;
			QTimer *nextConditionalSync;
		};
		QList<CopyInstance> copyList;
		int openNewCopy(CopyMode mode,bool ignoreMode,QStringList protocolsUsedForTheSources=QStringList(),QString protocolsUsedForTheDestination="");
		int openNewCopy(CopyMode mode,bool ignoreMode,QString name);
		int incrementId();
		int nextId;
		QList<int> idList;
		QTime lastProgressionTime;
		int indexCopySenderCopyEngine();
		int indexCopySenderInterface();
		void connectEngine(int index);
		void connectInterfaceAndSync(int index);
		void disconnectEngine(int index);
		void disconnectInterface(int index);
		void periodiqueSync(int index);
		void plannedConditionalSync();
		void conditionalSync(int index);
		QTimer forUpateInformation;
		void resetSpeedDetected(int index);
		int connectCopyEngine(CopyMode mode,bool ignoreMode,CopyEngineManager::returnCopyEngine returnInformations);
		LogThread log;
		bool log_enable;
		bool log_enable_transfer;
		bool log_enable_error;
		bool log_enable_folder;
		//temp variable
		int index,index_sub_loop,loop_size,loop_sub_size;
		double totTime;
		double totSpeed;
	signals:
		void copyFinished(const quint32 & orderId,bool withError);
		void copyCanceled(const quint32 & orderId);
	public slots:
		/** \brief do copy with sources, but ask the destination */
		void newCopy(quint32 orderId,QStringList protocolsUsedForTheSources,QStringList sources);
		/** \brief do copy with sources and destination */
		void newCopy(quint32 orderId,QStringList protocolsUsedForTheSources,QStringList sources,QString protocolsUsedForTheDestination,QString destination);
		/** \brief do move with sources, but ask the destination */
		void newMove(quint32 orderId,QStringList protocolsUsedForTheSources,QStringList sources);
		/** \brief do move with sources and destination */
		void newMove(quint32 orderId,QStringList protocolsUsedForTheSources,QStringList sources,QString protocolsUsedForTheDestination,QString destination);
		/** \brief open copy/move windows with specific engine */
		void addWindowCopyMove(CopyMode mode,QString name);
		/** \brief open transfer (copy+move) windows with specific engine */
		void addWindowTransfer(QString name);
	private slots:
		void copyInstanceCanceledByEngine();
		void copyInstanceCanceledByInterface();
		void copyInstanceCanceledByIndex(int index);
		void actionInProgess(EngineActionInProgress action);
		void newTransferStart(const ItemOfCopyList &item);
		void newTransferStop(const quint64 &id);
		void newFolderListing(const QString &path);
		void newCollisionAction(QString action);
		void newErrorAction(QString action);
		void isInPause(bool);
		void periodiqueSync();
		void newActionOnList();
		void resetSpeedDetectedEngine();
		void resetSpeedDetectedInterface();
		void loadInterface();
		void unloadInterface();
		void newOptionValue(const QString &group,const QString &name,const QVariant &value);
		//error occurred
		void error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error);
		//for the extra logging
		void rmPath(const QString &path);
		void mkPath(const QString &path);
		void urlDropped(QList<QUrl> urls);
};

#endif // CORE_H
