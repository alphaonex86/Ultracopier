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
		struct RunningTransfer
		{
			ItemOfCopyList item;
			bool progression;
		};
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
			QList<RunningTransfer> transferItemList;//full info of started item, to have wich progression to poll
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
		int openNewCopy(const CopyMode &mode,const bool &ignoreMode,const QStringList &protocolsUsedForTheSources=QStringList(),const QString &protocolsUsedForTheDestination="");
		int openNewCopy(const CopyMode &mode,const bool &ignoreMode,const QString &name);
		int incrementId();
		int nextId;
		QList<int> idList;
		QTime lastProgressionTime;
		int indexCopySenderCopyEngine();
		int indexCopySenderInterface();
		void connectEngine(const int &index);
		void connectInterfaceAndSync(const int &index);
		void disconnectEngine(const int &index);
		void disconnectInterface(const int &index);
		void periodiqueSync(const int &index);
		QTimer forUpateInformation;
		void resetSpeedDetected(const int &index);
		int connectCopyEngine(const CopyMode &mode,bool ignoreMode,const CopyEngineManager::returnCopyEngine &returnInformations);
		LogThread log;
		//temp variable
		int index,index_sub_loop,loop_size,loop_sub_size;
		double totTime;
		double totSpeed;
	signals:
		void copyFinished(const quint32 & orderId,bool withError);
		void copyCanceled(const quint32 & orderId);
	public slots:
		/** \brief do copy with sources, but ask the destination */
		void newCopy(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources);
		/** \brief do copy with sources and destination */
		void newCopy(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources,const QString &protocolsUsedForTheDestination,const QString &destination);
		/** \brief do move with sources, but ask the destination */
		void newMove(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources);
		/** \brief do move with sources and destination */
		void newMove(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources,const QString &protocolsUsedForTheDestination,const QString &destination);
		/** \brief open copy/move windows with specific engine */
		void addWindowCopyMove(const CopyMode &mode,const QString &name);
		/** \brief open transfer (copy+move) windows with specific engine */
		void addWindowTransfer(const QString &name);
	private slots:
		void copyInstanceCanceledByEngine();
		void copyInstanceCanceledByInterface();
		void copyInstanceCanceledByIndex(const int &index);
		void actionInProgess(const EngineActionInProgress &action);
		void newFolderListing(const QString &path);
		void newCollisionAction(const QString &action);
		void newErrorAction(const QString &action);
		void isInPause(const bool&);
		void periodiqueSync();
		void resetSpeedDetectedEngine();
		void resetSpeedDetectedInterface();
		void loadInterface();
		void unloadInterface();
		//error occurred
		void error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error);
		//for the extra logging
		void rmPath(const QString &path);
		void mkPath(const QString &path);
		void urlDropped(const QList<QUrl> &urls);
};

#endif // CORE_H
