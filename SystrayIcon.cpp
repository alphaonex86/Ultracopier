/** \file SystrayIcon.cpp
\brief Define the class of the systray icon
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QMessageBox>

#include "SystrayIcon.h"

/// \brief Initiate and show the icon in the systray
SystrayIcon::SystrayIcon()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//setup the systray icon
	haveListenerInfo	= false;
	havePluginLoaderInfo	= false;
	sysTrayIcon		= new QSystemTrayIcon();
	systrayMenu		= new QMenu();
	actionMenuAbout		= new QAction(this);
	actionMenuQuit		= new QAction(this);
	actionOptions		= new QAction(this);
	//actionTransfer		= new QAction(this);
	copyMenu		= new QMenu();
	//to prevent init bug
	stateListener=NotListening;
	statePluginLoader=Uncaught;

	sysTrayIcon->setContextMenu(systrayMenu);
	sysTrayIcon->setToolTip("Ultracopier");
	#ifdef Q_OS_WIN32
	sysTrayIcon->setIcon(QIcon(":/systray_Uncaught_Windows.png"));
	#else
	sysTrayIcon->setIcon(QIcon(":/systray_Uncaught_Unix.png"));
	#endif
	sysTrayIcon->show();
	//connect the action
	connect(&timerCheckSetTooltip,	SIGNAL(timeout()),					this,	SLOT(checkSetTooltip()));
	connect(actionMenuQuit,		SIGNAL(triggered()),					this,	SIGNAL(quit()));
	connect(actionMenuAbout,	SIGNAL(triggered()),					this,	SIGNAL(showHelp()));
	connect(actionOptions,		SIGNAL(triggered()),					this,	SIGNAL(showOptions()));
	connect(sysTrayIcon,		SIGNAL(activated(QSystemTrayIcon::ActivationReason)),	this,	SLOT(CatchAction(QSystemTrayIcon::ActivationReason)));
	connect(plugins,		SIGNAL(pluginListingIsfinish()),			this,	SLOT(reloadEngineList()));
	//display the icon
	updateCurrentTheme();
	//if theme/language change, update graphic part
	connect(themes,		SIGNAL(theThemeIsReloaded()),				this,	SLOT(updateCurrentTheme()));
	connect(languages,	SIGNAL(newLanguageLoaded(QString)),			this,	SLOT(retranslateTheUI()));
	systrayMenu->addMenu(copyMenu);
	systrayMenu->addAction(actionOptions);
	systrayMenu->addAction(actionMenuAbout);
	systrayMenu->addAction(actionMenuQuit);
	systrayMenu->insertSeparator(actionOptions);
	retranslateTheUI();
	updateSystrayIcon();

	timerCheckSetTooltip.setSingleShot(true);
	timerCheckSetTooltip.start(1000);
}

/// \brief Hide and destroy the icon in the systray
SystrayIcon::~SystrayIcon()
{
	delete actionMenuQuit;
	delete actionMenuAbout;
	delete actionOptions;
	delete systrayMenu;
	delete copyMenu;
	delete sysTrayIcon;
}

void SystrayIcon::checkSetTooltip()
{
	if(sysTrayIcon->isSystemTrayAvailable())
	{
		sysTrayIcon->setToolTip("Ultracopier");
		updateSystrayIcon();
	}
	else
		timerCheckSetTooltip.start();
}

void SystrayIcon::listenerReady(const ListeningState &state,const bool &havePlugin,const bool &someAreInWaitOfReply)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("state: %1, havePlugin: %2, someAreInWaitOfReply: %3").arg(state).arg(havePlugin).arg(someAreInWaitOfReply));
	Q_UNUSED(someAreInWaitOfReply);
	stateListener=state;
	haveListenerInfo=true;
	haveListener=havePlugin;
	updateSystrayIcon();
	if(!havePlugin)
		showTryCatchMessageWithNoListener();
}

void SystrayIcon::pluginLoaderReady(const CatchState &state,const bool &havePlugin,const bool &someAreInWaitOfReply)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("state: %1, havePlugin: %2, someAreInWaitOfReply: %3").arg(state).arg(havePlugin).arg(someAreInWaitOfReply));
	Q_UNUSED(someAreInWaitOfReply);
	statePluginLoader=state;
	havePluginLoaderInfo=true;
	havePluginLoader=havePlugin;
	updateSystrayIcon();
}

void SystrayIcon::showTryCatchMessageWithNoListener()
{
	showSystrayMessage(tr("No copy listener found. Do the copy manually by right click one the system tray icon."));
}

/// \brief To show a message linked to the systray icon
void SystrayIcon::showSystrayMessage(const QString& text)
{
	sysTrayIcon->showMessage(tr("Information"),text,QSystemTrayIcon::Information,0);
}

/// \brief To update the systray icon
void SystrayIcon::updateSystrayIcon()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("start, haveListenerInfo %1, havePluginLoaderInfo: %2").arg(haveListenerInfo).arg(havePluginLoaderInfo));
	QString toolTip="???";
	QString icon;
	if(!haveListenerInfo || !havePluginLoaderInfo)
	{
		toolTip=tr("Searching informations...");
		icon="Uncaught";
	}
	else
	{
		ListeningState stateListener=this->stateListener;
		CatchState statePluginLoader=this->statePluginLoader;
		if(!haveListener)
			stateListener=NotListening;
		if(!havePluginLoader)
			statePluginLoader=Caught;
		if((stateListener==NotListening && statePluginLoader==Uncaught) || (stateListener==SemiListening && statePluginLoader==Semiuncaught) || (stateListener==FullListening && statePluginLoader==Caught))
		{
			if(stateListener==NotListening)
			{
				toolTip=tr("Not catching the explorer copy/move");
				icon="Uncaught";
			}
			else if(stateListener==SemiListening)
			{
				toolTip=tr("Semi catching the explorer copy/move");
				icon="Semiuncaught";
			}
			else
			{
				toolTip=tr("Catching the explorer copy/move");
				icon="Caught";
			}
		}
		else
		{
			icon="Semiuncaught";
			QString first_part;
			QString second_part;
			if(stateListener==NotListening)
				first_part="No listening";
			else if(stateListener==SemiListening)
				first_part="Semi listening";
			else if(stateListener==FullListening)
				first_part="Full listening";
			else
				first_part="Unknow listening";
			if(statePluginLoader==Uncaught)
				second_part="No catching";
			else if(statePluginLoader==Semiuncaught)
				second_part="Semi catching";
			else if(statePluginLoader==Caught)
				second_part="Full catching";
			else
				second_part="Unknow catching";
			toolTip=first_part+"/"+second_part;
		}
	}
	QIcon theNewSystrayIcon;
	#ifdef Q_OS_WIN32
	theNewSystrayIcon=themes->loadIcon("SystemTrayIcon/systray_"+icon+"_Windows.png");
	#else
	theNewSystrayIcon=themes->loadIcon("SystemTrayIcon/systray_"+icon+"_Unix.png");
	#endif
	if(theNewSystrayIcon.isNull())
	{
		#ifdef Q_OS_WIN32
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"use the default systray icon: :/systray_"+icon+"_Windows.png");
		theNewSystrayIcon=QIcon(":/systray_"+icon+"_Windows.png");
		#else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"use the default systray icon: :/systray_"+icon+"_Unix.png");
		theNewSystrayIcon=QIcon(":/systray_"+icon+"_Unix.png");
		#endif
	}
	else
	{
		#ifdef Q_OS_WIN32
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"icon: systray_"+icon+"_Windows.png");
		#else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"icon: systray_"+icon+"_Unix.png");
		#endif
	}
	sysTrayIcon->setIcon(theNewSystrayIcon);
	sysTrayIcon->setToolTip("Ultracopier - "+toolTip);
}

/// \brief To update the current themes
void SystrayIcon::updateCurrentTheme()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"icon: start");
	//load the systray menu item
	QIcon tempIcon;

	tempIcon=themes->loadIcon("SystemTrayIcon/exit.png");
	if(!tempIcon.isNull())
		IconQuit=QIcon(tempIcon);
	else
		IconQuit=QIcon("");
	actionMenuQuit->setIcon(IconQuit);

	tempIcon=themes->loadIcon("SystemTrayIcon/informations.png");
	if(!tempIcon.isNull())
		IconInfo=QIcon(tempIcon);
	else
		IconInfo=QIcon("");
	actionMenuAbout->setIcon(IconInfo);

	tempIcon=themes->loadIcon("SystemTrayIcon/options.png");
	if(!tempIcon.isNull())
		IconOptions=QIcon(tempIcon);
	else
		IconOptions=QIcon("");
	actionOptions->setIcon(IconOptions);

	tempIcon=themes->loadIcon("SystemTrayIcon/add.png");
	if(!tempIcon.isNull())
		IconAdd=QIcon(tempIcon);
	else
		IconAdd=QIcon("");
	copyMenu->setIcon(IconAdd);

	//update the systray icon
	updateSystrayIcon();
	reloadEngineList();
}

/* \brief For catch an action on the systray icon
\param reason Why it activated */
void SystrayIcon::CatchAction(QSystemTrayIcon::ActivationReason reason)
{
	if(reason==QSystemTrayIcon::DoubleClick || reason==QSystemTrayIcon::Trigger)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"Double click on system tray icon");
		if(stateListener!=NotListening || statePluginLoader!=Uncaught)
			emit tryUncatchCopy();
		else
		{
			if(!haveListener)
				showTryCatchMessageWithNoListener();
			emit tryCatchCopy();
		}
	}
	else if(reason==QSystemTrayIcon::Context)//do nothing on right click to show as auto the menu
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("Action on the systray icon is unknown: %1").arg(reason));
	else
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Action on the systray icon is unknown: %1").arg(reason));
		QMessageBox::warning(NULL,tr("Warning"),tr("Action on the systray icon is unknown!"));
	}
}

