/** \file SystrayIcon.cpp
\brief Define the class of the systray icon
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QMessageBox>
#include <QMimeData>
#include <QDesktopServices>

#include "SystrayIcon.h"
#include "PluginsManager.h"
#include "ThemesManager.h"
#include "LanguagesManager.h"
#include "HelpDialog.h"
#include "ProductKey.h"

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
    #ifdef ULTRACOPIER_DEBUG
    actionSaveBugReport		= new QAction(this);
    #endif
    actionMenuQuit		= new QAction(this);
    actionOptions		= new QAction(this);
    actionProductKey    = new QAction(this);
    //actionTransfer		= new QAction(this);
    #if ! defined(Q_OS_LINUX) || (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    copyMenu		= NULL;
    #endif
    //to prevent init bug
    stateListener=Ultracopier::NotListening;
    statePluginLoader=Ultracopier::Uncaught;

    setContextMenu(systrayMenu);
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
     setToolTip(QStringLiteral("Supercopier"));
     #else
      setToolTip(QStringLiteral("Ultracopier"));
     #endif
    #ifdef Q_OS_WIN32
    setIcon(QIcon(QStringLiteral(":/systray_Uncaught_Windows.png")));
    #else
    setIcon(QIcon(QStringLiteral(":/systray_Uncaught_Unix.png")));
    #endif
    //connect the action
    connect(&timerCheckSetTooltip,	&QTimer::timeout,					this,	&SystrayIcon::checkSetTooltip);
    #ifdef ULTRACOPIER_DEBUG
    connect(actionSaveBugReport,	&QAction::triggered,			this,	&SystrayIcon::saveBugReport);
    #endif
    connect(actionMenuQuit,		&QAction::triggered,					this,	&SystrayIcon::hide);
    connect(actionMenuQuit,		&QAction::triggered,					this,	&SystrayIcon::quit);
    connect(actionMenuAbout,	&QAction::triggered,					this,	&SystrayIcon::showHelp);
    connect(actionOptions,		&QAction::triggered,					this,	&SystrayIcon::showOptions);
    connect(actionProductKey,	&QAction::triggered,					this,	&SystrayIcon::showProductKey);
    connect(this,			&SystrayIcon::activated,                    this,	&SystrayIcon::CatchAction);
    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    connect(this,			&QSystemTrayIcon::messageClicked,           this,	&SystrayIcon::messageClicked);
    #endif
    connect(PluginsManager::pluginsManager,		&PluginsManager::pluginListingIsfinish,			this,	&SystrayIcon::reloadEngineList);
    //display the icon
    updateCurrentTheme();
    //if theme/language change, update graphic part
    connect(ThemesManager::themesManager,			&ThemesManager::theThemeIsReloaded,             this,	&SystrayIcon::updateCurrentTheme, Qt::QueuedConnection);
    connect(LanguagesManager::languagesManager,	&LanguagesManager::newLanguageLoaded,			this,	&SystrayIcon::retranslateTheUI, Qt::QueuedConnection);

    systrayMenu->addAction(actionOptions);
    systrayMenu->addAction(actionMenuAbout);
    #ifdef ULTRACOPIER_DEBUG
    systrayMenu->addAction(actionSaveBugReport);
    #endif
    if(!ProductKey::productKey->isUltimate())
        systrayMenu->addAction(actionProductKey);
    systrayMenu->addAction(actionMenuQuit);
    #ifndef Q_OS_MAC
    systrayMenu->insertSeparator(actionOptions);
    #endif
    retranslateTheUI();
    updateSystrayIcon();

    #ifdef ULTRACOPIER_INTERNET_SUPPORT
    lastVersion=ULTRACOPIER_VERSION;
    #endif

    timerCheckSetTooltip.setSingleShot(true);
    timerCheckSetTooltip.start(1000);

    //impossible with Qt on systray
    /// \note important for drag and drop, \see dropEvent()
    systrayMenu->setAcceptDrops(true);

    #ifdef Q_OS_MAC
//    qt_mac_set_dock_menu(systrayMenu);
    #endif

    show();
}

/// \brief Hide and destroy the icon in the systray
SystrayIcon::~SystrayIcon()
{
    delete actionMenuQuit;
    #ifdef ULTRACOPIER_DEBUG
    delete actionSaveBugReport;
    #endif
    delete actionMenuAbout;
    delete actionOptions;
    delete actionProductKey;
    delete systrayMenu;
    #if ! defined(Q_OS_LINUX) || (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    if(copyMenu!=NULL)
    {
        delete copyMenu;
        copyMenu=NULL;
    }
    #endif
}

void SystrayIcon::checkSetTooltip()
{
    if(isSystemTrayAvailable())
    {
        #ifdef ULTRACOPIER_MODE_SUPERCOPIER
        setToolTip(QStringLiteral("Supercopier"));
        #else
        setToolTip(QStringLiteral("Ultracopier"));
        #endif
        updateSystrayIcon();
    }
    else
        timerCheckSetTooltip.start();
}

void SystrayIcon::listenerReady(const Ultracopier::ListeningState &state,const bool &havePlugin,const bool &someAreInWaitOfReply)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"state: "+std::to_string((int)state)+", havePlugin: "+std::to_string((int)havePlugin)+", someAreInWaitOfReply: "+std::to_string((int)someAreInWaitOfReply));
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"state: "+std::to_string((int)state)+", havePlugin: "+std::to_string((int)havePlugin)+", someAreInWaitOfReply: "+std::to_string((int)someAreInWaitOfReply));
    Q_UNUSED(someAreInWaitOfReply);
    statePluginLoader=state;
    havePluginLoaderInfo=true;
    havePluginLoader=havePlugin;
    updateSystrayIcon();
}

void SystrayIcon::showTryCatchMessageWithNoListener()
{
    showSystrayMessage(tr("No copy listener found. Do the copy manually by right click one the system tray icon.").toStdString());
}

/// \brief To show a message linked to the systray icon
void SystrayIcon::showSystrayMessage(const std::string& text)
{
    showMessage(tr("Information"),QString::fromStdString(text),QSystemTrayIcon::Information,0);
}

#ifdef ULTRACOPIER_INTERNET_SUPPORT
void SystrayIcon::messageClicked()
{
    if(!QDesktopServices::openUrl(QString::fromStdString(HelpDialog::getUpdateUrl())))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"start, haveListenerInfo "+std::to_string((int)haveListenerInfo)+", havePluginLoaderInfo: "+std::to_string((int)havePluginLoaderInfo));
}
#endif

/// \brief To update the systray icon
void SystrayIcon::updateSystrayIcon()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, haveListenerInfo "+std::to_string((int)haveListenerInfo)+", havePluginLoaderInfo: "+std::to_string((int)havePluginLoaderInfo));
    std::string toolTip="???";
    std::string icon;
    if(!haveListenerInfo || !havePluginLoaderInfo)
    {
        toolTip=tr("Searching information...").toStdString();
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
                toolTip=tr("Do not replace the explorer copy/move").toStdString();
                icon="Uncaught";
            }
            else if(stateListener==Ultracopier::SemiListening)
            {
                toolTip=tr("Semi replace the explorer copy/move").toStdString();
                icon="Semiuncaught";
            }
            else
            {
                toolTip=tr("Replace the explorer copy/move").toStdString();
                icon="Caught";
            }
        }
        else
        {
            icon="Semiuncaught";
            std::string first_part;
            std::string second_part;
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
        theNewSystrayIcon=QIcon(QString::fromStdString(":/systray_"+icon+"_Windows.png"));
        #else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use the default systray icon: :/systray_"+icon+"_Unix.png");
        theNewSystrayIcon=QIcon(QString::fromStdString(":/systray_"+icon+"_Unix.png"));
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
    #ifdef ULTRACOPIER_MODE_SUPERCOPIER
    setToolTip(QString::fromStdString("Supercopier - "+toolTip));
    #else
    setToolTip(QString::fromStdString("Ultracopier - "+toolTip));
    #endif
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"mimeData->urls().size()"+std::to_string(mimeData->urls().size()));
        std::vector<std::string> urls;
        unsigned int index=0;
        while(index<(unsigned int)mimeData->urls().size())
        {
            urls.push_back(mimeData->urls().at(static_cast<int>(index)).toString().toStdString());
            index++;
        }
        emit urlDropped(urls);
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

    #ifdef ULTRACOPIER_DEBUG
    actionSaveBugReport->setIcon(QIcon(":/warning.png"));
    #endif

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

    actionProductKey->setIcon(IconInfo);

    tempIcon=ThemesManager::themesManager->loadIcon("SystemTrayIcon/add.png");
    if(!tempIcon.isNull())
        IconAdd=QIcon(tempIcon);
    else
        IconAdd=QIcon("");

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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"The action on the systray icon is unknown: "+std::to_string((int)reason));
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"The action on the systray icon is unknown: "+std::to_string((int)reason));
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+currentAction->data().toString().toStdString());
    emit addWindowCopyMove(Ultracopier::Copy,currentAction->data().toString().toStdString());
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+currentAction->data().toString().toStdString());
    emit addWindowCopyMove(Ultracopier::Move,currentAction->data().toString().toStdString());
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+currentAction->data().toString().toStdString());
    emit addWindowTransfer(currentAction->data().toString().toStdString());
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
    #ifdef ULTRACOPIER_DEBUG
    actionSaveBugReport->setText(tr("&Save bug report"));
    #endif
    actionMenuQuit		->setText(tr("&Quit"));
    actionOptions		->setText(tr("&Options"));
    actionProductKey		->setText(tr("&Product key"));
    reloadEngineList();
    updateSystrayIcon();
}

void SystrayIcon::addCopyEngine(const std::string &name,const bool &canDoOnlyCopy)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    EngineEntry entry;
    entry.name=name;
    entry.canDoOnlyCopy=canDoOnlyCopy;
    engineEntryList.push_back(entry);
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        reloadEngineList();
}

void SystrayIcon::removeCopyEngine(const std::string &name)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    unsigned int index=0;
    while(index<engineEntryList.size())
    {
        if(engineEntryList.at(index).name==name)
        {
            engineEntryList.erase(engineEntryList.cbegin()+index);
            break;
        }
        index++;
    }
    reloadEngineList();
}

#ifdef ULTRACOPIER_INTERNET_SUPPORT
void SystrayIcon::newUpdate(const std::string &version)
{
    /*if(version==lastVersion)
        return;*/
    lastVersion=version;
    showSystrayMessage((tr("New version: %1").arg(QString::fromStdString(version))+"\n"+tr("Go to the download page:")).toStdString()+"\n"+HelpDialog::getUpdateUrl());
}
#endif

