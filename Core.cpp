/** \file Core.cpp
\brief Define the class for the core
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QMessageBox>
#include <QtPlugin>

#include "Core.h"

Core::Core(CopyEngineManager *copyEngineList)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	this->copyEngineList=copyEngineList;
	nextId=0;
	forUpateInformation.setInterval(ULTRACOPIER_TIME_INTERFACE_UPDATE);
	loadInterface();
	//connect(&copyEngineList,	SIGNAL(newCanDoOnlyCopy(bool)),				this,	SIGNAL(newCanDoOnlyCopy(bool)));
	connect(themes,			SIGNAL(theThemeNeedBeUnloaded()),			this,	SLOT(unloadInterface()));
	connect(themes,			SIGNAL(theThemeIsReloaded()),				this,	SLOT(loadInterface()));
	connect(&forUpateInformation,	SIGNAL(timeout()),					this,	SLOT(periodiqueSync()));

	//load the GUI option
	QString defaultLogFile="";
	if(resources->getWritablePath()!="")
		defaultLogFile=resources->getWritablePath()+"ultracopier-files.log";
	QList<QPair<QString, QVariant> > KeysList;
	KeysList.append(qMakePair(QString("enabled"),QVariant(false)));
	KeysList.append(qMakePair(QString("file"),QVariant(defaultLogFile)));
	KeysList.append(qMakePair(QString("transfer"),QVariant(true)));
	KeysList.append(qMakePair(QString("error"),QVariant(true)));
	KeysList.append(qMakePair(QString("folder"),QVariant(true)));
	KeysList.append(qMakePair(QString("sync"),QVariant(false)));
	KeysList.append(qMakePair(QString("transfer_format"),QVariant("[%time%] %source% (%size%) %destination%")));
	KeysList.append(qMakePair(QString("error_format"),QVariant("[%time%] %path%, %error%")));
	KeysList.append(qMakePair(QString("folder_format"),QVariant("[%time%] %operation% %path%")));
	options->addOptionGroup("Write_log",KeysList);
	newOptionValue("Write_log",	"transfer",			options->getOptionValue("Write_log","transfer"));
	newOptionValue("Write_log",	"error",			options->getOptionValue("Write_log","error"));
	newOptionValue("Write_log",	"folder",			options->getOptionValue("Write_log","folder"));
	newOptionValue("Write_log",	"sync",				options->getOptionValue("Write_log","sync"));

	log.openLogs();

	connect(options,SIGNAL(newOptionValue(QString,QString,QVariant)),	this,	SLOT(newOptionValue(QString,QString,QVariant)));
}

void Core::newCopy(quint32 orderId,QStringList protocolsUsedForTheSources,QStringList sources)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	openNewCopy(Copy,false,protocolsUsedForTheSources);
	copyList.last().orderId<<orderId;
	copyList.last().engine->newCopy(sources);
	copyList.last().interface->haveExternalOrder();
}

void Core::newCopy(quint32 orderId,QStringList protocolsUsedForTheSources,QStringList sources,QString protocolsUsedForTheDestination,QString destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//search to group the window
	int GroupWindowWhen=options->getOptionValue("Ultracopier","GroupWindowWhen").toInt();
	bool haveSameSource=false,haveSameDestination=false;
	if(GroupWindowWhen!=0)
	{
		int index=0;
		while(index<copyList.size())
		{
			if(!copyList.at(index).ignoreMode && copyList.at(index).mode==Copy)
			{
				if(GroupWindowWhen!=5)
				{
					if(GroupWindowWhen!=2)
						haveSameSource=copyList.at(index).engine->haveSameSource(sources);
					if(GroupWindowWhen!=1)
						haveSameDestination=copyList.at(index).engine->haveSameDestination(destination);
				}
				if(
					GroupWindowWhen==5 ||
					(GroupWindowWhen==1 && haveSameSource) ||
					(GroupWindowWhen==2 && haveSameDestination) ||
					(GroupWindowWhen==3 && (haveSameSource && haveSameDestination)) ||
					(GroupWindowWhen==4 && (haveSameSource || haveSameDestination))
					)
				{
					/*protocols are same*/
					if(copyEngineList->protocolsSupportedByTheCopyEngine(copyList.at(index).engine,protocolsUsedForTheSources,protocolsUsedForTheDestination))
					{
						copyList[index].orderId<<orderId;
						copyList.at(index).engine->newCopy(sources,destination);
						copyList.at(index).interface->haveExternalOrder();
						return;
					}
				}
			}
			index++;
		}
	}
	//else open new windows
	openNewCopy(Copy,false,protocolsUsedForTheSources,protocolsUsedForTheDestination);
	copyList.last().orderId<<orderId;
	copyList.last().engine->newCopy(sources,destination);
	copyList.last().interface->haveExternalOrder();
}

