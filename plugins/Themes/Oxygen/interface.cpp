/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>
#include <QMessageBox>

#include "interface.h"
#include "ui_interface.h"

InterfacePlugin::InterfacePlugin(bool checkBoxShowSpeed,FacilityInterface * facilityEngine) :
	ui(new Ui::interfaceCopy())
{
	this->facilityEngine=facilityEngine;
	ui->setupUi(this);
	ui->tabWidget->setCurrentIndex(0);
	ui->checkBoxShowSpeed->setChecked(checkBoxShowSpeed);
	currentFile	= 0;
	totalFile	= 0;
	currentSize	= 0;
	totalSize	= 0;
	haveError	= false;
	this->show();
	menu=new QMenu(this);
	ui->add->setMenu(menu);
	on_checkBoxShowSpeed_toggled(ui->checkBoxShowSpeed->isChecked());
	currentSpeed	= -1;
	updateSpeed();
	storeIsInPause	= false;
	isInPause(false);
	modeIsForced	= false;
	haveStarted	= false;
	connect(ui->limitSpeed,		SIGNAL(valueChanged(int)),	this,	SLOT(uiUpdateSpeed()));
	connect(ui->checkBox_limitSpeed,SIGNAL(toggled(bool)),		this,	SLOT(uiUpdateSpeed()));

	//setup the search part
	closeTheSearchBox();
	TimerForSearch  = new QTimer(this);
	TimerForSearch->setInterval(500);
	TimerForSearch->setSingleShot(true);
	searchShortcut  = new QShortcut(QKeySequence("Ctrl+F"),this);
	searchShortcut2 = new QShortcut(QKeySequence("F3"),this);

	//connect the search part
	connect(TimerForSearch,			SIGNAL(timeout()),	this,	SLOT(hilightTheSearch()));
	connect(searchShortcut,			SIGNAL(activated()),	this,	SLOT(searchBoxShortcut()));
	connect(searchShortcut2,		SIGNAL(activated()),	this,	SLOT(on_pushButtonSearchNext_clicked()));
	connect(ui->pushButtonCloseSearch,	SIGNAL(clicked()),	this,	SLOT(closeTheSearchBox()));

	//reload directly untranslatable text
	newLanguageLoaded();

	//unpush the more button
	ui->moreButton->setChecked(false);
	on_moreButton_toggled(false);
}

void InterfacePlugin::uiUpdateSpeed()
{
	if(!ui->checkBoxShowSpeed->isChecked())
		emit newSpeedLimitation(0);
	else
		emit newSpeedLimitation(ui->limitSpeed->value());
}

InterfacePlugin::~InterfacePlugin()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	disconnect(ui->actionAddFile);
	disconnect(ui->actionAddFolder);
	delete menu;
}

QWidget * InterfacePlugin::getOptionsEngineWidget()
{
	return &optionEngineWidget;
}

void InterfacePlugin::getOptionsEngineEnabled(bool isEnabled)
{
	if(isEnabled)
		ui->tabWidget->addTab(&optionEngineWidget,tr("Copy engine"));
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
	ui->overall->setText(tr("File %1/%2, size: %3/%4").arg(currentFile).arg(totalFile).arg(facilityEngine->sizeToString(currentSize)).arg(facilityEngine->sizeToString(totalSize)));
}

void InterfacePlugin::actionInProgess(EngineActionInProgress action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"start: "+QString::number(action));
	this->action=action;
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
			{
				switch(ui->comboBox_copyEnd->currentIndex())
				{
					case 2:
						emit cancel();
					break;
					case 0:
						if(!haveError)
							emit cancel();
					break;
					default:
					break;
				}
			}
		break;
		default:
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Very wrong switch case!");
		break;
	}
	switch(action)
	{
		case Copying:
		case CopyingAndListing:
			ui->pauseButton->setEnabled(true);
			haveStarted=true;
			ui->cancelButton->setText(tr("Quit"));
		break;
		case Idle:
			ui->pauseButton->setEnabled(false);
		break;
		default:
		break;
	}
}

