/** \file copyEngine.h
\brief Define the copy engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include "../../../interface/PluginInterface_CopyEngine.h"
#include "FileErrorDialog.h"
#include "FileExistsDialog.h"
#include "FolderExistsDialog.h"
#include "FileIsSameDialog.h"
#include "ui_copyEngineOptions.h"
#include "Environment.h"
#include "ListThread.h"
#include "Filters.h"
#include "RenamingRules.h"

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
#include "DebugDialog.h"
#include <QTimer>
#endif

#ifndef COPY_ENGINE_H
#define COPY_ENGINE_H

namespace Ui {
    class copyEngineOptions;
}

/// \brief the implementation of copy engine plugin, manage directly few stuff, else pass to ListThread class.
class CopyEngine : public PluginInterface_CopyEngine
{
        Q_OBJECT
public:
    CopyEngine(FacilityInterface * facilityEngine);
    ~CopyEngine();
    void connectTheSignalsSlots();
private:
    ListThread *            listThread;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    DebugDialog             debugDialogWindow;
    #endif
    QWidget *               tempWidget;
    Ui::copyEngineOptions *	ui;
    bool                    uiIsInstalled;
    QWidget *               interface;
    Filters *               filters;
    RenamingRules *			renamingRules;
    FacilityInterface *		facilityEngine;
    quint32                 maxSpeed;
    bool                    doRightTransfer;
    bool                    keepDate;
    int                     blockSize;
    int                     parallelBuffer;
    int                     sequentialBuffer;
    int                     parallelizeIfSmallerThan;
    bool                    followTheStrictOrder;
    bool                    deletePartiallyTransferredFiles;
    int                     inodeThreads;
    bool                    renameTheOriginalDestination;
    bool                    moveTheWholeFolder;
    bool                    autoStart;
    bool                    checkDestinationFolderExists;
    FileExistsAction		alwaysDoThisActionForFileExists;
    FileErrorAction			alwaysDoThisActionForFileError;
    FileErrorAction			alwaysDoThisActionForFolderError;
    FolderExistsAction		alwaysDoThisActionForFolderExists;
    TransferAlgorithm       transferAlgorithm;
    bool                    dialogIsOpen;
    volatile bool			stopIt;
    QString                 defaultDestinationFolder;
    /// \brief error queue
    struct errorQueueItem
    {
        TransferThread * transfer;	///< NULL if send by scan thread
        ScanFileOrFolder * scan;	///< NULL if send by transfer thread
        bool mkPath;
        bool rmPath;
        QFileInfo inode;
        QString errorString;
        ErrorType errorType;
    };
    QList<errorQueueItem> errorQueue;
    /// \brief already exists queue
    struct alreadyExistsQueueItem
    {
        TransferThread * transfer;	///< NULL if send by scan thread
        ScanFileOrFolder * scan;	///< NULL if send by transfer thread
        QFileInfo source;
        QFileInfo destination;
        bool isSame;
    };
    QList<alreadyExistsQueueItem> alreadyExistsQueue;
    quint64 size_for_speed;//because direct access to list thread into the main thread can't be do
    Ultracopier::CopyMode			mode;
    bool				forcedMode;

    bool doChecksum;
    bool checksumIgnoreIfImpossible;
    bool checksumOnlyOnError;
    bool osBuffer;
    bool osBufferLimited;
    bool checkDiskSpace;
    unsigned int osBufferLimit;
    QStringList includeStrings,includeOptions,excludeStrings,excludeOptions;
    QString firstRenamingRule;
    QString otherRenamingRule;

    //send action done timer
    QTimer timerActionDone;
    //send progression timer
    QTimer timerProgression;
    int putAtBottom;//to keep how many automatic put at bottom have been used
private slots:
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    void updateTheDebugInfo(QStringList,QStringList,int);
    #endif

    /************* External  call ********************/
    //dialog message
    /// \note Can be call without queue because all call will be serialized
    void fileAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFileSlot(QFileInfo fileInfo, QString errorString, TransferThread * thread, const ErrorType &errorType);
    /// \note Can be call without queue because all call will be serialized
    void folderAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,ScanFileOrFolder * thread);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFolderSlot(QFileInfo fileInfo, QString errorString, ScanFileOrFolder * thread, ErrorType errorType);
    //mkpath event
    void mkPathErrorOnFolderSlot(QFileInfo, QString, ErrorType errorType);

    //dialog message
    /// \note Can be call without queue because all call will be serialized
    void fileAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread,bool isCalledByShowOneNewDialog=false);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFile(QFileInfo fileInfo, QString errorString, TransferThread * thread, const ErrorType &errorType, bool isCalledByShowOneNewDialog=false);
    /// \note Can be call without queue because all call will be serialized
    void folderAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,ScanFileOrFolder * thread,bool isCalledByShowOneNewDialog=false);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFolder(QFileInfo fileInfo,QString errorString,ScanFileOrFolder * thread, ErrorType errorType,bool isCalledByShowOneNewDialog=false);
    //mkpath event
    void mkPathErrorOnFolder(QFileInfo, QString, const ErrorType &errorType, bool isCalledByShowOneNewDialog=false);

    //show one new dialog if needed
    void showOneNewDialog();
    void sendNewFilters();

    void doChecksum_toggled(bool);
    void checksumOnlyOnError_toggled(bool);
    void checksumIgnoreIfImpossible_toggled(bool);
    void osBuffer_toggled(bool);
    void osBufferLimited_toggled(bool);
    void osBufferLimit_editingFinished();
    void showFilterDialog();
    void sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
    void showRenamingRules();
    void get_realBytesTransfered(quint64 realBytesTransfered);
    void newActionInProgess(Ultracopier::EngineActionInProgress);
    void updatedBlockSize();
    void updateBufferCheckbox();
    void haveNeedPutAtBottom(bool needPutAtBottom, const QFileInfo &fileInfo, const QString &errorString, TransferThread *thread, const ErrorType &errorType);
    void missingDiskSpace(QList<Diskspace> list);
