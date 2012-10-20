/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "copyEngineUnitTester.h"
#include "folderExistsDialog.h"
#include "../../../interface/PluginInterface_CopyEngine.h"

copyEngineUnitTester::copyEngineUnitTester(FacilityInterface * facilityEngine) :
    ui(new Ui::options())
{
    listThread=new ListThread(facilityEngine);
    this->facilityEngine=facilityEngine;
    filters=NULL;
    renamingRules=NULL;

    interface			= NULL;
    tempWidget			= NULL;
    uiIsInstalled			= false;
    dialogIsOpen			= false;
    maxSpeed			= 0;
    alwaysDoThisActionForFileExists	= FileExists_NotSet;
    alwaysDoThisActionForFileError	= FileError_NotSet;
    checkDestinationFolderExists	= false;
    stopIt				= false;
    size_for_speed			= 0;
    forcedMode			= false;

    //implement the SingleShot in this class
    //timerActionDone.setSingleShot(true);
    timerActionDone.setInterval(ULTRACOPIER_PLUGIN_TIME_UPDATE_TRASNFER_LIST);
    //timerProgression.setSingleShot(true);
    timerProgression.setInterval(ULTRACOPIER_PLUGIN_TIME_UPDATE_PROGRESSION);

}

copyEngineUnitTester::~copyEngine()
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