void InterfacePlugin::newTransferStart(const ItemOfCopyList &item)
{
	index=0;
	loop_size=0;
	//is too heavy for normal usage, then enable it only in debug mode to develop
	#ifdef ULTRACOPIER_DEBUG
	loop_size=graphicItemList.size();
	while(index<loop_size)
	{
		if(graphicItemList.at(index).id==item.id)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Duplicate start transfer : "+QString::number(item.id));
			return;
		}
		index++;
	}
	#endif // ULTRACOPIER_DEBUG
	ItemOfCopyListWithMoreInformations newItem;
	newItem.currentProgression=0;
	newItem.generalData=item;
	currentProgressList<<newItem;
	ui->skipButton->setEnabled(true);
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Start transfer new item: "+QString::number(currentProgressList.last().generalData.id)+", with size: "+QString::number(currentProgressList.last().generalData.size));
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size)
	{
		if(graphicItemList.at(index).id==item.id)
		{
			graphicItemList.at(index).item->setIcon(0,QIcon(":/resources/player_play.png"));
			break;
		}
		index++;
	}
	updateCurrentFileInformation();
}

//is stopped, example: because error have occurred, and try later, don't remove the item!
void InterfacePlugin::newTransferStop(const quint64 &id)
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+QString::number(id));

	//update the icon, but the item can be removed before from the transfer list, then not found
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size)
	{
		if(graphicItemList.at(index).id==id)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"item to remove found");
			graphicItemList.at(index).item->setIcon(0,QIcon(":/resources/player_pause.png"));
			break;
		}
		index++;
	}

	//update the internal transfer list
	index=0;
	loop_size=currentProgressList.size();
	while(index<loop_size)
	{
		if(currentProgressList.at(index).generalData.id==id)
		{
			//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"at index: "+QString::number(index)+", remove item: "+QString::number(currentProgressList.at(index).generalData.id)+", with size: "+QString::number(currentProgressList.at(index).generalData.size));
			currentProgressList.removeAt(index);
			updateCurrentFileInformation();
			break;
		}
		index++;
	}

	//update skip button if needed
	if(currentProgressList.size()==0)
		ui->skipButton->setEnabled(false);
}

void InterfacePlugin::newFolderListing(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(action==Listing)
		ui->from->setText(path);
}

void InterfacePlugin::detectedSpeed(quint64 speed)//in byte per seconds
{
	ui->currentSpeed->setText(facilityEngine->speedToString(speed));
}

void InterfacePlugin::remainingTime(int remainingSeconds)
{
	if(remainingSeconds==-1)
		ui->labelTimeRemaining->setText("<html><body>&#8734;</body></html>");
	else
	{
		TimeDecomposition time=facilityEngine->secondsToTimeDecomposition(remainingSeconds);
		ui->labelTimeRemaining->setText(QString::number(time.hour)+":"+QString::number(time.minute)+":"+QString::number(time.second));
	}
}

void InterfacePlugin::newCollisionAction(QString action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(ui->comboBox_fileCollisions->findData(action)!=-1)
		ui->comboBox_fileCollisions->setCurrentIndex(ui->comboBox_fileCollisions->findData(action));
}

void InterfacePlugin::newErrorAction(QString action)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(ui->comboBox_copyErrors->findData(action)!=-1)
		ui->comboBox_copyErrors->setCurrentIndex(ui->comboBox_copyErrors->findData(action));
}

void InterfacePlugin::errorDetected()
{
	haveError=true;
}

//speed limitation
bool InterfacePlugin::setSpeedLimitation(qint64 speedLimitation)
{
	currentSpeed=speedLimitation;
	updateSpeed();
	return true;
}

//get information about the copy
void InterfacePlugin::setGeneralProgression(quint64 current,quint64 total)
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

void InterfacePlugin::setFileProgression(quint64 id,quint64 current,quint64 total)
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

