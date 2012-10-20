/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "factory.h"

Factory::Factory()
{
    optionsEngine=NULL;
    tempWidget=new QWidget();
    ui=new Ui::options();
    ui->setupUi(tempWidget);
}

Factory::~Factory()
{
}

PluginInterface_Themes * Factory::getInstance()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    Themes * newInterface=new Themes(
                optionsEngine->getOptionValue("checkBoxShowSpeed").toBool(),facilityEngine,optionsEngine->getOptionValue("moreButtonPushed").toBool()
                );
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(newInterface,&Themes::debugInformation,this,&PluginInterface_ThemesFactory::debugInformation);
    #endif
    connect(this,&Factory::reloadLanguage,newInterface,&Themes::newLanguageLoaded);
    return newInterface;
}

void Factory::setResources(OptionInterface * optionsEngine,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityEngine,const bool &portableVersion)
{
    Q_UNUSED(portableVersion);
    Q_UNUSED(writePath);
    Q_UNUSED(pluginPath);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath: "+pluginPath);
    this->facilityEngine=facilityEngine;
    if(optionsEngine!=NULL)
    {
        this->optionsEngine=optionsEngine;
        //load the options
        QList<QPair<QString, QVariant> > KeysList;
        KeysList.append(qMakePair(QString("checkBoxShowSpeed"),QVariant(false)));
        KeysList.append(qMakePair(QString("moreButtonPushed"),QVariant(false)));
        optionsEngine->addOptionGroup(KeysList);
        connect(optionsEngine,&OptionInterface::resetOptions,this,&Factory::resetOptions);
    }
    #ifndef __GNUC__
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"__GNUC__ is set");
    #endif
    #ifndef __GNUC__
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"__GNUC__ is set");
    #endif
}

QWidget * Factory::options()
{
    if(optionsEngine!=NULL)
    {
        ui->checkBoxShowSpeed->setChecked(optionsEngine->getOptionValue("checkBoxShowSpeed").toBool());
        ui->checkBoxStartWithMoreButtonPushed->setChecked(optionsEngine->getOptionValue("moreButtonPushed").toBool());
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
    connect(ui->checkBoxShowSpeed,&QAbstractButton::toggled,this,&Factory::checkBoxShowSpeedHaveChanged);
    connect(ui->checkBoxStartWithMoreButtonPushed,&QAbstractButton::toggled,this,&Factory::checkBoxStartWithMoreButtonPushedHaveChanged);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"return the options");
    return tempWidget;
}

QIcon Factory::getIcon(const QString &fileName)
{
    if(fileName=="SystemTrayIcon/exit.png")
    {
        QIcon tempIcon=QIcon::fromTheme("application-exit");
        if(!tempIcon.isNull())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("use substitution icon for: %1").arg(fileName));
            return tempIcon;
        }
    }
    if(fileName=="SystemTrayIcon/add.png")
    {
        QIcon tempIcon=QIcon::fromTheme("list-add");
        if(!tempIcon.isNull())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("use substitution icon for: %1").arg(fileName));
            return tempIcon;
        }
    }
    if(fileName=="SystemTrayIcon/informations.png")
    {
        QIcon tempIcon=QIcon::fromTheme("help-about");
        if(!tempIcon.isNull())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("use substitution icon for: %1").arg(fileName));
            return tempIcon;
        }
    }
    if(fileName=="SystemTrayIcon/options.png")
    {
        QIcon tempIcon=QIcon::fromTheme("applications-system");
        if(!tempIcon.isNull())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("use substitution icon for: %1").arg(fileName));
            return tempIcon;
        }
    }
    return QIcon(":/resources/"+fileName);
}

void Factory::resetOptions()
{
    ui->checkBoxShowSpeed->setChecked(true);
    ui->checkBoxStartWithMoreButtonPushed->setChecked(false);
}

void Factory::checkBoxShowSpeedHaveChanged(bool toggled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checkBoxShowSpeed",toggled);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::checkBoxStartWithMoreButtonPushedHaveChanged(bool toggled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("moreButtonPushed",toggled);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::newLanguageLoaded()
{
    ui->retranslateUi(tempWidget);
    emit reloadLanguage();
}
