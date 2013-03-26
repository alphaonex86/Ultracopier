/** \file SystrayIcon.cpp
\brief Define the class of the systray icon
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QMessageBox>
#include <QMimeData>

#include "SystrayIcon.h"
#include "PluginsManager.h"
#include "ThemesManager.h"
#include "LanguagesManager.h"

#ifdef Q_OS_MAC
//extern void qt_mac_set_dock_menu(QMenu *menu);
#endif

/// \brief Initiate and show the icon in the systray
SystrayIcon::SystrayIcon(QObject * parent) :
    QSystemTrayIcon(parent)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //setup the systray icon
    haveListenerInfo	= false;
    havePluginLoaderInfo	= false;
    systrayMenu         = new QMenu();
    actionMenuAbout		= new QAction(this);
    actionMenuQuit		= new QAction(this);
    actionOptions		= new QAction(this);
    //actionTransfer		= new QAction(this);
    copyMenu		= new QMenu();
    //to prevent init bug
    stateListener=Ultracopier::NotListening;
    statePluginLoader=Ultracopier::Uncaught;

    setContextMenu(systrayMenu);
    setToolTip("Ultracopier");
    #ifdef Q_OS_WIN32
    setIcon(QIcon(":/systray_Uncaught_Windows.png"));
    #else
    setIcon(QIcon(":/systray_Uncaught_Unix.png"));
    #endif
    //connect the action
    connect(&timerCheckSetTooltip,	&QTimer::timeout,					this,	&SystrayIcon::checkSetTooltip);
    connect(actionMenuQuit,		&QAction::triggered,					this,	&SystrayIcon::quit);
    connect(actionMenuAbout,	&QAction::triggered,					this,	&SystrayIcon::showHelp);
    connect(actionOptions,		&QAction::triggered,					this,	&SystrayIcon::showOptions);
    connect(this,			&SystrayIcon::activated,                    this,	&SystrayIcon::CatchAction);
    connect(PluginsManager::pluginsManager,		&PluginsManager::pluginListingIsfinish,			this,	&SystrayIcon::reloadEngineList);
    //display the icon
    updateCurrentTheme();
    //if theme/language change, update graphic part
    connect(ThemesManager::themesManager,			&ThemesManager::theThemeIsReloaded,             this,	&SystrayIcon::updateCurrentTheme, Qt::QueuedConnection);
    connect(LanguagesManager::languagesManager,	&LanguagesManager::newLanguageLoaded,			this,	&SystrayIcon::retranslateTheUI, Qt::QueuedConnection);
    systrayMenu->addMenu(copyMenu);
    systrayMenu->addAction(actionOptions);
    systrayMenu->addAction(actionMenuAbout);
    systrayMenu->addAction(actionMenuQuit);
    #ifndef Q_OS_MAC
    systrayMenu->insertSeparator(actionOptions);
    #endif
    retranslateTheUI();
    updateSystrayIcon();

    timerCheckSetTooltip.setSingleShot(true);
    timerCheckSetTooltip.start(1000);

    //impossible with Qt on systray
    /// \note important for drag and drop, \see dropEvent()
    systrayMenu->setAcceptDrops(true);

    #ifdef Q_OS_MAC
    //qt_mac_set_dock_menu(systrayMenu);
    #endif

    show();
}

/// \brief Hide and destroy the icon in the systray
SystrayIcon::~SystrayIcon()
{
    delete actionMenuQuit;
    delete actionMenuAbout;
    delete actionOptions;
    delete systrayMenu;
    delete copyMenu;
}

void SystrayIcon::checkSetTooltip()
{
    if(isSystemTrayAvailable())
    {
        setToolTip("Ultracopier");
        updateSystrayIcon();
    }
    else
        timerCheckSetTooltip.start();
}

void SystrayIcon::listenerReady(const Ultracopier::ListeningState &state,const bool &havePlugin,const bool &someAreInWaitOfReply)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("state: %1, havePlugin: %2, someAreInWaitOfReply: %3").arg(state).arg(havePlugin).arg(someAreInWaitOfReply));
    Q_UNUSED(someAreInWaitOfReply);
    stateListener=state;
    haveListenerInfo=true;
    haveListener=havePlugin;
    updateSystrayIcon();
    if(!havePlugin)
        showTryCatchMessageWithNoListener();
}

void SystrayIcon::pluginLoaderReady(const Ultracopier::CatchState &state,const bool &havePlugin,const bool &someAreInWaitOfReply)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("state: %1, havePlugin: %2, someAreInWaitOfReply: %3").arg(state).arg(havePlugin).arg(someAreInWaitOfReply));
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
    showMessage(tr("Information"),text,QSystemTrayIcon::Information,0);
}

/// \brief To update the systray icon
void SystrayIcon::updateSystrayIcon()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start, haveListenerInfo %1, havePluginLoaderInfo: %2").arg(haveListenerInfo).arg(havePluginLoaderInfo));
    QString toolTip="???";
    QString icon;
    if(!haveListenerInfo || !havePluginLoaderInfo)
    {
        toolTip=tr("Searching information...");
        icon="Uncaught";
    }
    else
    {
        Ultracopier::ListeningState stateListener=this->stateListener;
        Ultracopier::CatchState statePluginLoader=this->statePluginLoader;
        if(!haveListener)
            stateListener=Ultracopier::NotListening;
        if((stateListener==Ultracopier::NotListening && statePluginLoader==Ultracopier::Uncaught) || (stateListener==Ultracopier::SemiListening && statePluginLoader==Ultracopier::Semiuncaught) || (stateListener==Ultracopier::FullListening && statePluginLoader==Ultracopier::Caught))
        {
            if(stateListener==Ultracopier::NotListening)
            {
                toolTip=tr("Not replace the explorer copy/move");
                icon="Uncaught";
            }
            else if(stateListener==Ultracopier::SemiListening)
            {
                toolTip=tr("Semi replace the explorer copy/move");
                icon="Semiuncaught";
            }
            else
            {
                toolTip=tr("Replace the explorer copy/move");
                icon="Caught";
            }
        }
        else
        {
            icon="Semiuncaught";
            QString first_part;
            QString second_part;
            if(stateListener==Ultracopier::NotListening)
                first_part="No listening";
            else if(stateListener==Ultracopier::SemiListening)
                first_part="Semi listening";
            else if(stateListener==Ultracopier::FullListening)
                first_part="Full listening";
            else
                first_part="Unknow listening";
            if(statePluginLoader==Ultracopier::Uncaught)
                second_part="No replace";
            else if(statePluginLoader==Ultracopier::Semiuncaught)
                second_part="Semi replace";
            else if(statePluginLoader==Ultracopier::Caught)
                second_part="Full replace";
            else
                second_part="Unknow replace";
            toolTip=first_part+"/"+second_part;
        }
    }
    QIcon theNewSystrayIcon;
    #ifdef Q_OS_WIN32
    theNewSystrayIcon=ThemesManager::themesManager->loadIcon("SystemTrayIcon/systray_"+icon+"_Windows.png");
    #else
    theNewSystrayIcon=ThemesManager::themesManager->loadIcon("SystemTrayIcon/systray_"+icon+"_Unix.png");
    #endif
    if(theNewSystrayIcon.isNull())
    {
        #ifdef Q_OS_WIN32
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use the default systray icon: :/systray_"+icon+"_Windows.png");
        theNewSystrayIcon=QIcon(":/systray_"+icon+"_Windows.png");
        #else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use the default systray icon: :/systray_"+icon+"_Unix.png");
        theNewSystrayIcon=QIcon(":/systray_"+icon+"_Unix.png");
        #endif
    }
    else
    {
        #ifdef Q_OS_WIN32
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"icon: systray_"+icon+"_Windows.png");
        #else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"icon: systray_"+icon+"_Unix.png");
        #endif
    }
    if(theNewSystrayIcon.isNull())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"All the icon include the default icon remain null");
    setIcon(theNewSystrayIcon);
    setToolTip("Ultracopier - "+toolTip);
}

/* drag event processing (impossible with Qt on systray)

need setAcceptDrops(true); into the constructor
need implementation to accept the drop:
void dragEnterEvent(QDragEnterEvent* event);
void dragMoveEvent(QDragMoveEvent* event);
void dragLeaveEvent(QDragLeaveEvent* event);
*/
void SystrayIcon::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        //impossible with Qt on systray
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"mimeData->urls().size()"+QString::number(mimeData->urls().size()));
        emit urlDropped(mimeData->urls());
        event->acceptProposedAction();
    }
}

