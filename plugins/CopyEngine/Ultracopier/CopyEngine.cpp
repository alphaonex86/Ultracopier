/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QFileDialog>
#include <QMessageBox>
#include <math.h>

#include "CopyEngine.h"
#include "FolderExistsDialog.h"
#include "../../../interface/PluginInterface_CopyEngine.h"

CopyEngine::CopyEngine(FacilityInterface * facilityEngine) :
    ui(new Ui::copyEngineOptions())
{
    listThread=new ListThread(facilityEngine);
    this->facilityEngine            = facilityEngine;
    filters                         = NULL;
    renamingRules                   = NULL;

    blockSize                       = ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE;
    sequentialBuffer                = ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*ULTRACOPIER_PLUGIN_DEFAULT_SEQUENTIAL_NUMBER_OF_BLOCK;
    parallelBuffer                  = ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
    interface                       = NULL;
    tempWidget                      = NULL;
    uiIsInstalled                   = false;
    dialogIsOpen                    = false;
    maxSpeed                        = 0;
    alwaysDoThisActionForFileExists	= FileExists_NotSet;
    alwaysDoThisActionForFileError	= FileError_NotSet;
    checkDestinationFolderExists	= false;
    stopIt                          = false;
    size_for_speed                  = 0;
    putAtBottom                     = 0;
    forcedMode                      = false;
    followTheStrictOrder            = false;
    deletePartiallyTransferredFiles = true;
    moveTheWholeFolder              = true;

    //implement the SingleShot in this class
    //timerActionDone.setSingleShot(true);
    timerActionDone.setInterval(ULTRACOPIER_PLUGIN_TIME_UPDATE_TRASNFER_LIST);
    //timerProgression.setSingleShot(true);
    timerProgression.setInterval(ULTRACOPIER_PLUGIN_TIME_UPDATE_PROGRESSION);

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
    #endif
    if(!connect(listThread,&ListThread::actionInProgess,	this,&CopyEngine::actionInProgess,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect actionInProgess()");
    if(!connect(listThread,&ListThread::actionInProgess,	this,&CopyEngine::newActionInProgess,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect actionInProgess() to slot");
    if(!connect(listThread,&ListThread::newFolderListing,			this,&CopyEngine::newFolderListing,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newFolderListing()");
    if(!connect(listThread,&ListThread::isInPause,				this,&CopyEngine::isInPause,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect isInPause()");
    if(!connect(listThread,&ListThread::error,	this,&CopyEngine::error,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect error()");
    if(!connect(listThread,&ListThread::rmPath,				this,&CopyEngine::rmPath,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect rmPath()");
    if(!connect(listThread,&ListThread::mkPath,				this,&CopyEngine::mkPath,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect mkPath()");
    if(!connect(listThread,&ListThread::newActionOnList,	this,&CopyEngine::newActionOnList,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newActionOnList()");
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
    if(!connect(this,&CopyEngine::signal_resume,						listThread,&ListThread::resume,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_resume()");
    if(!connect(this,&CopyEngine::signal_skip,					listThread,&ListThread::skip,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_skip()");
    if(!connect(this,&CopyEngine::signal_setCollisionAction,		listThread,&ListThread::setAlwaysFileExistsAction,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_setCollisionAction()");
    if(!connect(this,&CopyEngine::signal_setTransferAlgorithm,		listThread,&ListThread::setTransferAlgorithm,	Qt::QueuedConnection))
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
    if(!connect(this,&CopyEngine::send_osBufferLimit,					listThread,&ListThread::set_osBufferLimit,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_osBufferLimit()");
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    if(!connect(this,&CopyEngine::send_speedLimitation,					listThread,&ListThread::setSpeedLimitation,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_speedLimitation()");
    #endif
    if(!connect(this,&CopyEngine::send_blockSize,					listThread,&ListThread::setBlockSize,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_blockSize()");
    if(!connect(this,&CopyEngine::send_parallelBuffer,					listThread,&ListThread::setParallelBuffer,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect setParallelBuffer()");
    if(!connect(this,&CopyEngine::send_sequentialBuffer,					listThread,&ListThread::setSequentialBuffer,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect setSequentialBuffer()");
    if(!connect(this,&CopyEngine::send_parallelizeIfSmallerThan,					listThread,&ListThread::setParallelizeIfSmallerThan,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect setParallelizeIfSmallerThan()");
    if(!connect(this,&CopyEngine::send_moveTheWholeFolder,					listThread,&ListThread::setMoveTheWholeFolder,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect moveTheWholeFolder()");
    if(!connect(this,&CopyEngine::send_deletePartiallyTransferredFiles,					listThread,&ListThread::setDeletePartiallyTransferredFiles,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect deletePartiallyTransferredFiles()");
    if(!connect(this,&CopyEngine::send_followTheStrictOrder,					listThread,&ListThread::setFollowTheStrictOrder,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect followTheStrictOrder()");
    if(!connect(this,&CopyEngine::send_setFilters,listThread,&ListThread::set_setFilters,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_setFilters()");
    if(!connect(this,&CopyEngine::send_sendNewRenamingRules,listThread,&ListThread::set_sendNewRenamingRules,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_sendNewRenamingRules()");
    if(!connect(this,&CopyEngine::send_setDrive,listThread,&ListThread::setDrive,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_setDrive()");
    if(!connect(&timerActionDone,&QTimer::timeout,							listThread,&ListThread::sendActionDone))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect timerActionDone");
    if(!connect(&timerProgression,&QTimer::timeout,							listThread,&ListThread::sendProgression))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect timerProgression");

    if(!connect(this,&CopyEngine::queryOneNewDialog,this,&CopyEngine::showOneNewDialog,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect queryOneNewDialog()");
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
void CopyEngine::updateTheDebugInfo(QStringList newList,QStringList newList2,int numberOfInodeOperation)
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
    ui->blockSize->setMaximum(ULTRACOPIER_PLUGIN_MAX_BLOCK_SIZE);
    connect(tempWidget,		&QWidget::destroyed,		this,			&CopyEngine::resetTempWidget);
    //conect the ui widget
    uiIsInstalled=true;
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    if(!setSpeedLimitation(maxSpeed))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to set the speed limitation");
    #endif

    setBlockSize(blockSize);
    setSequentialBuffer(sequentialBuffer);
    setParallelBuffer(parallelBuffer);
    setAutoStart(autoStart);
    setCheckDestinationFolderExists(checkDestinationFolderExists);
    set_doChecksum(doChecksum);
    set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
    set_checksumOnlyOnError(checksumOnlyOnError);
    set_osBuffer(osBuffer);
    set_osBufferLimited(osBufferLimited);
    set_osBufferLimit(osBufferLimit);
    setRightTransfer(doRightTransfer);
    setKeepDate(keepDate);
    setParallelizeIfSmallerThan(parallelizeIfSmallerThan);
    setFollowTheStrictOrder(followTheStrictOrder);
    setDeletePartiallyTransferredFiles(deletePartiallyTransferredFiles);
    setMoveTheWholeFolder(moveTheWholeFolder);

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
    switch(alwaysDoThisActionForFileError)
    {
        case FileError_NotSet:
            ui->comboBoxFolderError->setCurrentIndex(0);
        break;
        case FileError_Skip:
            ui->comboBoxFolderError->setCurrentIndex(1);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
            ui->comboBoxFolderError->setCurrentIndex(0);
        break;
    }
    switch(transferAlgorithm)
    {
        case TransferAlgorithm_Automatic:
            ui->transferAlgorithm->setCurrentIndex(0);
        break;
        case TransferAlgorithm_Sequential:
            ui->transferAlgorithm->setCurrentIndex(1);
        break;
        case TransferAlgorithm_Parallel:
            ui->transferAlgorithm->setCurrentIndex(2);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
            ui->transferAlgorithm->setCurrentIndex(0);
        break;
    }
    return true;
}

//to have interface widget to do modal dialog
void CopyEngine::setInterfacePointer(QWidget * interface)
{
    this->interface=interface;
    filters=new Filters(tempWidget);
    renamingRules=new RenamingRules(tempWidget);

    if(uiIsInstalled)
    {
        connect(ui->doRightTransfer,                    &QCheckBox::toggled,		this,&CopyEngine::setRightTransfer);
        connect(ui->keepDate,                           &QCheckBox::toggled,		this,&CopyEngine::setKeepDate);
        connect(ui->blockSize,                          static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&CopyEngine::setBlockSize);
        connect(ui->autoStart,                          &QCheckBox::toggled,		this,&CopyEngine::setAutoStart);
        connect(ui->doChecksum,                         &QCheckBox::toggled,		this,&CopyEngine::doChecksum_toggled);
        connect(ui->checksumIgnoreIfImpossible,         &QCheckBox::toggled,		this,&CopyEngine::checksumIgnoreIfImpossible_toggled);
        connect(ui->checksumOnlyOnError,                &QCheckBox::toggled,		this,&CopyEngine::checksumOnlyOnError_toggled);
        connect(ui->osBuffer,                           &QCheckBox::toggled,		this,&CopyEngine::osBuffer_toggled);
        connect(ui->osBufferLimited,                    &QCheckBox::toggled,		this,&CopyEngine::osBufferLimited_toggled);
        connect(ui->osBufferLimit,                      &QSpinBox::editingFinished,	this,&CopyEngine::osBufferLimit_editingFinished);
        connect(ui->moveTheWholeFolder,                 &QCheckBox::toggled,		this,&CopyEngine::setMoveTheWholeFolder);
        connect(ui->deletePartiallyTransferredFiles,    &QCheckBox::toggled,		this,&CopyEngine::setDeletePartiallyTransferredFiles);
        connect(ui->followTheStrictOrder,               &QCheckBox::toggled,        this,&CopyEngine::setFollowTheStrictOrder);
        connect(ui->checkBoxDestinationFolderExists,	&QCheckBox::toggled,        this,&CopyEngine::setCheckDestinationFolderExists);
        connect(filters,                                &Filters::haveNewFilters,   this,&CopyEngine::sendNewFilters);
        connect(ui->filters,                            &QPushButton::clicked,      this,&CopyEngine::showFilterDialog);

        connect(ui->sequentialBuffer,           static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),               this,&CopyEngine::setSequentialBuffer);
        connect(ui->parallelBuffer,             static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),               this,&CopyEngine::setParallelBuffer);
        connect(ui->parallelizeIfSmallerThan,	static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),               this,&CopyEngine::setParallelizeIfSmallerThan);
        connect(ui->comboBoxFolderError,        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngine::setFolderError);
        connect(ui->comboBoxFolderCollision,	static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngine::setFolderCollision);
        connect(ui->comboBoxFileError,          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngine::setFileError);
        connect(ui->comboBoxFileCollision,      static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngine::setFileCollision);
        connect(ui->transferAlgorithm,          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngine::setTransferAlgorithm);

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

bool CopyEngine::haveSameSource(const QStringList &sources)
{
    return listThread->haveSameSource(sources);
}

bool CopyEngine::haveSameDestination(const QString &destination)
{
    return listThread->haveSameDestination(destination);
}

bool CopyEngine::newCopy(const QStringList &sources)
{
    if(forcedMode && mode!=Ultracopier::Copy)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to move, you can't copy with it");
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The engine is forced to move, you can't copy with it"));
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QString destination = QFileDialog::getExistingDirectory(interface,facilityEngine->translateText("Select destination directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(destination.isEmpty() || destination.isNull() || destination=="")
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Canceled by the user");
        return false;
    }
    return listThread->newCopy(sources,destination);
}

bool CopyEngine::newCopy(const QStringList &sources,const QString &destination)
{
    if(forcedMode && mode!=Ultracopier::Copy)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to move, you can't copy with it");
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The engine is forced to move, you can't copy with it"));
        return false;
    }
    return listThread->newCopy(sources,destination);
}

bool CopyEngine::newMove(const QStringList &sources)
{
    if(forcedMode && mode!=Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to copy, you can't move with it");
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The engine is forced to copy, you can't move with it"));
        return false;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QString destination = QFileDialog::getExistingDirectory(interface,facilityEngine->translateText("Select destination directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(destination.isEmpty() || destination.isNull() || destination=="")
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Canceled by the user");
        return false;
    }
    return listThread->newMove(sources,destination);
}

bool CopyEngine::newMove(const QStringList &sources,const QString &destination)
{
    if(forcedMode && mode!=Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to copy, you can't move with it");
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The engine is forced to copy, you can't move with it"));
        return false;
    }
    return listThread->newMove(sources,destination);
}

void CopyEngine::newTransferList(const QString &file)
{
    emit signal_importTransferList(file);
}

//because direct access to list thread into the main thread can't be do
quint64 CopyEngine::realByteTransfered()
{
    return size_for_speed;
}

//speed limitation
bool CopyEngine::supportSpeedLimitation() const
{
    #ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
    return true;
    #else
    return false;
    #endif
}

void CopyEngine::setDrive(const QStringList &mountSysPoint, const QList<QStorageInfo::DriveType> &driveType)
{
    emit send_setDrive(mountSysPoint,driveType);
}

/** \brief to sync the transfer list
 * Used when the interface is changed, useful to minimize the memory size */
void CopyEngine::syncTransferList()
{
    listThread->syncTransferList();
}

void CopyEngine::set_doChecksum(bool doChecksum)
{
    listThread->set_doChecksum(doChecksum);
    if(uiIsInstalled)
    {
        ui->doChecksum->setChecked(doChecksum);
        ui->checksumOnlyOnError->setEnabled(ui->doChecksum->isChecked());
        ui->checksumIgnoreIfImpossible->setEnabled(ui->doChecksum->isChecked());
    }
    this->doChecksum=doChecksum;
}

void CopyEngine::set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible)
{
    listThread->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
    if(uiIsInstalled)
        ui->checksumIgnoreIfImpossible->setChecked(checksumIgnoreIfImpossible);
    this->checksumIgnoreIfImpossible=checksumIgnoreIfImpossible;
}

void CopyEngine::set_checksumOnlyOnError(bool checksumOnlyOnError)
{
    listThread->set_checksumOnlyOnError(checksumOnlyOnError);
    if(uiIsInstalled)
        ui->checksumOnlyOnError->setChecked(checksumOnlyOnError);
    this->checksumOnlyOnError=checksumOnlyOnError;
}

void CopyEngine::set_osBuffer(bool osBuffer)
{
    listThread->set_osBuffer(osBuffer);
    if(uiIsInstalled)
    {
        ui->osBuffer->setChecked(osBuffer);
        updateBufferCheckbox();
    }
    this->osBuffer=osBuffer;
}

void CopyEngine::set_osBufferLimited(bool osBufferLimited)
{
    listThread->set_osBufferLimited(osBufferLimited);
    if(uiIsInstalled)
    {
        ui->osBufferLimited->setChecked(osBufferLimited);
        updateBufferCheckbox();
    }
    this->osBufferLimited=osBufferLimited;
}

void CopyEngine::set_osBufferLimit(unsigned int osBufferLimit)
{
    emit send_osBufferLimit(osBufferLimit);
    if(uiIsInstalled)
        ui->osBufferLimit->setValue(osBufferLimit);
    this->osBufferLimit=osBufferLimit;
}

void CopyEngine::updateBufferCheckbox()
{
    ui->osBufferLimited->setEnabled(ui->osBuffer->isChecked());
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void CopyEngine::set_setFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions)
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

void CopyEngine::setRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
    sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

bool CopyEngine::userAddFolder(const Ultracopier::CopyMode &mode)
{
    QString source = QFileDialog::getExistingDirectory(interface,facilityEngine->translateText("Select source directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(source.isEmpty() || source.isNull() || source=="")
        return false;
    if(mode==Ultracopier::Copy)
        return newCopy(QStringList() << source);
    else
        return newMove(QStringList() << source);
}

bool CopyEngine::userAddFile(const Ultracopier::CopyMode &mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QStringList sources = QFileDialog::getOpenFileNames(
        interface,
        facilityEngine->translateText("Select one or more files to open"),
        "",
        facilityEngine->translateText("All files")+" (*)");
    if(sources.isEmpty())
        return false;
    if(mode==Ultracopier::Copy)
        return newCopy(sources);
    else
        return newMove(sources);
}

void CopyEngine::pause()
{
    emit signal_pause();
}

void CopyEngine::resume()
{
    emit signal_resume();
}

void CopyEngine::skip(const quint64 &id)
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

void CopyEngine::removeItems(const QList<int> &ids)
{
    emit signal_removeItems(ids);
}

void CopyEngine::moveItemsOnTop(const QList<int> &ids)
{
    emit signal_moveItemsOnTop(ids);
}

void CopyEngine::moveItemsUp(const QList<int> &ids)
{
    emit signal_moveItemsUp(ids);
}

void CopyEngine::moveItemsDown(const QList<int> &ids)
{
    emit signal_moveItemsDown(ids);
}

void CopyEngine::moveItemsOnBottom(const QList<int> &ids)
{
    emit signal_moveItemsOnBottom(ids);
}

/** \brief give the forced mode, to export/import transfer list */
void CopyEngine::forceMode(const Ultracopier::CopyMode &mode)
{
    if(forcedMode)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Mode forced previously"));
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The mode have been forced previously, it's internal error, please report it"));
        return;
    }
    if(mode==Ultracopier::Copy)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Force mode to copy"));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Force mode to move"));
    this->mode=mode;
    forcedMode=true;
    emit signal_forceMode(mode);
}

void CopyEngine::exportTransferList()
{
    QString fileName = QFileDialog::getSaveFileName(interface,facilityEngine->translateText("Save transfer list"),"transfer-list.lst",facilityEngine->translateText("Transfer list")+" (*.lst)");
    if(fileName.isEmpty())
        return;
    emit signal_exportTransferList(fileName);
}

void CopyEngine::importTransferList()
{
    QString fileName = QFileDialog::getOpenFileName(interface,facilityEngine->translateText("Open transfer list"),"transfer-list.lst",facilityEngine->translateText("Transfer list")+" (*.lst)");
    if(fileName.isEmpty())
        return;
    emit signal_importTransferList(fileName);
}

void CopyEngine::warningTransferList(const QString &warning)
{
    QMessageBox::warning(interface,facilityEngine->translateText("Error"),warning);
}

void CopyEngine::errorTransferList(const QString &error)
{
    QMessageBox::critical(interface,facilityEngine->translateText("Error"),error);
}

bool CopyEngine::setSpeedLimitation(const qint64 &speedLimitation)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"maxSpeed: "+QString::number(speedLimitation));
    maxSpeed=speedLimitation;
    emit send_speedLimitation(speedLimitation);
    return true;
}

void CopyEngine::setFileCollision(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("action index: %1").arg(index));
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("action index: %1").arg(index));
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

void CopyEngine::setTransferAlgorithm(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("action index: %1").arg(index));
    if(uiIsInstalled)
        if(index!=ui->transferAlgorithm->currentIndex())
            ui->transferAlgorithm->setCurrentIndex(index);
    switch(index)
    {
        case 0:
            transferAlgorithm=TransferAlgorithm_Automatic;
        break;
        case 1:
            transferAlgorithm=TransferAlgorithm_Sequential;
        break;
        case 2:
            transferAlgorithm=TransferAlgorithm_Parallel;
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
            transferAlgorithm=TransferAlgorithm_Automatic;
        break;
    }
    if(transferAlgorithm==TransferAlgorithm_Sequential)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"transferAlgorithm==TransferAlgorithm_Sequential");
    else if(transferAlgorithm==TransferAlgorithm_Automatic)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"transferAlgorithm==TransferAlgorithm_Automatic");
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"transferAlgorithm==TransferAlgorithm_Parallel");
    emit signal_setTransferAlgorithm(transferAlgorithm);
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

//set block size in KB
void CopyEngine::setBlockSize(const int blockSize)
{
    this->blockSize=blockSize;
    if(uiIsInstalled)
    {
        ui->blockSize->setValue(blockSize);
        ui->sequentialBuffer->setSingleStep(blockSize);
        ui->parallelBuffer->setSingleStep(blockSize);
    }
    emit send_blockSize(blockSize);
    updatedBlockSize();
}

void CopyEngine::setParallelBuffer(int parallelBuffer)
{
    parallelBuffer=round((float)parallelBuffer/(float)blockSize)*blockSize;
    this->parallelBuffer=parallelBuffer;
    if(uiIsInstalled)
        ui->parallelBuffer->setValue(parallelBuffer);
    emit send_parallelBuffer(parallelBuffer/blockSize);
}

void CopyEngine::setSequentialBuffer(int sequentialBuffer)
{
    sequentialBuffer=round((float)sequentialBuffer/(float)blockSize)*blockSize;
    this->sequentialBuffer=sequentialBuffer;
    if(uiIsInstalled)
        ui->sequentialBuffer->setValue(sequentialBuffer);
    emit send_sequentialBuffer(sequentialBuffer/blockSize);
}

void CopyEngine::setParallelizeIfSmallerThan(int parallelizeIfSmallerThan)
{
    this->parallelizeIfSmallerThan=parallelizeIfSmallerThan;
    if(uiIsInstalled)
        ui->parallelizeIfSmallerThan->setValue(parallelizeIfSmallerThan);
    emit send_parallelizeIfSmallerThan(parallelizeIfSmallerThan*1024);
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
    emit send_followTheStrictOrder(followTheStrictOrder);
}

void CopyEngine::setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles)
{
    this->deletePartiallyTransferredFiles=deletePartiallyTransferredFiles;
    if(uiIsInstalled)
        ui->deletePartiallyTransferredFiles->setChecked(deletePartiallyTransferredFiles);
    emit send_deletePartiallyTransferredFiles(deletePartiallyTransferredFiles);
}

//set auto start
void CopyEngine::setAutoStart(const bool autoStart)
{
    this->autoStart=autoStart;
    if(uiIsInstalled)
        ui->autoStart->setChecked(autoStart);
    listThread->setAutoStart(autoStart);
}

//set check destination folder
void CopyEngine::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
    this->checkDestinationFolderExists=checkDestinationFolderExists;
    if(uiIsInstalled)
        ui->checkBoxDestinationFolderExists->setChecked(checkDestinationFolderExists);
    listThread->setCheckDestinationFolderExists(checkDestinationFolderExists);
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

        ui->transferAlgorithm->setItemText(0,tr("Automatic"));
        ui->transferAlgorithm->setItemText(1,tr("Sequential"));
        ui->transferAlgorithm->setItemText(2,tr("Parallel"));
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

void CopyEngine::doChecksum_toggled(bool doChecksum)
{
    listThread->set_doChecksum(doChecksum);
}

void CopyEngine::checksumOnlyOnError_toggled(bool checksumOnlyOnError)
{
    listThread->set_checksumOnlyOnError(checksumOnlyOnError);
}

void CopyEngine::checksumIgnoreIfImpossible_toggled(bool checksumIgnoreIfImpossible)
{
    listThread->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
}

void CopyEngine::osBuffer_toggled(bool osBuffer)
{
    listThread->set_osBuffer(osBuffer);
    updateBufferCheckbox();
}

void CopyEngine::osBufferLimited_toggled(bool osBufferLimited)
{
    listThread->set_osBufferLimited(osBufferLimited);
    updateBufferCheckbox();
}

void CopyEngine::osBufferLimit_editingFinished()
{
    emit send_osBufferLimit(ui->osBufferLimit->value());
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

void CopyEngine::sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
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
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded, can't access to the filters"));
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

void CopyEngine::updatedBlockSize()
{
    if(uiIsInstalled)
    {
        ui->sequentialBuffer->setMinimum(ui->blockSize->value());
        ui->sequentialBuffer->setSingleStep(ui->blockSize->value());
        ui->sequentialBuffer->setMaximum(ui->blockSize->value()*ULTRACOPIER_PLUGIN_MAX_SEQUENTIAL_NUMBER_OF_BLOCK);
        ui->parallelBuffer->setMinimum(ui->blockSize->value());
        ui->parallelBuffer->setSingleStep(ui->blockSize->value());
        ui->parallelBuffer->setMaximum(ui->blockSize->value()*ULTRACOPIER_PLUGIN_MAX_PARALLEL_NUMBER_OF_BLOCK);
    }
    setParallelBuffer(parallelBuffer);
    setSequentialBuffer(sequentialBuffer);
}