/// \brief To catch copy menu action
void SystrayIcon::CatchCopyQuery()
{
	QAction * currentAction=qobject_cast<QAction *>(QObject::sender());
	if(currentAction==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"action not found");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+currentAction->data().toString());
	emit addWindowCopyMove(Copy,currentAction->data().toString());
}

/// \brief To catch move menu action
void SystrayIcon::CatchMoveQuery()
{
	QAction * currentAction=qobject_cast<QAction *>(QObject::sender());
	if(currentAction==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"action not found");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+currentAction->data().toString());
	emit addWindowCopyMove(Move,currentAction->data().toString());
}

/// \brief To catch transfer menu action
void SystrayIcon::CatchTransferQuery()
{
	QAction * currentAction=qobject_cast<QAction *>(QObject::sender());
	if(currentAction==NULL)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"action not found");
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+currentAction->data().toString());
	emit addWindowTransfer(currentAction->data().toString());
}

/// \brief to retranslate the ui
void SystrayIcon::retranslateTheUI()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"retranslateTheUI");
	#ifdef ULTRACOPIER_DEBUG
	actionMenuAbout		->setText(tr("&About/Debug report"));
	#else // ULTRACOPIER_DEBUG
	actionMenuAbout		->setText(tr("&About"));
	#endif // ULTRACOPIER_DEBUG
	actionMenuQuit		->setText(tr("&Quit"));
	actionOptions		->setText(tr("&Options"));
	copyMenu		->setTitle(tr("A&dd copy/moving"));
	reloadEngineList();
	updateSystrayIcon();
}

