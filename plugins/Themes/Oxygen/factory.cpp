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
    currentSpeed	= 0;
    updateSpeed();
}

Factory::~Factory()
{
}

PluginInterface_Themes * Factory::getInstance()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("start, currentSpeed: %1").arg(currentSpeed));

    Themes * newInterface=new Themes(
                currentSpeed,optionsEngine->getOptionValue("checkBoxShowSpeed").toBool(),facilityEngine,optionsEngine->getOptionValue("moreButtonPushed").toBool()
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
        KeysList.append(qMakePair(QString("currentSpeed"),QVariant(0)));
        optionsEngine->addOptionGroup(KeysList);
        connect(optionsEngine,&OptionInterface::resetOptions,this,&Factory::resetOptions);
        bool ok;
        currentSpeed=optionsEngine->getOptionValue("currentSpeed").toUInt(&ok);
        if(!ok)
            currentSpeed=0;
        updateSpeed();
        connect(ui->checkBoxShowSpeed,&QCheckBox::stateChanged,this,&Factory::on_checkBoxShowSpeed_toggled);
        connect(ui->checkBox_limitSpeed,&QCheckBox::stateChanged,this,&Factory::uiUpdateSpeed);
        connect(ui->SliderSpeed,&QAbstractSlider::valueChanged,this,&Factory::on_SliderSpeed_valueChanged);
        connect(ui->limitSpeed,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,	&Factory::uiUpdateSpeed);
        connect(ui->checkBoxShowSpeed,&QAbstractButton::toggled,this,&Factory::checkBoxShowSpeedHaveChanged);
        connect(ui->checkBoxStartWithMoreButtonPushed,&QAbstractButton::toggled,this,&Factory::checkBoxStartWithMoreButtonPushedHaveChanged);
    }
    #ifndef __GNUC__
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"__GNUC__ is not set");
    #else
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"__GNUC__ is set");
    #endif
}

QWidget * Factory::options()
{
    bool ok;
    currentSpeed=optionsEngine->getOptionValue("currentSpeed").toUInt(&ok);
    if(!ok)
        currentSpeed=0;
    if(optionsEngine!=NULL)
    {
        ui->checkBoxShowSpeed->setChecked(optionsEngine->getOptionValue("checkBoxShowSpeed").toBool());
        ui->checkBoxStartWithMoreButtonPushed->setChecked(optionsEngine->getOptionValue("moreButtonPushed").toBool());
        updateSpeed();
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
    updateSpeed();
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

void Factory::on_checkBoxShowSpeed_toggled(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    Q_UNUSED(checked);
    updateSpeed();
}

void Factory::on_SliderSpeed_valueChanged(int value)
{
    if(!ui->checkBoxShowSpeed->isChecked())
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("value: %1").arg(value));
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
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("currentSpeed",currentSpeed);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
    updateSpeed();
}

void Factory::uiUpdateSpeed()
{
    if(ui->checkBoxShowSpeed->isChecked())
        return;
    if(!ui->checkBox_limitSpeed->isChecked())
        currentSpeed=0;
    else
        currentSpeed=ui->limitSpeed->value();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("emit newSpeedLimitation(%1)").arg(currentSpeed));
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("currentSpeed",currentSpeed);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::updateSpeed()
{
    ui->label_Slider_speed->setVisible(ui->checkBoxShowSpeed->isChecked());
    ui->SliderSpeed->setVisible(ui->checkBoxShowSpeed->isChecked());
    ui->label_SpeedMaxValue->setVisible(ui->checkBoxShowSpeed->isChecked());
    ui->limitSpeed->setVisible(!ui->checkBoxShowSpeed->isChecked());
    ui->checkBox_limitSpeed->setVisible(!ui->checkBoxShowSpeed->isChecked());

    if(ui->checkBoxShowSpeed->isChecked())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("checked, currentSpeed: %1").arg(currentSpeed));
        ui->limitSpeed->setEnabled(false);
        if(currentSpeed==0)
        {
            ui->SliderSpeed->setValue(0);
            ui->label_SpeedMaxValue->setText(facilityEngine->translateText("Unlimited"));
        }
        else if(currentSpeed<=1024)
        {
            if(currentSpeed!=1024)
            {
                currentSpeed=1024;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",currentSpeed);
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
            }
            ui->SliderSpeed->setValue(1);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*1));
        }
        else if(currentSpeed<=1024*4)
        {
            if(currentSpeed!=1024*4)
            {
                currentSpeed=1024*4;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",currentSpeed);
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
            }
            ui->SliderSpeed->setValue(2);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*4));
        }
        else if(currentSpeed<=1024*16)
        {
            if(currentSpeed!=1024*16)
            {
                currentSpeed=1024*16;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",currentSpeed);
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
            }
            ui->SliderSpeed->setValue(3);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*16));
        }
        else if(currentSpeed<=1024*64)
        {
            if(currentSpeed!=1024*64)
            {
                currentSpeed=1024*64;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",currentSpeed);
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
            }
            ui->SliderSpeed->setValue(4);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*64));
        }
        else
        {
            if(currentSpeed!=1024*128)
            {
                currentSpeed=1024*128;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",currentSpeed);
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
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
