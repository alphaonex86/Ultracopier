/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QColorDialog>

#include "factory.h"

ThemesFactory::ThemesFactory()
{
    optionsEngine=NULL;
    tempWidget=new QWidget();
    ui=new Ui::themesOptions();
    ui->setupUi(tempWidget);
    ui->toolBox->setCurrentIndex(0);
    currentSpeed	= 0;
    updateSpeed();
    ui->labelDualProgression->hide();
    ui->showDualProgression->hide();
    
    qRegisterMetaType<QList<QPersistentModelIndex> >("QList<QPersistentModelIndex>");
}

ThemesFactory::~ThemesFactory()
{
}

PluginInterface_Themes * ThemesFactory::getInstance()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, currentSpeed: %1").arg(currentSpeed));

    Themes * newInterface=new Themes(
                ui->alwaysOnTop->isChecked(),
                ui->showProgressionInTheTitle->isChecked(),
                progressColorWrite,progressColorRead,progressColorRemaining,
                ui->showDualProgression->isChecked(),
                ui->comboBox_copyEnd->currentIndex(),
                ui->speedWithProgressBar->isChecked(),
                currentSpeed,
                ui->checkBoxShowSpeed->isChecked(),
                facilityEngine,
                ui->checkBoxStartWithMoreButtonPushed->isChecked(),
                ui->minimizeToSystray->isChecked()
                );
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(newInterface,&Themes::debugInformation,this,&PluginInterface_ThemesFactory::debugInformation);
    #endif
    connect(this,&ThemesFactory::reloadLanguage,newInterface,&Themes::newLanguageLoaded);
    return newInterface;
}