void copyEngineUnitTester::connectTheSignalsSlots()
{
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    debugDialogWindow.show();
    #endif
    if(!connect(listThread,&ListThread::actionInProgess,	this,&copyEngineUnitTester::actionInProgess,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect actionInProgess()");
    if(!connect(listThread,&ListThread::actionInProgess,	this,&copyEngineUnitTester::newActionInProgess,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect actionInProgess() to slot");
    if(!connect(listThread,&ListThread::newFolderListing,			this,&copyEngineUnitTester::newFolderListing,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newFolderListing()");
    if(!connect(listThread,&ListThread::newCollisionAction,			this,&copyEngineUnitTester::newCollisionAction,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newCollisionAction()");
    if(!connect(listThread,&ListThread::newErrorAction,			this,&copyEngineUnitTester::newErrorAction,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newErrorAction()");
    if(!connect(listThread,&ListThread::isInPause,				this,&copyEngineUnitTester::isInPause,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect isInPause()");
    if(!connect(listThread,&ListThread::error,	this,&copyEngineUnitTester::error,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect error()");
    if(!connect(listThread,&ListThread::rmPath,				this,&copyEngineUnitTester::rmPath,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect rmPath()");
    if(!connect(listThread,&ListThread::mkPath,				this,&copyEngineUnitTester::mkPath,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect mkPath()");
    if(!connect(listThread,&ListThread::newActionOnList,	this,&copyEngineUnitTester::newActionOnList,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newActionOnList()");
    if(!connect(listThread,&ListThread::pushFileProgression,		this,&copyEngineUnitTester::pushFileProgression,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect pushFileProgression()");
    if(!connect(listThread,&ListThread::pushGeneralProgression,		this,&copyEngineUnitTester::pushGeneralProgression,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect pushGeneralProgression()");
    if(!connect(listThread,&ListThread::syncReady,						this,&copyEngineUnitTester::syncReady,					Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect syncReady()");
    if(!connect(listThread,&ListThread::canBeDeleted,						this,&copyEngineUnitTester::canBeDeleted,					Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect canBeDeleted()");
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    if(!connect(listThread,&ListThread::debugInformation,			this,&copyEngineUnitTester::debugInformation,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect debugInformation()");
    #endif

    if(!connect(listThread,&ListThread::send_fileAlreadyExists,		this,&copyEngineUnitTester::fileAlreadyExistsSlot,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_fileAlreadyExists()");
    if(!connect(listThread,&ListThread::send_errorOnFile,			this,&copyEngineUnitTester::errorOnFileSlot,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_errorOnFile()");
    if(!connect(listThread,&ListThread::send_folderAlreadyExists,	this,&copyEngineUnitTester::folderAlreadyExistsSlot,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_folderAlreadyExists()");
    if(!connect(listThread,&ListThread::send_errorOnFolder,			this,&copyEngineUnitTester::errorOnFolderSlot,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_errorOnFolder()");
    if(!connect(listThread,&ListThread::updateTheDebugInfo,				this,&copyEngineUnitTester::updateTheDebugInfo,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect updateTheDebugInfo()");
    if(!connect(listThread,&ListThread::errorTransferList,							this,&copyEngineUnitTester::errorTransferList,						Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect errorTransferList()");
    if(!connect(listThread,&ListThread::warningTransferList,						this,&copyEngineUnitTester::warningTransferList,					Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect warningTransferList()");
    if(!connect(listThread,&ListThread::mkPathErrorOnFolder,					this,&copyEngineUnitTester::mkPathErrorOnFolderSlot,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect mkPathErrorOnFolder()");
    if(!connect(listThread,&ListThread::rmPathErrorOnFolder,					this,&copyEngineUnitTester::rmPathErrorOnFolderSlot,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect rmPathErrorOnFolder()");
    if(!connect(listThread,&ListThread::send_realBytesTransfered,					this,&copyEngineUnitTester::get_realBytesTransfered,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_realBytesTransfered()");

    if(!connect(this,&copyEngineUnitTester::tryCancel,						listThread,&ListThread::tryCancel,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect tryCancel()");
    if(!connect(this,&copyEngineUnitTester::signal_pause,						listThread,&ListThread::pause,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_pause()");
    if(!connect(this,&copyEngineUnitTester::signal_resume,						listThread,&ListThread::resume,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_resume()");
    if(!connect(this,&copyEngineUnitTester::signal_skip,					listThread,&ListThread::skip,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_skip()");
    if(!connect(this,&copyEngineUnitTester::signal_setCollisionAction,		listThread,&ListThread::setAlwaysFileExistsAction,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_setCollisionAction()");
    if(!connect(this,&copyEngineUnitTester::signal_setFolderColision,		listThread,&ListThread::setFolderColision,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_setFolderColision()");
    if(!connect(this,&copyEngineUnitTester::signal_removeItems,				listThread,&ListThread::removeItems,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_removeItems()");
    if(!connect(this,&copyEngineUnitTester::signal_moveItemsOnTop,				listThread,&ListThread::moveItemsOnTop,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsOnTop()");
    if(!connect(this,&copyEngineUnitTester::signal_moveItemsUp,				listThread,&ListThread::moveItemsUp,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsUp()");
    if(!connect(this,&copyEngineUnitTester::signal_moveItemsDown,				listThread,&ListThread::moveItemsDown,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsDown()");
    if(!connect(this,&copyEngineUnitTester::signal_moveItemsOnBottom,			listThread,&ListThread::moveItemsOnBottom,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsOnBottom()");
    if(!connect(this,&copyEngineUnitTester::signal_exportTransferList,			listThread,&ListThread::exportTransferList,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_exportTransferList()");
    if(!connect(this,&copyEngineUnitTester::signal_importTransferList,			listThread,&ListThread::importTransferList,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_importTransferList()");
    if(!connect(this,&copyEngineUnitTester::signal_forceMode,				listThread,&ListThread::forceMode,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_forceMode()");
    if(!connect(this,&copyEngineUnitTester::send_osBufferLimit,					listThread,&ListThread::set_osBufferLimit,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_osBufferLimit()");
    if(!connect(this,&copyEngineUnitTester::send_setFilters,listThread,&ListThread::set_setFilters,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_setFilters()");
    if(!connect(this,&copyEngineUnitTester::send_sendNewRenamingRules,listThread,&ListThread::set_sendNewRenamingRules,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_sendNewRenamingRules()");
    if(!connect(&timerActionDone,&QTimer::timeout,							listThread,&ListThread::sendActionDone))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect timerActionDone");
    if(!connect(&timerProgression,&QTimer::timeout,							listThread,&ListThread::sendProgression))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect timerProgression");

    if(!connect(this,&copyEngineUnitTester::queryOneNewDialog,this,&copyEngineUnitTester::showOneNewDialog,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect queryOneNewDialog()");
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
void copyEngineUnitTester::updateTheDebugInfo(QStringList newList,QStringList newList2,int numberOfInodeOperation)
{
    debugDialogWindow.setTransferThreadList(newList);
    debugDialogWindow.setTransferList(newList2);
    debugDialogWindow.setInodeUsage(numberOfInodeOperation);
}
#endif

//to send the options panel
bool copyEngineUnitTester::getOptionsEngine(QWidget * tempWidget)
{
    this->tempWidget=tempWidget;
    ui->setupUi(tempWidget);
    connect(tempWidget,		&QWidget::destroyed,		this,			&copyEngineUnitTester::resetTempWidget);
    //conect the ui widget
/*	connect(ui->doRightTransfer,	&QCheckBox::toggled,		&threadOfTheTransfer,	&copyEngine::setRightTransfer);
    connect(ui->keepDate,		&QCheckBox::toggled,		&threadOfTheTransfer,	&copyEngine::setKeepDate);
    connect(ui->blockSize,		&QCheckBox::valueChanged,	&threadOfTheTransfer,	&copyEngine::setBlockSize);*/
    connect(ui->autoStart,		&QCheckBox::toggled,		this,			&copyEngineUnitTester::setAutoStart);
    connect(ui->checkBoxDestinationFolderExists,	&QCheckBox::toggled,this,		&copyEngineUnitTester::setCheckDestinationFolderExists);
    uiIsInstalled=true;
    setRightTransfer(doRightTransfer);
    setKeepDate(keepDate);
    setSpeedLimitation(maxSpeed);
    setBlockSize(blockSize);
    setAutoStart(autoStart);
    setCheckDestinationFolderExists(checkDestinationFolderExists);
    set_doChecksum(doChecksum);
    set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
    set_checksumOnlyOnError(checksumOnlyOnError);
    set_osBuffer(osBuffer);
    set_osBufferLimited(osBufferLimited);
    set_osBufferLimit(osBufferLimit);
    return true;
}

//to have interface widget to do modal dialog
void copyEngineUnitTester::setInterfacePointer(QWidget * interface)
{
    this->interface=interface;
    filters=new Filters(tempWidget);
    renamingRules=new RenamingRules(tempWidget);

    if(uiIsInstalled)
    {
        connect(ui->doRightTransfer,		&QCheckBox::toggled,		this,&copyEngineUnitTester::setRightTransfer);
        connect(ui->keepDate,			&QCheckBox::toggled,		this,&copyEngineUnitTester::setKeepDate);
        connect(ui->blockSize,			static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&copyEngineUnitTester::setBlockSize);
        connect(ui->autoStart,			&QCheckBox::toggled,		this,&copyEngineUnitTester::setAutoStart);
        connect(ui->doChecksum,			&QCheckBox::toggled,		this,&copyEngineUnitTester::doChecksum_toggled);
        connect(ui->checksumIgnoreIfImpossible,	&QCheckBox::toggled,		this,&copyEngineUnitTester::checksumIgnoreIfImpossible_toggled);
        connect(ui->checksumOnlyOnError,	&QCheckBox::toggled,		this,&copyEngineUnitTester::checksumOnlyOnError_toggled);
        connect(ui->osBuffer,			&QCheckBox::toggled,		this,&copyEngineUnitTester::osBuffer_toggled);
        connect(ui->osBufferLimited,		&QCheckBox::toggled,		this,&copyEngineUnitTester::osBufferLimited_toggled);
        connect(ui->osBufferLimit,		&QSpinBox::editingFinished,	this,&copyEngineUnitTester::osBufferLimit_editingFinished);

        connect(filters,&Filters::haveNewFilters,this,&copyEngineUnitTester::sendNewFilters);
        connect(ui->filters,&QPushButton::clicked,this,&copyEngineUnitTester::showFilterDialog);

        if(!connect(renamingRules,&RenamingRules::sendNewRenamingRules,this,&copyEngineUnitTester::sendNewRenamingRules))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect sendNewRenamingRules()");
        if(!connect(ui->renamingRules,&QPushButton::clicked,this,&copyEngineUnitTester::showRenamingRules))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect renamingRules.clicked()");
    }

    filters->setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
    set_setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);

    renamingRules->setRenamingRules(firstRenamingRule,otherRenamingRule);
    emit send_sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

bool copyEngineUnitTester::haveSameSource(const QStringList &sources)
{
    return listThread->haveSameSource(sources);
}

bool copyEngineUnitTester::haveSameDestination(const QString &destination)
{
    return listThread->haveSameDestination(destination);
}

bool copyEngineUnitTester::newCopy(const QStringList &sources)
{
    if(forcedMode && mode!=Copy)
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

bool copyEngineUnitTester::newCopy(const QStringList &sources,const QString &destination)
{
    if(forcedMode && mode!=Copy)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to move, you can't copy with it");
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The engine is forced to move, you can't copy with it"));
        return false;
    }
    return listThread->newCopy(sources,destination);
}

bool copyEngineUnitTester::newMove(const QStringList &sources)
{
    if(forcedMode && mode!=Move)
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

bool copyEngineUnitTester::newMove(const QStringList &sources,const QString &destination)
{
    if(forcedMode && mode!=Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to copy, you can't move with it");
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The engine is forced to copy, you can't move with it"));
        return false;
    }
    return listThread->newMove(sources,destination);
}

void copyEngineUnitTester::newTransferList(const QString &file)
{
    emit signal_importTransferList(file);
}

//because direct access to list thread into the main thread can't be do
quint64 copyEngineUnitTester::realByteTransfered()
{
    return size_for_speed;
}

//speed limitation
qint64 copyEngineUnitTester::getSpeedLimitation()
{
    return listThread->getSpeedLimitation();
}

//get collision action
QList<QPair<QString,QString> > copyEngineUnitTester::getCollisionAction()
{
    QPair<QString,QString> tempItem;
    QList<QPair<QString,QString> > list;
    tempItem.first=facilityEngine->translateText("Ask");tempItem.second="ask";list << tempItem;
    tempItem.first=facilityEngine->translateText("Skip");tempItem.second="skip";list << tempItem;
    tempItem.first=facilityEngine->translateText("Overwrite");tempItem.second="overwrite";list << tempItem;
    tempItem.first=facilityEngine->translateText("Overwrite if newer");tempItem.second="overwriteIfNewer";list << tempItem;
    tempItem.first=facilityEngine->translateText("Overwrite if the last modification dates are different");tempItem.second="overwriteIfNotSameModificationDate";list << tempItem;
    tempItem.first=facilityEngine->translateText("Rename");tempItem.second="rename";list << tempItem;
    return list;
}

QList<QPair<QString,QString> > copyEngineUnitTester::getErrorAction()
{
    QPair<QString,QString> tempItem;
    QList<QPair<QString,QString> > list;
    tempItem.first=facilityEngine->translateText("Ask");tempItem.second="ask";list << tempItem;
    tempItem.first=facilityEngine->translateText("Skip");tempItem.second="skip";list << tempItem;
    tempItem.first=facilityEngine->translateText("Put to end of the list");tempItem.second="putToEndOfTheList";list << tempItem;
    return list;
}

void copyEngineUnitTester::setDrive(const QStringList &drives)
{
    listThread->setDrive(drives);
}

/** \brief to sync the transfer list
 * Used when the interface is changed, useful to minimize the memory size */
void copyEngineUnitTester::syncTransferList()
{
    listThread->syncTransferList();
}

void copyEngineUnitTester::set_doChecksum(bool doChecksum)
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

void copyEngineUnitTester::set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible)
{
    listThread->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
    if(uiIsInstalled)
        ui->checksumIgnoreIfImpossible->setChecked(checksumIgnoreIfImpossible);
    this->checksumIgnoreIfImpossible=checksumIgnoreIfImpossible;
}

void copyEngineUnitTester::set_checksumOnlyOnError(bool checksumOnlyOnError)
{
    listThread->set_checksumOnlyOnError(checksumOnlyOnError);
    if(uiIsInstalled)
        ui->checksumOnlyOnError->setChecked(checksumOnlyOnError);
    this->checksumOnlyOnError=checksumOnlyOnError;
}

void copyEngineUnitTester::set_osBuffer(bool osBuffer)
{
    listThread->set_osBuffer(osBuffer);
    if(uiIsInstalled)
    {
        ui->osBuffer->setChecked(osBuffer);
        updateBufferCheckbox();
    }
    this->osBuffer=osBuffer;
}

void copyEngineUnitTester::set_osBufferLimited(bool osBufferLimited)
{
    listThread->set_osBufferLimited(osBufferLimited);
    if(uiIsInstalled)
    {
        ui->osBufferLimited->setChecked(osBufferLimited);
        updateBufferCheckbox();
    }
    this->osBufferLimited=osBufferLimited;
}

void copyEngineUnitTester::set_osBufferLimit(unsigned int osBufferLimit)
{
    emit send_osBufferLimit(osBufferLimit);
    if(uiIsInstalled)
        ui->osBufferLimit->setValue(osBufferLimit);
    this->osBufferLimit=osBufferLimit;
}

void copyEngineUnitTester::updateBufferCheckbox()
{
    ui->osBufferLimited->setEnabled(ui->osBuffer->isChecked());
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void copyEngineUnitTester::set_setFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions)
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

void copyEngineUnitTester::setRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
    sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

bool copyEngineUnitTester::userAddFolder(const CopyMode &mode)
{
    QString source = QFileDialog::getExistingDirectory(interface,facilityEngine->translateText("Select source directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(source.isEmpty() || source.isNull() || source=="")
        return false;
    if(mode==Copy)
        return newCopy(QStringList() << source);
    else
        return newMove(QStringList() << source);
}

bool copyEngineUnitTester::userAddFile(const CopyMode &mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QStringList sources = QFileDialog::getOpenFileNames(
        interface,
        facilityEngine->translateText("Select one or more files to open"),
        "",
        facilityEngine->translateText("All files")+" (*)");
    if(sources.isEmpty())
        return false;
    if(mode==Copy)
        return newCopy(sources);
    else
        return newMove(sources);
}

void copyEngineUnitTester::pause()
{
    emit signal_pause();
}

void copyEngineUnitTester::resume()
{
    emit signal_resume();
}

void copyEngineUnitTester::skip(const quint64 &id)
{
    emit signal_skip(id);
}

void copyEngineUnitTester::cancel()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    stopIt=true;
    timerProgression.stop();
    timerActionDone.stop();
    emit tryCancel();
}

void copyEngineUnitTester::removeItems(const QList<int> &ids)
{
    emit signal_removeItems(ids);
}

void copyEngineUnitTester::moveItemsOnTop(const QList<int> &ids)
{
    emit signal_moveItemsOnTop(ids);
}

void copyEngineUnitTester::moveItemsUp(const QList<int> &ids)
{
    emit signal_moveItemsUp(ids);
}

void copyEngineUnitTester::moveItemsDown(const QList<int> &ids)
{
    emit signal_moveItemsDown(ids);
}

void copyEngineUnitTester::moveItemsOnBottom(const QList<int> &ids)
{
    emit signal_moveItemsOnBottom(ids);
}

/** \brief give the forced mode, to export/import transfer list */
void copyEngineUnitTester::forceMode(const CopyMode &mode)
{
    if(forcedMode)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Mode forced previously"));
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The mode have been forced previously, it's internal error, please report it"));
        return;
    }
    if(mode==Copy)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Force mode to copy"));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("Force mode to move"));
    this->mode=mode;
    forcedMode=true;
    emit signal_forceMode(mode);
}

void copyEngineUnitTester::exportTransferList()
{
    QString fileName = QFileDialog::getSaveFileName(NULL,facilityEngine->translateText("Save transfer list"),"transfer-list.lst",facilityEngine->translateText("Transfer list")+" (*.lst)");
    if(fileName.isEmpty())
        return;
    emit signal_exportTransferList(fileName);
}

void copyEngineUnitTester::importTransferList()
{
    QString fileName = QFileDialog::getOpenFileName(NULL,facilityEngine->translateText("Open transfer list"),"transfer-list.lst",facilityEngine->translateText("Transfer list")+" (*.lst)");
    if(fileName.isEmpty())
        return;
    emit signal_importTransferList(fileName);
}

void copyEngineUnitTester::warningTransferList(const QString &warning)
{
    QMessageBox::warning(interface,facilityEngine->translateText("Error"),warning);
}

void copyEngineUnitTester::errorTransferList(const QString &error)
{
    QMessageBox::critical(interface,facilityEngine->translateText("Error"),error);
}

bool copyEngineUnitTester::setSpeedLimitation(const qint64 &speedLimitation)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"maxSpeed: "+QString::number(speedLimitation));
    maxSpeed=speedLimitation;
    return listThread->setSpeedLimitation(speedLimitation);
}

void copyEngineUnitTester::setCollisionAction(const QString &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action: "+action);
    if(action=="skip")
        alwaysDoThisActionForFileExists=FileExists_Skip;
    else if(action=="overwrite")
        alwaysDoThisActionForFileExists=FileExists_Overwrite;
    else if(action=="overwriteIfNewer")
        alwaysDoThisActionForFileExists=FileExists_OverwriteIfNewer;
    else if(action=="overwriteIfNotSameModificationDate")
        alwaysDoThisActionForFileExists=FileExists_OverwriteIfNotSameModificationDate;
    else if(action=="rename")
        alwaysDoThisActionForFileExists=FileExists_Rename;
    else
        alwaysDoThisActionForFileExists=FileExists_NotSet;
    emit signal_setCollisionAction(alwaysDoThisActionForFileExists);
}

void copyEngineUnitTester::setErrorAction(const QString &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action: "+action);
    if(action=="skip")
        alwaysDoThisActionForFileError=FileError_Skip;
    else if(action=="putToEndOfTheList")
        alwaysDoThisActionForFileError=FileError_PutToEndOfTheList;
    else
        alwaysDoThisActionForFileError=FileError_NotSet;
}

void copyEngineUnitTester::setRightTransfer(const bool doRightTransfer)
{
    this->doRightTransfer=doRightTransfer;
    if(uiIsInstalled)
        ui->doRightTransfer->setChecked(doRightTransfer);
    listThread->setRightTransfer(doRightTransfer);
}

//set keep date
void copyEngineUnitTester::setKeepDate(const bool keepDate)
{
    this->keepDate=keepDate;
    if(uiIsInstalled)
        ui->keepDate->setChecked(keepDate);
    listThread->setKeepDate(keepDate);
}

//set block size in KB
void copyEngineUnitTester::setBlockSize(const int blockSize)
{
    this->blockSize=blockSize;
    if(uiIsInstalled)
        ui->blockSize->setValue(blockSize);
    listThread->setBlockSize(blockSize);
}

//set auto start
void copyEngineUnitTester::setAutoStart(const bool autoStart)
{
    this->autoStart=autoStart;
    if(uiIsInstalled)
        ui->autoStart->setChecked(autoStart);
    listThread->setAutoStart(autoStart);
}

//set check destination folder
void copyEngineUnitTester::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
    this->checkDestinationFolderExists=checkDestinationFolderExists;
    if(uiIsInstalled)
        ui->checkBoxDestinationFolderExists->setChecked(checkDestinationFolderExists);
    listThread->setCheckDestinationFolderExists(checkDestinationFolderExists);
}

//reset widget
void copyEngineUnitTester::resetTempWidget()
{
    tempWidget=NULL;
}

void copyEngineUnitTester::on_comboBoxFolderColision_currentIndexChanged(int index)
{
    switch(index)
    {
        case 0:
            setComboBoxFolderColision(FolderExists_NotSet,false);
        break;
        case 1:
            setComboBoxFolderColision(FolderExists_Merge,false);
        break;
        case 2:
            setComboBoxFolderColision(FolderExists_Skip,false);
        break;
        case 3:
            setComboBoxFolderColision(FolderExists_Rename,false);
        break;
    }
}

void copyEngineUnitTester::on_comboBoxFolderError_currentIndexChanged(int index)
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
void copyEngineUnitTester::newLanguageLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, retranslate the widget options");
    if(tempWidget!=NULL)
        ui->retranslateUi(tempWidget);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"ui not loaded!");
}

void copyEngineUnitTester::setComboBoxFolderColision(FolderExistsAction action,bool changeComboBox)
{
    alwaysDoThisActionForFolderExists=action;
    emit signal_setFolderColision(alwaysDoThisActionForFolderExists);
    if(!changeComboBox || !uiIsInstalled)
        return;
    switch(action)
    {
        case FolderExists_Merge:
            ui->comboBoxFolderColision->setCurrentIndex(1);
        break;
        case FolderExists_Skip:
            ui->comboBoxFolderColision->setCurrentIndex(2);
        break;
        case FolderExists_Rename:
            ui->comboBoxFolderColision->setCurrentIndex(3);
        break;
        default:
            ui->comboBoxFolderColision->setCurrentIndex(0);
        break;
    }
}

void copyEngineUnitTester::setComboBoxFolderError(FileErrorAction action,bool changeComboBox)
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

void copyEngineUnitTester::doChecksum_toggled(bool doChecksum)
{
    listThread->set_doChecksum(doChecksum);
}

void copyEngineUnitTester::checksumOnlyOnError_toggled(bool checksumOnlyOnError)
{
    listThread->set_checksumOnlyOnError(checksumOnlyOnError);
}

void copyEngineUnitTester::checksumIgnoreIfImpossible_toggled(bool checksumIgnoreIfImpossible)
{
    listThread->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
}

void copyEngineUnitTester::osBuffer_toggled(bool osBuffer)
{
    listThread->set_osBuffer(osBuffer);
    updateBufferCheckbox();
}

void copyEngineUnitTester::osBufferLimited_toggled(bool osBufferLimited)
{
    listThread->set_osBufferLimited(osBufferLimited);
    updateBufferCheckbox();
}

void copyEngineUnitTester::osBufferLimit_editingFinished()
{
    emit send_osBufferLimit(ui->osBufferLimit->value());
}

void copyEngineUnitTester::showFilterDialog()
{
    if(filters!=NULL)
        filters->exec();
}

void copyEngineUnitTester::sendNewFilters()
{
    if(filters!=NULL)
        emit send_setFilters(filters->getInclude(),filters->getExclude());
}

void copyEngineUnitTester::sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    emit send_sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void copyEngineUnitTester::showRenamingRules()
{
    if(renamingRules==NULL)
    {
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded, can't access to the filters"));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"options not loaded");
        return;
    }
    renamingRules->exec();
}

void copyEngineUnitTester::get_realBytesTransfered(quint64 realBytesTransfered)
{
    size_for_speed=realBytesTransfered;
}

void copyEngineUnitTester::newActionInProgess(EngineActionInProgress action)
{
    if(action==Idle)
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

//dialog message
/// \note Can be call without queue because all call will be serialized
void copyEngineUnitTester::fileAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread)
{
    fileAlreadyExists(source,destination,isSame,thread);
}

/// \note Can be call without queue because all call will be serialized
void copyEngineUnitTester::errorOnFileSlot(QFileInfo fileInfo,QString errorString,TransferThread * thread)
{
    errorOnFile(fileInfo,errorString,thread);
}

/// \note Can be call without queue because all call will be serialized
void copyEngineUnitTester::folderAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,scanFileOrFolder * thread)
{
    folderAlreadyExists(source,destination,isSame,thread);
}

/// \note Can be call without queue because all call will be serialized
void copyEngineUnitTester::errorOnFolderSlot(QFileInfo fileInfo,QString errorString,scanFileOrFolder * thread)
{
    errorOnFolder(fileInfo,errorString,thread);
}

//mkpath event
void copyEngineUnitTester::mkPathErrorOnFolderSlot(QFileInfo folder,QString error)
{
    mkPathErrorOnFolder(folder,error);
}

//rmpath event
void copyEngineUnitTester::rmPathErrorOnFolderSlot(QFileInfo folder,QString error)
{
    rmPathErrorOnFolder(folder,error);
}


/// \note Can be call without queue because all call will be serialized
void copyEngineUnitTester::fileAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    if(thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to locate the thread");
        return;
    }
    //load the action
    if(isSame)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file is same: "+source.absoluteFilePath());
        tempFileExistsAction=alwaysDoThisActionForFileExists;
        if(tempFileExistsAction==FileExists_Overwrite || tempFileExistsAction==FileExists_OverwriteIfNewer || tempFileExistsAction==FileExists_OverwriteIfNotSameModificationDate)
            tempFileExistsAction=FileExists_NotSet;
        switch(tempFileExistsAction)
        {
            case FileExists_Skip:
            case FileExists_Rename:
                thread->setFileExistsAction(tempFileExistsAction);
            break;
            default:
                if(dialogIsOpen)
                {
                    alreadyExistsQueueItem newItem;
                    newItem.source=source;
                    newItem.destination=destination;
                    newItem.isSame=isSame;
                    newItem.transfer=thread;
                    newItem.scan=NULL;
                    alreadyExistsQueue << newItem;
                    return;
                }
                dialogIsOpen=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
                fileIsSameDialog dialog(interface,source,firstRenamingRule,otherRenamingRule);
                emit isInPause(true);
                dialog.exec();/// \bug crash when external close
                FileExistsAction newAction=dialog.getAction();
                emit isInPause(false);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
                if(newAction==FileExists_Cancel)
                {
                    emit cancelAll();
                    return;
                }
                if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileExists)
                {
                    alwaysDoThisActionForFileExists=newAction;
                    listThread->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
                    switch(newAction)
                    {
                        default:
                        case FileExists_Skip:
                            emit newCollisionAction("skip");
                        break;
                        case FileExists_Rename:
                            emit newCollisionAction("rename");
                        break;
                    }
                }
                if(dialog.getAlways() || newAction!=FileExists_Rename)
                    thread->setFileExistsAction(newAction);
                else
                    thread->setFileRename(dialog.getNewName());
                dialogIsOpen=false;
                if(!isCalledByShowOneNewDialog)
                    emit queryOneNewDialog();
                return;
            break;
        }
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file already exists: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
        tempFileExistsAction=alwaysDoThisActionForFileExists;
        switch(tempFileExistsAction)
        {
            case FileExists_Skip:
            case FileExists_Rename:
            case FileExists_Overwrite:
            case FileExists_OverwriteIfNewer:
            case FileExists_OverwriteIfNotSameModificationDate:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"always do this action: "+QString::number(tempFileExistsAction));
                thread->setFileExistsAction(tempFileExistsAction);
            break;
            default:
                if(dialogIsOpen)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("dialog open, put in queue: %1 %2")
                                 .arg(source.absoluteFilePath())
                                 .arg(destination.absoluteFilePath())
                                 );
                    alreadyExistsQueueItem newItem;
                    newItem.source=source;
                    newItem.destination=destination;
                    newItem.isSame=isSame;
                    newItem.transfer=thread;
                    newItem.scan=NULL;
                    alreadyExistsQueue << newItem;
                    return;
                }
                dialogIsOpen=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
                fileExistsDialog dialog(interface,source,destination,firstRenamingRule,otherRenamingRule);
                emit isInPause(true);
                dialog.exec();/// \bug crash when external close
                FileExistsAction newAction=dialog.getAction();
                emit isInPause(false);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
                if(newAction==FileExists_Cancel)
                {
                    emit cancelAll();
                    return;
                }
                if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileExists)
                {
                    alwaysDoThisActionForFileExists=newAction;
                    listThread->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
                    switch(newAction)
                    {
                        default:
                        case FileExists_Skip:
                            emit newCollisionAction("skip");
                        break;
                        case FileExists_Rename:
                            emit newCollisionAction("rename");
                        break;
                        case FileExists_Overwrite:
                            emit newCollisionAction("overwrite");
                        break;
                        case FileExists_OverwriteIfNewer:
                            emit newCollisionAction("overwriteIfNewer");
                        break;
                        case FileExists_OverwriteIfNotSameModificationDate:
                            emit newCollisionAction("overwriteIfNotSameModificationDate");
                        break;
                    }
                }
                if(dialog.getAlways() || newAction!=FileExists_Rename)
                    thread->setFileExistsAction(newAction);
                else
                    thread->setFileRename(dialog.getNewName());
                dialogIsOpen=false;
                if(!isCalledByShowOneNewDialog)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit queryOneNewDialog()");
                    emit queryOneNewDialog();
                }
                return;
            break;
        }
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

/// \note Can be call without queue because all call will be serialized
void copyEngineUnitTester::errorOnFile(QFileInfo fileInfo,QString errorString,TransferThread * thread,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+fileInfo.absoluteFilePath()+", error: "+errorString);
    if(thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to locate the thread");
        return;
    }
    //load the action
    tempFileErrorAction=alwaysDoThisActionForFileError;
    switch(tempFileErrorAction)
    {
        case FileError_Skip:
            thread->skip();
        return;
        case FileError_Retry:
            thread->retryAfterError();
        return;
        case FileError_PutToEndOfTheList:
            /// \todo do the read transfer locator and put at the end
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"todo list item not found");
        return;
        default:
            if(dialogIsOpen)
            {
                errorQueueItem newItem;
                newItem.errorString=errorString;
                newItem.inode=fileInfo;
                newItem.mkPath=false;
                newItem.rmPath=false;
                newItem.scan=NULL;
                newItem.transfer=thread;
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(fileInfo.absoluteFilePath(),fileInfo.size(),fileInfo.lastModified(),errorString);
            fileErrorDialog dialog(interface,fileInfo,errorString);
            emit isInPause(true);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            emit isInPause(false);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
            if(newAction==FileError_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileError)
            {
                alwaysDoThisActionForFileError=newAction;
                switch(newAction)
                {
                    default:
                    case FileError_Skip:
                        emit newErrorAction("skip");
                    break;
                    case FileError_PutToEndOfTheList:
                        emit newErrorAction("putToEndOfTheList");
                    break;
                }
            }
            switch(newAction)
            {
                case FileError_Skip:
                    thread->skip();
                break;
                case FileError_Retry:
                    thread->retryAfterError();
                break;
                case FileError_PutToEndOfTheList:
                    thread->putAtBottom();
                    /// \todo do the read transfer locator and put at the end
                                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"todo");
                break;
                default:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"file error action wrong");
                break;
            }
            dialogIsOpen=false;
            if(!isCalledByShowOneNewDialog)
                emit queryOneNewDialog();
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"isCalledByShowOneNewDialog==true then not show other dial");
            return;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

/// \note Can be call without queue because all call will be serialized
void copyEngineUnitTester::folderAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,scanFileOrFolder * thread,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"folder already exists: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
    if(thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to locate the thread");
        return;
    }
    //load the always action
    tempFolderExistsAction=alwaysDoThisActionForFolderExists;
    switch(tempFolderExistsAction)
    {
        case FolderExists_Skip:
        case FolderExists_Rename:
        case FolderExists_Merge:
            thread->setFolderExistsAction(tempFolderExistsAction);
        break;
        default:
            if(dialogIsOpen)
            {
                alreadyExistsQueueItem newItem;
                newItem.source=source;
                newItem.destination=destination;
                newItem.isSame=isSame;
                newItem.transfer=NULL;
                newItem.scan=thread;
                alreadyExistsQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            folderExistsDialog dialog(interface,source,isSame,destination,firstRenamingRule,otherRenamingRule);
            dialog.exec();/// \bug crash when external close
            FolderExistsAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
            if(newAction==FolderExists_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFolderExists)
                setComboBoxFolderColision(newAction);
            if(!dialog.getAlways() && newAction==FolderExists_Rename)
                thread->setFolderExistsAction(newAction,dialog.getNewName());
            else
                thread->setFolderExistsAction(newAction);
            dialogIsOpen=false;
            if(!isCalledByShowOneNewDialog)
                emit queryOneNewDialog();
            return;
        break;
    }
}

/// \note Can be call without queue because all call will be serialized
/// \todo all this part
void copyEngineUnitTester::errorOnFolder(QFileInfo fileInfo,QString errorString,scanFileOrFolder * thread,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+fileInfo.absoluteFilePath()+", error: "+errorString);
    if(thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to locate the thread");
        return;
    }
    //load the always action
    tempFileErrorAction=alwaysDoThisActionForFolderError;
    switch(tempFileErrorAction)
    {
        case FileError_Skip:
        case FileError_Retry:
        case FileError_PutToEndOfTheList:
            thread->setFolderErrorAction(tempFileErrorAction);
        break;
        default:
            if(dialogIsOpen)
            {
                errorQueueItem newItem;
                newItem.errorString=errorString;
                newItem.inode=fileInfo;
                newItem.mkPath=false;
                newItem.rmPath=false;
                newItem.scan=thread;
                newItem.transfer=NULL;
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(fileInfo.absoluteFilePath(),fileInfo.size(),fileInfo.lastModified(),errorString);
            fileErrorDialog dialog(interface,fileInfo,errorString);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
            if(newAction==FileError_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileError)
                setComboBoxFolderError(newAction);
            dialogIsOpen=false;
            thread->setFolderErrorAction(newAction);
            if(!isCalledByShowOneNewDialog)
                emit queryOneNewDialog();
            return;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

// -----------------------------------------------------

//mkpath event
void copyEngineUnitTester::mkPathErrorOnFolder(QFileInfo folder,QString errorString,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+folder.absoluteFilePath()+", error: "+errorString);
    //load the always action
    tempFileErrorAction=alwaysDoThisActionForFolderError;
    error_index=0;
    switch(tempFileErrorAction)
    {
        case FileError_Skip:
            listThread->mkPathQueue.skip();
        return;
        case FileError_Retry:
            listThread->mkPathQueue.retry();
        return;
        default:
            if(dialogIsOpen)
            {
                errorQueueItem newItem;
                newItem.errorString=errorString;
                newItem.inode=folder;
                newItem.mkPath=true;
                newItem.rmPath=false;
                newItem.scan=NULL;
                newItem.transfer=NULL;
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(folder.absoluteFilePath(),folder.size(),folder.lastModified(),errorString);
            fileErrorDialog dialog(interface,folder,errorString,false);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
            if(newAction==FileError_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileError)
            {
                setComboBoxFolderError(newAction);
                alwaysDoThisActionForFolderError=newAction;
            }
            dialogIsOpen=false;
            switch(newAction)
            {
                case FileError_Skip:
                    listThread->mkPathQueue.skip();
                break;
                case FileError_Retry:
                    listThread->mkPathQueue.retry();
                break;
                default:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unknow switch case: "+QString::number(newAction));
                break;
            }
            if(!isCalledByShowOneNewDialog)
                emit queryOneNewDialog();
            return;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

//rmpath event
void copyEngineUnitTester::rmPathErrorOnFolder(QFileInfo folder,QString errorString,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+folder.absoluteFilePath()+", error: "+errorString);
    //load the always action
    tempFileErrorAction=alwaysDoThisActionForFolderError;
    error_index=0;
    switch(tempFileErrorAction)
    {
        case FileError_Skip:
            listThread->rmPathQueue.skip();
        return;
        case FileError_Retry:
            listThread->rmPathQueue.retry();
        return;
        default:
            if(dialogIsOpen)
            {
                errorQueueItem newItem;
                newItem.errorString=errorString;
                newItem.inode=folder;
                newItem.mkPath=false;
                newItem.rmPath=true;
                newItem.scan=NULL;
                newItem.transfer=NULL;
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(folder.absoluteFilePath(),folder.size(),folder.lastModified(),errorString);
            fileErrorDialog dialog(interface,folder,errorString,false);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
            if(newAction==FileError_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileError)
            {
                setComboBoxFolderError(newAction);
                alwaysDoThisActionForFolderError=newAction;
            }
            dialogIsOpen=false;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"do the action");
            switch(newAction)
            {
                case FileError_Skip:
                    listThread->rmPathQueue.skip();
                break;
                case FileError_Retry:
                    listThread->rmPathQueue.retry();
                break;
                default:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unknow switch case: "+QString::number(newAction));
                break;
            }
            if(!isCalledByShowOneNewDialog)
                emit queryOneNewDialog();
            return;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

//show one new dialog if needed
void copyEngineUnitTester::showOneNewDialog()
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"alreadyExistsQueue.size(): "+QString::number(alreadyExistsQueue.size()));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"errorQueue.size(): "+QString::number(errorQueue.size()));
    loop_size=alreadyExistsQueue.size();
    while(loop_size>0)
    {
        if(alreadyExistsQueue.first().transfer!=NULL)
        {
            fileAlreadyExists(alreadyExistsQueue.first().source,
                      alreadyExistsQueue.first().destination,
                      alreadyExistsQueue.first().isSame,
                      alreadyExistsQueue.first().transfer,
                      true);
        }
        else if(alreadyExistsQueue.first().scan!=NULL)
            folderAlreadyExists(alreadyExistsQueue.first().source,
                        alreadyExistsQueue.first().destination,
                        alreadyExistsQueue.first().isSame,
                        alreadyExistsQueue.first().scan,
                        true);
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"bug, no thread actived");
        alreadyExistsQueue.removeFirst();
        loop_size--;
    }
    loop_size=errorQueue.size();
    while(errorQueue.size()>0)
    {
        if(errorQueue.first().transfer!=NULL)
            errorOnFile(errorQueue.first().inode,errorQueue.first().errorString,errorQueue.first().transfer,true);
        else if(errorQueue.first().scan!=NULL)
            errorOnFolder(errorQueue.first().inode,errorQueue.first().errorString,errorQueue.first().scan,true);
        else if(errorQueue.first().mkPath)
            mkPathErrorOnFolder(errorQueue.first().inode,errorQueue.first().errorString,true);
        else if(errorQueue.first().rmPath)
            rmPathErrorOnFolder(errorQueue.first().inode,errorQueue.first().errorString,true);
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"bug, no thread actived");
        errorQueue.removeFirst();
        loop_size--;
    }
}