void InterfacePlugin::setCollisionAction(QList<QPair<QString,QString> > list)
{
	ui->comboBox_fileCollisions->clear();
	index=0;
	loop_size=list.size();
	while(index<loop_size)
	{
		ui->comboBox_fileCollisions->addItem(list.at(index).first,list.at(index).second);
		index++;
	}
}

void InterfacePlugin::setErrorAction(QList<QPair<QString,QString> > list)
{
	ui->comboBox_fileCollisions->clear();
	index=0;
	loop_size=list.size();
	while(index<loop_size)
	{
		ui->comboBox_copyErrors->addItem(list.at(index).first,list.at(index).second);
		index++;
	}
}

void InterfacePlugin::speed(qint64)
{
}

//edit the transfer list
void InterfacePlugin::getActionOnList(QList<returnActionOnCopyList> returnActions)
{
	//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, returnActions.size(): "+QString::number(returnActions.size()));
	indexAction=0;
	index=0;
	loop_size=returnActions.size();
	while(indexAction<loop_size)
	{
		if(returnActions.at(indexAction).type==AddingItem)
		{
			graphicItem newItem;
			newItem.id=returnActions.at(indexAction).addAction.id;
			QStringList listString;
			listString << returnActions.at(indexAction).addAction.sourceFullPath << facilityEngine->sizeToString(returnActions.at(indexAction).addAction.size) << returnActions.at(indexAction).addAction.destinationFullPath;
			newItem.item=new QTreeWidgetItem(listString);
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
								//ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Remove: "+QString::number(returnActions.at(indexAction).userAction.id));
								delete graphicItemList.at(index).item;
								graphicItemList.removeAt(index);
								currentFile++;
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
		this->setWindowTitle("Ultracopier - "+facilityEngine->translateText("Copy"));
	else
		this->setWindowTitle("Ultracopier - "+facilityEngine->translateText("Move"));
	updateModeAndType();
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
	{
		ui->pauseButton->setIcon(QIcon(":/resources/player_play.png"));
		ui->pauseButton->setText(facilityEngine->translateText("Resume"));
	}
	else
	{
		ui->pauseButton->setIcon(QIcon(":/resources/player_pause.png"));
		ui->pauseButton->setText(facilityEngine->translateText("Pause"));
	}
}

void InterfacePlugin::updateCurrentFileInformation()
{
	if(currentProgressList.size()>0)
	{
		ui->from->setText(currentProgressList.first().generalData.sourceFullPath);
		ui->to->setText(currentProgressList.first().generalData.destinationFullPath);
		ui->current_file->setText(currentProgressList.first().generalData.destinationFileName+", "+facilityEngine->sizeToString(currentProgressList.first().generalData.size));
		if(currentProgressList.first().generalData.size>0)
			ui->progressBar_file->setValue(((double)currentProgressList.first().currentProgression/currentProgressList.first().generalData.size)*65535);
		else
			ui->progressBar_file->setValue(0);
	}
	else
	{
		ui->from->setText("");
		ui->to->setText("");
		ui->current_file->setText("-");
		if(haveStarted)
			ui->progressBar_file->setValue(65535);
		else
			ui->progressBar_file->setValue(0);
	}
}


void InterfacePlugin::on_putOnTop_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)

			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit moveItemsOnTop(ids);
}

void InterfacePlugin::on_pushUp_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit moveItemsUp(ids);
}

void InterfacePlugin::on_pushDown_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit moveItemsDown(ids);
}

void InterfacePlugin::on_putOnBottom_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit moveItemsOnBottom(ids);
}