void ThemesFactory::setResources(OptionInterface * optionsEngine,const QString &
                                 #ifdef ULTRACOPIER_PLUGIN_DEBUG
                                 writePath
                                 #endif
                                 ,const QString &
                                 #ifdef ULTRACOPIER_PLUGIN_DEBUG
                                 pluginPath
                                 #endif
                                 ,FacilityInterface * facilityEngine,const bool &)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath: "+pluginPath);
    this->facilityEngine=facilityEngine;
    if(optionsEngine!=NULL)
    {
        this->optionsEngine=optionsEngine;
        //load the options
        QList<QPair<QString, QVariant> > KeysList;
        KeysList.append(qMakePair(QStringLiteral("checkBoxShowSpeed"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("moreButtonPushed"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("speedWithProgressBar"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("currentSpeed"),QVariant(0)));
        KeysList.append(qMakePair(QStringLiteral("comboBox_copyEnd"),QVariant(0)));
        KeysList.append(qMakePair(QStringLiteral("showDualProgression"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("showProgressionInTheTitle"),QVariant(true)));
        KeysList.append(qMakePair(QStringLiteral("progressColorWrite"),QVariant(QApplication::palette().color(QPalette::Highlight))));
        KeysList.append(qMakePair(QStringLiteral("progressColorRead"),QVariant(QApplication::palette().color(QPalette::AlternateBase))));
        KeysList.append(qMakePair(QStringLiteral("progressColorRemaining"),QVariant(QApplication::palette().color(QPalette::Base))));
        KeysList.append(qMakePair(QStringLiteral("alwaysOnTop"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("minimizeToSystray"),QVariant(false)));
        optionsEngine->addOptionGroup(KeysList);
        connect(optionsEngine,&OptionInterface::resetOptions,this,&ThemesFactory::resetOptions);
        updateSpeed();
    }
    #ifndef __GNUC__
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"__GNUC__ is not set");
    #else
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"__GNUC__ is set");
    #endif
}

QWidget * ThemesFactory::options()
{
    if(optionsEngine!=NULL)
    {
        bool ok;
        currentSpeed=optionsEngine->getOptionValue("currentSpeed").toUInt(&ok);
        if(!ok)
            currentSpeed=0;
        ui->comboBox_copyEnd->setCurrentIndex(optionsEngine->getOptionValue(QStringLiteral("comboBox_copyEnd")).toUInt());
        ui->speedWithProgressBar->setChecked(optionsEngine->getOptionValue(QStringLiteral("speedWithProgressBar")).toBool());
        ui->checkBoxShowSpeed->setChecked(optionsEngine->getOptionValue(QStringLiteral("checkBoxShowSpeed")).toBool());
        ui->checkBoxStartWithMoreButtonPushed->setChecked(optionsEngine->getOptionValue(QStringLiteral("moreButtonPushed")).toBool());
        ui->showDualProgression->setChecked(optionsEngine->getOptionValue(QStringLiteral("showDualProgression")).toBool());
        ui->showProgressionInTheTitle->setChecked(optionsEngine->getOptionValue(QStringLiteral("showProgressionInTheTitle")).toBool());
        ui->alwaysOnTop->setChecked(optionsEngine->getOptionValue(QStringLiteral("alwaysOnTop")).toBool());
    ui->minimizeToSystray->setChecked(optionsEngine->getOptionValue(QStringLiteral("minimizeToSystray")).toBool());

        progressColorWrite=optionsEngine->getOptionValue(QStringLiteral("progressColorWrite")).value<QColor>();
        progressColorRead=optionsEngine->getOptionValue(QStringLiteral("progressColorRead")).value<QColor>();
        progressColorRemaining=optionsEngine->getOptionValue(QStringLiteral("progressColorRemaining")).value<QColor>();

        QPixmap pixmap(75,20);
        pixmap.fill(progressColorWrite);
        ui->progressColorWrite->setIcon(pixmap);
        pixmap.fill(progressColorRead);
        ui->progressColorRead->setIcon(pixmap);
        pixmap.fill(progressColorRemaining);
        ui->progressColorRemaining->setIcon(pixmap);
        updateSpeed();
        updateProgressionColorBar();

        connect(ui->alwaysOnTop,&QCheckBox::stateChanged,this,&ThemesFactory::alwaysOnTop);
        connect(ui->checkBoxShowSpeed,&QCheckBox::stateChanged,this,&ThemesFactory::checkBoxShowSpeed);
    connect(ui->minimizeToSystray,&QCheckBox::stateChanged,this,&ThemesFactory::minimizeToSystray);
        connect(ui->checkBox_limitSpeed,&QCheckBox::stateChanged,this,&ThemesFactory::uiUpdateSpeed);
        connect(ui->SliderSpeed,&QAbstractSlider::valueChanged,this,&ThemesFactory::on_SliderSpeed_valueChanged);
        connect(ui->limitSpeed,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,	&ThemesFactory::uiUpdateSpeed);
        connect(ui->checkBoxShowSpeed,&QAbstractButton::toggled,this,&ThemesFactory::checkBoxShowSpeedHaveChanged);
        connect(ui->checkBoxStartWithMoreButtonPushed,&QAbstractButton::toggled,this,&ThemesFactory::checkBoxStartWithMoreButtonPushedHaveChanged);
        connect(ui->speedWithProgressBar,&QAbstractButton::toggled,this,&ThemesFactory::speedWithProgressBar);
        connect(ui->comboBox_copyEnd,	static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&ThemesFactory::comboBox_copyEnd);
        connect(ui->showDualProgression,&QCheckBox::stateChanged,this,&ThemesFactory::showDualProgression);
        connect(ui->showDualProgression,&QCheckBox::stateChanged,this,&ThemesFactory::updateProgressionColorBar);
        connect(ui->showProgressionInTheTitle,&QCheckBox::stateChanged,this,&ThemesFactory::setShowProgressionInTheTitle);
        connect(ui->progressColorWrite,&QAbstractButton::clicked,this,&ThemesFactory::progressColorWrite_clicked);
        connect(ui->progressColorRead,	&QAbstractButton::clicked,this,&ThemesFactory::progressColorRead_clicked);
        connect(ui->progressColorRemaining,&QAbstractButton::clicked,this,&ThemesFactory::progressColorRemaining_clicked);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"return the options");
    return tempWidget;
}

QIcon ThemesFactory::getIcon(const QString &fileName) const
{
    return QIcon(QStringLiteral(":/Themes/Supercopier/resources/")+fileName);
}

void ThemesFactory::resetOptions()
{
    ui->checkBoxShowSpeed->setChecked(true);
    ui->checkBoxStartWithMoreButtonPushed->setChecked(false);
}

void ThemesFactory::checkBoxShowSpeedHaveChanged(bool toggled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("checkBoxShowSpeed"),toggled);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::checkBoxStartWithMoreButtonPushedHaveChanged(bool toggled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("moreButtonPushed"),toggled);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::comboBox_copyEnd(int value)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("comboBox_copyEnd"),value);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::speedWithProgressBar(bool toggled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("speedWithProgressBar"),toggled);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::newLanguageLoaded()
{
    ui->retranslateUi(tempWidget);
    ui->comboBox_copyEnd->setItemText(0,tr("Don't close if errors are found"));
    ui->comboBox_copyEnd->setItemText(1,tr("Never close"));
    ui->comboBox_copyEnd->setItemText(2,tr("Always close"));
    emit reloadLanguage();
}

void ThemesFactory::checkBoxShowSpeed(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    Q_UNUSED(checked);
    updateSpeed();
}

void ThemesFactory::minimizeToSystray(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("minimizeToSystray"),checked);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::alwaysOnTop(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("alwaysOnTop"),checked);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::showDualProgression(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("showDualProgression"),checked);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::on_SliderSpeed_valueChanged(int value)
{
    if(optionsEngine==NULL)
        return;
    if(!ui->checkBoxShowSpeed->isChecked())
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("value: %1").arg(value));
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
        optionsEngine->setOptionValue(QStringLiteral("currentSpeed"),currentSpeed);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
    updateSpeed();
}