void SystrayIcon::addEngineAction(const QString &name, const QIcon &icon, const QString &label, QMenu *menu, void (SystrayIcon::*query)())
{
    QAction *copy = new QAction(icon, label, menu);
    connect(copy,&QAction::triggered, this, query);
    copy->setData(name);
    #if ! defined(Q_OS_LINUX) || (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    copyMenu->addAction(copy);
    #else
    actions.push_back(copy);
    systrayMenu->insertAction(actionOptions, copy);
    #endif
}

void SystrayIcon::reloadEngineList()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    #if ! defined(Q_OS_LINUX) || (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    if(copyMenu!=NULL)
    {
        delete copyMenu;
        copyMenu=NULL;
    }
    copyMenu=new QMenu();
    copyMenu		->setTitle(tr("A&dd copy/moving"));
    copyMenu->setIcon(IconAdd);
    systrayMenu->insertMenu(actionOptions,copyMenu);
    copyMenu->setEnabled(true);
    if(engineEntryList.size()==0)
    {
        copyMenu->setEnabled(false);
        return;
    }
    #else
    {
        for(unsigned int index=0; index<actions.size(); index++)
        {
            delete actions.at(index);
        }
        actions.clear();
    }
    #endif

    if(engineEntryList.size()==0)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"engineEntryList.size(): "+std::to_string(engineEntryList.size()));
    for(unsigned int index=0; index<engineEntryList.size(); index++)
    {
        const EngineEntry &engineEntry = engineEntryList.at(index);
        const QString &name = QString::fromStdString(engineEntry.name);
        QString labelCopy     = tr("Add &copy");
        QString labelTransfer = tr("Add &transfer");
        QString labelMove     = tr("Add &move");
        QMenu *menu = nullptr;
        #if ! defined(Q_OS_LINUX) || (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
        if(engineEntryList.size()==1)
            menu = copyMenu;
        else
            menu = new QMenu(name);
        #else
        if(engineEntryList.size()!=1) {
            labelCopy     += " ("+name+")";
            labelTransfer += " ("+name+")";
            labelMove     += " ("+name+")";
        }
        #endif
        addEngineAction(name, IconAdd, labelCopy, menu, &SystrayIcon::CatchCopyQuery);
        if(!engineEntry.canDoOnlyCopy)
        {
            addEngineAction(name, IconAdd, labelTransfer, menu, &SystrayIcon::CatchTransferQuery);
            addEngineAction(name, IconAdd, labelMove, menu, &SystrayIcon::CatchMoveQuery);
        }
        #if ! defined(Q_OS_LINUX) || (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
        if(engineEntryList.size()!=1)
            copyMenu->addMenu(menu);
        #endif
    }
    setContextMenu(systrayMenu);
}