void InterfacePlugin::on_del_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	selectedItems=ui->CopyList->selectedItems();
	ids.clear();
	index=0;
	loop_size=graphicItemList.size();
	while(index<loop_size && selectedItems.size()>0)
	{
		#ifdef ULTRACOPIER_PLUGIN_DEBUG
		if(ui->CopyList->indexOfTopLevelItem(graphicItemList.at(index).item)==-1)
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"Unable to locate in the real gui the item id: "+QString::number(graphicItemList.at(index).id)+", pointer: "+QString::number((quint64)graphicItemList.at(index).item));
		else
		#endif
		if(selectedItems.contains(graphicItemList.at(index).item))
		{
			ids << graphicItemList.at(index).id;
			selectedItems.removeOne(graphicItemList.at(index).item);
		}
		index++;
	}
	if(ids.size()>0)
		emit removeItems(ids);
}

void InterfacePlugin::on_cancelButton_clicked()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->hide();
	emit cancel();
}

void InterfacePlugin::on_checkBoxShowSpeed_toggled(bool checked)
{
	if(checked==checked)
		updateSpeed();
}

void InterfacePlugin::on_SliderSpeed_valueChanged(int value)
{
	switch(value)
	{
		case 0:
			currentSpeed=0;
		break;
		case 1:
			currentSpeed=1024;
		break;
		case 2:
			currentSpeed=1024*4;
		break;
		case 3:
			currentSpeed=1024*16;
		break;
		case 4:
			currentSpeed=1024*64;
		break;
		case 5:
			currentSpeed=1024*128;
		break;
	}
	emit newSpeedLimitation(currentSpeed);
}

void InterfacePlugin::updateSpeed()
{
	bool checked;
	if(currentSpeed==-1)
	{
		ui->checkBoxShowSpeed->setEnabled(false);
		checked=false;
	}
	else
	{
		ui->checkBoxShowSpeed->setEnabled(true);
		checked=ui->checkBox_limitSpeed->isChecked();
	}
	ui->label_Slider_speed->setVisible(checked);
	ui->SliderSpeed->setVisible(checked);
	ui->label_SpeedMaxValue->setVisible(checked);
	ui->checkBox_limitSpeed->setEnabled(checked);
	if(checked)
	{
		ui->limitSpeed->setEnabled(false);
		if(currentSpeed==0)
		{
			ui->SliderSpeed->setValue(0);
			ui->label_SpeedMaxValue->setText(tr("Unlimited"));
		}
		else if(currentSpeed<=1024)
		{
			if(currentSpeed!=1024)
			{
				currentSpeed=1024;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(1);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*1));
		}
		else if(currentSpeed<=1024*4)
		{
			if(currentSpeed!=1024*4)
			{
				currentSpeed=1024*4;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(2);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*4));
		}
		else if(currentSpeed<=1024*16)
		{
			if(currentSpeed!=1024*16)
			{
				currentSpeed=1024*16;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(3);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*16));
		}
		else if(currentSpeed<=1024*64)
		{
			if(currentSpeed!=1024*64)
			{
				currentSpeed=1024*64;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(4);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*64));
		}
		else
		{
			if(currentSpeed!=1024*128)
			{
				currentSpeed=1024*128;
				emit newSpeedLimitation(currentSpeed);
			}
			ui->SliderSpeed->setValue(5);
			ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*128));
		}
	}
	else
	{
		ui->checkBox_limitSpeed->setChecked(currentSpeed>0);
		if(currentSpeed>0)
			ui->limitSpeed->setValue(currentSpeed);
		ui->checkBox_limitSpeed->setEnabled(currentSpeed!=-1);
		ui->limitSpeed->setEnabled(ui->checkBox_limitSpeed->isChecked());
	}
}

void InterfacePlugin::on_limitSpeed_valueChanged(int value)
{
	currentSpeed=value;
	emit newSpeedLimitation(currentSpeed);
}

