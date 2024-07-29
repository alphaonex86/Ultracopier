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
    bool                    os_spec_flags;
    bool                    native_copy;
    bool                    followTheStrictOrder;
    bool                    ignoreBlackList;
    bool                    deletePartiallyTransferredFiles;
    int                     inodeThreads;
    bool                    renameTheOriginalDestination;
    bool                    moveTheWholeFolder;
    bool                    autoStart;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    bool                    rsync;
    #endif
    bool                    checkDestinationFolderExists;
    bool                    mkFullPath;
    bool                    checksum;
    FileExistsAction		alwaysDoThisActionForFileExists;
    FileErrorAction			alwaysDoThisActionForFileError;
    FileErrorAction			alwaysDoThisActionForFolderError;
    FolderExistsAction		alwaysDoThisActionForFolderExists;
    volatile bool                    dialogIsOpen;
    volatile bool			stopIt;
    std::string                 defaultDestinationFolder;
    /// \brief error queue
    struct errorQueueItem
    {
        TransferThreadAsync * transfer;	///< NULL if send by scan thread
        ScanFileOrFolder * scan;	///< NULL if send by transfer thread
        bool mkPath;
        bool rmPath;
        INTERNALTYPEPATH inode;
        std::string errorString;
        ErrorType errorType;
    };
    std::vector<errorQueueItem> errorQueue;
    /// \brief already exists queue
    struct alreadyExistsQueueItem
    {
        TransferThreadAsync * transfer;	///< NULL if send by scan thread
        ScanFileOrFolder * scan;	///< NULL if send by transfer thread
        INTERNALTYPEPATH source;
        INTERNALTYPEPATH destination;
        bool isSame;
    };
    std::vector<alreadyExistsQueueItem> alreadyExistsQueue;
    uint64_t size_for_speed;//because direct access to list thread into the main thread can't be do
    Ultracopier::CopyMode			mode;
    bool				forcedMode;

    bool checkDiskSpace;
    bool buffer;
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
    friend class DebugDialog;
    #endif

    /************* External  call ********************/
    //dialog message
    /// \note Can be call without queue because all call will be serialized
    void fileAlreadyExistsSlot(INTERNALTYPEPATH source, INTERNALTYPEPATH destination, bool isSame, TransferThreadAsync * thread);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFileSlot(INTERNALTYPEPATH fileInfo, std::string errorString, TransferThreadAsync * thread, const ErrorType &errorType);
    /// \note Can be call without queue because all call will be serialized
    void folderAlreadyExistsSlot(INTERNALTYPEPATH source,INTERNALTYPEPATH destination,bool isSame,ScanFileOrFolder * thread);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFolderSlot(INTERNALTYPEPATH fileInfo, std::string errorString, ScanFileOrFolder * thread, ErrorType errorType);
    //mkpath event
    void mkPathErrorOnFolderSlot(INTERNALTYPEPATH, std::string, ErrorType errorType);

    //dialog message
    /// \note Can be call without queue because all call will be serialized
    void fileAlreadyExists(INTERNALTYPEPATH source, INTERNALTYPEPATH destination, bool isSame, TransferThreadAsync * thread);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFile(INTERNALTYPEPATH fileInfo, std::string errorString, TransferThreadAsync * thread, const ErrorType &errorType);
    /// \note Can be call without queue because all call will be serialized
    void folderAlreadyExists(INTERNALTYPEPATH source,INTERNALTYPEPATH destination,bool isSame,ScanFileOrFolder * thread);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFolder(INTERNALTYPEPATH fileInfo, std::string errorString, ScanFileOrFolder * thread, ErrorType errorType);
    //mkpath event
    void mkPathErrorOnFolder(INTERNALTYPEPATH, std::string, const ErrorType &errorType);

    //show one new dialog if needed
    void showOneNewDialog();
    void sendNewFilters();

    void showFilterDialog();
    void sendNewRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule);
    void showRenamingRules();
    void get_realBytesTransfered(quint64 realBytesTransfered);
    void newActionInProgess(Ultracopier::EngineActionInProgress);
    void haveNeedPutAtBottom(bool needPutAtBottom, const INTERNALTYPEPATH &fileInfo, const std::string &errorString, TransferThreadAsync *thread, const ErrorType &errorType);
    void missingDiskSpace(std::vector<Diskspace> list);
    void exportErrorIntoTransferList() override;
