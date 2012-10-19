/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QMessageBox>

#include "interface.h"
#include "ui_interface.h"

InterfacePlugin::InterfacePlugin(FacilityInterface * facilityEngine) :
	ui(new Ui::interfaceCopy())
{
	this->facilityEngine=facilityEngine;
	ui->setupUi(this);
	ui->TransferList->setModel(&transferModel);
	transferModel.setFacilityEngine(facilityEngine);

	currentFile		= 0;
	totalFile		= 0;
	currentSize		= 0;
	totalSize		= 0;
	this->show();
	storeIsInPause		= false;
	isInPause(false);
	modeIsForced		= false;
	haveStarted		= false;
	speedString		= facilityEngine->speedToString(0);
	ui->toolButtonMenu->setMenu(&menu);
	
	connect(ui->actionAddFile,SIGNAL(triggered()),this,SLOT(forcedModeAddFile()));
	connect(ui->actionAddFileToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToCopy()));
	connect(ui->actionAddFileToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToMove()));
	connect(ui->actionAddFolderToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToCopy()));
	connect(ui->actionAddFolderToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToMove()));
	connect(ui->actionAddFolder,SIGNAL(triggered()),this,SLOT(forcedModeAddFolder()));

	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	connect(&transferModel,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	#endif
}

InterfacePlugin::~InterfacePlugin()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
}

QWidget * InterfacePlugin::getOptionsEngineWidget()
{
	return &optionEngineWidget;
}

void InterfacePlugin::getOptionsEngineEnabled(bool isEnabled)
{
	Q_UNUSED(isEnabled);
}

/// \brief set if transfer list is exportable/importable
void InterfacePlugin::setTransferListOperation(TransferListOperation transferListOperation)
{
	Q_UNUSED(transferListOperation);
}

void InterfacePlugin::closeEvent(QCloseEvent *event)
{
	event->ignore();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->hide();
	emit cancel();
}

void InterfacePlugin::updateOverallInformation()
{
	ui->overall->setText(tr("Total: %1 of %2").arg(facilityEngine->sizeToString(currentSize)).arg(facilityEngine->sizeToString(totalSize)));
	ui->labelNumberFile->setText(tr("%1 of %2").arg(currentFile).arg(totalFile));
}

void InterfacePlugin::actionInProgess(EngineActionInProgress action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"start: "+QString::number(action));
	this->action=action;
	ui->pauseButton->setEnabled(action!=Idle);
	switch(action)
	{
		case Copying:
		case CopyingAndListing:
			ui->progressBar_all->setMaximum(65535);
			ui->progressBar_all->setMinimum(0);
		break;
		case Listing:
			ui->progressBar_all->setMaximum(0);
			ui->progressBar_all->setMinimum(0);
		break;
		case Idle:
			if(haveStarted)
				emit cancel();
		break;
		default:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Very wrong switch case!");
		break;
	}
	switch(action)
	{
		case Copying:
		case CopyingAndListing:
			haveStarted=true;
		break;
		case Idle:
			ui->cancelButton->setText(facilityEngine->translateText("Quit"));
		break;
		default:
		break;
	}
}

void InterfacePlugin::newFolderListing(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(action==Listing)
		ui->from->setText(path);
}

void InterfacePlugin::detectedSpeed(const quint64 &speed)//in byte per seconds
{
	speedString=facilityEngine->speedToString(speed);
}

void InterfacePlugin::remainingTime(const int &remainingSeconds)
{
	if(remainingSeconds==-1)
		ui->labelTimeRemaining->setText("<html><body>&#8734;</body></html>");
	else
	{
		TimeDecomposition time=facilityEngine->secondsToTimeDecomposition(remainingSeconds);
		ui->labelTimeRemaining->setText(QString::number(time.hour)+":"+QString::number(time.minute)+":"+QString::number(time.second));
	}
}

void InterfacePlugin::newCollisionAction(const QString &action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	Q_UNUSED(action);
}