void Core::newMove(quint32 orderId,QStringList protocolsUsedForTheSources,QStringList sources)
{
	openNewCopy(Move,false,protocolsUsedForTheSources);
	copyList.last().orderId<<orderId;
	copyList.last().engine->newMove(sources);
	copyList.last().interface->haveExternalOrder();
}

void Core::newMove(quint32 orderId,QStringList protocolsUsedForTheSources,QStringList sources,QString protocolsUsedForTheDestination,QString destination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//search to group the window
	int GroupWindowWhen=options->getOptionValue("Ultracopier","GroupWindowWhen").toInt();
	bool haveSameSource=false,haveSameDestination=false;
	if(GroupWindowWhen!=0)
	{
		int index=0;
		while(index<copyList.size())
		{
			if(!copyList.at(index).ignoreMode && copyList.at(index).mode==Move)
			{
				if(GroupWindowWhen!=5)
				{
					if(GroupWindowWhen!=2)
						haveSameSource=copyList.at(index).engine->haveSameSource(sources);
					if(GroupWindowWhen!=1)
						haveSameDestination=copyList.at(index).engine->haveSameDestination(destination);
				}
				if(
					GroupWindowWhen==5 ||
					(GroupWindowWhen==1 && haveSameSource) ||
					(GroupWindowWhen==2 && haveSameDestination) ||
					(GroupWindowWhen==3 && (haveSameSource && haveSameDestination)) ||
					(GroupWindowWhen==4 && (haveSameSource || haveSameDestination))
					)
				{
					/*protocols are same*/
					if(copyEngineList->protocolsSupportedByTheCopyEngine(copyList.at(index).engine,protocolsUsedForTheSources,protocolsUsedForTheDestination))
					{
						copyList[index].orderId<<orderId;
						copyList.at(index).engine->newCopy(sources,destination);
						copyList.at(index).interface->haveExternalOrder();
						return;
					}
				}
			}
			index++;
		}
	}
	//else open new windows
	openNewCopy(Move,false,protocolsUsedForTheSources,protocolsUsedForTheDestination);
	copyList.last().orderId<<orderId;
	copyList.last().engine->newMove(sources,destination);
	copyList.last().interface->haveExternalOrder();
}

/// \todo name to open the right copy engine
void Core::addWindowCopyMove(CopyMode mode,QString name)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+name);
	openNewCopy(mode,false,name);
	ActionOnManualOpen ActionOnManualOpen_value=(ActionOnManualOpen)options->getOptionValue("Ultracopier","ActionOnManualOpen").toInt();
	if(ActionOnManualOpen_value!=ActionOnManualOpen_Nothing)
	{
		if(ActionOnManualOpen_value==ActionOnManualOpen_Folder)
			copyList.last().engine->userAddFolder(mode);
		else
			copyList.last().engine->userAddFile(mode);
	}
}

/// \todo name to open the right copy engine
void Core::addWindowTransfer(QString name)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start"+name);
	openNewCopy(Copy,true,name);
}

void Core::loadInterface()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//load the extra files to check the themes availability
	if(copyList.size()>0)
	{
		bool error=false;
		index=0;
		loop_size=copyList.size();
		while(index<loop_size)
		{
			copyList[index].interface=themes->getThemesInstance();
			if(copyList[index].interface==NULL)
			{
				copyInstanceCanceledByIndex(index);
				index--;
				error=true;
			}
			else
			{
				if(!copyList.at(index).ignoreMode)
					copyList.at(index).interface->forceCopyMode(copyList.at(index).mode);
				connectInterfaceAndSync(copyList.count()-1);
			}
			index++;
		}
		if(error)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to load the interface, copy aborted");
			QMessageBox::critical(NULL,tr("Error"),tr("Unable to load the interface, copy aborted"));
		}
	}
}

void Core::unloadInterface()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	index=0;
	loop_size=copyList.size();
	while(index<loop_size)
	{
		if(copyList.at(index).interface!=NULL)
		{
			disconnectInterface(index);
			delete copyList.at(index).interface;
			copyList[index].interface=NULL;
		}
		index++;
	}
}

int Core::incrementId()
{
	do
	{
		nextId++;
		if(nextId>2000000)
			nextId=0;
	} while(idList.contains(nextId));
	return nextId;
}

