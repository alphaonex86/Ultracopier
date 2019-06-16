/** \file factory.cpp
\brief Define the factory core
\author alpha_one_x86 */

#include <QColorDialog>

#include "ThemesFactory.h"
#include "../../../cpp11addition.h"

ThemesFactory::ThemesFactory()
{
    optionsEngine=NULL;
    tempWidget=new QWidget();
    ui=new Ui::themesOptions();
    ui->setupUi(tempWidget);
    ui->toolBox->setCurrentIndex(0);
    currentSpeed	= 0;
    updateSpeed();

    qRegisterMetaType<QList<QPersistentModelIndex> >("QList<QPersistentModelIndex>");
}

ThemesFactory::~ThemesFactory()
{
}

PluginInterface_Themes * ThemesFactory::getInstance()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, currentSpeed: "+std::to_string(currentSpeed));

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
                ui->minimizeToSystray->isChecked(),
                ui->startMinimized->isChecked(),
                ui->savePosition->isChecked()
                );
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(newInterface,&Themes::debugInformation,this,&PluginInterface_ThemesFactory::debugInformation))
        abort();
    #endif
    if(!connect(this,&ThemesFactory::reloadLanguage,newInterface,&Themes::newLanguageLoaded))
        abort();
    if(!connect(newInterface,&Themes::destroyed,this,&ThemesFactory::savePositionBeforeClose))
        abort();
    newInterface->move(
                stringtouint32(optionsEngine->getOptionValue("savePositionX")),
                stringtouint32(optionsEngine->getOptionValue("savePositionY"))
                );
    return newInterface;
}

void ThemesFactory::savePositionBeforeClose(QObject *obj)
{
    if(!ui->savePosition->isChecked())
        return;
    if(obj == nullptr)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"obj == nullptr");
        return;
    }
    const QWidget * const widget=static_cast<QWidget *>(obj);
    optionsEngine->setOptionValue("savePositionX",std::to_string(widget->x()));
    optionsEngine->setOptionValue("savePositionY",std::to_string(widget->y()));
}

