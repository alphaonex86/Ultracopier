/** \file copyEngine.h
\brief Define the copy engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QWidget>
#include <QObject>
#include <QList>
#include <vector>
#include <string>
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
    QWidget *               uiinterface;
    Filters *               filters;
    RenamingRules *			renamingRules;
    FacilityInterface *		facilityEngine;
    bool                    doRightTransfer;
    bool                    keepDate;
    bool                    followTheStrictOrder;
    bool                    deletePartiallyTransferredFiles;
    int                     inodeThreads;
    bool                    renameTheOriginalDestination;
    bool                    moveTheWholeFolder;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    bool                    rsync;
    #endif
    bool                    checkDestinationFolderExists;
    bool                    mkFullPath;
    FileExistsAction		alwaysDoThisActionForFileExists;
    FileErrorAction			alwaysDoThisActionForFileError;
    FileErrorAction			alwaysDoThisActionForFolderError;
    FolderExistsAction		alwaysDoThisActionForFolderExists;
    bool                    dialogIsOpen;
    volatile bool			stopIt;
    std::string                 defaultDestinationFolder;
    /// \brief error queue
    struct errorQueueItem
    {
        TransferThreadAsync * transfer;	///< NULL if send by scan thread
        ScanFileOrFolder * scan;	///< NULL if send by transfer thread
        bool mkPath;
        bool rmPath;
        std::string inode;
        std::string errorString;
        ErrorType errorType;
    };
    std::vector<errorQueueItem> errorQueue;
    /// \brief already exists queue
    struct alreadyExistsQueueItem
    {
        TransferThreadAsync * transfer;	///< NULL if send by scan thread
        ScanFileOrFolder * scan;	///< NULL if send by transfer thread
        std::string source;
        std::string destination;
        bool isSame;
    };
    std::vector<alreadyExistsQueueItem> alreadyExistsQueue;
    uint64_t size_for_speed;//because direct access to list thread into the main thread can't be do
    Ultracopier::CopyMode			mode;
    bool				forcedMode;

    bool checkDiskSpace;
    unsigned int osBufferLimit;
    std::vector<std::string> includeStrings,includeOptions,excludeStrings,excludeOptions;
    std::string firstRenamingRule;
    std::string otherRenamingRule;
    uint64_t errorPutAtEnd;

    //send action done timer
    QTimer timerActionDone;
    //send progression timer
    QTimer timerProgression;

    QTimer timerUpdateMount;
    int putAtBottom;//to keep how many automatic put at bottom have been used
private slots:
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    void updateTheDebugInfo(const std::vector<std::string> &newList, const std::vector<std::string> &newList2, const int &numberOfInodeOperation);
    #endif

    /************* External  call ********************/
    //dialog message
    /// \note Can be call without queue because all call will be serialized
    void fileAlreadyExistsSlot(std::string source, std::string destination, bool isSame, TransferThreadAsync * thread);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFileSlot(std::string fileInfo, std::string errorString, TransferThreadAsync * thread, const ErrorType &errorType);
    /// \note Can be call without queue because all call will be serialized
    void folderAlreadyExistsSlot(std::string source,std::string destination,bool isSame,ScanFileOrFolder * thread);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFolderSlot(std::string fileInfo, std::string errorString, ScanFileOrFolder * thread, ErrorType errorType);
    //mkpath event
    void mkPathErrorOnFolderSlot(std::string, std::string, ErrorType errorType);

    //dialog message
    /// \note Can be call without queue because all call will be serialized
    void fileAlreadyExists(std::string source,std::string destination,bool isSame,TransferThreadAsync * thread,bool isCalledByShowOneNewDialog=false);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFile(std::string fileInfo, std::string errorString, TransferThreadAsync * thread, const ErrorType &errorType, bool isCalledByShowOneNewDialog=false);
    /// \note Can be call without queue because all call will be serialized
    void folderAlreadyExists(std::string source,std::string destination,bool isSame,ScanFileOrFolder * thread,bool isCalledByShowOneNewDialog=false);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFolder(std::string fileInfo, std::string errorString, ScanFileOrFolder * thread, ErrorType errorType, bool isCalledByShowOneNewDialog=false);
    //mkpath event
    void mkPathErrorOnFolder(std::string, std::string, const ErrorType &errorType, bool isCalledByShowOneNewDialog=false);

    //show one new dialog if needed
    void showOneNewDialog();
    void sendNewFilters();

    void showFilterDialog();
    void sendNewRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule);
    void showRenamingRules();
    void get_realBytesTransfered(quint64 realBytesTransfered);
    void newActionInProgess(Ultracopier::EngineActionInProgress);
    void haveNeedPutAtBottom(bool needPutAtBottom, const std::string &fileInfo, const std::string &errorString, TransferThreadAsync *thread, const ErrorType &errorType);
    void missingDiskSpace(std::vector<Diskspace> list);
    void exportErrorIntoTransferList();