int Core::openNewCopy(CopyMode mode,bool ignoreMode,QStringList protocolsUsedForTheSources,QString protocolsUsedForTheDestination)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	CopyEngineManager::returnCopyEngine returnInformations=copyEngineList->getCopyEngine(mode,protocolsUsedForTheSources,protocolsUsedForTheDestination);
	return connectCopyEngine(mode,ignoreMode,returnInformations);
}

int Core::openNewCopy(CopyMode mode,bool ignoreMode,QString name)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, mode: "+QString::number(mode)+", name: "+name);
	CopyEngineManager::returnCopyEngine returnInformations=copyEngineList->getCopyEngine(mode,name);
	return connectCopyEngine(mode,ignoreMode,returnInformations);
}

int Core::connectCopyEngine(CopyMode mode,bool ignoreMode,CopyEngineManager::returnCopyEngine returnInformations)
{
	if(returnInformations.canDoOnlyCopy)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Mode force for unknow reason");
		ignoreMode=false;//force mode if need, normaly not used
	}
	CopyInstance newItem;
	newItem.engine=returnInformations.engine;
	if(newItem.engine!=NULL)
	{
		PluginInterface_Themes *theme=themes->getThemesInstance();
		if(theme!=NULL)
		{
			newItem.id=incrementId();
			newItem.lastProgression=0;
			newItem.interface=theme;
			newItem.ignoreMode=ignoreMode;
			newItem.mode=mode;
			newItem.type=returnInformations.type;
			newItem.transferListOperation=returnInformations.transferListOperation;
			newItem.baseTime=0;
			newItem.numberOfFile=0;
			newItem.numberOfTransferedFile=0;
			newItem.sizeToCopy=0;
			newItem.action=Idle;
			newItem.lastProgression=0;//store the real byte transfered, used in time remaining calculation
			newItem.isPaused=false;
			newItem.baseTime=0;//stored in ms
			newItem.isRunning=false;
			newItem.haveError=false;

			if(!ignoreMode)
				newItem.interface->forceCopyMode(mode);
			if(copyList.size()==0)
				forUpateInformation.start();
			copyList << newItem;
			connectEngine(copyList.count()-1);
			connectInterfaceAndSync(copyList.count()-1);
			return newItem.id;
		}
		delete newItem.engine;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to load the interface, copy aborted");
		QMessageBox::critical(NULL,tr("Error"),tr("Unable to load the interface, copy aborted"));
	}
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to load the copy engine, copy aborted");
		QMessageBox::critical(NULL,tr("Error"),tr("Unable to load the copy engine, copy aborted"));
	}
	return -1;
}

void Core::resetSpeedDetectedEngine()
{
	int index=indexCopySenderCopyEngine();
	if(index!=-1)
		resetSpeedDetected(index);
}

void Core::resetSpeedDetectedInterface()
{
	int index=indexCopySenderInterface();
	if(index!=-1)
		resetSpeedDetected(index);
}

void Core::resetSpeedDetected(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start on %1").arg(index));
	copyList[index].runningTime.restart();
	copyList[index].lastSpeedDetected.clear();
	copyList[index].lastSpeedTime.clear();
}

void Core::actionInProgess(EngineActionInProgress action)
{
	index=indexCopySenderCopyEngine();
	if(index!=-1)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("action: %1, from %2").arg(action).arg(index));
		//drop here the duplicate action
		if(copyList[index].action==action)
		{
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("The copy engine have send 2x the same EngineActionInProgress"));
			return;
		}
		//update time runing for time remaning caculation
		if(action==Copying || action==CopyingAndListing)
		{
			if(!copyList.at(index).isRunning)
			{
				copyList[index].isRunning=true;
				copyList[index].runningTime.restart();
			}
		}
		else
		{
			if(copyList.at(index).isRunning)
			{
				copyList[index].isRunning=false;
				copyList[index].baseTime+=copyList[index].runningTime.elapsed();
			}
		}
		//do sync
		conditionalSync(index);
		periodiqueSync(index);
		copyList[index].action=action;
		if(copyList.at(index).interface!=NULL)
			copyList.at(index).interface->actionInProgess(action);
		if(action==Idle)
		{
			index_sub_loop=0;
			loop_size=copyList.at(index).orderId.size();
			while(index_sub_loop<loop_size)
			{
				emit copyCanceled(copyList.at(index).orderId.at(index_sub_loop));
				index_sub_loop++;
			}
			copyList[index].orderId.clear();
			resetSpeedDetected(index);
		}
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to locate the interface sender");
}

