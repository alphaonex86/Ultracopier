/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86 */

#include <QFileDialog>
#include <QMessageBox>
#include <cmath>

#include "CopyEngine.h"
#include "FolderExistsDialog.h"
#include "../../../interface/PluginInterface_CopyEngine.h"

// The cmath header from MSVC does not contain round()
#if (defined(_WIN64) || defined(_WIN32)) && defined(_MSC_VER)
inline double round(double d) {
    return floor( d + 0.5 );
}
#endif

CopyEngine::CopyEngine(FacilityInterface * facilityEngine) :
    listThread(NULL),
    tempWidget(NULL),
    ui(new Ui::copyEngineOptions()),
    uiIsInstalled(false),
    uiinterface(NULL),
    filters(NULL),
    renamingRules(NULL),
    facilityEngine(NULL),
    doRightTransfer(false),
    keepDate(false),
    followTheStrictOrder(false),
    deletePartiallyTransferredFiles(false),
    inodeThreads(0),
    renameTheOriginalDestination(false),
    moveTheWholeFolder(false),
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    rsync(false),
    #endif
    checkDestinationFolderExists(false),
    mkFullPath(false),
    alwaysDoThisActionForFileExists(FileExistsAction::FileExists_NotSet),
    alwaysDoThisActionForFileError(FileErrorAction::FileError_NotSet),
    alwaysDoThisActionForFolderError(FileErrorAction::FileError_NotSet),
    alwaysDoThisActionForFolderExists(FolderExistsAction::FolderExists_NotSet),
    dialogIsOpen(false),
    stopIt(false),
    size_for_speed(0),
    mode(Ultracopier::CopyMode::Copy),
    forcedMode(false),
    checkDiskSpace(false),
    osBufferLimit(0),
    errorPutAtEnd(0),
    putAtBottom(0)
{
    listThread=new ListThread(facilityEngine);
    this->facilityEngine            = facilityEngine;
    filters                         = NULL;
    renamingRules                   = NULL;

    uiinterface                       = NULL;
    tempWidget                      = NULL;
    uiIsInstalled                   = false;
    dialogIsOpen                    = false;
    renameTheOriginalDestination    = false;
    alwaysDoThisActionForFileExists	= FileExists_NotSet;
    alwaysDoThisActionForFileError	= FileError_NotSet;
    checkDestinationFolderExists	= false;
    stopIt                          = false;
    size_for_speed                  = 0;
    putAtBottom                     = 0;
    forcedMode                      = false;
    followTheStrictOrder            = false;
    deletePartiallyTransferredFiles = true;
    inodeThreads                    = 16;
    moveTheWholeFolder              = true;

    //implement the SingleShot in this class
    //timerActionDone.setSingleShot(true);
    timerActionDone.setInterval(ULTRACOPIER_PLUGIN_TIME_UPDATE_TRASNFER_LIST);
    //timerProgression.setSingleShot(true);
    timerProgression.setInterval(ULTRACOPIER_PLUGIN_TIME_UPDATE_PROGRESSION);

    timerUpdateMount.setInterval(ULTRACOPIER_PLUGIN_TIME_UPDATE_MOUNT_MS);
}

CopyEngine::~CopyEngine()
{
    /*if(filters!=NULL)
        delete filters;
    if(renamingRules!=NULL)
        delete renamingRules;
        destroyed by the widget parent, here the interface
    */
    stopIt=true;
    delete listThread;
    delete ui;
}

