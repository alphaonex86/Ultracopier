/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>
#include <QFileDialog>
#include <QMessageBox>

#include "copyEngine.h"
#include "folderExistsDialog.h"
#include "../../../interface/PluginInterface_CopyEngine.h"

/// \todo do pushed or instant mount point (setDrive, ...)
/// \todo semaphore to prevent dual mkpath
/// \todo repair the mkpath, to use mkpath class before file transfer to have the folder
/** \todo when overwrite with large inode operation, it not start specificly the first in the list
  When that's is finish, send start file at real transfer start, not inode operation start **/

namespace Ui {
	class options;
}

copyEngine::copyEngine(FacilityInterface * facilityInterface) :
	ui(new Ui::options())
{
	listThread=new ListThread(facilityInterface);
	filters=NULL;
	qRegisterMetaType<TransferThread *>("TransferThread *");
	qRegisterMetaType<scanFileOrFolder *>("scanFileOrFolder *");
	qRegisterMetaType<EngineActionInProgress>("EngineActionInProgress");
	qRegisterMetaType<DebugLevel>("DebugLevel");
	qRegisterMetaType<FileExistsAction>("FileExistsAction");
	qRegisterMetaType<FolderExistsAction>("FolderExistsAction");
	qRegisterMetaType<QList<Filters_rules> >("QList<Filters_rules>");

	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	debugDialogWindow.show();
	#endif
	connect(listThread,SIGNAL(actionInProgess(EngineActionInProgress)),	this,SIGNAL(actionInProgess(EngineActionInProgress)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(newFolderListing(QString)),			this,SIGNAL(newFolderListing(QString)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(newCollisionAction(QString)),			this,SIGNAL(newCollisionAction(QString)),		Qt::QueuedConnection);
	connect(listThread,SIGNAL(newErrorAction(QString)),			this,SIGNAL(newErrorAction(QString)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(isInPause(bool)),				this,SIGNAL(isInPause(bool)),				Qt::QueuedConnection);
	connect(listThread,SIGNAL(cancelAll()),					this,SIGNAL(cancelAll()),				Qt::QueuedConnection);
	connect(listThread,SIGNAL(error(QString,quint64,QDateTime,QString)),	this,SIGNAL(error(QString,quint64,QDateTime,QString)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(rmPath(QString)),				this,SIGNAL(rmPath(QString)),				Qt::QueuedConnection);
	connect(listThread,SIGNAL(mkPath(QString)),				this,SIGNAL(mkPath(QString)),				Qt::QueuedConnection);
	connect(listThread,SIGNAL(newActionOnList(QList<returnActionOnCopyList>)),	this,SIGNAL(newActionOnList(QList<returnActionOnCopyList>)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(pushFileProgression(QList<ProgressionItem>)),		this,SIGNAL(pushFileProgression(QList<ProgressionItem>)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(pushGeneralProgression(quint64,quint64)),		this,SIGNAL(pushGeneralProgression(quint64,quint64)),		Qt::QueuedConnection);
	connect(listThread,SIGNAL(syncReady()),						this,SIGNAL(syncReady()),					Qt::QueuedConnection);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	connect(listThread,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),			this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),		Qt::QueuedConnection);
	#endif

	connect(listThread,SIGNAL(send_fileAlreadyExists(QFileInfo,QFileInfo,bool,TransferThread *)),		this,SLOT(fileAlreadyExists(QFileInfo,QFileInfo,bool,TransferThread *)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(send_errorOnFile(QFileInfo,QString,TransferThread *)),			this,SLOT(errorOnFile(QFileInfo,QString,TransferThread *)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(send_folderAlreadyExists(QFileInfo,QFileInfo,bool,scanFileOrFolder *)),	this,SLOT(folderAlreadyExists(QFileInfo,QFileInfo,bool,scanFileOrFolder *)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(send_errorOnFolder(QFileInfo,QString,scanFileOrFolder *)),			this,SLOT(errorOnFolder(QFileInfo,QString,scanFileOrFolder *)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(updateTheDebugInfo(QStringList,QStringList,int)),				this,SLOT(updateTheDebugInfo(QStringList,QStringList,int)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(errorTransferList(QString)),							this,SLOT(errorTransferList(QString)),						Qt::QueuedConnection);
	connect(listThread,SIGNAL(warningTransferList(QString)),						this,SLOT(warningTransferList(QString)),					Qt::QueuedConnection);
	connect(listThread,SIGNAL(mkPathErrorOnFolder(QFileInfo,QString)),					this,SLOT(mkPathErrorOnFolder(QFileInfo,QString)),				Qt::QueuedConnection);
	connect(listThread,SIGNAL(rmPathErrorOnFolder(QFileInfo,QString)),					this,SLOT(rmPathErrorOnFolder(QFileInfo,QString)),				Qt::QueuedConnection);

	connect(this,SIGNAL(signal_pause()),						listThread,SLOT(pause()),				Qt::QueuedConnection);
	connect(this,SIGNAL(signal_resume()),						listThread,SLOT(resume()),				Qt::QueuedConnection);
	connect(this,SIGNAL(signal_skip(quint64)),					listThread,SLOT(skip(quint64)),				Qt::QueuedConnection);
	connect(this,SIGNAL(signal_setCollisionAction(FileExistsAction)),		listThread,SLOT(setAlwaysFileExistsAction(FileExistsAction)),	Qt::QueuedConnection);
	connect(this,SIGNAL(signal_setFolderColision(FolderExistsAction)),		listThread,SLOT(setFolderColision(FolderExistsAction)),	Qt::QueuedConnection);
	connect(this,SIGNAL(signal_removeItems(QList<int>)),				listThread,SLOT(removeItems(QList<int>)),		Qt::QueuedConnection);
	connect(this,SIGNAL(signal_moveItemsOnTop(QList<int>)),				listThread,SLOT(moveItemsOnTop(QList<int>)),		Qt::QueuedConnection);
	connect(this,SIGNAL(signal_moveItemsUp(QList<int>)),				listThread,SLOT(moveItemsUp(QList<int>)),		Qt::QueuedConnection);
	connect(this,SIGNAL(signal_moveItemsDown(QList<int>)),				listThread,SLOT(moveItemsDown(QList<int>)),		Qt::QueuedConnection);
	connect(this,SIGNAL(signal_moveItemsOnBottom(QList<int>)),			listThread,SLOT(moveItemsOnBottom(QList<int>)),		Qt::QueuedConnection);
	connect(this,SIGNAL(signal_exportTransferList(QString)),			listThread,SLOT(exportTransferList(QString)),		Qt::QueuedConnection);
	connect(this,SIGNAL(signal_importTransferList(QString)),			listThread,SLOT(importTransferList(QString)),		Qt::QueuedConnection);
	connect(this,SIGNAL(send_osBufferLimit(uint)),					listThread,SLOT(set_osBufferLimit(uint)),		Qt::QueuedConnection);
	connect(this,SIGNAL(send_setFilters(QList<Filters_rules>,QList<Filters_rules>)),listThread,SLOT(set_setFilters(QList<Filters_rules>,QList<Filters_rules>)),		Qt::QueuedConnection);

	connect(this,SIGNAL(queryOneNewDialog()),SLOT(showOneNewDialog()),Qt::QueuedConnection);
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
}

copyEngine::~copyEngine()
{
	if(filters!=NULL)
		delete filters;
	stopIt=true;
	delete listThread;
        delete ui;
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
	connect(tempWidget,		SIGNAL(destroyed()),		this,			SLOT(resetTempWidget()));
	ui->setupUi(tempWidget);
	//conect the ui widget
/*	connect(ui->doRightTransfer,	SIGNAL(toggled(bool)),		&threadOfTheTransfer,	SLOT(setRightTransfer(bool)));
	connect(ui->keepDate,		SIGNAL(toggled(bool)),		&threadOfTheTransfer,	SLOT(setKeepDate(bool)));
	connect(ui->blockSize,		SIGNAL(valueChanged(int)),	&threadOfTheTransfer,	SLOT(setBlockSize(int)));*/
	connect(ui->autoStart,		SIGNAL(toggled(bool)),		this,			SLOT(setAutoStart(bool)));
	connect(ui->checkBoxDestinationFolderExists,	SIGNAL(toggled(bool)),		this,			SLOT(setCheckDestinationFolderExists(bool)));
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
	set_setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
	return true;
}

//to have interface widget to do modal dialog
void copyEngine::setInterfacePointer(QWidget * interface)
{
	this->interface=interface;
	filters=new Filters(interface);
	connect(ui->doRightTransfer,		SIGNAL(toggled(bool)),		this,SLOT(setRightTransfer(bool)));
	connect(ui->keepDate,			SIGNAL(toggled(bool)),		this,SLOT(setKeepDate(bool)));
	connect(ui->blockSize,			SIGNAL(valueChanged(int)),	this,SLOT(setBlockSize(int)));
	connect(ui->autoStart,			SIGNAL(toggled(bool)),		this,SLOT(setAutoStart(bool)));
	connect(ui->doChecksum,			SIGNAL(toggled(bool)),		this,SLOT(doChecksum_toggled(bool)));
	connect(ui->checksumIgnoreIfImpossible,	SIGNAL(toggled(bool)),		this,SLOT(set_checksumIgnoreIfImpossible(bool)));
	connect(ui->checksumOnlyOnError,	SIGNAL(toggled(bool)),		this,SLOT(checksumOnlyOnError_toggled(bool)));
	connect(ui->osBuffer,			SIGNAL(toggled(bool)),		this,SLOT(osBuffer_toggled(bool)));
	connect(ui->osBufferLimited,		SIGNAL(toggled(bool)),		this,SLOT(osBufferLimited_toggled(bool)));
	connect(ui->osBufferLimit,		SIGNAL(editingFinished()),	this,SLOT(osBufferLimit_editingFinished()));

	connect(filters,SIGNAL(sendNewFilters(QStringList,QStringList,QStringList,QStringList)),this,SLOT(sendNewFilters()));
	connect(ui->filters,SIGNAL(clicked()),this,SLOT(showFilterDialog()));

	filters->setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString destination = QFileDialog::getExistingDirectory(interface,tr("Select destination directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(destination.isEmpty() || destination.isNull() || destination=="")
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Canceled by the user");
		return false;
	}
	return listThread->newCopy(sources,destination);
}

bool copyEngine::newCopy(const QStringList &sources,const QString &destination)
{
	return listThread->newCopy(sources,destination);
}

bool copyEngine::newMove(const QStringList &sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QString destination = QFileDialog::getExistingDirectory(interface,tr("Select destination directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(destination.isEmpty() || destination.isNull() || destination=="")
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Canceled by the user");
		return false;
	}
	return listThread->newMove(sources,destination);
}

bool copyEngine::newMove(const QStringList &sources,const QString &destination)
{
	return listThread->newMove(sources,destination);
}

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
	tempItem.first=tr("Ask");tempItem.second="ask";list << tempItem;
	tempItem.first=tr("Skip");tempItem.second="skip";list << tempItem;
	tempItem.first=tr("Overwrite");tempItem.second="overwrite";list << tempItem;
	tempItem.first=tr("Overwrite if newer");tempItem.second="overwriteIfNewer";list << tempItem;
	tempItem.first=tr("Overwrite if the last modification dates are different");tempItem.second="overwriteIfNotSameModificationDate";list << tempItem;
	tempItem.first=tr("Rename");tempItem.second="rename";list << tempItem;
	return list;
}

QList<QPair<QString,QString> > copyEngine::getErrorAction()
{
	QPair<QString,QString> tempItem;
	QList<QPair<QString,QString> > list;
	tempItem.first=tr("Ask");tempItem.second="ask";list << tempItem;
	tempItem.first=tr("Skip");tempItem.second="skip";list << tempItem;
	tempItem.first=tr("Put to end of the list");tempItem.second="putToEndOfTheList";list << tempItem;
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
		ui->doChecksum->setChecked(doChecksum);
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
		ui->osBuffer->setChecked(osBuffer);
	this->osBuffer=osBuffer;
}

void copyEngine::set_osBufferLimited(bool osBufferLimited)
{
	listThread->set_osBufferLimited(osBufferLimited);
	if(uiIsInstalled)
		ui->osBufferLimited->setChecked(osBufferLimited);
	this->osBufferLimited=osBufferLimited;
}

void copyEngine::set_osBufferLimit(unsigned int osBufferLimit)
{
	emit send_osBufferLimit(osBufferLimit);
	if(uiIsInstalled)
		ui->osBufferLimit->setValue(osBufferLimit);
	this->osBufferLimit=osBufferLimit;
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

bool copyEngine::userAddFolder(const CopyMode &mode)
{
	QString source = QFileDialog::getExistingDirectory(interface,tr("Select source directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(source.isEmpty() || source.isNull() || source=="")
		return false;
	if(mode==Copy)
		return newCopy(QStringList() << source);
	else
		return newMove(QStringList() << source);
}

bool copyEngine::userAddFile(const CopyMode &mode)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QStringList sources = QFileDialog::getOpenFileNames(
		interface,
		tr("Select one or more files to open"),
		"",
		tr("All files")+" (*)");
	if(sources.isEmpty())
		return false;
	if(mode==Copy)
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	stopIt=true;
	listThread->cancel();
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

void copyEngine::exportTransferList()
{
	QString fileName = QFileDialog::getSaveFileName(NULL,tr("Save transfer list"),"transfer-list.lst",tr("Transfer list")+" (*.lst)");
	if(fileName.isEmpty())
		return;
	emit signal_exportTransferList(fileName);
}

void copyEngine::importTransferList()
{
	QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open transfer list"),"transfer-list.lst",tr("Transfer list")+" (*.lst)");
	if(fileName.isEmpty())
		return;
	emit signal_importTransferList(fileName);
}

void copyEngine::warningTransferList(const QString &warning)
{
	QMessageBox::warning(interface,tr("Error"),warning);
}

void copyEngine::errorTransferList(const QString &error)
{
	QMessageBox::critical(interface,tr("Error"),error);
}

bool copyEngine::setSpeedLimitation(const qint64 &speedLimitation)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"maxSpeed: "+QString::number(speedLimitation));
	maxSpeed=speedLimitation;
	return listThread->setSpeedLimitation(speedLimitation);
}

void copyEngine::setCollisionAction(const QString &action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"action: "+action);
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"action: "+action);
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, retranslate the widget options");
	if(tempWidget!=NULL)
		ui->retranslateUi(tempWidget);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"ui not loaded!");
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

void copyEngine::osBuffer_toggled(bool osBuffer)
{
	listThread->set_osBuffer(osBuffer);
	ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void copyEngine::osBufferLimited_toggled(bool osBufferLimited)
{
	listThread->set_osBufferLimited(osBufferLimited);
	ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
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