void SystrayIcon::dragEnterEvent(QDragEnterEvent* event)
{
    // if some actions should not be usable, like move, this code must be adopted
    event->acceptProposedAction();
}

void SystrayIcon::dragMoveEvent(QDragMoveEvent* event)
{
    // if some actions should not be usable, like move, this code must be adopted
    event->acceptProposedAction();
}

void SystrayIcon::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}


/// \brief To update the current themes
void SystrayIcon::updateCurrentTheme()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"icon: start");
    //load the systray menu item
    QIcon tempIcon;

    tempIcon=ThemesManager::themesManager->loadIcon("SystemTrayIcon/exit.png");
    if(!tempIcon.isNull())
        IconQuit=QIcon(tempIcon);
    else
        IconQuit=QIcon("");
    actionMenuQuit->setIcon(IconQuit);

    tempIcon=ThemesManager::themesManager->loadIcon("SystemTrayIcon/informations.png");
    if(!tempIcon.isNull())
        IconInfo=QIcon(tempIcon);
    else
        IconInfo=QIcon("");
    actionMenuAbout->setIcon(IconInfo);

    tempIcon=ThemesManager::themesManager->loadIcon("SystemTrayIcon/options.png");
    if(!tempIcon.isNull())
        IconOptions=QIcon(tempIcon);
    else
        IconOptions=QIcon("");
    actionOptions->setIcon(IconOptions);

    tempIcon=ThemesManager::themesManager->loadIcon("SystemTrayIcon/add.png");
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Double click on system tray icon");
        if(stateListener!=Ultracopier::NotListening)
            emit tryUncatchCopy();
        else
        {
            if(!haveListener)
            {
                showTryCatchMessageWithNoListener();
                return;
            }
            emit tryCatchCopy();
        }
    }
    else if(reason==QSystemTrayIcon::Context)//do nothing on right click to show as auto the menu
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("The action on the systray icon is unknown: %1").arg(reason));
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("The action on the systray icon is unknown: %1").arg(reason));
        QMessageBox::warning(NULL,tr("Warning"),tr("The action on the systray icon is unknown!"));
    }
}