void CopyEngine::connectTheSignalsSlots()
{
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    debugDialogWindow.show();
    debugDialogWindow.copyEngine=this;
    #endif
    if(!connect(listThread,&ListThread::actionInProgess,	this,&CopyEngine::actionInProgess,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect actionInProgess()");
    if(!connect(listThread,&ListThread::actionInProgess,	this,&CopyEngine::newActionInProgess,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect actionInProgess() to slot");
    if(!connect(listThread,&ListThread::newFolderListing,			this,&CopyEngine::newFolderListing,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newFolderListing()");
    if(!connect(listThread,&ListThread::error,	this,&CopyEngine::error,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect error()");
    if(!connect(listThread,&ListThread::rmPath,				this,&CopyEngine::rmPath,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect rmPath()");
    if(!connect(listThread,&ListThread::mkPath,				this,&CopyEngine::mkPath,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect mkPath()");
    if(!connect(listThread,&ListThread::newActionOnList,	this,&CopyEngine::newActionOnList,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newActionOnList()");
    if(!connect(listThread,&ListThread::doneTime,	this,&CopyEngine::doneTime,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect doneTime()");
    if(!connect(listThread,&ListThread::pushFileProgression,		this,&CopyEngine::pushFileProgression,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect pushFileProgression()");
    if(!connect(listThread,&ListThread::pushGeneralProgression,		this,&CopyEngine::pushGeneralProgression,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect pushGeneralProgression()");
    if(!connect(listThread,&ListThread::syncReady,						this,&CopyEngine::syncReady,					Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect syncReady()");
    if(!connect(listThread,&ListThread::canBeDeleted,						this,&CopyEngine::canBeDeleted,					Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect canBeDeleted()");
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    if(!connect(listThread,&ListThread::debugInformation,			this,&CopyEngine::debugInformation,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect debugInformation()");
    #endif

    if(!connect(listThread,&ListThread::send_fileAlreadyExists,		this,&CopyEngine::fileAlreadyExistsSlot,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_fileAlreadyExists()");
    if(!connect(listThread,&ListThread::send_errorOnFile,			this,&CopyEngine::errorOnFileSlot,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_errorOnFile()");
    if(!connect(listThread,&ListThread::send_folderAlreadyExists,	this,&CopyEngine::folderAlreadyExistsSlot,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_folderAlreadyExists()");
    if(!connect(listThread,&ListThread::send_errorOnFolder,			this,&CopyEngine::errorOnFolderSlot,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_errorOnFolder()");
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    if(!connect(listThread,&ListThread::updateTheDebugInfo,				this,&CopyEngine::updateTheDebugInfo,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect updateTheDebugInfo()");
    #endif
    if(!connect(listThread,&ListThread::errorTransferList,							this,&CopyEngine::errorTransferList,						Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect errorTransferList()");
    if(!connect(listThread,&ListThread::warningTransferList,						this,&CopyEngine::warningTransferList,					Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect warningTransferList()");
    if(!connect(listThread,&ListThread::mkPathErrorOnFolder,					this,&CopyEngine::mkPathErrorOnFolderSlot,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect mkPathErrorOnFolder()");
    if(!connect(listThread,&ListThread::send_realBytesTransfered,					this,&CopyEngine::get_realBytesTransfered,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_realBytesTransfered()");

    if(!connect(this,&CopyEngine::tryCancel,						listThread,&ListThread::cancel,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect tryCancel()");
    if(!connect(this,&CopyEngine::getNeedPutAtBottom,						listThread,&ListThread::getNeedPutAtBottom,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect getNeedPutAtBottom()");
    if(!connect(listThread,&ListThread::haveNeedPutAtBottom,		this,&CopyEngine::haveNeedPutAtBottom,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect haveNeedPutAtBottom()");


    if(!connect(this,&CopyEngine::signal_pause,						listThread,&ListThread::pause,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_pause()");
    if(!connect(this,&CopyEngine::signal_exportErrorIntoTransferList,listThread,&ListThread::exportErrorIntoTransferList,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_exportErrorIntoTransferList()");
    if(!connect(this,&CopyEngine::signal_resume,						listThread,&ListThread::resume,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_resume()");
    if(!connect(this,&CopyEngine::signal_skip,					listThread,&ListThread::skip,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_skip()");
    if(!connect(this,&CopyEngine::signal_setCollisionAction,		listThread,&ListThread::setAlwaysFileExistsAction,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_setCollisionAction()");
    if(!connect(this,&CopyEngine::signal_setFolderCollision,		listThread,&ListThread::setFolderCollision,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_setFolderCollision()");
    if(!connect(this,&CopyEngine::signal_removeItems,				listThread,&ListThread::removeItems,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_removeItems()");
    if(!connect(this,&CopyEngine::signal_moveItemsOnTop,				listThread,&ListThread::moveItemsOnTop,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsOnTop()");
    if(!connect(this,&CopyEngine::signal_moveItemsUp,				listThread,&ListThread::moveItemsUp,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsUp()");
    if(!connect(this,&CopyEngine::signal_moveItemsDown,				listThread,&ListThread::moveItemsDown,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsDown()");
    if(!connect(this,&CopyEngine::signal_moveItemsOnBottom,			listThread,&ListThread::moveItemsOnBottom,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsOnBottom()");
    if(!connect(this,&CopyEngine::signal_exportTransferList,			listThread,&ListThread::exportTransferList,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_exportTransferList()");
    if(!connect(this,&CopyEngine::signal_importTransferList,			listThread,&ListThread::importTransferList,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_importTransferList()");
    if(!connect(this,&CopyEngine::signal_forceMode,				listThread,&ListThread::forceMode,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_forceMode()");
    if(!connect(this,&CopyEngine::send_moveTheWholeFolder,					listThread,&ListThread::setMoveTheWholeFolder,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect moveTheWholeFolder()");
    if(!connect(this,&CopyEngine::send_deletePartiallyTransferredFiles,					listThread,&ListThread::setDeletePartiallyTransferredFiles,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect deletePartiallyTransferredFiles()");
    if(!connect(this,&CopyEngine::send_setRenameTheOriginalDestination,					listThread,&ListThread::setRenameTheOriginalDestination,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect setRenameTheOriginalDestination()");
    if(!connect(this,&CopyEngine::send_setInodeThreads,					listThread,&ListThread::setInodeThreads,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect setInodeThreads()");
    if(!connect(this,&CopyEngine::send_followTheStrictOrder,					listThread,&ListThread::setFollowTheStrictOrder,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect followTheStrictOrder()");
    if(!connect(this,&CopyEngine::send_setFilters,listThread,&ListThread::set_setFilters,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_setFilters()");
    if(!connect(this,&CopyEngine::send_sendNewRenamingRules,listThread,&ListThread::set_sendNewRenamingRules,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_sendNewRenamingRules()");
    if(!connect(&timerActionDone,&QTimer::timeout,							listThread,&ListThread::sendActionDone))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect timerActionDone");
    if(!connect(&timerProgression,&QTimer::timeout,							listThread,&ListThread::sendProgression))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect timerProgression");
    if(!connect(listThread,&ListThread::missingDiskSpace,					this,&CopyEngine::missingDiskSpace,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect timerProgression");

    if(!connect(this,&CopyEngine::queryOneNewDialog,this,&CopyEngine::showOneNewDialog,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect queryOneNewDialog()");
    if(!connect(listThread,&ListThread::errorToRetry,this,&CopyEngine::errorToRetry,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect errorToRetry()");

    if(!connect(&timerUpdateMount,&QTimer::timeout,listThread,&ListThread::set_updateMount,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect set_updateMount()");
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
void CopyEngine::updateTheDebugInfo(const std::vector<std::string> &newList, const std::vector<std::string> &newList2, const int &numberOfInodeOperation)
{
    debugDialogWindow.setTransferThreadList(newList);
    debugDialogWindow.setTransferList(newList2);
    debugDialogWindow.setInodeUsage(numberOfInodeOperation);
}
#endif

//to send the options panel
bool CopyEngine::getOptionsEngine(QWidget * tempWidget)
{
    this->tempWidget=tempWidget;
    ui->setupUi(tempWidget);
    ui->toolBox->setCurrentIndex(0);
    connect(tempWidget,		&QWidget::destroyed,		this,			&CopyEngine::resetTempWidget);

    //here else, the default settings can't be loaded
    uiIsInstalled=true;

    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    setRsync(rsync);
    #else
    ui->label_rsync->setVisible(false);
    ui->rsync->setVisible(false);
    #endif
    setCheckDestinationFolderExists(checkDestinationFolderExists);
    setMkFullPath(mkFullPath);
    setRightTransfer(doRightTransfer);
    setKeepDate(keepDate);
    setFollowTheStrictOrder(followTheStrictOrder);
    setDeletePartiallyTransferredFiles(deletePartiallyTransferredFiles);
    setInodeThreads(inodeThreads);
    setRenameTheOriginalDestination(renameTheOriginalDestination);
    setMoveTheWholeFolder(moveTheWholeFolder);
    setCheckDiskSpace(checkDiskSpace);
    setDefaultDestinationFolder(defaultDestinationFolder);

    switch(alwaysDoThisActionForFileExists)
    {
        case FileExists_NotSet:
            ui->comboBoxFileCollision->setCurrentIndex(0);
        break;
        case FileExists_Skip:
            ui->comboBoxFileCollision->setCurrentIndex(1);
        break;
        case FileExists_Overwrite:
            ui->comboBoxFileCollision->setCurrentIndex(2);
        break;
        case FileExists_OverwriteIfNotSame:
            ui->comboBoxFileCollision->setCurrentIndex(3);
        break;
        case FileExists_OverwriteIfNewer:
            ui->comboBoxFileCollision->setCurrentIndex(4);
        break;
        case FileExists_OverwriteIfOlder:
            ui->comboBoxFileCollision->setCurrentIndex(5);
        break;
        case FileExists_Rename:
            ui->comboBoxFileCollision->setCurrentIndex(6);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
            ui->comboBoxFileCollision->setCurrentIndex(0);
        break;
    }
    switch(alwaysDoThisActionForFileError)
    {
        case FileError_NotSet:
            ui->comboBoxFileError->setCurrentIndex(0);
        break;
        case FileError_Skip:
            ui->comboBoxFileError->setCurrentIndex(1);
        break;
        case FileError_PutToEndOfTheList:
            ui->comboBoxFileError->setCurrentIndex(2);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
            ui->comboBoxFileError->setCurrentIndex(0);
        break;
    }
    switch(alwaysDoThisActionForFolderExists)
    {
        case FolderExists_NotSet:
            ui->comboBoxFolderCollision->setCurrentIndex(0);
        break;
        case FolderExists_Merge:
            ui->comboBoxFolderCollision->setCurrentIndex(1);
        break;
        case FolderExists_Skip:
            ui->comboBoxFolderCollision->setCurrentIndex(2);
        break;
        case FolderExists_Rename:
            ui->comboBoxFolderCollision->setCurrentIndex(3);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
            ui->comboBoxFolderCollision->setCurrentIndex(0);
        break;
    }
    switch(alwaysDoThisActionForFolderError)
    {
        case FileError_NotSet:
            ui->comboBoxFolderError->setCurrentIndex(0);
        break;
        case FileError_Skip:
            ui->comboBoxFolderError->setCurrentIndex(1);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored: "+std::to_string(alwaysDoThisActionForFolderError));
            ui->comboBoxFolderError->setCurrentIndex(0);
        break;
    }
    return true;
}

//to have interface widget to do modal dialog
void CopyEngine::setInterfacePointer(QWidget * uiinterface)
{
    this->uiinterface=uiinterface;
    filters=new Filters(tempWidget);
    renamingRules=new RenamingRules(tempWidget);

    if(uiIsInstalled)
    {
        connect(ui->doRightTransfer,                    &QCheckBox::toggled,		this,&CopyEngine::setRightTransfer);
        connect(ui->keepDate,                           &QCheckBox::toggled,		this,&CopyEngine::setKeepDate);
        connect(ui->moveTheWholeFolder,                 &QCheckBox::toggled,		this,&CopyEngine::setMoveTheWholeFolder);
        connect(ui->deletePartiallyTransferredFiles,    &QCheckBox::toggled,		this,&CopyEngine::setDeletePartiallyTransferredFiles);
        connect(ui->followTheStrictOrder,               &QCheckBox::toggled,        this,&CopyEngine::setFollowTheStrictOrder);
        connect(ui->checkBoxDestinationFolderExists,	&QCheckBox::toggled,        this,&CopyEngine::setCheckDestinationFolderExists);
        connect(ui->mkpath,                             &QCheckBox::toggled,        this,&CopyEngine::setMkFullPath);
        #ifdef ULTRACOPIER_PLUGIN_RSYNC
        connect(ui->rsync,                              &QCheckBox::toggled,        this,&CopyEngine::setRsync);
        #endif
        connect(ui->renameTheOriginalDestination,       &QCheckBox::toggled,        this,&CopyEngine::setRenameTheOriginalDestination);
        connect(filters,                                &Filters::haveNewFilters,   this,&CopyEngine::sendNewFilters);
        connect(ui->filters,                            &QPushButton::clicked,      this,&CopyEngine::showFilterDialog);
        connect(ui->inodeThreads,                       &QSpinBox::editingFinished,	this,&CopyEngine::inodeThreadsFinished);
        connect(ui->inodeThreads,                       static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&CopyEngine::setInodeThreads);
        connect(ui->defaultDestinationFolderBrowse,     &QPushButton::clicked,      this,&CopyEngine::defaultDestinationFolderBrowse);

        connect(ui->comboBoxFolderError,        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngine::setFolderError);
        connect(ui->comboBoxFolderCollision,	static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngine::setFolderCollision);
        connect(ui->comboBoxFileError,          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngine::setFileError);
        connect(ui->comboBoxFileCollision,      static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngine::setFileCollision);

        if(!connect(renamingRules,&RenamingRules::sendNewRenamingRules,this,&CopyEngine::sendNewRenamingRules))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect sendNewRenamingRules()");
        if(!connect(ui->renamingRules,&QPushButton::clicked,this,&CopyEngine::showRenamingRules))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect renamingRules.clicked()");
    }

    filters->setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
    set_setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);

    renamingRules->setRenamingRules(firstRenamingRule,otherRenamingRule);
    emit send_sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

bool CopyEngine::haveSameSource(const std::vector<std::string> &sources)
{
    return listThread->haveSameSource(sources);
}

bool CopyEngine::haveSameDestination(const std::string &destination)
{
    return listThread->haveSameDestination(destination);
}

std::string CopyEngine::stringimplode(const std::vector<std::string>& elems, const std::string &delim)
{
    std::string newString;
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.cend(); ++ii)
    {
        newString += (*ii);
        if ( ii + 1 != elems.end() ) {
            newString += delim;
        }
    }
    return newString;
}

bool CopyEngine::newCopy(const std::vector<std::string> &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,stringimplode(sources,", "));
    if(forcedMode && mode!=Ultracopier::Copy)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to move, you can't copy with it");
        QMessageBox::critical(NULL,QString::fromStdString(facilityEngine->translateText("Internal error")),tr("The engine is forced to move, you can't copy with it"));
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    std::string destination;
    if(!defaultDestinationFolder.empty() && QDir(QString::fromStdString(defaultDestinationFolder)).exists())
        destination = defaultDestinationFolder;
    else
        destination = askDestination();
    if(destination.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Canceled by the user");
        return false;
    }
    return listThread->newCopy(sources,destination);
}

bool CopyEngine::newCopy(const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,stringimplode(sources,", ")+" "+destination);
    if(forcedMode && mode!=Ultracopier::Copy)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to move, you can't copy with it");
        QMessageBox::critical(NULL,QString::fromStdString(facilityEngine->translateText("Internal error")),tr("The engine is forced to move, you can't copy with it"));
        return false;
    }
    return listThread->newCopy(sources,destination);
}

bool CopyEngine::newMove(const std::vector<std::string> &sources)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,stringimplode(sources,", "));
    if(forcedMode && mode!=Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to copy, you can't move with it");
        QMessageBox::critical(NULL,QString::fromStdString(facilityEngine->translateText("Internal error")),tr("The engine is forced to copy, you can't move with it"));
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    std::string destination;
    if(!ui->defaultDestinationFolder->text().isEmpty() && QDir(ui->defaultDestinationFolder->text()).exists())
        destination = ui->defaultDestinationFolder->text().toStdString();
    else
        destination = askDestination();
    if(destination.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Canceled by the user");
        return false;
    }
    return listThread->newMove(sources,destination);
}

bool CopyEngine::newMove(const std::vector<std::string> &sources,const std::string &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,stringimplode(sources,", ")+" "+destination);
    if(forcedMode && mode!=Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to copy, you can't move with it");
        QMessageBox::critical(NULL,QString::fromStdString(facilityEngine->translateText("Internal error")),tr("The engine is forced to copy, you can't move with it"));
        return false;
    }
    return listThread->newMove(sources,destination);
}

void CopyEngine::defaultDestinationFolderBrowse()
{
    std::string destination = askDestination();
    if(destination.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Canceled by the user");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(uiIsInstalled)
        ui->defaultDestinationFolder->setText(QString::fromStdString(destination));
}

std::string CopyEngine::askDestination()
{
    std::string destination = listThread->getUniqueDestinationFolder();
    if(!destination.empty())
    {
        QMessageBox::StandardButton button=QMessageBox::question(uiinterface,tr("Destination"),tr("Use the actual destination \"%1\"?")
                                                                 .arg(QString::fromStdString(destination)),
                                                                 QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes);
        if(button==QMessageBox::Yes)
            return destination;
    }
    destination=QFileDialog::getExistingDirectory(uiinterface,QString::fromStdString(facilityEngine->translateText("Select destination directory")),QStringLiteral(""),QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdString();
    return destination;
}

void CopyEngine::newTransferList(const std::string &file)
{
    emit signal_importTransferList(file);
}

//because direct access to list thread into the main thread can't be do
uint64_t CopyEngine::realByteTransfered()
{
    return size_for_speed;
}

//speed limitation
bool CopyEngine::supportSpeedLimitation() const
{
    return false;
}

/** \brief to sync the transfer list
 * Used when the interface is changed, useful to minimize the memory size */
void CopyEngine::syncTransferList()
{
    listThread->syncTransferList();
}

void CopyEngine::set_setFilters(std::vector<std::string> includeStrings,std::vector<std::string> includeOptions,std::vector<std::string> excludeStrings,std::vector<std::string> excludeOptions)
{
    if(filters!=NULL)
    {
        filters->setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
        emit send_setFilters(filters->getInclude(),filters->getExclude());
    }
    this->includeStrings=includeStrings;
    this->includeOptions=includeOptions;
    this->excludeStrings=excludeStrings;
    this->excludeOptions=excludeOptions;
}

void CopyEngine::setRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule)
{
    sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

bool CopyEngine::userAddFolder(const Ultracopier::CopyMode &mode)
{
    std::string source = QFileDialog::getExistingDirectory(uiinterface,QString::fromStdString(facilityEngine->translateText("Select source directory")),
                                                           QStringLiteral(""),
                                                           QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdString();
    if(source.empty() || source=="")
        return false;
    std::vector<std::string> sources;
    sources.push_back(source);
    if(mode==Ultracopier::Copy)
        return newCopy(sources);
    else
        return newMove(sources);
}

bool CopyEngine::userAddFile(const Ultracopier::CopyMode &mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QStringList sources = QFileDialog::getOpenFileNames(
        uiinterface,
        QString::fromStdString(facilityEngine->translateText("Select one or more files to open")),
        QStringLiteral(""),
        QString::fromStdString(facilityEngine->translateText("All files"))+QStringLiteral(" (*)"));

    std::vector<std::string> sourcesstd;
    unsigned int index=0;
    while(index<(unsigned int)sources.size())
    {
        sourcesstd.push_back(sources.at(index).toStdString());
        index++;
    }

    if(sourcesstd.empty())
        return false;
    if(mode==Ultracopier::Copy)
        return newCopy(sourcesstd);
    else
        return newMove(sourcesstd);
}

void CopyEngine::pause()
{
    emit signal_pause();
}

void CopyEngine::resume()
{
    emit signal_resume();
}

void CopyEngine::skip(const uint64_t &id)
{
    emit signal_skip(id);
}

void CopyEngine::cancel()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    stopIt=true;
    timerProgression.stop();
    timerActionDone.stop();
    emit tryCancel();
}

void CopyEngine::removeItems(const std::vector<uint64_t> &ids)
{
    emit signal_removeItems(ids);
}

void CopyEngine::moveItemsOnTop(const std::vector<uint64_t> &ids)
{
    emit signal_moveItemsOnTop(ids);
}

void CopyEngine::moveItemsUp(const std::vector<uint64_t> &ids)
{
    emit signal_moveItemsUp(ids);
}

void CopyEngine::moveItemsDown(const std::vector<uint64_t> &ids)
{
    emit signal_moveItemsDown(ids);
}

void CopyEngine::moveItemsOnBottom(const std::vector<uint64_t> &ids)
{
    emit signal_moveItemsOnBottom(ids);
}

/** \brief give the forced mode, to export/import transfer list */
void CopyEngine::forceMode(const Ultracopier::CopyMode &mode)
{
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    if(mode==Ultracopier::Move)
    {
         listThread->setRsync(false);
         rsync=false;
    }
    if(uiIsInstalled)
         ui->rsync->setEnabled(mode==Ultracopier::Copy);
    #endif
    if(forcedMode)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Mode forced previously");
        QMessageBox::critical(NULL,QString::fromStdString(facilityEngine->translateText("Internal error")),tr("The mode has been forced previously. This is an internal error, please report it"));
        return;
    }
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    if(mode==Ultracopier::Move)
        rsync=false;
    #endif
    if(mode==Ultracopier::Copy)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Force mode to copy");
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Force mode to move");
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    if(uiIsInstalled)
         ui->rsync->setEnabled(mode==Ultracopier::Copy);
    #endif
    this->mode=mode;
    forcedMode=true;
    emit signal_forceMode(mode);
}

void CopyEngine::exportTransferList()
{
    std::string fileName = QFileDialog::getSaveFileName(uiinterface,QString::fromStdString(facilityEngine->translateText("Save transfer list")),QStringLiteral("transfer-list.lst"),QString::fromStdString(facilityEngine->translateText("Transfer list"))+QStringLiteral(" (*.lst)")).toStdString();
    if(fileName.empty())
        return;
    emit signal_exportTransferList(fileName);
}

void CopyEngine::importTransferList()
{
    std::string fileName = QFileDialog::getOpenFileName(uiinterface,QString::fromStdString(facilityEngine->translateText("Open transfer list")),QStringLiteral("transfer-list.lst"),QString::fromStdString(facilityEngine->translateText("Transfer list"))+QStringLiteral(" (*.lst)")).toStdString();
    if(fileName.empty())
        return;
    emit signal_importTransferList(fileName);
}

void CopyEngine::warningTransferList(const std::string &warning)
{
    QMessageBox::warning(uiinterface,QString::fromStdString(facilityEngine->translateText("Error")),QString::fromStdString(warning));
}

void CopyEngine::errorTransferList(const std::string &error)
{
    QMessageBox::critical(uiinterface,QString::fromStdString(facilityEngine->translateText("Error")),QString::fromStdString(error));
}

bool CopyEngine::setSpeedLimitation(const int64_t &speedLimitation)
{
    (void)speedLimitation;
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"maxSpeed: "+std::to_string(speedLimitation));
    return false;
}

void CopyEngine::setFileCollision(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action index: "+std::to_string(index));
    if(uiIsInstalled)
        if(index!=ui->comboBoxFileCollision->currentIndex())
            ui->comboBoxFileCollision->setCurrentIndex(index);
    switch(index)
    {
        case 0:
            alwaysDoThisActionForFileExists=FileExists_NotSet;
        break;
        case 1:
            alwaysDoThisActionForFileExists=FileExists_Skip;
        break;
        case 2:
            alwaysDoThisActionForFileExists=FileExists_Overwrite;
        break;
        case 3:
            alwaysDoThisActionForFileExists=FileExists_OverwriteIfNotSame;
        break;
        case 4:
            alwaysDoThisActionForFileExists=FileExists_OverwriteIfNewer;
        break;
        case 5:
            alwaysDoThisActionForFileExists=FileExists_OverwriteIfOlder;
        break;
        case 6:
            alwaysDoThisActionForFileExists=FileExists_Rename;
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
            alwaysDoThisActionForFileExists=FileExists_NotSet;
        break;
    }
    emit signal_setCollisionAction(alwaysDoThisActionForFileExists);
}

void CopyEngine::setFileError(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action index: "+std::to_string(index));
    if(uiIsInstalled)
        if(index!=ui->comboBoxFileError->currentIndex())
            ui->comboBoxFileError->setCurrentIndex(index);
    switch(index)
    {
        case 0:
            alwaysDoThisActionForFileError=FileError_NotSet;
        break;
        case 1:
            alwaysDoThisActionForFileError=FileError_Skip;
        break;
        case 2:
            alwaysDoThisActionForFileError=FileError_PutToEndOfTheList;
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
            alwaysDoThisActionForFileError=FileError_NotSet;
        break;
    }
    emit signal_setCollisionAction(alwaysDoThisActionForFileExists);
}

void CopyEngine::setRightTransfer(const bool doRightTransfer)
{
    this->doRightTransfer=doRightTransfer;
    if(uiIsInstalled)
        ui->doRightTransfer->setChecked(doRightTransfer);
    listThread->setRightTransfer(doRightTransfer);
}

//set keep date
void CopyEngine::setKeepDate(const bool keepDate)
{
    this->keepDate=keepDate;
    if(uiIsInstalled)
        ui->keepDate->setChecked(keepDate);
    listThread->setKeepDate(keepDate);
}

void CopyEngine::setMoveTheWholeFolder(const bool &moveTheWholeFolder)
{
    this->moveTheWholeFolder=moveTheWholeFolder;
    if(uiIsInstalled)
        ui->moveTheWholeFolder->setChecked(moveTheWholeFolder);
    emit send_moveTheWholeFolder(moveTheWholeFolder);
}

void CopyEngine::setFollowTheStrictOrder(const bool &followTheStrictOrder)
{
    this->followTheStrictOrder=followTheStrictOrder;
    if(uiIsInstalled)
        ui->followTheStrictOrder->setChecked(followTheStrictOrder);
    listThread->setFollowTheStrictOrder(followTheStrictOrder);
    emit send_followTheStrictOrder(followTheStrictOrder);
}

void CopyEngine::setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles)
{
    this->deletePartiallyTransferredFiles=deletePartiallyTransferredFiles;
    if(uiIsInstalled)
        ui->deletePartiallyTransferredFiles->setChecked(deletePartiallyTransferredFiles);
    emit send_deletePartiallyTransferredFiles(deletePartiallyTransferredFiles);
}

void CopyEngine::setInodeThreads(const int &inodeThreads)
{
    this->inodeThreads=inodeThreads;
    if(uiIsInstalled)
        ui->inodeThreads->setValue(inodeThreads);
    emit send_setInodeThreads(inodeThreads);
}

void CopyEngine::setRenameTheOriginalDestination(const bool &renameTheOriginalDestination)
{
    this->renameTheOriginalDestination=renameTheOriginalDestination;
    if(uiIsInstalled)
        ui->renameTheOriginalDestination->setChecked(renameTheOriginalDestination);
    emit send_setRenameTheOriginalDestination(renameTheOriginalDestination);
}

void CopyEngine::inodeThreadsFinished()
{
    this->inodeThreads=ui->inodeThreads->value();
    emit send_setInodeThreads(inodeThreads);
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
/// \brief set rsync
void CopyEngine::setRsync(const bool rsync)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"set rsync: "+std::to_string(rsync));
    this->rsync=rsync;
    if(uiIsInstalled)
    {
        ui->rsync->setChecked(rsync);
        ui->rsync->setEnabled(forcedMode && mode==Ultracopier::Copy);
        ui->label_rsync->setEnabled(forcedMode && mode==Ultracopier::Copy);
    }
    listThread->setRsync(rsync);
}
#endif

//set check destination folder
void CopyEngine::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
    this->checkDestinationFolderExists=checkDestinationFolderExists;
    if(uiIsInstalled)
        ui->checkBoxDestinationFolderExists->setChecked(checkDestinationFolderExists);
    listThread->setCheckDestinationFolderExists(checkDestinationFolderExists);
}

void CopyEngine::setMkFullPath(const bool mkFullPath)
{
    this->mkFullPath=mkFullPath;
    if(uiIsInstalled)
        ui->mkpath->setChecked(mkFullPath);
    listThread->setMkFullPath(mkFullPath);
}

//reset widget
void CopyEngine::resetTempWidget()
{
    uiIsInstalled=false;
    tempWidget=NULL;
}

void CopyEngine::setFolderCollision(int index)
{
    switch(index)
    {
        case 0:
            setComboBoxFolderCollision(FolderExists_NotSet,false);
        break;
        case 1:
            setComboBoxFolderCollision(FolderExists_Merge,false);
        break;
        case 2:
            setComboBoxFolderCollision(FolderExists_Skip,false);
        break;
        case 3:
            setComboBoxFolderCollision(FolderExists_Rename,false);
        break;
    }
}

void CopyEngine::setFolderError(int index)
{
    switch(index)
    {
        case 0:
            setComboBoxFolderError(FileError_NotSet,false);
        break;
        case 1:
            setComboBoxFolderError(FileError_Skip,false);
        break;
    }
}

//set the translate
void CopyEngine::newLanguageLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, retranslate the widget options");
    if(tempWidget!=NULL)
    {
        ui->retranslateUi(tempWidget);
        ui->comboBoxFolderError->setItemText(0,tr("Ask"));
        ui->comboBoxFolderError->setItemText(1,tr("Skip"));

        ui->comboBoxFolderCollision->setItemText(0,tr("Ask"));
        ui->comboBoxFolderCollision->setItemText(1,tr("Merge"));
        ui->comboBoxFolderCollision->setItemText(2,tr("Skip"));
        ui->comboBoxFolderCollision->setItemText(3,tr("Rename"));

        ui->comboBoxFileError->setItemText(0,tr("Ask"));
        ui->comboBoxFileError->setItemText(1,tr("Skip"));
        ui->comboBoxFileError->setItemText(2,tr("Put at the end"));

        ui->comboBoxFileCollision->setItemText(0,tr("Ask"));
        ui->comboBoxFileCollision->setItemText(1,tr("Skip"));
        ui->comboBoxFileCollision->setItemText(2,tr("Overwrite"));
        ui->comboBoxFileCollision->setItemText(3,tr("Overwrite if different"));
        ui->comboBoxFileCollision->setItemText(4,tr("Overwrite if newer"));
        ui->comboBoxFileCollision->setItemText(5,tr("Overwrite if older"));
        ui->comboBoxFileCollision->setItemText(6,tr("Rename"));
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"ui not loaded!");
}

void CopyEngine::setComboBoxFolderCollision(FolderExistsAction action,bool changeComboBox)
{
    alwaysDoThisActionForFolderExists=action;
    emit signal_setFolderCollision(alwaysDoThisActionForFolderExists);
    if(!changeComboBox || !uiIsInstalled)
        return;
    switch(action)
    {
        case FolderExists_Merge:
            ui->comboBoxFolderCollision->setCurrentIndex(1);
        break;
        case FolderExists_Skip:
            ui->comboBoxFolderCollision->setCurrentIndex(2);
        break;
        case FolderExists_Rename:
            ui->comboBoxFolderCollision->setCurrentIndex(3);
        break;
        default:
            ui->comboBoxFolderCollision->setCurrentIndex(0);
        break;
    }
}

void CopyEngine::setComboBoxFolderError(FileErrorAction action,bool changeComboBox)
{
    alwaysDoThisActionForFileError=action;
    if(!changeComboBox || !uiIsInstalled)
        return;
    switch(action)
    {
        case FileError_Skip:
            ui->comboBoxFolderError->setCurrentIndex(1);
        break;
        default:
            ui->comboBoxFolderError->setCurrentIndex(0);
        break;
    }
}


void CopyEngine::showFilterDialog()
{
    if(filters!=NULL)
        filters->exec();
}

void CopyEngine::sendNewFilters()
{
    if(filters!=NULL)
        emit send_setFilters(filters->getInclude(),filters->getExclude());
}

void CopyEngine::sendNewRenamingRules(std::string firstRenamingRule,std::string otherRenamingRule)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    emit send_sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void CopyEngine::showRenamingRules()
{
    if(renamingRules==NULL)
    {
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded. Unable to access the filters"));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"options not loaded");
        return;
    }
    renamingRules->exec();
}

void CopyEngine::get_realBytesTransfered(quint64 realBytesTransfered)
{
    size_for_speed=realBytesTransfered;
}

void CopyEngine::newActionInProgess(Ultracopier::EngineActionInProgress action)
{
    if(action==Ultracopier::Idle)
    {
        timerProgression.stop();
        timerActionDone.stop();
    }
    else
    {
        timerProgression.start();
        timerActionDone.start();
    }
}

void CopyEngine::setCheckDiskSpace(const bool &checkDiskSpace)
{
    this->checkDiskSpace=checkDiskSpace;
    if(uiIsInstalled)
        ui->checkDiskSpace->setChecked(checkDiskSpace);
    listThread->setCheckDiskSpace(checkDiskSpace);
}

void CopyEngine::setDefaultDestinationFolder(const std::string &defaultDestinationFolder)
{
    this->defaultDestinationFolder=defaultDestinationFolder;
    if(uiIsInstalled)
        ui->defaultDestinationFolder->setText(QString::fromStdString(defaultDestinationFolder));
}

void CopyEngine::exportErrorIntoTransferList()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"exportErrorIntoTransferList");
    std::string fileName = QFileDialog::getSaveFileName(uiinterface,QString::fromStdString(facilityEngine->translateText("Save transfer list")),QStringLiteral("transfer-list.lst"),QString::fromStdString(facilityEngine->translateText("Transfer list"))+QStringLiteral(" (*.lst)")).toStdString();
    if(fileName.empty())
        return;
    emit signal_exportErrorIntoTransferList(fileName);
}
