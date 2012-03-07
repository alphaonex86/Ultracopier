/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>
#include <QMessageBox>

#include "interface.h"
#include "ui_interface.h"

InterfacePlugin::InterfacePlugin(FacilityInterface * facilityEngine) :
	ui(new Ui::interfaceCopy())
{
	this->facilityEngine=facilityEngine;
	ui->setupUi(this);
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
	ui->overall->setText(tr("Total: %3 of %4").arg(facilityEngine->sizeToString(currentSize)).arg(facilityEngine->sizeToString(totalSize)));
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
			ui->cancelButton->setText(tr("Quit"));
		break;
		default:
		break;
	}
}

void InterfacePlugin::newTransferStart(const ItemOfCopyList &item)
{
	ItemOfCopyListWithMoreInformations newItem;
	newItem.currentProgression=0;
	newItem.generalData=item;
	currentProgressList<<newItem;
	ui->skipButton->setEnabled(true);
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Start transfer new item: "+QString::number(currentProgressList.last().generalData.id)+", with size: "+QString::number(currentProgressList.last().generalData.size));
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size)
	{
		if(graphicItemList.at(index).id==item.id)
		{
			graphicItemList.at(index).item->setIcon(0,QIcon(":/resources/player_play.png"));
			//ui->CopyList->scrollToItem(graphicItemList.at(index).item);
			break;
		}
		index++;
	}
	updateCurrentFileInformation();
}

void InterfacePlugin::newTransferStop(const quint64 &id)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size)
	{
		if(graphicItemList.at(index).id==id)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"item to remove found");
			graphicItemList.at(index).item->setIcon(0,QIcon(":/resources/checkbox.png"));
			ui->CopyList->scrollToItem(graphicItemList.at(index).item);
			currentFile++;
			updateOverallInformation();
			break;
		}
		index++;
	}
	index=0;
	loop_size=currentProgressList.size();
	while(index<loop_size)
	{
		if(currentProgressList.at(index).generalData.id==id)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Remove item: "+QString::number(currentProgressList.at(index).generalData.id)+", with size: "+QString::number(currentProgressList.at(index).generalData.size));
			currentProgressList.removeAt(index);
			updateCurrentFileInformation();
			break;
		}
		index++;
	}
	if(currentProgressList.size()==0)
		ui->skipButton->setEnabled(false);
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

void InterfacePlugin::setFileProgression(const quint64 &id,const quint64 &current,const quint64 &total)
{
	index=0;
	loop_size=currentProgressList.size();
	while(index<loop_size)
	{
		if(currentProgressList.at(index).generalData.id==id)
		{
			currentProgressList[index].generalData.size=total;
			currentProgressList[index].currentProgression=current;
			if(index==0)
				updateCurrentFileInformation();
			return;
		}
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to found the file");
}

void InterfacePlugin::setCollisionAction(const QList<QPair<QString,QString> > &list)
{
	Q_UNUSED(list);
}

void InterfacePlugin::setErrorAction(const QList<QPair<QString,QString> > &list)
{
	Q_UNUSED(list);
}

//edit the transfer list
void InterfacePlugin::getActionOnList(const QList<returnActionOnCopyList> &returnActions)
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, returnActions.size(): "+QString::number(returnActions.size()));
	indexAction=0;
	loop_size=returnActions.size();
	while(indexAction<loop_size)
	{
		if(returnActions.at(indexAction).type==AddingItem)
		{
			graphicItem newItem;
			newItem.id=returnActions.at(indexAction).addAction.id;
			newItem.item=new QTreeWidgetItem(QStringList() << returnActions.at(indexAction).addAction.sourceFullPath << facilityEngine->sizeToString(returnActions.at(indexAction).addAction.size) << returnActions.at(indexAction).addAction.destinationFullPath);
			ui->CopyList->addTopLevelItem(newItem.item);
			totalFile++;
			graphicItemList<<newItem;
			totalSize+=returnActions.at(indexAction).addAction.size;
			//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Add item: "+QString::number(newItem.id)+", with size: "+QString::number(returnActions.at(indexAction).addAction.size));
			updateOverallInformation();
		}
		else
		{
			index=0;
			loop_sub_size=graphicItemList.size();
			while(index<loop_sub_size)
			{
				if(graphicItemList.at(index).id==returnActions.at(indexAction).userAction.id)
				{
					int pos=ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item);
					if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
					{
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Warning, graphical item not located");
					}
					else
					{
						bool isSelected=graphicItemList.at(index).item->isSelected();
						switch(returnActions.at(indexAction).userAction.type)
						{
							case MoveItem:
								ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"MoveItem: "+QString::number(returnActions.at(indexAction).userAction.id)+", position: "+QString::number(returnActions.at(indexAction).position));
								ui->CopyList->insertTopLevelItem(returnActions.at(indexAction).position,ui->CopyList->takeTopLevelItem(pos));
								graphicItemList.at(index).item->setSelected(isSelected);
								graphicItemList.move(index,returnActions.at(indexAction).position);
							break;
							case RemoveItem:
								ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Remove: "+QString::number(returnActions.at(indexAction).userAction.id));
								graphicItemList.at(index).item->setIcon(0,QIcon(":/resources/checkbox.png"));
								updateOverallInformation();
							break;
						}
					}
					break;
				}
				index++;
			}
		}
		indexAction++;
	}
	if(graphicItemList.size()==0)
	{
		ui->progressBar_all->setValue(65535);
		ui->progressBar_file->setValue(65535);
		currentSize=totalSize;
		updateOverallInformation();
	}
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"graphicItemList.size(): "+QString::number(graphicItemList.size()));
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
		this->setWindowTitle("Ultracopier - "+tr("Copy"));
	else
		this->setWindowTitle("Ultracopier - "+tr("Move"));
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
	if(currentProgressList.size()>0)
	{
		ui->from->setText(currentProgressList.first().generalData.sourceFullPath);
		if(currentProgressList.first().generalData.size>0)
			ui->progressBar_file->setValue(((double)currentProgressList.first().currentProgression/currentProgressList.first().generalData.size)*65535);
		else
			ui->progressBar_file->setValue(0);
	}
	else
	{
		ui->from->setText("-");
		ui->progressBar_file->setValue(65535);
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
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(currentProgressList.size()>0)
		emit skip(currentProgressList.first().generalData.id);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"unable to skip the transfer, because no transfer running");
}

void InterfacePlugin::updateModeAndType()
{
	menu.clear();
	if(modeIsForced)
	{
		if(type==File || type==FileAndFolder)
		{
			menu.addAction(ui->actionAddFile);
			connect(ui->actionAddFile,SIGNAL(triggered()),this,SLOT(forcedModeAddFile()));
		}
		if(type==Folder || type==FileAndFolder)
		{
			menu.addAction(ui->actionAddFolder);
			connect(ui->actionAddFolder,SIGNAL(triggered()),this,SLOT(forcedModeAddFolder()));
		}
	}
	else
	{
		if(type==File || type==FileAndFolder)
		{
			menu.addAction(ui->actionAddFileToCopy);
			menu.addAction(ui->actionAddFileToMove);
			connect(ui->actionAddFileToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToCopy()));
			connect(ui->actionAddFileToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToMove()));
		}
		if(type==Folder || type==FileAndFolder)
		{
			menu.addAction(ui->actionAddFolderToCopy);
			menu.addAction(ui->actionAddFolderToMove);
			connect(ui->actionAddFolderToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToCopy()));
			connect(ui->actionAddFolderToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToMove()));

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