void ThemesFactory::setResources(OptionInterface * optionsEngine,const std::string &
                                 #ifdef ULTRACOPIER_PLUGIN_DEBUG
                                 writePath
                                 #endif
                                 ,const std::string &
                                 #ifdef ULTRACOPIER_PLUGIN_DEBUG
                                 pluginPath
                                 #endif
                                 ,FacilityInterface * facilityEngine,const bool &
                                )
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath: "+pluginPath);
    this->facilityEngine=facilityEngine;
    if(optionsEngine!=NULL)
    {
        this->optionsEngine=optionsEngine;
        //load the options
        std::vector<std::pair<std::string, std::string> > KeysList;
        KeysList.push_back(std::pair<std::string, std::string>("checkBoxShowSpeed","false"));
        KeysList.push_back(std::pair<std::string, std::string>("moreButtonPushed","false"));
        KeysList.push_back(std::pair<std::string, std::string>("speedWithProgressBar","true"));
        KeysList.push_back(std::pair<std::string, std::string>("currentSpeed","0"));
        KeysList.push_back(std::pair<std::string, std::string>("comboBox_copyEnd","0"));
        KeysList.push_back(std::pair<std::string, std::string>("showDualProgression","false"));
        KeysList.push_back(std::pair<std::string, std::string>("showProgressionInTheTitle","true"));
        KeysList.push_back(std::pair<std::string, std::string>("progressColorWrite",QApplication::palette().color(QPalette::Highlight).name().toStdString()));
        KeysList.push_back(std::pair<std::string, std::string>("progressColorRead",QApplication::palette().color(QPalette::AlternateBase).name().toStdString()));
        KeysList.push_back(std::pair<std::string, std::string>("progressColorRemaining",QApplication::palette().color(QPalette::Base).name().toStdString()));
        KeysList.push_back(std::pair<std::string, std::string>("alwaysOnTop","false"));
        KeysList.push_back(std::pair<std::string, std::string>("minimizeToSystray","false"));
        KeysList.push_back(std::pair<std::string, std::string>("startMinimized","false"));
        KeysList.push_back(std::pair<std::string, std::string>("savePosition","false"));
        KeysList.push_back(std::pair<std::string, std::string>("savePositionX","0"));
        KeysList.push_back(std::pair<std::string, std::string>("savePositionY","0"));
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
        currentSpeed=stringtouint32(optionsEngine->getOptionValue("currentSpeed"),&ok);
        if(!ok)
            currentSpeed=0;
        ui->comboBox_copyEnd->setCurrentIndex(stringtouint32(optionsEngine->getOptionValue("comboBox_copyEnd")));
        ui->speedWithProgressBar->setChecked(stringtobool(optionsEngine->getOptionValue("speedWithProgressBar")));
        ui->checkBoxShowSpeed->setChecked(stringtobool(optionsEngine->getOptionValue("checkBoxShowSpeed")));
        ui->checkBoxStartWithMoreButtonPushed->setChecked(stringtobool(optionsEngine->getOptionValue("moreButtonPushed")));
        ui->showDualProgression->setChecked(stringtobool(optionsEngine->getOptionValue("showDualProgression")));
        ui->showProgressionInTheTitle->setChecked(stringtobool(optionsEngine->getOptionValue("showProgressionInTheTitle")));
        ui->alwaysOnTop->setChecked(stringtobool(optionsEngine->getOptionValue("alwaysOnTop")));
        ui->minimizeToSystray->setChecked(stringtobool(optionsEngine->getOptionValue("minimizeToSystray")));
        ui->startMinimized->setChecked(stringtobool(optionsEngine->getOptionValue("startMinimized")));
        ui->savePosition->setChecked(stringtobool(optionsEngine->getOptionValue("savePosition")));

        progressColorWrite=QVariant(QString::fromStdString(optionsEngine->getOptionValue("progressColorWrite"))).value<QColor>();
        progressColorRead=QVariant(QString::fromStdString(optionsEngine->getOptionValue("progressColorRead"))).value<QColor>();
        progressColorRemaining=QVariant(QString::fromStdString(optionsEngine->getOptionValue("progressColorRemaining"))).value<QColor>();

        QPixmap pixmap(75,20);
        pixmap.fill(progressColorWrite);
        ui->progressColorWrite->setIcon(pixmap);
        pixmap.fill(progressColorRead);
        ui->progressColorRead->setIcon(pixmap);
        pixmap.fill(progressColorRemaining);
        ui->progressColorRemaining->setIcon(pixmap);
        updateSpeed();
        updateProgressionColorBar();

        if(!connect(ui->alwaysOnTop,&QCheckBox::stateChanged,this,&ThemesFactory::alwaysOnTop))
            abort();
        if(!connect(ui->checkBoxShowSpeed,&QCheckBox::stateChanged,this,&ThemesFactory::checkBoxShowSpeed))
            abort();
        if(!connect(ui->minimizeToSystray,&QCheckBox::stateChanged,this,&ThemesFactory::minimizeToSystray))
            abort();
        if(!connect(ui->checkBox_limitSpeed,&QCheckBox::stateChanged,this,&ThemesFactory::uiUpdateSpeed))
            abort();
        if(!connect(ui->SliderSpeed,&QAbstractSlider::valueChanged,this,&ThemesFactory::on_SliderSpeed_valueChanged))
            abort();
        if(!connect(ui->limitSpeed,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,	&ThemesFactory::uiUpdateSpeed))
            abort();
        if(!connect(ui->checkBoxShowSpeed,&QAbstractButton::toggled,this,&ThemesFactory::checkBoxShowSpeedHaveChanged))
            abort();
        if(!connect(ui->checkBoxStartWithMoreButtonPushed,&QAbstractButton::toggled,this,&ThemesFactory::checkBoxStartWithMoreButtonPushedHaveChanged))
            abort();
        if(!connect(ui->speedWithProgressBar,&QAbstractButton::toggled,this,&ThemesFactory::speedWithProgressBar))
            abort();
        if(!connect(ui->comboBox_copyEnd,	static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&ThemesFactory::comboBox_copyEnd))
            abort();
        if(!connect(ui->showDualProgression,&QCheckBox::stateChanged,this,&ThemesFactory::showDualProgression))
            abort();
        if(!connect(ui->showDualProgression,&QCheckBox::stateChanged,this,&ThemesFactory::updateProgressionColorBar))
            abort();
        if(!connect(ui->showProgressionInTheTitle,&QCheckBox::stateChanged,this,&ThemesFactory::setShowProgressionInTheTitle))
            abort();
        if(!connect(ui->progressColorWrite,&QAbstractButton::clicked,this,&ThemesFactory::progressColorWrite_clicked))
            abort();
        if(!connect(ui->progressColorRead,	&QAbstractButton::clicked,this,&ThemesFactory::progressColorRead_clicked))
            abort();
        if(!connect(ui->progressColorRemaining,&QAbstractButton::clicked,this,&ThemesFactory::progressColorRemaining_clicked))
            abort();
        if(!connect(ui->startMinimized,&QCheckBox::stateChanged,this,&ThemesFactory::startMinimized))
            abort();
        if(!connect(ui->savePosition,&QCheckBox::stateChanged,this,&ThemesFactory::savePositionHaveChanged))
            abort();
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"return the options");
    return tempWidget;
}