void Core::newTransferStart(const ItemOfCopyList &item)
{
	index=indexCopySenderCopyEngine();
	if(index!=-1)
	{
		//is too heavy for normal usage, then enable it only in debug mode to develop
		#ifdef ULTRACOPIER_DEBUG
		index_sub_loop=0;
		loop_size=copyList.at(index).transferItemList.size();
		while(index_sub_loop<loop_size)
		{
			if(copyList.at(index).transferItemList.at(index_sub_loop).id==item.id)
			{
				ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"duplicate id sended!");
				return;
			}
			index_sub_loop++;
		}
		#endif // ULTRACOPIER_DEBUG
		conditionalSync(index);
		copyList[index].transferItemList<<item;
		copyList[index].progressionList<<0;
		copyList.at(index).interface->newTransferStart(item);
		if(log_enable_transfer)
			log.newTransferStart(item);
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to locate the copy engine sender");
}

void Core::newTransferStop(const quint64 &id)
{
	int index=indexCopySenderCopyEngine();
	if(index!=-1)
	{
		index_sub_loop=0;
		loop_size=copyList.at(index).transferItemList.size();
		while(index_sub_loop<loop_size)
		{
			if(copyList.at(index).transferItemList.at(index_sub_loop).id==id)
			{
				conditionalSync(index);
				copyList[index].progressionList.removeAt(index_sub_loop);
				copyList[index].transferItemList.removeAt(index_sub_loop);
				copyList.at(index).interface->newTransferStop(id);
				return;
			}
			index_sub_loop++;
		}
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"id not found order: "+QString::number(id));
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"sender not found");
}

void Core::newFolderListing(const QString &path)
{
	int index=indexCopySenderCopyEngine();
	if(index!=-1)
	{
		conditionalSync(index);
		copyList[index].folderListing=path;
		copyList.at(index).interface->newFolderListing(path);
	}
}

void Core::newCollisionAction(QString action)
{
	int index=indexCopySenderCopyEngine();
	if(index!=-1)
	{
		copyList[index].collisionAction=action;
		copyList.at(index).interface->newCollisionAction(action);
	}
}

void Core::newErrorAction(QString action)
{
	int index=indexCopySenderCopyEngine();
	if(index!=-1)
	{
		copyList[index].errorAction=action;
		copyList.at(index).interface->newErrorAction(action);
	}
}

void Core::isInPause(bool isPaused)
{
	int index=indexCopySenderCopyEngine();
	if(index!=-1)
	{
		if(!isPaused)
			resetSpeedDetected(index);
		conditionalSync(index);
		copyList[index].isPaused=isPaused;
		copyList.at(index).interface->isInPause(isPaused);
	}
}

void Core::newActionOnList()
{
	int index=indexCopySenderCopyEngine();
	if(index!=-1)
		conditionalSync(index);
}

int Core::indexCopySenderCopyEngine()
{
	QObject * senderObject=sender();
	if(senderObject==NULL)
	{
		//QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Qt sender() NULL");
		return -1;
	}
	index=0;
	loop_size=copyList.size();
	while(index<loop_size)
	{
		if(copyList.at(index).engine==senderObject)
			return index;
		index++;
	}
	//QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Sender not located in the list");
	return -1;
}

