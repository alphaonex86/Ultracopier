/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>

#include "interface.h"
#include "ui_interface.h"

InterfacePlugin::InterfacePlugin(FacilityInterface * facilityEngine) :
	ui(new Ui::interface())
{
	ui->setupUi(this);
	remainingSeconds= 0;
	speed		= 0;
	progression_current=0;
	progression_total=0;
	modeIsForced	= false;
	haveStarted	= false;
	this->facilityEngine	= facilityEngine;
	transferModel.setFacilityEngine(facilityEngine);
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

	updateDetails();

	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	connect(&transferModel,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	#endif
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
	this->speed=speed;
	if(ui->more->isChecked())
		ui->label_speed->setText(facilityEngine->speedToString(speed));
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
	remainingTime(remainingSeconds);
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
	this->remainingSeconds=remainingSeconds;

	QString remainingTime;
	if(remainingSeconds>=0)
		remainingTime=facilityEngine->simplifiedRemainingTime(remainingSeconds);
	else
		remainingTime=facilityEngine->translateText(tr("Unknown remaining time"));

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
	this->setWindowTitle(remainingTime+" - "+actionString);

	if(ui->more->isChecked())
		ui->label_remaining_time->setText(remainingTime);
	else
		updateInformations();
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
	progression_current=current;
	progression_total=total;
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
	transferModel.synchronizeItems(returnActions);
	updateInformations();
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
	updateInformations();
}

void InterfacePlugin::setFileProgression(const QList<ProgressionItem> &progressionList)
{
	QList<ProgressionItem> progressionListBis=progressionList;
	transferModel.setFileProgression(progressionListBis);
	updateInformations();
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


void InterfacePlugin::on_more_toggled(bool checked)
{
	Q_UNUSED(checked);
	updateDetails();
	updateInformations();
}

void InterfacePlugin::updateDetails()
{
	ui->text->setShown(!ui->more->isChecked());
	ui->details->setShown(ui->more->isChecked());
	if(ui->more->isChecked())
	{
		this->setMinimumHeight(242);
		this->setMaximumHeight(242);
		ui->more->setIcon(QIcon(":/resources/arrow-up.png"));
	}
	else
	{
		this->setMinimumHeight(168);
		this->setMaximumHeight(168);
		ui->more->setIcon(QIcon(":/resources/arrow-down.png"));
	}

	// usefull under windows
	this->updateGeometry();
	this->update();
	this->adjustSize();

	updateInformations();
}

void InterfacePlugin::updateInformations()
{
	TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
	if(!modeIsForced)
	{
		if(transferModel.totalFile>1)
			ui->label_main->setText(tr("Transferring %1 items (%2)").arg(transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
		else
			ui->label_main->setText(tr("Transferring %1 item (%2)").arg(transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
	}
	else
	{
		if(mode==Copy)
		{
			if(transferModel.totalFile>1)
				ui->label_main->setText(tr("Copying %1 items (%2)").arg(transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
			else
				ui->label_main->setText(tr("Copying %1 item (%2)").arg(transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
		}
		else
		{
			if(transferModel.totalFile>1)
				ui->label_main->setText(tr("Moving %1 items (%2)").arg(transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
			else
				ui->label_main->setText(tr("Moving %1 item (%2)").arg(transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
		}
	}

	if(transfertItem.haveItem)
	{
		if(transfertItem.progressBar_file!=-1)
		{
			ui->progressBar->setRange(0,65535);
			ui->progressBar->setValue(transfertItem.progressBar_file);
		}
		else
			ui->progressBar->setRange(0,0);
	}
	else
	{
		if(haveStarted && transferModel.rowCount()==0)
			ui->progressBar->setValue(65535);
		else if(!haveStarted)
			ui->progressBar->setValue(0);
	}

	if(ui->more->isChecked())
	{
		if(transfertItem.haveItem)
		{
			QString simplifiedFrom=transfertItem.from;
			QString simplifiedTo=transfertItem.to;
			simplifiedFrom.remove(QRegExp("/$"));
			simplifiedTo.remove(QRegExp("/$"));
			simplifiedFrom.replace('\\','/');
			simplifiedTo.replace('\\','/');
			simplifiedFrom.replace(QRegExp("^.*/([^/]+)$"), "\\1");
			simplifiedTo.replace(QRegExp("^.*/([^/]+)$"), "\\1");
			ui->label_file->setText(transfertItem.current_file);
			ui->label_from->setText(QString("<b>%1</b> (%2)").arg(simplifiedFrom).arg(transfertItem.from));
			ui->label_to->setText(QString("<b>%1</b> (%2)").arg(simplifiedTo).arg(transfertItem.to));
			ui->label_items->setText(QString("%1 (%2)").arg(transferModel.totalFile-transferModel.currentFile).arg(facilityEngine->sizeToString(progression_total-progression_current)));
		}
		else
		{
			ui->label_file->setText("");
			ui->label_from->setText("");
			ui->label_to->setText("");
			ui->label_items->setText(QString("%1 (%2)").arg(transferModel.totalFile-transferModel.currentFile).arg(facilityEngine->sizeToString(progression_total-progression_current)));
		}
	}
	else
	{
		if(transfertItem.haveItem)
		{
			QString remainingTime;
			if(remainingSeconds>=0)
				remainingTime=facilityEngine->simplifiedRemainingTime(remainingSeconds);
			else
				remainingTime=facilityEngine->translateText(tr("Unknown remaining time"));
			QString simplifiedFrom=transfertItem.from;
			QString simplifiedTo=transfertItem.to;
			simplifiedFrom.remove(QRegExp("/$"));
			simplifiedTo.remove(QRegExp("/$"));
			simplifiedFrom.replace('\\','/');
			simplifiedTo.replace('\\','/');
			simplifiedFrom.replace(QRegExp("^.*/([^/]+)$"), "\\1");
			simplifiedTo.replace(QRegExp("^.*/([^/]+)$"), "\\1");
			ui->text->setText(tr("from <b>%1</b> (%2) to <b>%3</b> (%4)<br />%5")
					  .arg(transfertItem.from)
					  .arg(simplifiedFrom)
					  .arg(transfertItem.to)
					  .arg(simplifiedTo)
					  .arg(remainingTime)
					  );
		}
		else
			ui->text->setText(tr("In waiting"));
	}
}
