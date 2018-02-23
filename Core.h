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
        struct RemainingTimeLogarithmicColumn
        {
            std::vector<int> lastProgressionSpeed;
            uint64_t totalSize;
            uint64_t transferedSize;
        };

        struct CopyInstance
        {
            unsigned int id;
            PluginInterface_CopyEngine * engine;
            PluginInterface_Themes * interface;
            bool ignoreMode;
            Ultracopier::CopyMode mode;
            uint64_t numberOfFile;
            uint64_t numberOfTransferedFile;
            uint64_t currentProgression,totalProgression;//store the file byte transfered, used into the remaining time
            Ultracopier::EngineActionInProgress action;
            uint64_t lastProgression;//store the real byte transfered, used in speed calculation
            std::vector<RunningTransfer> transferItemList;//full info of started item, to have wich progression to poll
            std::vector<uint32_t> orderId;//external order send via listener plugin
            std::string folderListing;
            std::string collisionAction;
            std::string errorAction;
            bool isPaused;
            bool isRunning;
            Ultracopier::CopyType type;
            Ultracopier::TransferListOperation transferListOperation;
            bool haveError;
            QTime lastConditionalSync;
            QTimer *nextConditionalSync;
            bool copyEngineIsSync;
            bool canceled;//to not try groun when is in canceling

            Ultracopier::RemainingTimeAlgo remainingTimeAlgo;

            /** for RemainingTimeAlgo_Traditional **/
            //this speed is for instant speed
            std::vector<uint64_t> lastSpeedDetected;//stored in bytes
            std::vector<double> lastSpeedTime;//stored in ms
            //this speed is average speed on more time to calculate the remaining time
            std::vector<uint64_t> lastAverageSpeedDetected;//stored in bytes
            std::vector<double> lastAverageSpeedTime;//stored in ms

            /** for RemainingTimeAlgo_Logarithmic **/
            std::vector<RemainingTimeLogarithmicColumn> remainingTimeLogarithmicValue;
        };
        std::vector<CopyInstance> copyList;
        /** open with specific source/destination
        \param move Copy or move
        \param ignoreMode if need ignore the mode
        \param protocolsUsedForTheSources protocols used for sources
        \param protocolsUsedForTheDestination protocols used for destination
        */
        int openNewCopyEngineInstance(const Ultracopier::CopyMode &mode,const bool &ignoreMode,const std::vector<std::string> &protocolsUsedForTheSources=std::vector<std::string>(),const std::string &protocolsUsedForTheDestination="");
        /** open with specific copy engine
        \param move Copy or move
        \param ignoreMode if need ignore the mode
        \param name protocols used for sources
        */
        int openNewCopyEngineInstance(const Ultracopier::CopyMode &mode,const bool &ignoreMode,const std::string &name);

        /// \brief get the right copy instance (copy engine + interface), by signal emited from copy engine
        int indexCopySenderCopyEngine();
        /// \brief get the right copy instance (copy engine + interface), by signal emited from interface
        int indexCopySenderInterface();

        void connectEngine(const unsigned int &index);
        void connectInterfaceAndSync(const unsigned int &index);
        //void disconnectEngine(const int &index);
        //void disconnectInterface(const int &index);

        /** \brief update at periodic interval, the synchronization between copy engine and interface, but for specific entry
        \see forUpateInformation */
        void periodicSynchronizationWithIndex(const int &index);

        //for the internal management
        unsigned int incrementId();
        unsigned int nextId;
        std::vector<unsigned int> idList;
        QTime lastProgressionTime;
        QTimer forUpateInformation;///< used to call \see periodicSynchronization()
        void resetSpeedDetected(const unsigned int &bindex);

        /** Connect the copy engine instance provided previously to the management */
        int connectCopyEngine(const Ultracopier::CopyMode &mode,bool ignoreMode,const CopyEngineManager::returnCopyEngine &returnInformations);

        LogThread log;///< To save the log like mkpath, rmpath, error, copy, ...
        uint64_t realByteTransfered;

        static uint8_t fileCatNumber(uint64_t size);
    signals:
        void copyFinished(const uint32_t & orderId,bool withError) const;
        void copyCanceled(const uint32_t & orderId) const;
    public slots:
        /** \brief do copy with sources, but ask the destination */
        void newCopyWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources);
        void newTransfer(const Ultracopier::CopyMode &mode,const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources,const std::string &protocolsUsedForTheDestination,const std::string &destination);
        /** \brief do copy with sources and destination */
        void newCopy(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources,const std::string &protocolsUsedForTheDestination,const std::string &destination);
        /** \brief do move with sources, but ask the destination */
        void newMoveWithoutDestination(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources);
        /** \brief do move with sources and destination */
        void newMove(const uint32_t &orderId,const std::vector<std::string> &protocolsUsedForTheSources,const std::vector<std::string> &sources,const std::string &protocolsUsedForTheDestination,const std::string &destination);
        /** \brief open copy/move windows with specific engine */
        void addWindowCopyMove(const Ultracopier::CopyMode &mode,const std::string &name);
        /** \brief open transfer (copy+move) windows with specific engine */
        void addWindowTransfer(const std::string &name);
        /** new transfer list pased by the CLI */
        void newTransferList(std::string engine,std::string mode,std::string file);

        bool startNewTransferOneUniqueCopyEngine();
    private slots:
        /// \brief the copy engine have canceled the transfer
        void copyInstanceCanceledByEngine();
        /// \brief the interface have canceled the transfer
        void copyInstanceCanceledByInterface();
        /// \brief the transfer have been canceled
        void copyInstanceCanceledByIndex(const unsigned int &index);
        /// \brief only when the copy engine say it's ready to delete them self, it call this
        void deleteCopyEngine();

        // some stat update
        void actionInProgess(const Ultracopier::EngineActionInProgress &action);
        void newFolderListing(const std::string &path);
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
        void error(const std::string &path,const uint64_t &size,const uint64_t &mtime,const std::string &error);
        //for the extra logging
        void rmPath(const std::string &path);
        void mkPath(const std::string &path);

        /// \brief used to drag and drop files
        void urlDropped(const std::vector<std::string> &urls);
        /// \brief to rsync after a new interface connection
        void syncReady();
        void doneTime(const std::vector<std::pair<uint64_t,uint32_t> > &timeList);

        void getActionOnList(const std::vector<Ultracopier::ReturnActionOnCopyList> & actionList);
        void pushGeneralProgression(const uint64_t &current,const uint64_t &total);
};

#endif // CORE_H