/// \brief To catch copy menu action
void SystrayIcon::CatchCopyQuery()
{
    QAction * currentAction=qobject_cast<QAction *>(QObject::sender());
    if(currentAction==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action not found");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+currentAction->data().toString());
    emit addWindowCopyMove(Ultracopier::Copy,currentAction->data().toString());
}

/// \brief To catch move menu action
void SystrayIcon::CatchMoveQuery()
{
    QAction * currentAction=qobject_cast<QAction *>(QObject::sender());
    if(currentAction==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action not found");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+currentAction->data().toString());
    emit addWindowCopyMove(Ultracopier::Move,currentAction->data().toString());
}

/// \brief To catch transfer menu action
void SystrayIcon::CatchTransferQuery()
{
    QAction * currentAction=qobject_cast<QAction *>(QObject::sender());
    if(currentAction==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action not found");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+currentAction->data().toString());
    emit addWindowTransfer(currentAction->data().toString());
}

/// \brief to retranslate the ui
void SystrayIcon::retranslateTheUI()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"retranslateTheUI");
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    EngineEntry entry;
    entry.name=name;
    entry.canDoOnlyCopy=canDoOnlyCopy;
    engineEntryList << entry;
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        reloadEngineList();
}

void SystrayIcon::removeCopyEngine(const QString &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
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
    reloadEngineList();
}

#ifdef ULTRACOPIER_INTERNET_SUPPORT
void SystrayIcon::newUpdate(const QString &version)
{
    showSystrayMessage(tr("New version: %1").arg(version));
}
#endif

void SystrayIcon::reloadEngineList()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    copyMenu->clear();
    if(engineEntryList.size()==0)
    {
        copyMenu->setEnabled(false);
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"engineEntryList.size(): "+QString::number(engineEntryList.size()));
    copyMenu->setEnabled(true);
    if(engineEntryList.size()==1)
    {
        QAction *copy=new QAction(IconAdd,tr("Add &copy"),copyMenu);
        connect(copy,&QAction::triggered,this,&SystrayIcon::CatchCopyQuery);
        copy->setData(engineEntryList.first().name);
        copyMenu->addAction(copy);
        if(!engineEntryList.first().canDoOnlyCopy)
        {
            QAction *transfer=new QAction(IconAdd,tr("Add &transfer"),copyMenu);
            connect(transfer,&QAction::triggered,this,&SystrayIcon::CatchTransferQuery);
            transfer->setData(engineEntryList.first().name);
            copyMenu->addAction(transfer);
            QAction *move=new QAction(IconAdd,tr("Add &move"),copyMenu);
            connect(move,&QAction::triggered,this,&SystrayIcon::CatchMoveQuery);
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
            connect(copy,&QAction::triggered,this,&SystrayIcon::CatchCopyQuery);
            copy->setData(engineEntryList.at(index).name);
            menu->addAction(copy);
            if(!engineEntryList.at(index).canDoOnlyCopy)
            {
                QAction *transfer=new QAction(IconAdd,tr("Add &transfer"),menu);
                connect(transfer,&QAction::triggered,this,&SystrayIcon::CatchTransferQuery);
                transfer->setData(engineEntryList.at(index).name);
                menu->addAction(transfer);
                QAction *move=new QAction(IconAdd,tr("Add &move"),menu);
                connect(move,&QAction::triggered,this,&SystrayIcon::CatchMoveQuery);
                move->setData(engineEntryList.at(index).name);
                menu->addAction(move);
            }
            copyMenu->addMenu(menu);
            index++;
        }
    }
}
