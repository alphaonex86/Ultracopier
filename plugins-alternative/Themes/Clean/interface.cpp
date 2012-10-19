/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
*/

#include "interface.h"
#include "ui_interface.h"

InterfacePlugin::InterfacePlugin(FacilityInterface * facilityEngine) :
	ui(new Ui::interface())
{
	ui->setupUi(this);
	currentFile	= 0;
	totalFile	= 0;
	currentSize	= 0;
	totalSize	= 0;
	modeIsForced	= false;
	haveStarted	= false;
	this->facilityEngine	= facilityEngine;
	this->show();
	menu=new QMenu(this);
	ui->toolButton->setMenu(menu);
	updateModeAndType();

	connect(ui->actionAddFile,SIGNAL(triggered()),this,SLOT(forcedModeAddFile()));
	connect(ui->actionAddFileToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToCopy()));
	connect(ui->actionAddFileToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToMove()));
	connect(ui->actionAddFolderToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToCopy()));
	connect(ui->actionAddFolderToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToMove()));
	connect(ui->actionAddFolder,SIGNAL(triggered()),this,SLOT(forcedModeAddFolder()));
}

InterfacePlugin::~InterfacePlugin()
{
	delete menu;
}

void InterfacePlugin::forcedModeAddFile()
{
	emit userAddFile(mode);
}

void InterfacePlugin::forcedModeAddFolder()
{
	emit userAddFolder(mode);
}

void InterfacePlugin::forcedModeAddFileToCopy()
{
	emit userAddFile(Copy);
}

void InterfacePlugin::forcedModeAddFolderToCopy()
{
	emit userAddFolder(Copy);
}

void InterfacePlugin::forcedModeAddFileToMove()
{
	emit userAddFile(Move);
}

void InterfacePlugin::forcedModeAddFolderToMove()
{
	emit userAddFolder(Move);
}

void InterfacePlugin::updateModeAndType()
{
	menu->clear();
	if(modeIsForced)
	{
		menu->addAction(ui->actionAddFile);
		if(type==FileAndFolder)
			menu->addAction(ui->actionAddFolder);
	}
	else
	{
		menu->addAction(ui->actionAddFileToCopy);
		menu->addAction(ui->actionAddFileToMove);
		if(type==FileAndFolder)
		{
			menu->addAction(ui->actionAddFolderToCopy);
			menu->addAction(ui->actionAddFolderToMove);
		}
	}
}

void InterfacePlugin::closeEvent(QCloseEvent *event)
{
	event->ignore();
	this->hide();
	emit cancel();
}

void InterfacePlugin::detectedSpeed(const quint64 &speed)
{
        Q_UNUSED(speed)
}

QWidget * InterfacePlugin::getOptionsEngineWidget()
{
        return NULL;
}

void InterfacePlugin::getOptionsEngineEnabled(bool isEnabled)
{
        Q_UNUSED(isEnabled)
}

void InterfacePlugin::setCopyType(CopyType type)
{
	this->type=type;
	updateModeAndType();
}

void InterfacePlugin::forceCopyMode(CopyMode mode)
{
	modeIsForced=true;
	this->mode=mode;
	updateModeAndType();
}

void InterfacePlugin::updateTitle()
{
	QString actionString;
	switch(action)
	{
		case Listing:
			actionString=facilityEngine->translateText("Listing");
		break;
		case Copying:
			actionString=facilityEngine->translateText("Copying");
		break;
		case CopyingAndListing:
			actionString=facilityEngine->translateText("Listing and copying");
		break;
		case Idle:
			actionString="Ultracopier";
		break;
	}
	this->setWindowTitle(actionString+" - "+tr("%1/%2 files, %3/%4").arg(currentFile).arg(totalFile).arg(currentSize).arg(totalSize));
}

void InterfacePlugin::actionInProgess(EngineActionInProgress action)
{
	this->action=action;
	switch(action)
	{
		case Copying:
		case CopyingAndListing:
			ui->progressBar->setMaximum(65535);
			ui->progressBar->setMinimum(0);
		break;
		case Listing:
			ui->progressBar->setMaximum(0);
			ui->progressBar->setMinimum(0);
		break;
		case Idle:
			if(haveStarted)
				emit cancel();
		break;
		default:
		break;
	}
	switch(action)
	{
		case Copying:
		case CopyingAndListing:
			haveStarted=true;
		break;
		default:
		break;
	}
}

void InterfacePlugin::newTransferStart(const ItemOfCopyList &item)
{
	ui->text->setText(item.sourceFullPath);
}

void InterfacePlugin::newTransferStop(const quint64 &id)
{
	Q_UNUSED(id)
}

void InterfacePlugin::newFolderListing(const QString &path)
{
	if(action==Listing)
		ui->text->setText(path);
}

void InterfacePlugin::remainingTime(const int &remainingSeconds)
{
	Q_UNUSED(remainingSeconds)
}

