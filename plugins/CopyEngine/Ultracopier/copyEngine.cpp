/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QFileDialog>
#include <QMessageBox>

#include "copyEngine.h"
#include "folderExistsDialog.h"
#include "../../../interface/PluginInterface_CopyEngine.h"

copyEngine::copyEngine(FacilityInterface * facilityEngine) :
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

copyEngine::~copyEngine()
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

void copyEngine::connectTheSignalsSlots()
{
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    debugDialogWindow.show();
    #endif
    if(!connect(listThread,&ListThread::actionInProgess,	this,&copyEngine::actionInProgess,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect actionInProgess()");
    if(!connect(listThread,&ListThread::actionInProgess,	this,&copyEngine::newActionInProgess,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect actionInProgess() to slot");
    if(!connect(listThread,&ListThread::newFolderListing,			this,&copyEngine::newFolderListing,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newFolderListing()");
    if(!connect(listThread,&ListThread::newCollisionAction,			this,&copyEngine::newCollisionAction,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newCollisionAction()");
    if(!connect(listThread,&ListThread::newErrorAction,			this,&copyEngine::newErrorAction,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newErrorAction()");
    if(!connect(listThread,&ListThread::isInPause,				this,&copyEngine::isInPause,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect isInPause()");
    if(!connect(listThread,&ListThread::error,	this,&copyEngine::error,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect error()");
    if(!connect(listThread,&ListThread::rmPath,				this,&copyEngine::rmPath,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect rmPath()");
    if(!connect(listThread,&ListThread::mkPath,				this,&copyEngine::mkPath,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect mkPath()");
    if(!connect(listThread,&ListThread::newActionOnList,	this,&copyEngine::newActionOnList,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect newActionOnList()");
    if(!connect(listThread,&ListThread::pushFileProgression,		this,&copyEngine::pushFileProgression,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect pushFileProgression()");
    if(!connect(listThread,&ListThread::pushGeneralProgression,		this,&copyEngine::pushGeneralProgression,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect pushGeneralProgression()");
    if(!connect(listThread,&ListThread::syncReady,						this,&copyEngine::syncReady,					Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect syncReady()");
    if(!connect(listThread,&ListThread::canBeDeleted,						this,&copyEngine::canBeDeleted,					Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect canBeDeleted()");
    #ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
    if(!connect(listThread,&ListThread::debugInformation,			this,&copyEngine::debugInformation,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect debugInformation()");
    #endif

    if(!connect(listThread,&ListThread::send_fileAlreadyExists,		this,&copyEngine::fileAlreadyExistsSlot,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_fileAlreadyExists()");
    if(!connect(listThread,&ListThread::send_errorOnFile,			this,&copyEngine::errorOnFileSlot,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_errorOnFile()");
    if(!connect(listThread,&ListThread::send_folderAlreadyExists,	this,&copyEngine::folderAlreadyExistsSlot,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_folderAlreadyExists()");
    if(!connect(listThread,&ListThread::send_errorOnFolder,			this,&copyEngine::errorOnFolderSlot,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_errorOnFolder()");
    if(!connect(listThread,&ListThread::updateTheDebugInfo,				this,&copyEngine::updateTheDebugInfo,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect updateTheDebugInfo()");
    if(!connect(listThread,&ListThread::errorTransferList,							this,&copyEngine::errorTransferList,						Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect errorTransferList()");
    if(!connect(listThread,&ListThread::warningTransferList,						this,&copyEngine::warningTransferList,					Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect warningTransferList()");
    if(!connect(listThread,&ListThread::mkPathErrorOnFolder,					this,&copyEngine::mkPathErrorOnFolderSlot,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect mkPathErrorOnFolder()");
    if(!connect(listThread,&ListThread::rmPathErrorOnFolder,					this,&copyEngine::rmPathErrorOnFolderSlot,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect rmPathErrorOnFolder()");
    if(!connect(listThread,&ListThread::send_realBytesTransfered,					this,&copyEngine::get_realBytesTransfered,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_realBytesTransfered()");

    if(!connect(this,&copyEngine::tryCancel,						listThread,&ListThread::tryCancel,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect tryCancel()");
    if(!connect(this,&copyEngine::signal_pause,						listThread,&ListThread::pause,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_pause()");
    if(!connect(this,&copyEngine::signal_resume,						listThread,&ListThread::resume,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_resume()");
    if(!connect(this,&copyEngine::signal_skip,					listThread,&ListThread::skip,				Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_skip()");
    if(!connect(this,&copyEngine::signal_setCollisionAction,		listThread,&ListThread::setAlwaysFileExistsAction,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_setCollisionAction()");
    if(!connect(this,&copyEngine::signal_setFolderColision,		listThread,&ListThread::setFolderColision,	Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_setFolderColision()");
    if(!connect(this,&copyEngine::signal_removeItems,				listThread,&ListThread::removeItems,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_removeItems()");
    if(!connect(this,&copyEngine::signal_moveItemsOnTop,				listThread,&ListThread::moveItemsOnTop,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsOnTop()");
    if(!connect(this,&copyEngine::signal_moveItemsUp,				listThread,&ListThread::moveItemsUp,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsUp()");
    if(!connect(this,&copyEngine::signal_moveItemsDown,				listThread,&ListThread::moveItemsDown,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsDown()");
    if(!connect(this,&copyEngine::signal_moveItemsOnBottom,			listThread,&ListThread::moveItemsOnBottom,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_moveItemsOnBottom()");
    if(!connect(this,&copyEngine::signal_exportTransferList,			listThread,&ListThread::exportTransferList,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_exportTransferList()");
    if(!connect(this,&copyEngine::signal_importTransferList,			listThread,&ListThread::importTransferList,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_importTransferList()");
    if(!connect(this,&copyEngine::signal_forceMode,				listThread,&ListThread::forceMode,			Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect signal_forceMode()");
    if(!connect(this,&copyEngine::send_osBufferLimit,					listThread,&ListThread::set_osBufferLimit,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_osBufferLimit()");
    if(!connect(this,&copyEngine::send_setFilters,listThread,&ListThread::set_setFilters,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_setFilters()");
    if(!connect(this,&copyEngine::send_sendNewRenamingRules,listThread,&ListThread::set_sendNewRenamingRules,		Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect send_sendNewRenamingRules()");
    if(!connect(&timerActionDone,&QTimer::timeout,							listThread,&ListThread::sendActionDone))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect timerActionDone");
    if(!connect(&timerProgression,&QTimer::timeout,							listThread,&ListThread::sendProgression))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect timerProgression");

    if(!connect(this,&copyEngine::queryOneNewDialog,this,&copyEngine::showOneNewDialog,Qt::QueuedConnection))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect queryOneNewDialog()");
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
void copyEngine::updateTheDebugInfo(QStringList newList,QStringList newList2,int numberOfInodeOperation)
{
    debugDialogWindow.setTransferThreadList(newList);
    debugDialogWindow.setTransferList(newList2);
    debugDialogWindow.setInodeUsage(numberOfInodeOperation);
}
#endif

//to send the options panel
bool copyEngine::getOptionsEngine(QWidget * tempWidget)
{
    this->tempWidget=tempWidget;
    ui->setupUi(tempWidget);
    connect(tempWidget,		&QWidget::destroyed,		this,			&copyEngine::resetTempWidget);
    //conect the ui widget
/*	connect(ui->doRightTransfer,	&QCheckBox::toggled,		&threadOfTheTransfer,	&copyEngine::setRightTransfer);
    connect(ui->keepDate,		&QCheckBox::toggled,		&threadOfTheTransfer,	&copyEngine::setKeepDate);
    connect(ui->blockSize,		&QCheckBox::valueChanged,	&threadOfTheTransfer,	&copyEngine::setBlockSize);*/
    connect(ui->autoStart,		&QCheckBox::toggled,		this,			&copyEngine::setAutoStart);
    connect(ui->checkBoxDestinationFolderExists,	&QCheckBox::toggled,this,		&copyEngine::setCheckDestinationFolderExists);
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
void copyEngine::setInterfacePointer(QWidget * interface)
{
    this->interface=interface;
    filters=new Filters(tempWidget);
    renamingRules=new RenamingRules(tempWidget);

    if(uiIsInstalled)
    {
        connect(ui->doRightTransfer,		&QCheckBox::toggled,		this,&copyEngine::setRightTransfer);
        connect(ui->keepDate,			&QCheckBox::toggled,		this,&copyEngine::setKeepDate);
        connect(ui->blockSize,			static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&copyEngine::setBlockSize);
        connect(ui->autoStart,			&QCheckBox::toggled,		this,&copyEngine::setAutoStart);
        connect(ui->doChecksum,			&QCheckBox::toggled,		this,&copyEngine::doChecksum_toggled);
        connect(ui->checksumIgnoreIfImpossible,	&QCheckBox::toggled,		this,&copyEngine::checksumIgnoreIfImpossible_toggled);
        connect(ui->checksumOnlyOnError,	&QCheckBox::toggled,		this,&copyEngine::checksumOnlyOnError_toggled);
        connect(ui->osBuffer,			&QCheckBox::toggled,		this,&copyEngine::osBuffer_toggled);
        connect(ui->osBufferLimited,		&QCheckBox::toggled,		this,&copyEngine::osBufferLimited_toggled);
        connect(ui->osBufferLimit,		&QSpinBox::editingFinished,	this,&copyEngine::osBufferLimit_editingFinished);

        connect(filters,&Filters::haveNewFilters,this,&copyEngine::sendNewFilters);
        connect(ui->filters,&QPushButton::clicked,this,&copyEngine::showFilterDialog);

        if(!connect(renamingRules,&RenamingRules::sendNewRenamingRules,this,&copyEngine::sendNewRenamingRules))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect sendNewRenamingRules()");
        if(!connect(ui->renamingRules,&QPushButton::clicked,this,&copyEngine::showRenamingRules))
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to connect renamingRules.clicked()");
    }

    filters->setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
    set_setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);

    renamingRules->setRenamingRules(firstRenamingRule,otherRenamingRule);
    emit send_sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

bool copyEngine::haveSameSource(const QStringList &sources)
{
    return listThread->haveSameSource(sources);
}

bool copyEngine::haveSameDestination(const QString &destination)
{
    return listThread->haveSameDestination(destination);
}

bool copyEngine::newCopy(const QStringList &sources)
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

bool copyEngine::newCopy(const QStringList &sources,const QString &destination)
{
    if(forcedMode && mode!=Ultracopier::Copy)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to move, you can't copy with it");
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The engine is forced to move, you can't copy with it"));
        return false;
    }
    return listThread->newCopy(sources,destination);
}

bool copyEngine::newMove(const QStringList &sources)
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

bool copyEngine::newMove(const QStringList &sources,const QString &destination)
{
    if(forcedMode && mode!=Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The engine is forced to copy, you can't move with it");
        QMessageBox::critical(NULL,facilityEngine->translateText("Internal error"),tr("The engine is forced to copy, you can't move with it"));
        return false;
    }
    return listThread->newMove(sources,destination);
}

void copyEngine::newTransferList(const QString &file)
{
    emit signal_importTransferList(file);
}

//because direct access to list thread into the main thread can't be do
quint64 copyEngine::realByteTransfered()
{
    return size_for_speed;
}

//speed limitation
qint64 copyEngine::getSpeedLimitation()
{
    return listThread->getSpeedLimitation();
}

//get collision action
QList<QPair<QString,QString> > copyEngine::getCollisionAction()
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

QList<QPair<QString,QString> > copyEngine::getErrorAction()
{
    QPair<QString,QString> tempItem;
    QList<QPair<QString,QString> > list;
    tempItem.first=facilityEngine->translateText("Ask");tempItem.second="ask";list << tempItem;
    tempItem.first=facilityEngine->translateText("Skip");tempItem.second="skip";list << tempItem;
    tempItem.first=facilityEngine->translateText("Put to end of the list");tempItem.second="putToEndOfTheList";list << tempItem;
    return list;
}

void copyEngine::setDrive(const QStringList &drives)
{
    listThread->setDrive(drives);
}

/** \brief to sync the transfer list
 * Used when the interface is changed, useful to minimize the memory size */
void copyEngine::syncTransferList()
{
    listThread->syncTransferList();
}

void copyEngine::set_doChecksum(bool doChecksum)
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

void copyEngine::set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible)
{
    listThread->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
    if(uiIsInstalled)
        ui->checksumIgnoreIfImpossible->setChecked(checksumIgnoreIfImpossible);
    this->checksumIgnoreIfImpossible=checksumIgnoreIfImpossible;
}

void copyEngine::set_checksumOnlyOnError(bool checksumOnlyOnError)
{
    listThread->set_checksumOnlyOnError(checksumOnlyOnError);
    if(uiIsInstalled)
        ui->checksumOnlyOnError->setChecked(checksumOnlyOnError);
    this->checksumOnlyOnError=checksumOnlyOnError;
}

void copyEngine::set_osBuffer(bool osBuffer)
{
    listThread->set_osBuffer(osBuffer);
    if(uiIsInstalled)
    {
        ui->osBuffer->setChecked(osBuffer);
        updateBufferCheckbox();
    }
    this->osBuffer=osBuffer;
}

void copyEngine::set_osBufferLimited(bool osBufferLimited)
{
    listThread->set_osBufferLimited(osBufferLimited);
    if(uiIsInstalled)
    {
        ui->osBufferLimited->setChecked(osBufferLimited);
        updateBufferCheckbox();
    }
    this->osBufferLimited=osBufferLimited;
}

void copyEngine::set_osBufferLimit(unsigned int osBufferLimit)
{
    emit send_osBufferLimit(osBufferLimit);
    if(uiIsInstalled)
        ui->osBufferLimit->setValue(osBufferLimit);
    this->osBufferLimit=osBufferLimit;
}

void copyEngine::updateBufferCheckbox()
{
    ui->osBufferLimited->setEnabled(ui->osBuffer->isChecked());
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void copyEngine::set_setFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions)
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

void copyEngine::setRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
    sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

bool copyEngine::userAddFolder(const Ultracopier::CopyMode &mode)
{
    QString source = QFileDialog::getExistingDirectory(interface,facilityEngine->translateText("Select source directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(source.isEmpty() || source.isNull() || source=="")
        return false;
    if(mode==Ultracopier::Copy)
        return newCopy(QStringList() << source);
    else
        return newMove(QStringList() << source);
}

bool copyEngine::userAddFile(const Ultracopier::CopyMode &mode)
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

void copyEngine::pause()
{
    emit signal_pause();
}

void copyEngine::resume()
{
    emit signal_resume();
}

void copyEngine::skip(const quint64 &id)
{
    emit signal_skip(id);
}

void copyEngine::cancel()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    stopIt=true;
    timerProgression.stop();
    timerActionDone.stop();
    emit tryCancel();
}

void copyEngine::removeItems(const QList<int> &ids)
{
    emit signal_removeItems(ids);
}

void copyEngine::moveItemsOnTop(const QList<int> &ids)
{
    emit signal_moveItemsOnTop(ids);
}

void copyEngine::moveItemsUp(const QList<int> &ids)
{
    emit signal_moveItemsUp(ids);
}

void copyEngine::moveItemsDown(const QList<int> &ids)
{
    emit signal_moveItemsDown(ids);
}

void copyEngine::moveItemsOnBottom(const QList<int> &ids)
{
    emit signal_moveItemsOnBottom(ids);
}

/** \brief give the forced mode, to export/import transfer list */
void copyEngine::forceMode(const Ultracopier::CopyMode &mode)
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

void copyEngine::exportTransferList()
{
    QString fileName = QFileDialog::getSaveFileName(NULL,facilityEngine->translateText("Save transfer list"),"transfer-list.lst",facilityEngine->translateText("Transfer list")+" (*.lst)");
    if(fileName.isEmpty())
        return;
    emit signal_exportTransferList(fileName);
}

void copyEngine::importTransferList()
{
    QString fileName = QFileDialog::getOpenFileName(NULL,facilityEngine->translateText("Open transfer list"),"transfer-list.lst",facilityEngine->translateText("Transfer list")+" (*.lst)");
    if(fileName.isEmpty())
        return;
    emit signal_importTransferList(fileName);
}

void copyEngine::warningTransferList(const QString &warning)
{
    QMessageBox::warning(interface,facilityEngine->translateText("Error"),warning);
}

void copyEngine::errorTransferList(const QString &error)
{
    QMessageBox::critical(interface,facilityEngine->translateText("Error"),error);
}

bool copyEngine::setSpeedLimitation(const qint64 &speedLimitation)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"maxSpeed: "+QString::number(speedLimitation));
    maxSpeed=speedLimitation;
    return listThread->setSpeedLimitation(speedLimitation);
}

void copyEngine::setCollisionAction(const QString &action)
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

void copyEngine::setErrorAction(const QString &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action: "+action);
    if(action=="skip")
        alwaysDoThisActionForFileError=FileError_Skip;
    else if(action=="putToEndOfTheList")
        alwaysDoThisActionForFileError=FileError_PutToEndOfTheList;
    else
        alwaysDoThisActionForFileError=FileError_NotSet;
}

void copyEngine::setRightTransfer(const bool doRightTransfer)
{
    this->doRightTransfer=doRightTransfer;
    if(uiIsInstalled)
        ui->doRightTransfer->setChecked(doRightTransfer);
    listThread->setRightTransfer(doRightTransfer);
}

//set keep date
void copyEngine::setKeepDate(const bool keepDate)
{
    this->keepDate=keepDate;
    if(uiIsInstalled)
        ui->keepDate->setChecked(keepDate);
    listThread->setKeepDate(keepDate);
}

//set block size in KB
void copyEngine::setBlockSize(const int blockSize)
{
    this->blockSize=blockSize;
    if(uiIsInstalled)
        ui->blockSize->setValue(blockSize);
    listThread->setBlockSize(blockSize);
}

//set auto start
void copyEngine::setAutoStart(const bool autoStart)
{
    this->autoStart=autoStart;
    if(uiIsInstalled)
        ui->autoStart->setChecked(autoStart);
    listThread->setAutoStart(autoStart);
}

//set check destination folder
void copyEngine::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
    this->checkDestinationFolderExists=checkDestinationFolderExists;
    if(uiIsInstalled)
        ui->checkBoxDestinationFolderExists->setChecked(checkDestinationFolderExists);
    listThread->setCheckDestinationFolderExists(checkDestinationFolderExists);
}

//reset widget
void copyEngine::resetTempWidget()
{
    tempWidget=NULL;
}

void copyEngine::on_comboBoxFolderColision_currentIndexChanged(int index)
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

void copyEngine::on_comboBoxFolderError_currentIndexChanged(int index)
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
void copyEngine::newLanguageLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, retranslate the widget options");
    if(tempWidget!=NULL)
        ui->retranslateUi(tempWidget);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"ui not loaded!");
}

void copyEngine::setComboBoxFolderColision(FolderExistsAction action,bool changeComboBox)
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

void copyEngine::setComboBoxFolderError(FileErrorAction action,bool changeComboBox)
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

void copyEngine::doChecksum_toggled(bool doChecksum)
{
    listThread->set_doChecksum(doChecksum);
}

void copyEngine::checksumOnlyOnError_toggled(bool checksumOnlyOnError)
{
    listThread->set_checksumOnlyOnError(checksumOnlyOnError);
}

void copyEngine::checksumIgnoreIfImpossible_toggled(bool checksumIgnoreIfImpossible)
{
    listThread->set_checksumIgnoreIfImpossible(checksumIgnoreIfImpossible);
}

void copyEngine::osBuffer_toggled(bool osBuffer)
{
    listThread->set_osBuffer(osBuffer);
    updateBufferCheckbox();
}

void copyEngine::osBufferLimited_toggled(bool osBufferLimited)
{
    listThread->set_osBufferLimited(osBufferLimited);
    updateBufferCheckbox();
}

void copyEngine::osBufferLimit_editingFinished()
{
    emit send_osBufferLimit(ui->osBufferLimit->value());
}

void copyEngine::showFilterDialog()
{
    if(filters!=NULL)
        filters->exec();
}

void copyEngine::sendNewFilters()
{
    if(filters!=NULL)
        emit send_setFilters(filters->getInclude(),filters->getExclude());
}

void copyEngine::sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    emit send_sendNewRenamingRules(firstRenamingRule,otherRenamingRule);
}

void copyEngine::showRenamingRules()
{
    if(renamingRules==NULL)
    {
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded, can't access to the filters"));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"options not loaded");
        return;
    }
    renamingRules->exec();
}

void copyEngine::get_realBytesTransfered(quint64 realBytesTransfered)
{
    size_for_speed=realBytesTransfered;
}

void copyEngine::newActionInProgess(Ultracopier::EngineActionInProgress action)
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