int Core::indexCopySenderInterface()
{
	QObject * senderObject=sender();
	if(senderObject==NULL)
	{
		//QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Qt sender() NULL");
		return -1;
	}
	index=0;
	loop_size=copyList.size();
	while(index<loop_size)
	{
		if(copyList.at(index).interface==senderObject)
			return index;
		index++;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to locate QObject * sender");
	PluginInterface_Themes * interface = qobject_cast<PluginInterface_Themes *>(senderObject);
	if(interface==NULL)
	{
		//QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Qt sender themes NULL");
		return -1;
	}
	index=0;
	while(index<loop_size)
	{
		if(copyList.at(index).interface==interface)
			return index;
		index++;
	}
	//QMessageBox::critical(NULL,tr("Internal error"),tr("A communication error occured between the interface and the copy plugin. Please report this bug."));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Sender not located in the list");
	return -1;
}

void Core::connectEngine(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start with index: %1: %2").arg(index).arg((quint64)sender()));
	disconnectEngine(index);

	connect(copyList.at(index).engine,SIGNAL(newTransferStart(ItemOfCopyList)),		this,SLOT(newTransferStart(ItemOfCopyList)),Qt::QueuedConnection);//to check to change
	connect(copyList.at(index).engine,SIGNAL(newTransferStop(quint64)),			this,SLOT(newTransferStop(quint64)),Qt::QueuedConnection);
	connect(copyList.at(index).engine,SIGNAL(newFolderListing(QString)),			this,SLOT(newFolderListing(QString)),Qt::QueuedConnection);//to check to change
	connect(copyList.at(index).engine,SIGNAL(newCollisionAction(QString)),			this,SLOT(newCollisionAction(QString)),Qt::QueuedConnection);
	connect(copyList.at(index).engine,SIGNAL(newErrorAction(QString)),			this,SLOT(newErrorAction(QString)),Qt::QueuedConnection);
	connect(copyList.at(index).engine,SIGNAL(actionInProgess(EngineActionInProgress)),	this,SLOT(actionInProgess(EngineActionInProgress)),Qt::QueuedConnection);
	connect(copyList.at(index).engine,SIGNAL(newActionOnList()),				this,SLOT(newActionOnList()),Qt::QueuedConnection);//to check to change
	connect(copyList.at(index).engine,SIGNAL(isInPause(bool)),				this,SLOT(isInPause(bool)),Qt::QueuedConnection);//to check to change
	connect(copyList.at(index).engine,SIGNAL(cancelAll()),					this,SLOT(copyInstanceCanceledByEngine()),Qt::QueuedConnection);
	connect(copyList.at(index).engine,SIGNAL(error(QString,quint64,QDateTime,QString)),	this,SLOT(error(QString,quint64,QDateTime,QString)),Qt::QueuedConnection);
	connect(copyList.at(index).engine,SIGNAL(rmPath(QString)),				this,SLOT(rmPath(QString)),Qt::QueuedConnection);
	connect(copyList.at(index).engine,SIGNAL(mkPath(QString)),				this,SLOT(mkPath(QString)),Qt::QueuedConnection);
}

void Core::connectInterfaceAndSync(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start with index: %1: %2").arg(index).arg((quint64)sender()));
	disconnectInterface(index);

	connect(copyList.at(index).interface,SIGNAL(pause()),					copyList.at(index).engine,SLOT(pause()));
	connect(copyList.at(index).interface,SIGNAL(resume()),					copyList.at(index).engine,SLOT(resume()));
	connect(copyList.at(index).interface,SIGNAL(skip(quint64)),				copyList.at(index).engine,SLOT(skip(quint64)));
	connect(copyList.at(index).interface,SIGNAL(sendErrorAction(QString)),			copyList.at(index).engine,SLOT(setErrorAction(QString)));
	connect(copyList.at(index).interface,SIGNAL(newSpeedLimitation(qint64)),		copyList.at(index).engine,SLOT(setSpeedLimitation(qint64)));
	connect(copyList.at(index).interface,SIGNAL(sendCollisionAction(QString)),		copyList.at(index).engine,SLOT(setCollisionAction(QString)));
	connect(copyList.at(index).interface,SIGNAL(userAddFolder(CopyMode)),			copyList.at(index).engine,SLOT(userAddFolder(CopyMode)));
	connect(copyList.at(index).interface,SIGNAL(userAddFile(CopyMode)),			copyList.at(index).engine,SLOT(userAddFile(CopyMode)));

	connect(copyList.at(index).interface,SIGNAL(removeItems(QList<int>)),			copyList.at(index).engine,SLOT(removeItems(QList<int>)));
	connect(copyList.at(index).interface,SIGNAL(moveItemsOnTop(QList<int>)),		copyList.at(index).engine,SLOT(moveItemsOnTop(QList<int>)));
	connect(copyList.at(index).interface,SIGNAL(moveItemsUp(QList<int>)),			copyList.at(index).engine,SLOT(moveItemsUp(QList<int>)));
	connect(copyList.at(index).interface,SIGNAL(moveItemsDown(QList<int>)),			copyList.at(index).engine,SLOT(moveItemsDown(QList<int>)));
	connect(copyList.at(index).interface,SIGNAL(moveItemsOnBottom(QList<int>)),		copyList.at(index).engine,SLOT(moveItemsOnBottom(QList<int>)));
	connect(copyList.at(index).interface,SIGNAL(exportTransferList()),			copyList.at(index).engine,SLOT(exportTransferList()));
	connect(copyList.at(index).interface,SIGNAL(importTransferList()),			copyList.at(index).engine,SLOT(importTransferList()));

	connect(copyList.at(index).interface,SIGNAL(newSpeedLimitation(qint64)),		this,SLOT(resetSpeedDetectedInterface()));
	connect(copyList.at(index).interface,SIGNAL(resume()),					this,SLOT(resetSpeedDetectedInterface()));
	connect(copyList.at(index).interface,SIGNAL(cancel()),					this,SLOT(copyInstanceCanceledByInterface()),Qt::QueuedConnection);
	connect(copyList.at(index).interface,SIGNAL(urlDropped(QList<QUrl>)),			this,SLOT(urlDropped(QList<QUrl>)),Qt::QueuedConnection);

	copyList.at(index).interface->setSpeedLimitation(copyList.at(index).engine->getSpeedLimitation());
	copyList.at(index).interface->setErrorAction(copyList.at(index).engine->getErrorAction());
	copyList.at(index).interface->setCollisionAction(copyList.at(index).engine->getCollisionAction());
	copyList.at(index).interface->setCopyType(copyList.at(index).type);
	copyList.at(index).interface->setTransferListOperation(copyList.at(index).transferListOperation);
	copyList.at(index).interface->actionInProgess(copyList.at(index).action);
	copyList.at(index).interface->isInPause(copyList.at(index).isPaused);
	if(copyList.at(index).haveError)
		copyList.at(index).interface->errorDetected();
	QWidget *tempWidget=copyList.at(index).interface->getOptionsEngineWidget();
	if(tempWidget!=NULL)
		copyList.at(index).interface->getOptionsEngineEnabled(copyList.at(index).engine->getOptionsEngine(tempWidget));

	//put entry into the interface
	QList<ItemOfCopyList> transferList=copyList.at(index).engine->getTransferList();
	loop_size=transferList.size();
	if(loop_size)
	{
		QList<returnActionOnCopyList> realActionToSend;
		index_sub_loop=0;
		while(index_sub_loop<loop_size)
		{
			returnActionOnCopyList newAction;
			newAction.addAction	= transferList.at(index_sub_loop);
			newAction.type		= AddingItem;
			realActionToSend << newAction;
			index_sub_loop++;
		}
		copyList.at(index).interface->getActionOnList(realActionToSend);
	}

	//start the already started item
	loop_size=copyList.at(index).transferItemList.size();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("copyList.at(index).transferItemList.size(): %1").arg(loop_size));
	if(loop_size>0)
	{
		index_sub_loop=0;
		while(index_sub_loop<loop_size)
		{
			copyList.at(index).interface->newTransferStart(copyList.at(index).transferItemList.at(index_sub_loop));
			index_sub_loop++;
		}
	}

	//force the updating, without wait the timer
	periodiqueSync(index);
}