void InterfacePlugin::newErrorAction(const QString &action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	Q_UNUSED(action);
}

void InterfacePlugin::errorDetected()
{
}

//speed limitation
bool InterfacePlugin::setSpeedLimitation(const qint64 &speedLimitation)
{
	if(speedLimitation>0)
		emit newSpeedLimitation(0);
	return true;
}

//get information about the copy
void InterfacePlugin::setGeneralProgression(const quint64 &current,const quint64 &total)
{
	currentSize=current;
	totalSize=total;
	if(total>0)
	{
		int newIndicator=((double)current/total)*65535;
		ui->progressBar_all->setValue(newIndicator);
	}
	else
		ui->progressBar_all->setValue(0);
}

void InterfacePlugin::setCollisionAction(const QList<QPair<QString,QString> > &list)
{
	Q_UNUSED(list);
}

void InterfacePlugin::setErrorAction(const QList<QPair<QString,QString> > &list)
{
	Q_UNUSED(list);
}

void InterfacePlugin::setCopyType(CopyType type)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->type=type;
	updateModeAndType();
}

void InterfacePlugin::forceCopyMode(CopyMode mode)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	modeIsForced=true;
	this->mode=mode;
	if(mode==Copy)
		this->setWindowTitle("Ultracopier - "+facilityEngine->translateText("Copy"));
	else
		this->setWindowTitle("Ultracopier - "+facilityEngine->translateText("Move"));
	updateModeAndType();
}

void InterfacePlugin::updateTitle()
{
	QString startString;
	if(action==Copying || action==CopyingAndListing)
		startString=tr("%1% done").arg(((double)currentSize/totalSize)*100);
	else
		startString="Ultracopier";
	startString+=" - ";
	if(mode==Copy)
		this->setWindowTitle(startString+facilityEngine->translateText("Copy")+" ("+speedString+")");
	else
		this->setWindowTitle(startString+facilityEngine->translateText("Move")+" ("+speedString+")");
}

void InterfacePlugin::haveExternalOrder()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
//	ui->moreButton->toggle();
}

void InterfacePlugin::isInPause(bool isInPause)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"isInPause: "+QString::number(isInPause));
	//resume in auto the pause
	storeIsInPause=isInPause;
	if(isInPause)
		ui->pauseButton->setText(facilityEngine->translateText("Resume"));
	else
		ui->pauseButton->setText(facilityEngine->translateText("Pause"));
}

void InterfacePlugin::updateCurrentFileInformation()
{
	TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
	if(transfertItem.haveItem)
	{
		ui->from->setText(transfertItem.from);
		//commented because not displayed on this interface
		//ui->to->setText(transfertItem.to);
		//ui->current_file->setText(transfertItem.current_file);
		if(transfertItem.progressBar_file!=-1)
		{
			ui->progressBar_file->setRange(0,65535);
			ui->progressBar_file->setValue(transfertItem.progressBar_file);
		}
		else
			ui->progressBar_file->setRange(0,0);
	}
	else
	{
		ui->from->setText("");
		//commented because not displayed on this interface
		//ui->to->setText("");
		//ui->current_file->setText("-");
		if(haveStarted && transferModel.rowCount()==0)
			ui->progressBar_file->setValue(65535);
		else if(!haveStarted)
			ui->progressBar_file->setValue(0);
	}
}


void InterfacePlugin::on_cancelButton_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->hide();
	emit cancel();
}


void InterfacePlugin::on_pauseButton_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(storeIsInPause)
		emit resume();
	else
		emit pause();
}

void InterfacePlugin::on_skipButton_clicked()
{
	TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
	if(transfertItem.haveItem)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("skip at running: %1").arg(transfertItem.id));
		emit skip(transfertItem.id);
	}
	else
	{
		if(transferModel.rowCount()>1)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("skip at idle: %1").arg(transferModel.firstId()));
			emit skip(transferModel.firstId());
		}
		else
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to skip the transfer, because no transfer running");
	}
}