public:
    /** \brief to send the options panel
     * \return return false if have not the options
     * \param tempWidget the widget to generate on it the options */
    bool getOptionsEngine(QWidget * tempWidget);
    /** \brief to have interface widget to do modal dialog
     * \param interface to have the widget of the interface, useful for modal dialog */
    void setInterfacePointer(QWidget * uiinterface);
    //return empty if multiple
    /** \brief compare the current sources of the copy, with the passed arguments
     * \param sources the sources list to compares with the current sources list
     * \return true if have same sources, else false (or empty) */
    bool haveSameSource(const std::vector<std::string> &sources);
    /** \brief compare the current destination of the copy, with the passed arguments
     * \param destination the destination to compares with the current destination
     * \return true if have same destination, else false (or empty) */
    bool haveSameDestination(const std::string &destination);
    //external soft like file browser have send copy/move list to do
    /** \brief send copy without destination, ask the destination
     * \param sources the sources list to copy
     * \return true if the copy have been accepted */
    bool newCopy(const std::vector<std::string> &sources);
    /** \brief send copy with destination
     * \param sources the sources list to copy
     * \param destination the destination to copy
     * \return true if the copy have been accepted */
    bool newCopy(const std::vector<std::string> &sources,const std::string &destination);
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \return true if the move have been accepted */
    bool newMove(const std::vector<std::string> &sources);
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \param destination the destination to move
     * \return true if the move have been accepted */
    bool newMove(const std::vector<std::string> &sources,const std::string &destination);
    /** \brief send the new transfer list
     * \param file the transfer list */
    void newTransferList(const std::string &file);

    /** \brief to get byte read, use by Ultracopier for the speed calculation
     * real size transfered to right speed calculation */
    uint64_t realByteTransfered();
    /** \brief support speed limitation */
    bool supportSpeedLimitation() const;

    /** \brief to set drives detected
     * specific to this copy engine */

    /** \brief to sync the transfer list
     * Used when the interface is changed, useful to minimize the memory size */
    void syncTransferList();

    void set_setFilters(std::vector<std::string> includeStrings,std::vector<std::string> includeOptions,std::vector<std::string> excludeStrings,std::vector<std::string> excludeOptions);
    void setRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    void setRsync(const bool rsync);
    #endif
    void setCheckDiskSpace(const bool &checkDiskSpace);
    void setDefaultDestinationFolder(const std::string &defaultDestinationFolder);
    void defaultDestinationFolderBrowse();
    std::string askDestination();
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
    void skip(const uint64_t &id);
    /// \brief cancel all the transfer
    void cancel();
    //edit the transfer list
    /** \brief remove the selected item
     * \param ids ids is the id list of the selected items */
    void removeItems(const std::vector<uint64_t> &ids);
    /** \brief move on top of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnTop(const std::vector<uint64_t> &ids);
    /** \brief move up the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsUp(const std::vector<uint64_t> &ids);
    /** \brief move down the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsDown(const std::vector<uint64_t> &ids);
    /** \brief move on bottom of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnBottom(const std::vector<uint64_t> &ids);

    /** \brief give the forced mode, to export/import transfer list */
    void forceMode(const Ultracopier::CopyMode &mode);
    /// \brief export the transfer list into a file
    void exportTransferList();
    /// \brief import the transfer list into a file
    void importTransferList();

    /** \brief to set the speed limitation
     * -1 if not able, 0 if disabled */
    bool setSpeedLimitation(const int64_t &speedLimitation);

    // specific to this copy engine

    /// \brief set if the rights shoul be keep
    void setRightTransfer(const bool doRightTransfer);
    /// \brief set keep date
    void setKeepDate(const bool keepDate);

    void setMoveTheWholeFolder(const bool &moveTheWholeFolder);
    void setFollowTheStrictOrder(const bool &followTheStrictOrder);
    void setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
    void setInodeThreads(const int &inodeThreads);
    void setRenameTheOriginalDestination(const bool &renameTheOriginalDestination);
    void inodeThreadsFinished();

    /// \brief set if need check if the destination folder exists
    void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
    void setMkFullPath(const bool mkFullPath);
    /// \brief reset widget
    void resetTempWidget();
    //autoconnect
    void setFolderCollision(int index);
    void setFolderError(int index);
    void setFileCollision(int index);
    void setFileError(int index);
    /// \brief need retranslate the insterface
    void newLanguageLoaded();
private slots:
    void setComboBoxFolderCollision(FolderExistsAction action,bool changeComboBox=true);
    void setComboBoxFolderError(FileErrorAction action,bool changeComboBox=true);
    void warningTransferList(const std::string &warning);
    void errorTransferList(const std::string &error);
signals:
    //action on the copy
    void signal_pause() const;
    void signal_resume() const;
    void signal_skip(const uint64_t &id) const;

    //edit the transfer list
    void signal_removeItems(const std::vector<uint64_t> &ids) const;
    void signal_moveItemsOnTop(const std::vector<uint64_t> &ids) const;
    void signal_moveItemsUp(const std::vector<uint64_t> &ids) const;
    void signal_moveItemsDown(const std::vector<uint64_t> &ids) const;
    void signal_moveItemsOnBottom(const std::vector<uint64_t> &ids) const;

    void signal_forceMode(const Ultracopier::CopyMode &mode) const;
    void signal_exportTransferList(const std::string &fileName) const;
    void signal_importTransferList(const std::string &fileName) const;
    void signal_exportErrorIntoTransferList(const std::string &fileName) const;

    //action
    void signal_setCollisionAction(FileExistsAction alwaysDoThisActionForFileExists) const;
    void signal_setComboBoxFolderCollision(FolderExistsAction action) const;
    void signal_setFolderCollision(FolderExistsAction action) const;

    //internal cancel
    void tryCancel() const;
    void getNeedPutAtBottom(const std::string &fileInfo,const std::string &errorString,TransferThreadAsync * thread,const ErrorType &errorType) const;

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,std::string fonction,std::string text,std::string file,int ligne) const;
    #endif

    //other signals
    void queryOneNewDialog() const;

    void send_setFilters(const std::vector<Filters_rules> &include,const std::vector<Filters_rules> &exclude) const;
    void send_sendNewRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule) const;
    void send_followTheStrictOrder(const bool &followTheStrictOrder) const;
    void send_deletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles) const;
    void send_setInodeThreads(const int &inodeThreads) const;
    void send_moveTheWholeFolder(const bool &moveTheWholeFolder) const;
    void send_setRenameTheOriginalDestination(const bool &renameTheOriginalDestination) const;
};

#endif // COPY_ENGINE_H