public:
    /** \brief to send the options panel
     * \return return false if have not the options
     * \param tempWidget the widget to generate on it the options */
    bool getOptionsEngine(QWidget * tempWidget) override;
    /** \brief to have interface widget to do modal dialog
     * \param interface to have the widget of the interface, useful for modal dialog */
    void setInterfacePointer(QWidget * uiinterface) override;
    //return empty if multiple
    /** \brief compare the current sources of the copy, with the passed arguments
     * \param sources the sources list to compares with the current sources list
     * \return true if have same sources, else false (or empty) */
    bool haveSameSource(const std::vector<std::string> &sources) override;
    /** \brief compare the current destination of the copy, with the passed arguments
     * \param destination the destination to compares with the current destination
     * \return true if have same destination, else false (or empty) */
    bool haveSameDestination(const std::string &destination) override;
    //external soft like file browser have send copy/move list to do
    /** \brief send copy without destination, ask the destination
     * \param sources the sources list to copy
     * \return true if the copy have been accepted */
    bool newCopy(const std::vector<std::string> &sources) override;
    /** \brief send copy with destination
     * \param sources the sources list to copy
     * \param destination the destination to copy
     * \return true if the copy have been accepted */
    bool newCopy(const std::vector<std::string> &sources,const std::string &destination) override;
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \return true if the move have been accepted */
    bool newMove(const std::vector<std::string> &sources) override;
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \param destination the destination to move
     * \return true if the move have been accepted */
    bool newMove(const std::vector<std::string> &sources,const std::string &destination) override;
    /** \brief send the new transfer list
     * \param file the transfer list */
    void newTransferList(const std::string &file) override;

    /** \brief to get byte read, use by Ultracopier for the speed calculation
     * real size transfered to right speed calculation */
    uint64_t realByteTransfered() override;
    /** \brief support speed limitation */
    bool supportSpeedLimitation() const override;

    /** \brief to set drives detected
     * specific to this copy engine */

    /** \brief to sync the transfer list
     * Used when the interface is changed, useful to minimize the memory size */
    void syncTransferList() override;

    void set_setFilters(std::vector<std::string> includeStrings,std::vector<std::string> includeOptions,std::vector<std::string> excludeStrings,std::vector<std::string> excludeOptions);
    void setRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    void setRsync(const bool rsync);
    #endif
    void setCheckDiskSpace(const bool &checkDiskSpace);
    void setDefaultDestinationFolder(const std::string &defaultDestinationFolder);
    void defaultDestinationFolderBrowse();
    std::string askDestination();
    static std::string stringimplode(const std::vector<std::string>& elems, const std::string &delim);
    void setBuffer(const bool &buffer);
public slots:
    //user ask ask to add folder (add it with interface ask source/destination)
    /** \brief add folder called on the interface
     * Used by manual adding */
    bool userAddFolder(const Ultracopier::CopyMode &mode) override;
    /** \brief add file called on the interface
     * Used by manual adding */
    bool userAddFile(const Ultracopier::CopyMode &mode) override;
    //action on the copy
    /// \brief put the transfer in pause
    void pause() override;
    /// \brief resume the transfer
    void resume() override;
    /** \brief skip one transfer entry
     * \param id id of the file to remove */
    void skip(const uint64_t &id) override;
    /// \brief cancel all the transfer
    void cancel() override;
    //edit the transfer list
    /** \brief remove the selected item
     * \param ids ids is the id list of the selected items */
    void removeItems(const std::vector<uint64_t> &ids) override;
    /** \brief move on top of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnTop(const std::vector<uint64_t> &ids) override;
    /** \brief move up the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsUp(const std::vector<uint64_t> &ids) override;
    /** \brief move down the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsDown(const std::vector<uint64_t> &ids) override;
    /** \brief move on bottom of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnBottom(const std::vector<uint64_t> &ids) override;

    /** \brief give the forced mode, to export/import transfer list */
    void forceMode(const Ultracopier::CopyMode &mode) override;
    /// \brief export the transfer list into a file
    void exportTransferList() override;
    /// \brief import the transfer list into a file
    void importTransferList() override;

    /** \brief to set the speed limitation
     * -1 if not able, 0 if disabled */
    bool setSpeedLimitation(const int64_t &speedLimitation) override;

    // specific to this copy engine

    /// \brief set if the rights shoul be keep
    void setRightTransfer(const bool doRightTransfer);
    /// \brief set keep date
    void setKeepDate(const bool keepDate);
    void setOsSpecFlags(bool os_spec_flags);
    void setNativeCopy(bool native_copy);

    void setMoveTheWholeFolder(const bool &moveTheWholeFolder);
    void setFollowTheStrictOrder(const bool &followTheStrictOrder);
    void setignoreBlackList(const bool &ignoreBlackList);
    void setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
    void setInodeThreads(const int &inodeThreads);
    void setRenameTheOriginalDestination(const bool &renameTheOriginalDestination);
    void inodeThreadsFinished();

    /// \brief set auto start
    void setAutoStart(const bool autoStart);
    /// \brief set if need check if the destination folder exists
    void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
    void setMkFullPath(const bool mkFullPath);
    void setChecksum(const bool checksum);
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
    void signal_setSpeedLimitation(const int64_t &speedLimitation);

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
    //void signal_setComboBoxFolderCollision(FolderExistsAction action) const;// -> duplicate with signal_setFolderCollision ?
    void signal_setFolderCollision(FolderExistsAction action) const;

    //internal cancel
    void tryCancel() const;
    void getNeedPutAtBottom(const INTERNALTYPEPATH &fileInfo,const std::string &errorString,TransferThreadAsync * thread,const ErrorType &errorType) const;

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,std::string fonction,std::string text,std::string file,int ligne) const;
    #endif

    //other signals
    void queryOneNewDialog() const;

    void send_setFilters(const std::vector<Filters_rules> &include,const std::vector<Filters_rules> &exclude) const;
    void send_sendNewRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule) const;
    void send_followTheStrictOrder(const bool &followTheStrictOrder) const;
    void send_ignoreBlackList(const bool &ignoreBlackList) const;
    void send_deletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles) const;
    void send_setInodeThreads(const int &inodeThreads) const;
    void send_moveTheWholeFolder(const bool &moveTheWholeFolder) const;
    void send_setRenameTheOriginalDestination(const bool &renameTheOriginalDestination) const;
};

#endif // COPY_ENGINE_H