void InterfacePlugin::on_checkBox_limitSpeed_clicked()
{
	if(ui->checkBox_limitSpeed->isChecked())
	{
		if(ui->checkBoxShowSpeed->isChecked())
			on_SliderSpeed_valueChanged(ui->SliderSpeed->value());
		else
			on_limitSpeed_valueChanged(ui->limitSpeed->value());
	}
	else
		currentSpeed=0;
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

void InterfacePlugin::newLanguageLoaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	if(modeIsForced)
		forceCopyMode(mode);
	ui->retranslateUi(this);
	if(!haveStarted)
		ui->current_file->setText(tr("File Name, 0KB"));
	else
		updateCurrentFileInformation();
	updateOverallInformation();
	updateSpeed();
	ui->tabWidget->setTabText(4,facilityEngine->translateText("Copy engine"));
	on_moreButton_toggled(ui->moreButton->isChecked());
}

void InterfacePlugin::on_pushButtonCloseSearch_clicked()
{
	closeTheSearchBox();
}

//close the search box
void InterfacePlugin::closeTheSearchBox()
{
	currentIndexSearch = -1;
	ui->lineEditSearch->clear();
	ui->lineEditSearch->hide();
	ui->pushButtonSearchPrev->hide();
	ui->pushButtonSearchNext->hide();
	ui->pushButtonCloseSearch->hide();
	hilightTheSearch();
}

//search box shortcut
void InterfacePlugin::searchBoxShortcut()
{
	if(ui->lineEditSearch->isHidden())
	{
		ui->lineEditSearch->show();
		ui->pushButtonSearchPrev->show();
		ui->pushButtonSearchNext->show();
		ui->pushButtonCloseSearch->show();
		ui->lineEditSearch->setFocus(Qt::ShortcutFocusReason);
	}
	else
		closeTheSearchBox();
}

//hilight the search
void InterfacePlugin::hilightTheSearch()
{
	QFont *fontNormal=new QFont();
	QTreeWidgetItem * item=NULL;
	//get the ids to do actions
	int i=0;
	loop_size=ui->CopyList->topLevelItemCount();
	if(ui->lineEditSearch->text().isEmpty())
	{
		while(i<loop_size)
		{
			item=ui->CopyList->topLevelItem(i);
			item->setBackgroundColor(0,QColor(255,255,255,0));
			item->setBackgroundColor(1,QColor(255,255,255,0));
			item->setBackgroundColor(2,QColor(255,255,255,0));
			item->setFont(0,*fontNormal);
			item->setFont(1,*fontNormal);
			item->setFont(2,*fontNormal);
			i++;
		}
		ui->lineEditSearch->setStyleSheet("");
	}
	else
	{
		bool itemFound=false;
		while(i<loop_size)
		{
			item=ui->CopyList->topLevelItem(i);
			if(item->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1 || item->text(2).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1)
			{
				itemFound=true;
				item->setBackgroundColor(0,QColor(255,255,0,100));
				item->setBackgroundColor(1,QColor(255,255,0,100));
				item->setBackgroundColor(2,QColor(255,255,0,100));
			}
			else
			{
				item->setBackgroundColor(0,QColor(255,255,255,0));
				item->setBackgroundColor(1,QColor(255,255,255,0));
				item->setBackgroundColor(2,QColor(255,255,255,0));
			}
			item->setFont(0,*fontNormal);
			item->setFont(1,*fontNormal);
			item->setFont(2,*fontNormal);
			i++;
		}
		if(!itemFound)
			ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
		else
			ui->lineEditSearch->setStyleSheet("");
	}
	delete fontNormal;
}