void InterfacePlugin::newCollisionAction(const QString &action)
{
	Q_UNUSED(action)
}

void InterfacePlugin::newErrorAction(const QString &action)
{
	Q_UNUSED(action)
}

void InterfacePlugin::errorDetected()
{
}

void InterfacePlugin::setTransferListOperation(TransferListOperation transferListOperation)
{
	Q_UNUSED(transferListOperation)
}

//speed limitation
bool InterfacePlugin::setSpeedLimitation(const qint64 &speedLimitation)
{
	Q_UNUSED(speedLimitation)
        return false;
}

//get information about the copy
void InterfacePlugin::setGeneralProgression(const quint64 &current,const quint64 &total)
{
	ui->progressBar->setValue(((double)current/total)*65535);
}

void InterfacePlugin::setCollisionAction(const QList<QPair<QString,QString> > &list)
{
	Q_UNUSED(list)
}

void InterfacePlugin::setErrorAction(const QList<QPair<QString,QString> > &list)
{
	Q_UNUSED(list)
}

//edit the transfer list
void InterfacePlugin::getActionOnList(const QList<returnActionOnCopyList> &returnActions)
{
	Q_UNUSED(returnActions)
}

void InterfacePlugin::haveExternalOrder()
{
	ui->toolButton->hide();
}

void InterfacePlugin::isInPause(bool isInPause)
{
	//resume in auto the pause
	if(isInPause)
		emit resume();
}

void InterfacePlugin::newLanguageLoaded()
{
	ui->retranslateUi(this);
	updateTitle();
}

/*
  Return[0]: totalFile
  Return[1]: totalSize
  Return[2]: currentFile
  */
void InterfacePlugin::synchronizeItems(const QList<returnActionOnCopyList>& returnActions)
{
	loop_size=returnActions.size();
	index_for_loop=0;
	while(index_for_loop<loop_size)
	{
		const returnActionOnCopyList& action=returnActions.at(index_for_loop);
		switch(action.type)
		{
			case AddingItem:
			{
				totalFile++;
				totalSize+=action.addAction.size;
			}
			break;
			case RemoveItem:
				currentFile++;
			break;
			case PreOperation:
			{
				ItemOfCopyListWithMoreInformations tempItem;
				tempItem.currentProgression=0;
				tempItem.generalData=action.addAction;
				currentSize+=action.addAction.size;
				InternalRunningOperation << tempItem;
			}
			break;
			case Transfer:
			{
				sub_index_for_loop=0;
				sub_loop_size=InternalRunningOperation.size();
				while(sub_index_for_loop<sub_loop_size)
				{
					if(InternalRunningOperation.at(sub_index_for_loop).generalData.id==action.addAction.id)
					{
						InternalRunningOperation[sub_index_for_loop].actionType=action.type;
						break;
					}
					sub_index_for_loop++;
				}
			}
			break;
			case PostOperation:
			{
				sub_index_for_loop=0;
				sub_loop_size=InternalRunningOperation.size();
				while(sub_index_for_loop<sub_loop_size)
				{
					if(InternalRunningOperation.at(sub_index_for_loop).generalData.id==action.addAction.id)
					{
						InternalRunningOperation.removeAt(sub_index_for_loop);
						break;
					}
					sub_index_for_loop++;
				}
			}
			break;
			case CustomOperation:
			{
				bool custom_with_progression=(action.addAction.size==1);
				sub_index_for_loop=0;
				sub_loop_size=InternalRunningOperation.size();
				while(sub_index_for_loop<sub_loop_size)
				{
					if(InternalRunningOperation.at(sub_index_for_loop).generalData.id==action.addAction.id)
					{
						InternalRunningOperation[sub_index_for_loop].actionType=action.type;
						InternalRunningOperation[sub_index_for_loop].custom_with_progression=custom_with_progression;
						InternalRunningOperation[sub_index_for_loop].currentProgression=0;
						break;
					}
					sub_index_for_loop++;
				}
			}
			break;
			default:
				//unknow code, ignore it
			break;
		}
		index_for_loop++;
	}
}

void InterfacePlugin::setFileProgression(const QList<ProgressionItem> &progressionList)
{
	loop_size=InternalRunningOperation.size();
	sub_loop_size=progressionList.size();
	index_for_loop=0;
	while(index_for_loop<loop_size)
	{
		sub_index_for_loop=0;
		while(sub_index_for_loop<sub_loop_size)
		{
			if(progressionList.at(sub_index_for_loop).id==InternalRunningOperation.at(index_for_loop).generalData.id)
			{
				InternalRunningOperation[index_for_loop].generalData.size=progressionList.at(sub_index_for_loop).total;
				InternalRunningOperation[index_for_loop].currentProgression=progressionList.at(sub_index_for_loop).current;
				break;
			}
			sub_index_for_loop++;
		}
		index_for_loop++;
	}
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