QIcon ThemesFactory::getIcon(const std::string &fileName) const
{
    if(fileName=="SystemTrayIcon/exit.png")
    {
        QIcon tempIcon=QIcon::fromTheme("application-exit");
        if(!tempIcon.isNull())
            return tempIcon;
    }
    else if(fileName=="SystemTrayIcon/add.png")
    {
        QIcon tempIcon=QIcon::fromTheme("list-add");
        if(!tempIcon.isNull())
            return tempIcon;
    }
    else if(fileName=="SystemTrayIcon/informations.png")
    {
        QIcon tempIcon=QIcon::fromTheme("help-about");
        if(!tempIcon.isNull())
            return tempIcon;
    }
    else if(fileName=="SystemTrayIcon/options.png")
    {
        QIcon tempIcon=QIcon::fromTheme("applications-system");
        if(!tempIcon.isNull())
            return tempIcon;
    }
    #ifdef SUPERCOPIER
    return QIcon(":/Themes/Supercopier/resources/"+QString::fromStdString(fileName));
    #else
    return QIcon(":/Themes/Oxygen/resources/"+QString::fromStdString(fileName));
    #endif
}

void ThemesFactory::resetOptions()
{
    ui->checkBoxShowSpeed->setChecked(true);
    ui->checkBoxStartWithMoreButtonPushed->setChecked(false);
    ui->savePosition->setChecked(false);
}

void ThemesFactory::checkBoxShowSpeedHaveChanged(bool toggled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checkBoxShowSpeed",std::to_string(toggled));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::checkBoxStartWithMoreButtonPushedHaveChanged(bool toggled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("moreButtonPushed",std::to_string(toggled));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::savePositionHaveChanged(bool toggled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("savePosition",std::to_string(toggled));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::comboBox_copyEnd(int value)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("comboBox_copyEnd",std::to_string(value));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::speedWithProgressBar(bool toggled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("speedWithProgressBar",std::to_string(toggled));
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
        optionsEngine->setOptionValue("minimizeToSystray",std::to_string(checked));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::alwaysOnTop(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("alwaysOnTop",std::to_string(checked));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::showDualProgression(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("showDualProgression",std::to_string(checked));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::startMinimized(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("startMinimized",std::to_string(checked));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void ThemesFactory::on_SliderSpeed_valueChanged(int value)
{
    if(optionsEngine==NULL)
        return;
    if(!ui->checkBoxShowSpeed->isChecked())
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"value: "+std::to_string(value));
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
        optionsEngine->setOptionValue("currentSpeed",std::to_string(currentSpeed));
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit newSpeedLimitation: "+std::to_string(currentSpeed));
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("currentSpeed",std::to_string(currentSpeed));
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"checked, currentSpeed: "+std::to_string(currentSpeed));
        ui->limitSpeed->setEnabled(false);
        if(currentSpeed==0)
        {
            ui->SliderSpeed->setValue(0);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->translateText("Unlimited")));
        }
        else if(currentSpeed<=1024)
        {
            if(currentSpeed!=1024)
            {
                currentSpeed=1024;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",std::to_string(currentSpeed));
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
            }
            ui->SliderSpeed->setValue(1);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*1)));
        }
        else if(currentSpeed<=1024*4)
        {
            if(currentSpeed!=1024*4)
            {
                currentSpeed=1024*4;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",std::to_string(currentSpeed));
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
            }
            ui->SliderSpeed->setValue(2);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*4)));
        }
        else if(currentSpeed<=1024*16)
        {
            if(currentSpeed!=1024*16)
            {
                currentSpeed=1024*16;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",std::to_string(currentSpeed));
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
            }
            ui->SliderSpeed->setValue(3);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*16)));
        }
        else if(currentSpeed<=1024*64)
        {
            if(currentSpeed!=1024*64)
            {
                currentSpeed=1024*64;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",std::to_string(currentSpeed));
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
            }
            ui->SliderSpeed->setValue(4);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*64)));
        }
        else
        {
            if(currentSpeed!=1024*128)
            {
                currentSpeed=1024*128;
                if(optionsEngine!=NULL)
                    optionsEngine->setOptionValue("currentSpeed",std::to_string(currentSpeed));
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
            }
            ui->SliderSpeed->setValue(5);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*128)));
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
        optionsEngine->setOptionValue("progressColorWrite",progressColorWrite.name().toStdString());
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
        optionsEngine->setOptionValue("progressColorRead",progressColorRead.name().toStdString());
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
        optionsEngine->setOptionValue("progressColorRemaining",progressColorRemaining.name().toStdString());
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
        optionsEngine->setOptionValue("showProgressionInTheTitle",std::to_string(ui->showProgressionInTheTitle->isChecked()));
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}