public:
    /** \brief to send the options panel
     * \return return false if have not the options
     * \param tempWidget the widget to generate on it the options */
    bool getOptionsEngine(QWidget * tempWidget);
    /** \brief to have interface widget to do modal dialog
     * \param interface to have the widget of the interface, useful for modal dialog */
    void setInterfacePointer(QWidget * interface);
    //return empty if multiple
    /** \brief compare the current sources of the copy, with the passed arguments
     * \param sources the sources list to compares with the current sources list
     * \return true if have same sources, else false (or empty) */
    bool haveSameSource(const QStringList &sources);
    /** \brief compare the current destination of the copy, with the passed arguments
     * \param destination the destination to compares with the current destination
     * \return true if have same destination, else false (or empty) */
    bool haveSameDestination(const QString &destination);
    //external soft like file browser have send copy/move list to do
    /** \brief send copy without destination, ask the destination
     * \param sources the sources list to copy
     * \return true if the copy have been accepted */
    bool newCopy(const QStringList &sources);
    /** \brief send copy with destination
     * \param sources the sources list to copy
     * \param destination the destination to copy
     * \return true if the copy have been accepted */
    bool newCopy(const QStringList &sources,const QString &destination);
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \return true if the move have been accepted */
    bool newMove(const QStringList &sources);
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \param destination the destination to move
     * \return true if the move have been accepted */
    bool newMove(const QStringList &sources,const QString &destination);
    /** \brief send the new transfer list
     * \param file the transfer list */
    void newTransferList(const QString &file);

    /** \brief to get byte read, use by Ultracopier for the speed calculation
     * real size transfered to right speed calculation */
    quint64 realByteTransfered();
    /** \brief support speed limitation */
    bool supportSpeedLimitation() const;

    /** \brief to set drives detected
     * specific to this copy engine */
    void setDrive(const QStringList &mountSysPoint,const QList<QStorageInfo::DriveType> &driveType);

    /** \brief to sync the transfer list
     * Used when the interface is changed, useful to minimize the memory size */
    void syncTransferList();

    void set_doChecksum(bool doChecksum);
    void set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible);
    void set_checksumOnlyOnError(bool checksumOnlyOnError);
    void set_osBuffer(bool osBuffer);
    void set_osBufferLimited(bool osBufferLimited);
    void set_osBufferLimit(unsigned int osBufferLimit);
    void set_setFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions);
    void setRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
    void setCheckDiskSpace(const bool &checkDiskSpace);
    void setDefaultDestinationFolder(const QString &defaultDestinationFolder);
    void defaultDestinationFolderBrowse();
