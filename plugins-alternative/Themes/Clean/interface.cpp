/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>

#include "interface.h"
#include "ui_interface.h"

InterfacePlugin::InterfacePlugin() :
	ui(new Ui::interface())
{
	ui->setupUi(this);
	currentFile	= 0;
	totalFile	= 0;
	currentSize	= 0;
	totalSize	= 0;
	modeIsForced	= false;
	haveStarted	= false;
	this->show();
	menu=new QMenu(this);
	ui->toolButton->setMenu(menu);
	updateModeAndType();
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
		if(type==File || type==FileAndFolder)
		{
			menu->addAction(ui->actionAddFile);
			connect(ui->actionAddFile,SIGNAL(triggered()),this,SLOT(forcedModeAddFile()));
		}
		if(type==Folder || type==FileAndFolder)
		{
			menu->addAction(ui->actionAddFolder);
			connect(ui->actionAddFolder,SIGNAL(triggered()),this,SLOT(forcedModeAddFolder()));
		}
	}
	else
	{
		if(type==File || type==FileAndFolder)
		{
			menu->addAction(ui->actionAddFileToCopy);
			menu->addAction(ui->actionAddFileToMove);
			connect(ui->actionAddFileToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToCopy()));
			connect(ui->actionAddFileToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFileToMove()));
		}
		if(type==Folder || type==FileAndFolder)
		{
			menu->addAction(ui->actionAddFolderToCopy);
			menu->addAction(ui->actionAddFolderToMove);
			connect(ui->actionAddFolderToCopy,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToCopy()));
			connect(ui->actionAddFolderToMove,SIGNAL(triggered()),this,SLOT(forcedModeAddFolderToMove()));

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
			actionString=tr("Listing");
		break;
		case Copying:
			actionString=tr("Copying");
		break;
		case CopyingAndListing:
			actionString=tr("Listing & Copying");
		break;
		case Idle:
			actionString=tr("Ultracopier");
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

void InterfacePlugin::setFileProgression(const quint64 &id,const quint64 &current,const quint64 &total)
{
	Q_UNUSED(id)
	Q_UNUSED(current)
	Q_UNUSED(total)
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