void ThemesFactory::uiUpdateSpeed()
{
    if(optionsEngine==NULL)
        return;
    if(ui->checkBoxShowSpeed->isChecked())
        return;
    if(!ui->checkBox_limitSpeed->isChecked())
        currentSpeed=0;
    else
        currentSpeed=ui->limitSpeed->value();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("emit newSpeedLimitation(%1)").arg(currentSpeed));
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("currentSpeed"),currentSpeed);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::updateSpeed()
{
    if(optionsEngine==NULL)
        return;
    ui->label_Slider_speed->setVisible(ui->checkBoxShowSpeed->isChecked());
    ui->SliderSpeed->setVisible(ui->checkBoxShowSpeed->isChecked());
    ui->label_SpeedMaxValue->setVisible(ui->checkBoxShowSpeed->isChecked());
    ui->limitSpeed->setVisible(!ui->checkBoxShowSpeed->isChecked());
    ui->checkBox_limitSpeed->setVisible(!ui->checkBoxShowSpeed->isChecked());

    if(ui->checkBoxShowSpeed->isChecked())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("checked, currentSpeed: %1").arg(currentSpeed));
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
                    optionsEngine->setOptionValue(QStringLiteral("currentSpeed"),currentSpeed);
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
                    optionsEngine->setOptionValue(QStringLiteral("currentSpeed"),currentSpeed);
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
                    optionsEngine->setOptionValue(QStringLiteral("currentSpeed"),currentSpeed);
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
                    optionsEngine->setOptionValue(QStringLiteral("currentSpeed"),currentSpeed);
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
                    optionsEngine->setOptionValue(QStringLiteral("currentSpeed"),currentSpeed);
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

void ThemesFactory::progressColorWrite_clicked()
{
    QColor color=QColorDialog::getColor(progressColorWrite,NULL,tr("Select a color"));
    if(!color.isValid())
        return;
    progressColorWrite=color;
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorWrite);
    ui->progressColorWrite->setIcon(pixmap);
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("progressColorWrite"),progressColorWrite);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::progressColorRead_clicked()
{
    QColor color=QColorDialog::getColor(progressColorRead,NULL,tr("Select a color"));
    if(!color.isValid())
        return;
    progressColorRead=color;
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorRead);
    ui->progressColorRead->setIcon(pixmap);
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("progressColorRead"),progressColorRead);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::progressColorRemaining_clicked()
{
    QColor color=QColorDialog::getColor(progressColorRemaining,NULL,tr("Select a color"));
    if(!color.isValid())
        return;
    progressColorRemaining=color;
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorRemaining);
    ui->progressColorRemaining->setIcon(pixmap);
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("progressColorRemaining"),progressColorRemaining);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::updateProgressionColorBar()
{
    ui->labelProgressionColor->setVisible(ui->showDualProgression->isChecked());
    ui->frameProgressionColor->setVisible(ui->showDualProgression->isChecked());
}

void ThemesFactory::setShowProgressionInTheTitle()
{
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("showProgressionInTheTitle"),ui->showProgressionInTheTitle->isChecked());
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}