void InterfacePlugin::updateModeAndType()
{
	menu.clear();
	if(modeIsForced)
	{
		menu.addAction(ui->actionAddFile);
		if(type==FileAndFolder)
			menu.addAction(ui->actionAddFolder);
	}
	else
	{
		menu.addAction(ui->actionAddFileToCopy);
		menu.addAction(ui->actionAddFileToMove);
		if(type==FileAndFolder)
		{
			menu.addAction(ui->actionAddFolderToCopy);
			menu.addAction(ui->actionAddFolderToMove);
		}
	}
}

void InterfacePlugin::forcedModeAddFile()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFile(mode);
}

void InterfacePlugin::forcedModeAddFolder()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFolder(mode);
}

void InterfacePlugin::forcedModeAddFileToCopy()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFile(Copy);
}

void InterfacePlugin::forcedModeAddFolderToCopy()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFolder(Copy);
}

void InterfacePlugin::forcedModeAddFileToMove()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFile(Move);
}

void InterfacePlugin::forcedModeAddFolderToMove()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit userAddFolder(Move);
}

//set the translate
void InterfacePlugin::newLanguageLoaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(modeIsForced)
		forceCopyMode(mode);
	ui->retranslateUi(this);
	if(haveStarted)
		updateCurrentFileInformation();
	updateOverallInformation();
}

/*
  Return[0]: totalFile
  Return[1]: totalSize
  Return[2]: currentFile
  */
void InterfacePlugin::getActionOnList(const QList<returnActionOnCopyList>& returnActions)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, returnActions.size(): "+QString::number(returnActions.size()));
	QList<quint64> returnValue=transferModel.synchronizeItems(returnActions);
	totalFile+=returnValue[0];
	totalSize+=returnValue[1];
	currentFile+=returnValue[2];
	if(transferModel.rowCount()==0)
	{
		ui->skipButton->setEnabled(false);
		ui->progressBar_all->setValue(65535);
		ui->progressBar_file->setValue(65535);
		currentSize=totalSize;
	}
	else
		ui->skipButton->setEnabled(true);
	updateOverallInformation();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"transferModel.rowCount(): "+QString::number(transferModel.rowCount()));
}

void InterfacePlugin::setFileProgression(const QList<ProgressionItem> &progressionList)
{
	QList<ProgressionItem> progressionListBis=progressionList;
	transferModel.setFileProgression(progressionListBis);
	updateCurrentFileInformation();
}

InterfacePlugin::currentTransfertItem InterfacePlugin::getCurrentTransfertItem()
{
	currentTransfertItem returnItem;
	returnItem.haveItem=InternalRunningOperation.size()>0;
	if(returnItem.haveItem)
	{
		const ItemOfCopyListWithMoreInformations &itemTransfer=InternalRunningOperation.first();
		returnItem.from=itemTransfer.generalData.sourceFullPath;
		returnItem.to=itemTransfer.generalData.destinationFullPath;
		returnItem.current_file=itemTransfer.generalData.destinationFileName+", "+facilityEngine->sizeToString(itemTransfer.generalData.size);
		switch(itemTransfer.actionType)
		{
			case CustomOperation:
			if(!itemTransfer.custom_with_progression)
				returnItem.progressBar_file=0;
			else
			{
				if(itemTransfer.generalData.size>0)
					returnItem.progressBar_file=((double)itemTransfer.currentProgression/itemTransfer.generalData.size)*65535;
				else
					returnItem.progressBar_file=0;
			}
			break;
			case Transfer:
			if(itemTransfer.generalData.size>0)
				returnItem.progressBar_file=((double)itemTransfer.currentProgression/itemTransfer.generalData.size)*65535;
			else
				returnItem.progressBar_file=0;
			break;
			case PostOperation:
				returnItem.progressBar_file=65535;
			break;
			default:
				returnItem.progressBar_file=0;
		}
	}
	return returnItem;
}