public slots:
    //user ask ask to add folder (add it with interface ask source/destination)
    /** \brief add folder called on the interface
     * Used by manual adding */
    bool userAddFolder(const Ultracopier::CopyMode &mode);
    /** \brief add file called on the interface
     * Used by manual adding */
    bool userAddFile(const Ultracopier::CopyMode &mode);
    //action on the copy
    /// \brief put the transfer in pause
    void pause();
    /// \brief resume the transfer
    void resume();
    /** \brief skip one transfer entry
     * \param id id of the file to remove */
    void skip(const quint64 &id);
    /// \brief cancel all the transfer
    void cancel();
    //edit the transfer list
    /** \brief remove the selected item
     * \param ids ids is the id list of the selected items */
    void removeItems(const QList<int> &ids);
    /** \brief move on top of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnTop(const QList<int> &ids);
    /** \brief move up the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsUp(const QList<int> &ids);
    /** \brief move down the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsDown(const QList<int> &ids);
    /** \brief move on bottom of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnBottom(const QList<int> &ids);

    /** \brief give the forced mode, to export/import transfer list */
    void forceMode(const Ultracopier::CopyMode &mode);
    /// \brief export the transfer list into a file
    void exportTransferList();
    /// \brief import the transfer list into a file
    void importTransferList();

    /** \brief to set the speed limitation
     * -1 if not able, 0 if disabled */
    bool setSpeedLimitation(const qint64 &speedLimitation);

    // specific to this copy engine

    /// \brief set if the rights shoul be keep
    void setRightTransfer(const bool doRightTransfer);
    /// \brief set keep date
    void setKeepDate(const bool keepDate);
    /// \brief set block size in KB
    void setBlockSize(const int blockSize);

    void setParallelBuffer(int parallelBuffer);
    void setSequentialBuffer(int sequentialBuffer);
    void setParallelizeIfSmallerThan(int parallelizeIfSmallerThan);
    void setMoveTheWholeFolder(const bool &moveTheWholeFolder);
    void setFollowTheStrictOrder(const bool &followTheStrictOrder);
    void setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
    void setInodeThreads(const int &inodeThreads);
    void setRenameTheOriginalDestination(const bool &renameTheOriginalDestination);
    void inodeThreadsFinished();

    /// \brief set auto start
    void setAutoStart(const bool autoStart);
    /// \brief set if need check if the destination folder exists
    void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
    /// \brief reset widget
    void resetTempWidget();
    //autoconnect
    void setFolderCollision(int index);
    void setFolderError(int index);
    void setFileCollision(int index);
    void setFileError(int index);
    void setTransferAlgorithm(int index);
    /// \brief need retranslate the insterface
    void newLanguageLoaded();
private slots:
    void setComboBoxFolderCollision(FolderExistsAction action,bool changeComboBox=true);
    void setComboBoxFolderError(FileErrorAction action,bool changeComboBox=true);
    void warningTransferList(const QString &warning);
    void errorTransferList(const QString &error);
signals:
    //action on the copy
    void signal_pause();
    void signal_resume();
    void signal_skip(const quint64 &id);

    //edit the transfer list
    void signal_removeItems(const QList<int> &ids);
    void signal_moveItemsOnTop(const QList<int> &ids);
    void signal_moveItemsUp(const QList<int> &ids);
    void signal_moveItemsDown(const QList<int> &ids);
    void signal_moveItemsOnBottom(const QList<int> &ids);

    void signal_forceMode(const Ultracopier::CopyMode &mode);
    void signal_exportTransferList(const QString &fileName);
    void signal_importTransferList(const QString &fileName);

    //action
    void signal_setTransferAlgorithm(TransferAlgorithm transferAlgorithm);
    void signal_setCollisionAction(FileExistsAction alwaysDoThisActionForFileExists);
    void signal_setComboBoxFolderCollision(FolderExistsAction action);
    void signal_setFolderCollision(FolderExistsAction action);

    //internal cancel
    void tryCancel();
    void getNeedPutAtBottom(const QFileInfo &fileInfo,const QString &errorString,TransferThread * thread,const ErrorType &errorType);

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,QString fonction,QString text,QString file,int ligne);
    #endif

    //other signals
    void queryOneNewDialog();

    void send_setDrive(const QStringList &mountSysPoint,const QList<QStorageInfo::DriveType> &driveType);
    void send_speedLimitation(const qint64 &speedLimitation);
    void send_blockSize(const int &blockSize);
    void send_osBufferLimit(const unsigned int &osBufferLimit);
    void send_setFilters(const QList<Filters_rules> &include,const QList<Filters_rules> &exclude);
    void send_sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
    void send_parallelBuffer(const int &parallelBuffer);
    void send_sequentialBuffer(const int &sequentialBuffer);
    void send_parallelizeIfSmallerThan(const int &parallelizeIfSmallerThan);
    void send_followTheStrictOrder(const bool &followTheStrictOrder);
    void send_deletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
    void send_setInodeThreads(const int &inodeThreads);
    void send_moveTheWholeFolder(const bool &moveTheWholeFolder);
    void send_setRenameTheOriginalDestination(const bool &renameTheOriginalDestination);
};

#endif // COPY_ENGINE_H