void Core::disconnectEngine(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start with index: %1").arg(index));
	disconnect(copyList.at(index).engine,SIGNAL(newTransferStart(ItemOfCopyList)),		this,SLOT(newTransferStart(ItemOfCopyList)));//to check to change
	disconnect(copyList.at(index).engine,SIGNAL(newTransferStop(quint64)),			this,SLOT(newTransferStop(quint64)));
	disconnect(copyList.at(index).engine,SIGNAL(newFolderListing(QString)),			this,SLOT(newFolderListing(QString)));//to check to change
	disconnect(copyList.at(index).engine,SIGNAL(newCollisionAction(QString)),		this,SLOT(newCollisionAction(QString)));
	disconnect(copyList.at(index).engine,SIGNAL(newErrorAction(QString)),			this,SLOT(newErrorAction(QString)));
	disconnect(copyList.at(index).engine,SIGNAL(actionInProgess(EngineActionInProgress)),	this,SLOT(actionInProgess(EngineActionInProgress)));
	disconnect(copyList.at(index).engine,SIGNAL(newActionOnList()),				this,SLOT(newActionOnList()));//to check to change
	disconnect(copyList.at(index).engine,SIGNAL(isInPause(bool)),				this,SLOT(isInPause(bool)));//to check to change
	disconnect(copyList.at(index).engine,SIGNAL(cancelAll()),				this,SLOT(copyInstanceCanceledByEngine()));
	disconnect(copyList.at(index).engine,SIGNAL(error(QString,quint64,QDateTime,QString)),	this,SLOT(error(QString,quint64,QDateTime,QString)));
	disconnect(copyList.at(index).engine,SIGNAL(rmPath(QString)),				this,SLOT(rmPath(QString)));
	disconnect(copyList.at(index).engine,SIGNAL(mkPath(QString)),				this,SLOT(mkPath(QString)));

}

