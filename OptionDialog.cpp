/** \file OptionDialog.cpp
\brief To have an interface to control the options
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "OptionDialog.h"
#include "ui_OptionDialog.h"

#include <QDomElement>
#include <QFileDialog>
#include <QMessageBox>

OptionDialog::OptionDialog() :
    ui(new Ui::OptionDialog)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    ignoreCopyEngineListEdition=false;
    allPluginsIsLoaded=false;
    ui->setupUi(this);
    ui->treeWidget->topLevelItem(0)->setSelected(true);
    ui->treeWidget->topLevelItem(3)->setTextColor(0,QColor(150, 150, 150, 255));
    ui->treeWidget->topLevelItem(4)->setTextColor(0,QColor(150, 150, 150, 255));
    ui->treeWidget->topLevelItem(5)->setTextColor(0,QColor(150, 150, 150, 255));
    ui->treeWidget->expandAll();
    ui->pluginList->expandAll();
    number_of_listener=0;
    ui->labelCatchCopyDefault->setEnabled(number_of_listener>0);
    ui->CatchCopyAsDefault->setEnabled(number_of_listener>0);
    ui->Language->setEnabled(false);
    on_treeWidget_itemSelectionChanged();


    //load the plugins
    plugins->lockPluginListEdition();
    QList<PluginsAvailable> list=plugins->getPlugins();
    qRegisterMetaType<PluginsAvailable>("PluginsAvailable");
    connect(this,&OptionDialog::previouslyPluginAdded,			this,	&OptionDialog::onePluginAdded,Qt::QueuedConnection);
    connect(plugins,	&PluginsManager::onePluginAdded,		this,	&OptionDialog::onePluginAdded);
    connect(plugins,	&PluginsManager::onePluginWillBeRemoved,	this,	&OptionDialog::onePluginWillBeRemoved,Qt::DirectConnection);
    connect(plugins,	&PluginsManager::pluginListingIsfinish,		this,	&OptionDialog::loadOption,Qt::QueuedConnection);
    connect(plugins,	&PluginsManager::manuallyAdded,		this,	&OptionDialog::manuallyAdded,Qt::QueuedConnection);
    connect(options,	&OptionEngine::newOptionValue,			this,	&OptionDialog::newOptionValue);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    plugins->unlockPluginListEdition();
    defaultImportBackend=PluginsManager::ImportBackend_File;
    #ifndef ULTRACOPIER_PLUGIN_SUPPORT
    ui->pluginAdd->show();
    ui->pluginRemove->show();
    #endif
}

OptionDialog::~OptionDialog()
{
    delete ui;
}

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

void OptionDialog::manuallyAdded(const PluginsAvailable &plugin)
{
    if(plugin.category==PluginType_Themes)
    {
        if(QMessageBox::question(this,tr("Load"),tr("Load the themes?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)==QMessageBox::Yes)
        {
            int index=ui->Ultracopier_current_theme->findData(plugin.name);
            if(index!=-1)
                ui->Ultracopier_current_theme->setCurrentIndex(index);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"theme plugin not found!");
        }
    }
}

void OptionDialog::addLanguage(PluginsAvailable plugin)
{
    QList<QPair<QString,QString> > listChildAttribute;
    QPair<QString,QString> temp;
    temp.first = "mainCode";
    temp.second = "true";
    listChildAttribute << temp;
    ui->Language->addItem(QIcon(plugin.path+"flag.png"),plugins->getDomSpecific(plugin.categorySpecific,"fullName"),plugins->getDomSpecific(plugin.categorySpecific,"shortName",listChildAttribute));
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
    int index=ui->Language->findData(plugins->getDomSpecific(plugin.categorySpecific,"shortName",listChildAttribute));
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
        ui->labelLoadAtSession->setToolTip(tr("Disabled because you have any SessionLoader plugin"));
        ui->LoadAtSessionStarting->setToolTip(tr("Disabled because you have any SessionLoader plugin"));
        ui->ActionOnManualOpen->setItemText(0,tr("Do nothing"));
        ui->ActionOnManualOpen->setItemText(1,tr("Ask source as folder"));
        ui->ActionOnManualOpen->setItemText(2,tr("Ask sources as files"));
        ui->GroupWindowWhen->setItemText(0,tr("Never"));
        ui->GroupWindowWhen->setItemText(1,tr("When source is same"));
        ui->GroupWindowWhen->setItemText(2,tr("When destination is same"));
        ui->GroupWindowWhen->setItemText(3,tr("When source and destination are same"));
        ui->GroupWindowWhen->setItemText(4,tr("When source or destination are same"));
        ui->GroupWindowWhen->setItemText(5,tr("Always"));
        break;
    default:
        break;
    }
}

void OptionDialog::on_treeWidget_itemSelectionChanged()
{
    QList<QTreeWidgetItem *> listSelectedItem=ui->treeWidget->selectedItems();
    if(listSelectedItem.size()!=1)
        return;
    QTreeWidgetItem * selectedItem=listSelectedItem.first();
    //general
    if(selectedItem==ui->treeWidget->topLevelItem(0))
        ui->stackedWidget->setCurrentIndex(0);
    //plugins
    else if(selectedItem==ui->treeWidget->topLevelItem(1))
        ui->stackedWidget->setCurrentIndex(1);
    //Copy engine
    else if(selectedItem==ui->treeWidget->topLevelItem(2))
        ui->stackedWidget->setCurrentIndex(2);
    //Listener
        //do nothing
    //PluginLoader
        //do nothing
    //SessionLoader
        //do nothing
    //Themes
    else if(selectedItem==ui->treeWidget->topLevelItem(6))
        ui->stackedWidget->setCurrentIndex(7);
    //log
    else if(selectedItem==ui->treeWidget->topLevelItem(7))
        ui->stackedWidget->setCurrentIndex(8);
    else
    {
        int index;
        if(selectedItem->parent()==ui->treeWidget->topLevelItem(2))
        {
            ui->stackedWidget->setCurrentIndex(3);
            index=selectedItem->parent()->indexOfChild(selectedItem);
            if(index!=-1)
                ui->stackedOptionsCopyEngine->setCurrentIndex(index);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"selection into of sub item wrong???");
        }
        else if(selectedItem->parent()==ui->treeWidget->topLevelItem(3))
        {
            ui->stackedWidget->setCurrentIndex(4);
            index=selectedItem->parent()->indexOfChild(selectedItem);
            if(index!=-1)
                ui->stackedOptionsListener->setCurrentIndex(index);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"selection into of sub item wrong???");
        }
        else if(selectedItem->parent()==ui->treeWidget->topLevelItem(4))
        {
            ui->stackedWidget->setCurrentIndex(5);
            index=selectedItem->parent()->indexOfChild(selectedItem);
            if(index!=-1)
                ui->stackedOptionsPluginLoader->setCurrentIndex(index);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"selection into of sub item wrong???");
        }
        else if(selectedItem->parent()==ui->treeWidget->topLevelItem(5))
        {
            ui->stackedWidget->setCurrentIndex(6);
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
        options->queryResetOptions();
    else
        this->close();
}

void OptionDialog::loadOption()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    newOptionValue("Themes",	"Ultracopier_current_theme",	options->getOptionValue("Themes","Ultracopier_current_theme"));
    newOptionValue("Ultracopier",	"ActionOnManualOpen",		options->getOptionValue("Ultracopier","ActionOnManualOpen"));
    newOptionValue("Ultracopier",	"GroupWindowWhen",		options->getOptionValue("Ultracopier","GroupWindowWhen"));
    newOptionValue("Language",	"Language",			options->getOptionValue("Language","Language"));
    newOptionValue("Language",	"Language_force",		options->getOptionValue("Language","Language_force"));
    newOptionValue("SessionLoader",	"LoadAtSessionStarting",	options->getOptionValue("SessionLoader","LoadAtSessionStarting"));
    newOptionValue("CopyListener",	"CatchCopyAsDefault",		options->getOptionValue("CopyListener","CatchCopyAsDefault"));
    newOptionValue("CopyEngine",	"List",				options->getOptionValue("CopyEngine","List"));
    if(resources->getWritablePath()=="")
        ui->checkBox_Log->setEnabled(false);
    else
    {
        newOptionValue("Write_log",	"enabled",			options->getOptionValue("Write_log","enabled"));
        newOptionValue("Write_log",	"file",				options->getOptionValue("Write_log","file"));
        newOptionValue("Write_log",	"transfer",			options->getOptionValue("Write_log","transfer"));
        newOptionValue("Write_log",	"error",			options->getOptionValue("Write_log","error"));
        newOptionValue("Write_log",	"folder",			options->getOptionValue("Write_log","folder"));
        newOptionValue("Write_log",	"transfer_format",		options->getOptionValue("Write_log","transfer_format"));
        newOptionValue("Write_log",	"error_format",			options->getOptionValue("Write_log","error_format"));
        newOptionValue("Write_log",	"folder_format",		options->getOptionValue("Write_log","folder_format"));
        newOptionValue("Write_log",	"sync",				options->getOptionValue("Write_log","sync"));
    }
    on_checkBox_Log_clicked();
    if(plugins->getPluginsByCategory(PluginType_SessionLoader).size()>0)
    {
        ui->labelLoadAtSession->setToolTip("");
        ui->LoadAtSessionStarting->setToolTip("");
        ui->labelLoadAtSession->setEnabled(true);
        ui->LoadAtSessionStarting->setEnabled(true);
    }
    else
    {
        ui->labelLoadAtSession->setToolTip(tr("Disabled because you have any SessionLoader plugin"));
        ui->LoadAtSessionStarting->setToolTip(tr("Disabled because you have any SessionLoader plugin"));
        ui->labelLoadAtSession->setEnabled(false);
        ui->LoadAtSessionStarting->setEnabled(false);
    }
    allPluginsIsLoaded=true;
    on_Ultracopier_current_theme_currentIndexChanged(ui->Ultracopier_current_theme->currentIndex());
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
                    options->setOptionValue("Themes","Ultracopier_current_theme",ui->Ultracopier_current_theme->itemData(ui->Ultracopier_current_theme->currentIndex()));
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
                options->setOptionValue("Language","Language",ui->Language->itemData(ui->Language->currentIndex()));
            }
        }
        else if(name=="Language_force")
        {
            ui->Language_force->setChecked(value.toBool());
            ui->Language->setEnabled(ui->Language_force->isChecked() && ui->Language->count());
        }
    }
    else if(group=="SessionLoader")
    {
        if(name=="LoadAtSessionStarting")
        {
            ui->LoadAtSessionStarting->setChecked(value.toBool());
        }
    }
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
        {
            ui->checkBox_Log->setChecked(value.toBool());
        }
        else if(name=="file")
        {
            ui->lineEditLog_File->setText(value.toString());
        }
        else if(name=="transfer")
        {
            ui->checkBoxLog_transfer->setChecked(value.toBool());
        }
        else if(name=="sync")
        {
            ui->checkBoxLog_sync->setChecked(value.toBool());
        }
        else if(name=="error")
        {
            ui->checkBoxLog_error->setChecked(value.toBool());
        }
        else if(name=="folder")
        {
            ui->checkBoxLog_folder->setChecked(value.toBool());
        }
        else if(name=="transfer_format")
        {
            ui->lineEditLog_transfer_format->setText(value.toString());
        }
        else if(name=="error_format")
        {
            ui->lineEditLog_error_format->setText(value.toString());
        }
        else if(name=="folder_format")
        {
            ui->lineEditLog_folder_format->setText(value.toString());
        }
    }
    else if(group=="Ultracopier")
    {
        if(name=="ActionOnManualOpen")
        {
            ui->ActionOnManualOpen->setCurrentIndex(value.toInt());
        }
        if(name=="GroupWindowWhen")
        {
            ui->GroupWindowWhen->setCurrentIndex(value.toInt());
        }
    }
}

void OptionDialog::on_Ultracopier_current_theme_currentIndexChanged(int index)
{
    if(index!=-1 && allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"data value: "+ui->Ultracopier_current_theme->itemData(index).toString()+", string value: "+ui->Ultracopier_current_theme->itemText(index)+", index: "+QString::number(index));
        options->setOptionValue("Themes","Ultracopier_current_theme",ui->Ultracopier_current_theme->itemData(index));
        int index_loop=0;
        loop_size=pluginOptionsWidgetList.size();
        while(index_loop<loop_size)
        {
            if(pluginOptionsWidgetList.at(index_loop).name==ui->Ultracopier_current_theme->itemData(index).toString())
            {
                if(pluginOptionsWidgetList.at(index_loop).options==NULL)
                    ui->stackedWidgetThemes->setCurrentIndex(1);
                else
                    ui->stackedWidgetThemes->setCurrentWidget(pluginOptionsWidgetList.at(index_loop).options);
                return;
            }
            index_loop++;
        }
        ui->stackedWidgetThemes->setCurrentIndex(0);
    }
}

void OptionDialog::on_Language_currentIndexChanged(int index)
{
    if(index!=-1 && allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"data value: "+ui->Language->itemData(index).toString()+", string value: "+ui->Language->itemText(index)+", index: "+QString::number(index));
        options->setOptionValue("Language","Language",ui->Language->itemData(index));
    }
}

void OptionDialog::on_Language_force_toggled(bool checked)
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        options->setOptionValue("Language","Language_force",checked);
        ui->Language->setEnabled(ui->Language_force->isChecked() && ui->Language->count());
    }
}

void OptionDialog::on_CatchCopyAsDefault_toggled(bool checked)
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        options->setOptionValue("CopyListener","CatchCopyAsDefault",checked);
    }
}

void OptionDialog::on_LoadAtSessionStarting_toggled(bool checked)
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        options->setOptionValue("SessionLoader","LoadAtSessionStarting",checked);
    }
}

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
        options->setOptionValue("CopyEngine","List",copyEngineStringList());
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
        options->setOptionValue("CopyEngine","List",copyEngineStringList());
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
        ui->stackedWidgetThemes->addWidget(theNewOptionsWidget);
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

void OptionDialog::on_pluginRemove_clicked()
{
    treeWidgetItem=ui->pluginList->selectedItems().first();
    index=0;
    loop_size=pluginLink.size();
    while(index<loop_size)
    {
        if(pluginLink.at(index).item==treeWidgetItem)
        {
            plugins->removeThePluginSelected(pluginLink.at(index).path);
            return;
        }
        index++;
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
            plugins->showInformation(pluginLink.at(index).path);
            return;
        }
        index++;
    }
}

void OptionDialog::on_pluginAdd_clicked()
{
    plugins->addPlugin(defaultImportBackend);
}

void OptionDialog::on_checkBox_Log_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        options->setOptionValue("Write_log","enabled",ui->checkBox_Log->isChecked());
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
        options->setOptionValue("Write_log","file",ui->lineEditLog_File->text());
    }
}

void OptionDialog::on_lineEditLog_transfer_format_editingFinished()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        options->setOptionValue("Write_log","transfer_format",ui->lineEditLog_transfer_format->text());
    }
}

void OptionDialog::on_lineEditLog_error_format_editingFinished()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        options->setOptionValue("Write_log","error_format",ui->lineEditLog_error_format->text());
    }
}

void OptionDialog::on_checkBoxLog_transfer_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        options->setOptionValue("Write_log","transfer",ui->checkBoxLog_transfer->isChecked());
    }
}

void OptionDialog::on_checkBoxLog_error_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        options->setOptionValue("Write_log","error",ui->checkBoxLog_error->isChecked());
    }
}

void OptionDialog::on_checkBoxLog_folder_clicked()
{
    if(allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
        options->setOptionValue("Write_log","folder",ui->checkBoxLog_folder->isChecked());
    }
}

void OptionDialog::on_pushButton_clicked()
{
    QString file=QFileDialog::getSaveFileName(this,tr("Save logs as: "),resources->getWritablePath());
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
        options->setOptionValue("Write_log","sync",ui->checkBoxLog_sync->isChecked());
    }
}

void OptionDialog::on_ActionOnManualOpen_currentIndexChanged(int index)
{
    if(index!=-1 && allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"data value: "+ui->ActionOnManualOpen->itemData(index).toString()+", string value: "+ui->ActionOnManualOpen->itemText(index)+", index: "+QString::number(index));
        options->setOptionValue("Ultracopier","ActionOnManualOpen",index);
    }
}

void OptionDialog::on_GroupWindowWhen_currentIndexChanged(int index)
{
    if(index!=-1 && allPluginsIsLoaded)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"data value: "+ui->GroupWindowWhen->itemData(index).toString()+", string value: "+ui->GroupWindowWhen->itemText(index)+", index: "+QString::number(index));
        options->setOptionValue("Ultracopier","GroupWindowWhen",index);
    }
}
