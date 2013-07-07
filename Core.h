/** \file Core.h
\brief Define the class definition for core, the Copy of each copy/move window
\author alpha_one_x86
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
#include "LogThread.h"
#include "interface/PluginInterface_CopyEngine.h"
#include "interface/PluginInterface_Themes.h"

/** \brief Define the class definition for core, the Copy of each copy/move window

This class provide a core for dispatch the event of signal/slot, it checks too if not other instance is running */
class Core : public QObject
{
    Q_OBJECT
    public:
        /// \brief Initate the core of one copy or move window, dispatch the event specific at this window
        Core(CopyEngineManager *copyEngineList);
        ~Core();
    private:
        CopyEngineManager *copyEngineList;
        struct RunningTransfer
        {
            Ultracopier::ItemOfCopyList item;
            bool progression;
        };
        struct CopyInstance
        {
            int id;
            PluginInterface_CopyEngine * engine;
            PluginInterface_Themes * interface;
            bool ignoreMode;
            Ultracopier::CopyMode mode;
            quint64 numberOfFile;
            quint64 numberOfTransferedFile;
            quint64 currentProgression,totalProgression;//store the file byte transfered, used into the remaining time
            Ultracopier::EngineActionInProgress action;
            quint64 lastProgression;//store the real byte transfered, used in speed calculation
            //this speed is for instant speed
            QList<quint64> lastSpeedDetected;//stored in bytes
            QList<double> lastSpeedTime;//stored in ms
            //this speed is average speed on more time to calculate the remaining time
            QList<quint64> lastAverageSpeedDetected;//stored in bytes
            QList<double> lastAverageSpeedTime;//stored in ms
            QList<RunningTransfer> transferItemList;//full info of started item, to have wich progression to poll
            QList<quint32> orderId;//external order send via listener plugin
            QString folderListing;
            QString collisionAction;
            QString errorAction;
            bool isPaused;
            bool isRunning;
            Ultracopier::CopyType type;
            Ultracopier::TransferListOperation transferListOperation;
            bool haveError;
            QTime lastConditionalSync;
            QTimer *nextConditionalSync;
            bool copyEngineIsSync;
            bool canceled;//to not try groun when is in canceling
        };
        QList<CopyInstance> copyList;
        /** open with specific source/destination
        \param move Copy or move
        \param ignoreMode if need ignore the mode
        \param protocolsUsedForTheSources protocols used for sources
        \param protocolsUsedForTheDestination protocols used for destination
        */
        int openNewCopyEngineInstance(const Ultracopier::CopyMode &mode,const bool &ignoreMode,const QStringList &protocolsUsedForTheSources=QStringList(),const QString &protocolsUsedForTheDestination="");
        /** open with specific copy engine
        \param move Copy or move
        \param ignoreMode if need ignore the mode
        \param protocolsUsedForTheSources protocols used for sources
        \param protocolsUsedForTheDestination protocols used for destination
        */
        int openNewCopyEngineInstance(const Ultracopier::CopyMode &mode,const bool &ignoreMode,const QString &name);

        /// \brief get the right copy instance (copy engine + interface), by signal emited from copy engine
        int indexCopySenderCopyEngine();
        /// \brief get the right copy instance (copy engine + interface), by signal emited from interface
        int indexCopySenderInterface();

        void connectEngine(const int &index);
        void connectInterfaceAndSync(const int &index);
        //void disconnectEngine(const int &index);
        //void disconnectInterface(const int &index);

        /** \brief update at periodic interval, the synchronization between copy engine and interface, but for specific entry
        \see forUpateInformation */
        void periodicSynchronizationWithIndex(const int &index);

        //for the internal management
        int incrementId();
        int nextId;
        QList<int> idList;
        QTime lastProgressionTime;
        QTimer forUpateInformation;///< used to call \see periodicSynchronization()
        void resetSpeedDetected(const int &index);

        /** Connect the copy engine instance provided previously to the management */
        int connectCopyEngine(const Ultracopier::CopyMode &mode,bool ignoreMode,const CopyEngineManager::returnCopyEngine &returnInformations);

        LogThread log;///< To save the log like mkpath, rmpath, error, copy, ...
        quint64 realByteTransfered;
    signals:
        void copyFinished(const quint32 & orderId,bool withError);
        void copyCanceled(const quint32 & orderId);
    public slots:
        /** \brief do copy with sources, but ask the destination */
        void newCopyWithoutDestination(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources);
        void newTransfer(const Ultracopier::CopyMode &mode,const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources,const QString &protocolsUsedForTheDestination,const QString &destination);
        /** \brief do copy with sources and destination */
        void newCopy(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources,const QString &protocolsUsedForTheDestination,const QString &destination);
        /** \brief do move with sources, but ask the destination */
        void newMoveWithoutDestination(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources);
        /** \brief do move with sources and destination */
        void newMove(const quint32 &orderId,const QStringList &protocolsUsedForTheSources,const QStringList &sources,const QString &protocolsUsedForTheDestination,const QString &destination);
        /** \brief open copy/move windows with specific engine */
        void addWindowCopyMove(const Ultracopier::CopyMode &mode,const QString &name);
        /** \brief open transfer (copy+move) windows with specific engine */
        void addWindowTransfer(const QString &name);
        /** new transfer list pased by the CLI */
        void newTransferList(QString engine,QString mode,QString file);
    private slots:
        /// \brief the copy engine have canceled the transfer
        void copyInstanceCanceledByEngine();
        /// \brief the interface have canceled the transfer
        void copyInstanceCanceledByInterface();
        /// \brief the transfer have been canceled
        void copyInstanceCanceledByIndex(const int &index);
        /// \brief only when the copy engine say it's ready to delete them self, it call this
        void deleteCopyEngine();

        // some stat update
        void actionInProgess(const Ultracopier::EngineActionInProgress &action);
        void newFolderListing(const QString &path);
        void isInPause(const bool&);

        /** \brief update at periodic interval, the synchronization between copy engine and interface
        \see forUpateInformation */
        void periodicSynchronization();

        //reset some information
        void resetSpeedDetectedEngine();
        void resetSpeedDetectedInterface();

        //load the interface
        void loadInterface();
        void unloadInterface();

        //error occurred
        void error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error);
        //for the extra logging
        void rmPath(const QString &path);
        void mkPath(const QString &path);

        /// \brief used to drag and drop files
        void urlDropped(const QList<QUrl> &urls);
        /// \brief to rsync after a new interface connection
        void syncReady();

        void getActionOnList(const QList<Ultracopier::ReturnActionOnCopyList> & actionList);
        void pushGeneralProgression(const quint64 &current,const quint64 &total);
};

#endif // CORE_H
