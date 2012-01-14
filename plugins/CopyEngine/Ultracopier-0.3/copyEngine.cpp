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

/// \todo do pushed or instant mount point (setDrive, ...)
/// \todo semaphore to prevent dual mkpath
/// \todo repair the mkpath, to use mkpath class before file transfer to have the folder
/** \todo when overwrite with large inode operation, it not start specificly the first in the list
  When that's is finish, send start file at real transfer start, not inode operation start **/

namespace Ui {
	class options;
}

copyEngine::copyEngine() :
	ui(new Ui::options())
{
	listThread=new ListThread();
	qRegisterMetaType<TransferThread *>("TransferThread *");
	qRegisterMetaType<scanFileOrFolder *>("scanFileOrFolder *");
	qRegisterMetaType<EngineActionInProgress>("EngineActionInProgress");
	qRegisterMetaType<DebugLevel>("DebugLevel");
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	debugDialogWindow.show();
	#endif
	connect(listThread,SIGNAL(actionInProgess(EngineActionInProgress)),	this,SIGNAL(actionInProgess(EngineActionInProgress)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(newTransferStart(ItemOfCopyList)),		this,SIGNAL(newTransferStart(ItemOfCopyList)),		Qt::QueuedConnection);
	connect(listThread,SIGNAL(newTransferStop(quint64)),			this,SIGNAL(newTransferStop(quint64)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(newFolderListing(QString)),			this,SIGNAL(newFolderListing(QString)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(newCollisionAction(QString)),			this,SIGNAL(newCollisionAction(QString)),		Qt::QueuedConnection);
	connect(listThread,SIGNAL(newErrorAction(QString)),			this,SIGNAL(newErrorAction(QString)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(isInPause(bool)),				this,SIGNAL(isInPause(bool)),				Qt::QueuedConnection);
	connect(listThread,SIGNAL(newActionOnList()),				this,SIGNAL(newActionOnList()),				Qt::QueuedConnection);
	connect(listThread,SIGNAL(cancelAll()),					this,SIGNAL(cancelAll()),				Qt::QueuedConnection);
	connect(listThread,SIGNAL(error(QString,quint64,QDateTime,QString)),	this,SIGNAL(error(QString,quint64,QDateTime,QString)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(rmPath(QString)),				this,SIGNAL(rmPath(QString)),				Qt::QueuedConnection);
	connect(listThread,SIGNAL(mkPath(QString)),				this,SIGNAL(mkPath(QString)),				Qt::QueuedConnection);
	#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
	connect(listThread,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),			this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),		Qt::QueuedConnection);
	#endif
	connect(listThread,SIGNAL(send_fileAlreadyExists(QFileInfo,QFileInfo,bool,TransferThread *)),		this,SLOT(fileAlreadyExists(QFileInfo,QFileInfo,bool,TransferThread *)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(send_errorOnFile(QFileInfo,QString,TransferThread *)),			this,SLOT(errorOnFile(QFileInfo,QString,TransferThread *)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(send_folderAlreadyExists(QFileInfo,QFileInfo,bool,scanFileOrFolder *)),	this,SLOT(folderAlreadyExists(QFileInfo,QFileInfo,bool,scanFileOrFolder *)),	Qt::QueuedConnection);
	connect(listThread,SIGNAL(send_errorOnFolder(QFileInfo,QString,scanFileOrFolder *)),			this,SLOT(errorOnFolder(QFileInfo,QString,scanFileOrFolder *)),			Qt::QueuedConnection);
	connect(listThread,SIGNAL(updateTheDebugInfo(QStringList,QStringList,int)),				this,SLOT(updateTheDebugInfo(QStringList,QStringList,int)),			Qt::QueuedConnection);
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
}

copyEngine::~copyEngine()
{
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
	return true;
}

//to have interface widget to do modal dialog
void copyEngine::setInterfacePointer(QWidget * interface)
{
	this->interface=interface;
}

bool copyEngine::haveSameSource(QStringList sources)
{
	return listThread->haveSameSource(sources);
}

bool copyEngine::haveSameDestination(QString destination)
{
	return listThread->haveSameDestination(destination);
}

bool copyEngine::newCopy(QStringList sources)
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

bool copyEngine::newCopy(QStringList sources,QString destination)
{
	return listThread->newCopy(sources,destination);
}

bool copyEngine::newMove(QStringList sources)
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

bool copyEngine::newMove(QStringList sources,QString destination)
{
	return listThread->newMove(sources,destination);
}

//get information about the copy
QPair<quint64,quint64> copyEngine::getGeneralProgression()
{
	return listThread->getGeneralProgression();
}

returnSpecificFileProgression copyEngine::getFileProgression(quint64 id)
{
	return listThread->getFileProgression(id);
}

quint64 copyEngine::realByteTransfered()
{
	return listThread->realByteTransfered();
}

//edit the transfer list
QList<returnActionOnCopyList> copyEngine::getActionOnList()
{
	return listThread->getActionOnList();
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
	tempItem.first=tr("Overwrite if last modification dates are different");tempItem.second="overwriteIfNotSameModificationDate";list << tempItem;
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

//transfer list
QList<ItemOfCopyList> copyEngine::getTransferList()
{
	return listThread->getTransferList();
}

ItemOfCopyList copyEngine::getTransferListEntry(quint64 id)
{
	return listThread->getTransferListEntry(id);
}

void copyEngine::setDrive(QStringList drives)
{
	listThread->setDrive(drives);
}

bool copyEngine::userAddFolder(CopyMode mode)
{
	QString source = QFileDialog::getExistingDirectory(interface,tr("Select source directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(source.isEmpty() || source.isNull() || source=="")
		return false;
	if(mode==Copy)
		return newCopy(QStringList() << source);
	else
		return newMove(QStringList() << source);
}

bool copyEngine::userAddFile(CopyMode mode)
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
	listThread->pause();
}

void copyEngine::resume()
{
	listThread->resume();
}

void copyEngine::skip(quint64 id)
{
	listThread->skip(id);
}

void copyEngine::cancel()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	stopIt=true;
	listThread->cancel();
}

void copyEngine::removeItems(QList<int> ids)
{
	listThread->removeItems(ids);
}

void copyEngine::moveItemsOnTop(QList<int> ids)
{
	listThread->moveItemsOnTop(ids);
}

void copyEngine::moveItemsUp(QList<int> ids)
{
	listThread->moveItemsUp(ids);
}

void copyEngine::moveItemsDown(QList<int> ids)
{
	listThread->moveItemsDown(ids);
}

void copyEngine::moveItemsOnBottom(QList<int> ids)
{
	listThread->moveItemsOnBottom(ids);
}

bool copyEngine::setSpeedLimitation(qint64 speedLimitation)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"maxSpeed: "+QString::number(speedLimitation));
	maxSpeed=speedLimitation;
	return listThread->setSpeedLimitation(speedLimitation);
}

void copyEngine::setCollisionAction(QString action)
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
	listThread->setCollisionAction(alwaysDoThisActionForFileExists);
}

void copyEngine::setErrorAction(QString action)
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
	listThread->setFolderColision(alwaysDoThisActionForFolderExists);
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