void Core::disconnectInterface(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start with index: %1").arg(index));
	disconnect(copyList.at(index).interface,SIGNAL(pause()),				copyList.at(index).engine,SLOT(pause()));
	disconnect(copyList.at(index).interface,SIGNAL(resume()),				copyList.at(index).engine,SLOT(resume()));
	disconnect(copyList.at(index).interface,SIGNAL(skip(quint64)),				copyList.at(index).engine,SLOT(skip(quint64)));
	disconnect(copyList.at(index).interface,SIGNAL(sendErrorAction(QString)),		copyList.at(index).engine,SLOT(setErrorAction(QString)));
	disconnect(copyList.at(index).interface,SIGNAL(newSpeedLimitation(qint64)),		copyList.at(index).engine,SLOT(setSpeedLimitation(qint64)));
	disconnect(copyList.at(index).interface,SIGNAL(sendCollisionAction(QString)),		copyList.at(index).engine,SLOT(setCollisionAction(QString)));
	disconnect(copyList.at(index).interface,SIGNAL(userAddFolder(CopyMode)),		copyList.at(index).engine,SLOT(userAddFolder(CopyMode)));
	disconnect(copyList.at(index).interface,SIGNAL(userAddFile(CopyMode)),			copyList.at(index).engine,SLOT(userAddFile(CopyMode)));

	disconnect(copyList.at(index).interface,SIGNAL(removeItems(QList<int>)),		copyList.at(index).engine,SLOT(removeItems(QList<int>)));
	disconnect(copyList.at(index).interface,SIGNAL(moveItemsOnTop(QList<int>)),		copyList.at(index).engine,SLOT(moveItemsOnTop(QList<int>)));
	disconnect(copyList.at(index).interface,SIGNAL(moveItemsUp(QList<int>)),		copyList.at(index).engine,SLOT(moveItemsUp(QList<int>)));
	disconnect(copyList.at(index).interface,SIGNAL(moveItemsDown(QList<int>)),		copyList.at(index).engine,SLOT(moveItemsDown(QList<int>)));
	disconnect(copyList.at(index).interface,SIGNAL(moveItemsOnBottom(QList<int>)),		copyList.at(index).engine,SLOT(moveItemsOnBottom(QList<int>)));

	disconnect(copyList.at(index).interface,SIGNAL(newSpeedLimitation(qint64)),		this,SLOT(resetSpeedDetectedInterface()));
	disconnect(copyList.at(index).interface,SIGNAL(resume()),				this,SLOT(resetSpeedDetectedInterface()));
	disconnect(copyList.at(index).interface,SIGNAL(cancel()),				this,SLOT(copyInstanceCanceledByInterface()));
	disconnect(copyList.at(index).interface,SIGNAL(urlDropped(QList<QUrl>)),		this,SLOT(urlDropped(QList<QUrl>)));
}

void Core::periodiqueSync()
{
	index_sub_loop=0;
	loop_size=copyList.size();
	while(index_sub_loop<loop_size)
	{
		if(copyList.at(index_sub_loop).action==Copying || copyList.at(index_sub_loop).action==CopyingAndListing)
			periodiqueSync(index_sub_loop);
		index_sub_loop++;
	}
}

void Core::periodiqueSync(int index)
{
	if(copyList.at(index).engine==NULL || copyList.at(index).interface==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"some thread is null");
		return;
	}
	//transfer to interface the progression
	QPair<quint64,quint64> byteProgression=copyList.at(index).engine->getGeneralProgression();
	copyList.at(index).interface->setGeneralProgression(byteProgression.first,byteProgression.second);

	/** ***************** Do time calcul ******************* **/
	if(!copyList.at(index).isPaused)
	{
		//calcul the last difference of the transfere
		quint64 realByteTransfered=copyList.at(index).engine->realByteTransfered();
		quint64 diffCopiedSize=0;
		if(realByteTransfered>=copyList.at(index).lastProgression)
			diffCopiedSize=realByteTransfered-copyList.at(index).lastProgression;
		copyList[index].lastProgression=realByteTransfered;
		//do the remaining time calculation
		//byte per ms: lastProgression/(baseTime+copyList.at(index).runningTime.elapsed()
		//copyList.at(index).lastProgression
		if(copyList.at(index).lastProgression==0)
			copyList.at(index).interface->remainingTime(-1);
		else
			copyList.at(index).interface->remainingTime(
				(
				(double)(byteProgression.second-byteProgression.first)
				*(copyList.at(index).baseTime+copyList.at(index).runningTime.elapsed())
				/(copyList.at(index).lastProgression)
				)/1000
			);
		if(lastProgressionTime.isNull())
			lastProgressionTime.start();
		else
		{
			if((copyList.at(index).action==Copying || copyList.at(index).action==CopyingAndListing))
			{
				copyList[index].lastSpeedTime << lastProgressionTime.elapsed();
				copyList[index].lastSpeedDetected << diffCopiedSize;
				while(copyList.at(index).lastSpeedDetected.size()>ULTRACOPIER_MAXVALUESPEEDSTORED)
				{
					copyList[index].lastSpeedTime.removeFirst();
					copyList[index].lastSpeedDetected.removeFirst();
				}
				totTime=0;
				totSpeed=0;
				index_sub_loop=0;
				loop_size=copyList.at(index).lastSpeedDetected.size();
				while(index_sub_loop<loop_size)
				{
					totTime+=copyList.at(index).lastSpeedTime.at(index_sub_loop);
					totSpeed+=copyList.at(index).lastSpeedDetected.at(index_sub_loop);
					index_sub_loop++;
				}
				totTime/=1000;
				copyList.at(index).interface->detectedSpeed(totSpeed/totTime);
			}
			lastProgressionTime.restart();
		}
	}

	//transfer of the progression for each transfer
	index_sub_loop=0;
	loop_size=copyList.at(index).transferItemList.size();
	while(index_sub_loop<loop_size)
	{
		returnSpecificFileProgression progression=copyList.at(index).engine->getFileProgression(copyList.at(index).transferItemList.at(index_sub_loop).id);
		if(progression.haveBeenLocated)
			copyList.at(index).interface->setFileProgression(copyList.at(index).transferItemList.at(index_sub_loop).id,progression.copiedSize,progression.totalSize);
		index_sub_loop++;
	}
}