void SystrayIcon::addCopyEngine(const QString &name,const bool &canDoOnlyCopy)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	EngineEntry entry;
	entry.name=name;
	entry.canDoOnlyCopy=canDoOnlyCopy;
	engineEntryList << entry;
	if(plugins->allPluginHaveBeenLoaded())
		reloadEngineList();
}

void SystrayIcon::removeCopyEngine(const QString &name)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	int index=0;
	while(index<engineEntryList.size())
	{
		if(engineEntryList.at(index).name==name)
		{
			engineEntryList.removeAt(index);
			break;
		}
		index++;
	}
	void reloadEngineList();
}

void SystrayIcon::reloadEngineList()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	copyMenu->clear();
	if(engineEntryList.size()==0)
	{
		copyMenu->setEnabled(false);
		return;
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"engineEntryList.size(): "+QString::number(engineEntryList.size()));
	copyMenu->setEnabled(true);
	if(engineEntryList.size()==1)
	{
		QAction *copy=new QAction(IconAdd,tr("Add &copy"),copyMenu);
		connect(copy,SIGNAL(triggered()),this,SLOT(CatchCopyQuery()));
		copy->setData(engineEntryList.first().name);
		copyMenu->addAction(copy);
		if(!engineEntryList.first().canDoOnlyCopy)
		{
			QAction *transfer=new QAction(IconAdd,tr("Add &transfer"),copyMenu);
			connect(transfer,SIGNAL(triggered()),this,SLOT(CatchTransferQuery()));
			transfer->setData(engineEntryList.first().name);
			copyMenu->addAction(transfer);
			QAction *move=new QAction(IconAdd,tr("Add &move"),copyMenu);
			connect(move,SIGNAL(triggered()),this,SLOT(CatchMoveQuery()));
			move->setData(engineEntryList.first().name);
			copyMenu->addAction(move);
		}
	}
	else
	{
		int index=0;
		while(index<engineEntryList.size())
		{
			QMenu * menu=new QMenu(engineEntryList.at(index).name);
			QAction *copy=new QAction(IconAdd,tr("Add &copy"),menu);
			connect(copy,SIGNAL(triggered()),this,SLOT(CatchCopyQuery()));
			copy->setData(engineEntryList.at(index).name);
			menu->addAction(copy);
			if(!engineEntryList.at(index).canDoOnlyCopy)
			{
				QAction *transfer=new QAction(IconAdd,tr("Add &transfer"),menu);
				connect(transfer,SIGNAL(triggered()),this,SLOT(CatchTransferQuery()));
				transfer->setData(engineEntryList.at(index).name);
				menu->addAction(transfer);
				QAction *move=new QAction(IconAdd,tr("Add &move"),menu);
				connect(move,SIGNAL(triggered()),this,SLOT(CatchMoveQuery()));
				move->setData(engineEntryList.at(index).name);
				menu->addAction(move);
			}
			copyMenu->addMenu(menu);
			index++;
		}
	}
}