void InterfacePlugin::on_pushButtonSearchPrev_clicked()
{
	if(!ui->lineEditSearch->text().isEmpty() && ui->CopyList->topLevelItemCount()>0)
	{
		hilightTheSearch();
		int searchStart;
		if(currentIndexSearch<0 || currentIndexSearch>=ui->CopyList->topLevelItemCount())
			searchStart=ui->CopyList->topLevelItemCount()-1;
		else
			searchStart=ui->CopyList->topLevelItemCount()-currentIndexSearch-2;
		int count=0;
		QTreeWidgetItem *curs=NULL;
		loop_size=ui->CopyList->topLevelItemCount();
		while(count<loop_size)
		{
			if(searchStart<0)
				searchStart+=ui->CopyList->topLevelItemCount();
			if(ui->CopyList->topLevelItem(searchStart)->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1 || ui->CopyList->topLevelItem(searchStart)->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1)
			{
				curs=ui->CopyList->topLevelItem(searchStart);
				break;
			}
			searchStart--;
			count++;
		}
		if(curs!=NULL)
		{
			currentIndexSearch=ui->CopyList->topLevelItemCount()-1-ui->CopyList->indexOfTopLevelItem(curs);
			ui->lineEditSearch->setStyleSheet("");
			QFont *bold=new QFont();
			bold->setBold(true);
			curs->setFont(0,*bold);
			curs->setFont(1,*bold);
			curs->setFont(2,*bold);
			curs->setBackgroundColor(0,QColor(255,255,0,200));
			curs->setBackgroundColor(1,QColor(255,255,0,200));
			curs->setBackgroundColor(2,QColor(255,255,0,200));
			ui->CopyList->scrollToItem(curs);
			delete bold;
		}
		else
			ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
	}
}

void InterfacePlugin::on_pushButtonSearchNext_clicked()
{
	if(ui->lineEditSearch->text().isEmpty())
	{
		if(ui->lineEditSearch->isHidden())
			searchBoxShortcut();
	}
	else
	{
		if(ui->CopyList->topLevelItemCount()>0)
		{
			hilightTheSearch();
			int searchStart;
			if(currentIndexSearch<0 || currentIndexSearch>=ui->CopyList->topLevelItemCount())
				searchStart=0;
			else
				searchStart=ui->CopyList->topLevelItemCount()-currentIndexSearch;
			int count=0;
			QTreeWidgetItem *curs=NULL;
			loop_size=ui->CopyList->topLevelItemCount();
			while(count<loop_size)
			{
				if(searchStart>=ui->CopyList->topLevelItemCount())
					searchStart-=ui->CopyList->topLevelItemCount();
				if(ui->CopyList->topLevelItem(searchStart)->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1 || ui->CopyList->topLevelItem(searchStart)->text(0).indexOf(ui->lineEditSearch->text(),0,Qt::CaseInsensitive)!=-1)
				{
					curs=ui->CopyList->topLevelItem(searchStart);
					break;
				}
				searchStart++;
				count++;
			}
			if(curs!=NULL)
			{
				currentIndexSearch=ui->CopyList->topLevelItemCount()-1-ui->CopyList->indexOfTopLevelItem(curs);
				ui->lineEditSearch->setStyleSheet("");
				QFont *bold=new QFont();
				bold->setBold(true);
				curs->setFont(0,*bold);
				curs->setFont(1,*bold);
				curs->setFont(2,*bold);
				curs->setBackgroundColor(0,QColor(255,255,0,200));
				curs->setBackgroundColor(1,QColor(255,255,0,200));
				curs->setBackgroundColor(2,QColor(255,255,0,200));
				ui->CopyList->scrollToItem(curs);
				delete bold;
			}
			else
				ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
		}
	}
}

void InterfacePlugin::on_lineEditSearch_returnPressed()
{
	hilightTheSearch();
}

void InterfacePlugin::on_lineEditSearch_textChanged(QString text)
{
	if(text=="")
	{
		TimerForSearch->stop();
		hilightTheSearch();
	}
	else
		TimerForSearch->start();
}

void InterfacePlugin::on_moreButton_toggled(bool checked)
{
	if(checked)
		this->setMaximumHeight(16777215);
	else
		this->setMaximumHeight(130);
	// usefull under windows
	this->updateGeometry();
	this->update();
	this->adjustSize();
}

void InterfacePlugin::on_comboBox_copyErrors_currentIndexChanged(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit sendErrorAction(ui->comboBox_copyErrors->itemData(index).toString());
}

void InterfacePlugin::on_comboBox_fileCollisions_currentIndexChanged(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	emit sendCollisionAction(ui->comboBox_fileCollisions->itemData(index).toString());
}