void Core::conditionalSync(int index)
{
	QList<returnActionOnCopyList> returnActions=copyList.at(index).engine->getActionOnList();
	if(returnActions.size()>0)
		copyList.at(index).interface->getActionOnList(returnActions);
}

void Core::copyInstanceCanceledByEngine()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=indexCopySenderCopyEngine();
	if(index!=-1)
		copyInstanceCanceledByIndex(index);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to locate the copy engine sender");
}

void Core::copyInstanceCanceledByInterface()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=indexCopySenderInterface();
	if(index!=-1)
		copyInstanceCanceledByIndex(index);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to locate the copy engine sender");
}

void Core::copyInstanceCanceledByIndex(int index)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, remove with the index: "+QString::number(index));
	disconnectEngine(index);
	disconnectInterface(index);
	copyList.at(index).engine->cancel();
	delete copyList.at(index).engine;
	delete copyList.at(index).interface;
	index_sub_loop=0;
	loop_size=copyList.at(index).orderId.size();
	while(index_sub_loop<loop_size)
	{
		emit copyCanceled(copyList.at(index).orderId.at(index_sub_loop));
		index_sub_loop++;
	}
	copyList[index].orderId.clear();
	copyList.removeAt(index);
	if(copyList.size()==0)
		forUpateInformation.stop();
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"copyList.size(): "+QString::number(copyList.size()));
}

//error occurred
void Core::error(const QString &path,const quint64 &size,const QDateTime &mtime,const QString &error)
{
	if(log_enable_error)
		log.error(path,size,mtime,error);
	int index=indexCopySenderCopyEngine();
	if(index!=-1)
	{
		copyList[index].haveError=true;
		copyList.at(index).interface->errorDetected();
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to locate the copy engine sender");
}

void Core::newOptionValue(const QString &group,const QString &name,const QVariant &value)
{
	if(group=="Write_log")
	{
		if(name=="transfer")
			log_enable_transfer=options->getOptionValue("Write_log","transfer").toBool() && value.toBool();
		else if(name=="error")
			log_enable_error=options->getOptionValue("Write_log","transfer").toBool() && value.toBool();
		else if(name=="folder")
			log_enable_folder=options->getOptionValue("Write_log","transfer").toBool() && value.toBool();
	}
}

//for the extra logging
void Core::rmPath(const QString &path)
{
	if(log_enable_error)
		log.rmPath(path);
}

void Core::mkPath(const QString &path)
{
	if(log_enable_error)
		log.mkPath(path);
}

void Core::urlDropped(QList<QUrl> urls)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=indexCopySenderInterface();
	if(index!=-1)
	{
		QStringList sources;
		int index_loop=0;
		while(index_loop<urls.size())
		{
			if(!urls.at(index_loop).isEmpty())
				sources << urls.at(index_loop).toLocalFile();
			index_loop++;
		}
		if(sources.size()==0)
			return;
		else
		{
			if(copyList.at(index).ignoreMode)
			{
				QMessageBox::StandardButton reply=QMessageBox::question(copyList.at(index).interface,tr("Transfer mode"),tr("Do you want do as a copy? Else if you reply no, it will be moved."),QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,QMessageBox::Cancel);
				if(reply==QMessageBox::Yes)
					copyList.at(index).engine->newCopy(sources);
				if(reply==QMessageBox::No)
					copyList.at(index).engine->newMove(sources);
			}
			else
			{
				if(copyList.at(index).mode==Copy)
					copyList.at(index).engine->newCopy(sources);
				else
					copyList.at(index).engine->newMove(sources);
			}
		}
	}
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"unable to locate the copy engine sender");
}
