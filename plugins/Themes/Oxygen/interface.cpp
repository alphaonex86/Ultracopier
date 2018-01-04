/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86 */

#include <QMessageBox>
#include <QMimeData>
#include <QScrollArea>
#include <QColorDialog>
#include <QRect>
#include <QPainter>
#include <QDebug>
#include <cmath>
#include <chrono>
#include <ctime>
#ifdef Q_OS_WIN
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

#include "interface.h"
#include "ui_interface.h"
#include "ThemesFactory.h"

QIcon Themes::player_play;
QIcon Themes::player_pause;
QIcon Themes::tempExitIcon;
QIcon Themes::editDelete;
QIcon Themes::skinIcon;
QIcon Themes::editFind;
QIcon Themes::documentOpen;
QIcon Themes::documentSave;
QIcon Themes::listAdd;
bool Themes::iconLoaded=false;

// The cmath header from MSVC does not contain round()
#if (defined(_WIN64) || defined(_WIN32)) && defined(_MSC_VER)
inline double round(double d) {
    return floor( d + 0.5 );
}
#endif

Themes::Themes(const bool &alwaysOnTop,
               const bool &showProgressionInTheTitle,
               const QColor &progressColorWrite,
               const QColor &progressColorRead,
               const QColor &progressColorRemaining,
               const bool &showDualProgression,
               const quint8 &comboBox_copyEnd,
               const bool &speedWithProgressBar,
               const qint32 &currentSpeed,
               const bool &checkBoxShowSpeed,
               FacilityInterface * facilityEngine,
               const bool &moreButtonPushed,
               const bool &minimizeToSystray,
               const bool &startMinimized) :
    ui(new Ui::interfaceCopy()),
    uiOptions(new Ui::themesOptions())
{
    this->facilityEngine=facilityEngine;
    ui->setupUi(this);
    uiOptions->setupUi(ui->optionsTab);

    currentFile     = 0;
    totalFile       = 0;
    currentSize     = 0;
    totalSize       = 0;
    getOldProgression = 200;
    haveError       = false;
    stat            = status_never_started;
    modeIsForced	= false;
    haveStarted     = false;
    storeIsInPause	= false;
    durationStarted = false;
    if(startMinimized)
        this->showMinimized();

    this->progressColorWrite    = progressColorWrite;
    this->progressColorRead     = progressColorRead;
    this->progressColorRemaining= progressColorRemaining;
    this->currentSpeed          = currentSpeed;
    uiOptions->showProgressionInTheTitle->setChecked(showProgressionInTheTitle);
    uiOptions->speedWithProgressBar->setChecked(speedWithProgressBar);
    uiOptions->showDualProgression->setChecked(showDualProgression);
    uiOptions->startMinimized->setEnabled(false);
    uiOptions->alwaysOnTop->setChecked(alwaysOnTop);
    uiOptions->minimizeToSystray->setChecked(minimizeToSystray);
    //uiOptions->setupUi(ui->tabWidget->widget(ui->tabWidget->count()-1));
    uiOptions->labelStartWithMoreButtonPushed->setVisible(false);
    uiOptions->checkBoxStartWithMoreButtonPushed->setVisible(false);
    uiOptions->label_Slider_speed->setVisible(false);
    uiOptions->SliderSpeed->setVisible(false);
    uiOptions->label_SpeedMaxValue->setVisible(false);
    uiOptions->comboBox_copyEnd->setCurrentIndex(comboBox_copyEnd);
    speedWithProgressBar_toggled(speedWithProgressBar);
    showDualProgression_toggled(showDualProgression);
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorWrite);
    uiOptions->progressColorWrite->setIcon(pixmap);
    pixmap.fill(progressColorRead);
    uiOptions->progressColorRead->setIcon(pixmap);
    pixmap.fill(progressColorRemaining);
    uiOptions->progressColorRemaining->setIcon(pixmap);

    transferModel.setFacilityEngine(facilityEngine);//need be before ui->TransferList->setModel(&transferModel); due to call of TransferModel::headerData()
    ui->TransferList->setModel(&transferModel);
    ui->tabWidget->setCurrentIndex(0);
    uiOptions->toolBox->setCurrentIndex(0);
    uiOptions->checkBoxShowSpeed->setChecked(checkBoxShowSpeed);
    menu=new QMenu(this);
    ui->add->setMenu(menu);

    //connect the options
    checkBoxShowSpeed_toggled(uiOptions->checkBoxShowSpeed->isChecked());
    connect(uiOptions->checkBoxShowSpeed,&QCheckBox::stateChanged,this,&Themes::checkBoxShowSpeed_toggled);
    connect(uiOptions->speedWithProgressBar,&QCheckBox::stateChanged,this,&Themes::speedWithProgressBar_toggled);
    connect(uiOptions->showProgressionInTheTitle,&QCheckBox::stateChanged,this,&Themes::updateTitle);
    connect(uiOptions->showDualProgression,&QCheckBox::stateChanged,this,&Themes::showDualProgression_toggled);
    connect(uiOptions->progressColorWrite,&QAbstractButton::clicked,this,&Themes::progressColorWrite_clicked);
    connect(uiOptions->progressColorRead,	&QAbstractButton::clicked,this,&Themes::progressColorRead_clicked);
    connect(uiOptions->progressColorRemaining,&QAbstractButton::clicked,this,&Themes::progressColorRemaining_clicked);
    connect(uiOptions->alwaysOnTop,&QAbstractButton::clicked,this,&Themes::alwaysOnTop_clickedSlot);

    isInPause(false);

    connect(uiOptions->limitSpeed,		static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,	&Themes::uiUpdateSpeed);
    connect(uiOptions->checkBox_limitSpeed,&QAbstractButton::toggled,		this,	&Themes::uiUpdateSpeed);

    connect(ui->actionAddFile,&QAction::triggered,this,&Themes::forcedModeAddFile);
    connect(ui->actionAddFileToCopy,&QAction::triggered,this,&Themes::forcedModeAddFileToCopy);
    connect(ui->actionAddFileToMove,&QAction::triggered,this,&Themes::forcedModeAddFileToMove);
    connect(ui->actionAddFolderToCopy,&QAction::triggered,this,&Themes::forcedModeAddFolderToCopy);
    connect(ui->actionAddFolderToMove,&QAction::triggered,this,&Themes::forcedModeAddFolderToMove);
    connect(ui->actionAddFolder,&QAction::triggered,this,&Themes::forcedModeAddFolder);
    connect(ui->exportErrorToTransferList,&QToolButton::triggered,this,&Themes::exportErrorIntoTransferList);

    //setup the search part
    closeTheSearchBox();
    TimerForSearch  = new QTimer(this);
    TimerForSearch->setInterval(500);
    TimerForSearch->setSingleShot(true);
    searchShortcut  = new QShortcut(QKeySequence(QKeySequence::Find),this);
    searchShortcut2 = new QShortcut(QKeySequence(QKeySequence::FindNext),this);
    searchShortcut3 = new QShortcut(QKeySequence(Qt::Key_Escape),this);

    //connect the search part
    connect(TimerForSearch,			&QTimer::timeout,	this,	&Themes::hilightTheSearchSlot);
    connect(searchShortcut,			&QShortcut::activated,	this,	&Themes::searchBoxShortcut);
    connect(searchShortcut2,		&QShortcut::activated,	this,	&Themes::on_pushButtonSearchNext_clicked);
    connect(ui->pushButtonCloseSearch,	&QPushButton::clicked,	this,	&Themes::closeTheSearchBox);
    connect(searchShortcut3,		&QShortcut::activated,	this,	&Themes::closeTheSearchBox);

    //reload directly untranslatable text
    newLanguageLoaded();

    //unpush the more button
    ui->moreButton->setChecked(moreButtonPushed);
    on_moreButton_toggled(moreButtonPushed);

    /// \note important for drag and drop, \see dropEvent()
    setAcceptDrops(true);

    #ifdef SUPERCOPIER
    const QString themePath=":/Themes/Supercopier/";
    #else
    const QString themePath=":/Themes/Oxygen/";
    #endif

    // try set the OS icon
    if(!iconLoaded)
    {
        iconLoaded=true;
        tempExitIcon=QIcon::fromTheme(QStringLiteral("application-exit"));
        editDelete=QIcon::fromTheme(QStringLiteral("edit-delete"));
        player_pause=QIcon::fromTheme(QStringLiteral("media-playback-pause"));
        if(player_pause.isNull())
            player_pause=QIcon(themePath+QStringLiteral("resources/player_pause.png"));
        player_play=QIcon::fromTheme(QStringLiteral("media-playback-play"));
        if(player_play.isNull())
            player_play=QIcon(themePath+QStringLiteral("resources/player_play.png"));
        skinIcon=QIcon::fromTheme(QStringLiteral("media-skip-forward"));
        editFind=QIcon::fromTheme(QStringLiteral("edit-find"));
        documentOpen=QIcon::fromTheme(QStringLiteral("document-open"));
        documentSave=QIcon::fromTheme(QStringLiteral("document-save"));
        listAdd=QIcon::fromTheme(QStringLiteral("list-add"));
   }
    if(!tempExitIcon.isNull())
    {
        ui->cancelButton->setIcon(tempExitIcon);
        ui->pushButtonCloseSearch->setIcon(tempExitIcon);
        ui->shutdown->setIcon(tempExitIcon);
    }
    if(!editDelete.isNull())
        ui->del->setIcon(editDelete);
    if(!player_pause.isNull())
        ui->pauseButton->setIcon(player_pause);
    if(!skinIcon.isNull())
        ui->skipButton->setIcon(skinIcon);
    if(!editFind.isNull())
        ui->searchButton->setIcon(editFind);
    if(!documentOpen.isNull())
        ui->importTransferList->setIcon(documentOpen);
    if(!documentSave.isNull())
    {
        ui->exportTransferList->setIcon(documentSave);
        ui->exportErrorToTransferList->setIcon(documentSave);
    }
    if(!listAdd.isNull())
    {
        ui->add->setIcon(listAdd);
        ui->actionAddFile->setIcon(listAdd);
        ui->actionAddFileToCopy->setIcon(listAdd);
        ui->actionAddFileToMove->setIcon(listAdd);
        ui->actionAddFolder->setIcon(listAdd);
        ui->actionAddFolderToCopy->setIcon(listAdd);
        ui->actionAddFolderToMove->setIcon(listAdd);
    }
    #ifdef Q_OS_WIN32
    pixmapTop=QPixmap(themePath+QStringLiteral("resources/SystemTrayIcon/systray_Uncaught_Windows.png"));
    pixmapBottom=QPixmap(themePath+QStringLiteral("resources/SystemTrayIcon/systray_Caught_Windows.png"));
    #else
    pixmapTop=QPixmap(themePath+QStringLiteral("resources/SystemTrayIcon/systray_Uncaught_Unix.png"));
    pixmapBottom=QPixmap(themePath+QStringLiteral("resources/SystemTrayIcon/systray_Caught_Unix.png"));
    #endif

    #ifdef SUPERCOPIER
    /*QIcon icon;
    icon.addFile(themePath+QStringLiteral("resources/main.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->interfaceCopy->setWindowIcon(icon);*/
    QIcon icon1;
    icon1.addFile(themePath+QStringLiteral("resources/add.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->actionAddFile->setIcon(icon1);
    QIcon icon2;
    icon2.addFile(themePath+QStringLiteral("resources/SystemTrayIcon/exit.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->shutdown->setIcon(icon2);
    QIcon icon3;
    icon3.addFile(themePath+QStringLiteral("resources/moveDown.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->moreButton->setIcon(icon3);
    ui->pushDown->setIcon(icon3);
    if(!QFile::exists(themePath+QStringLiteral("resources/moveDown.png")))
    {
        qDebug() << "File not found, can't continue: " << themePath+QStringLiteral("resources/moveDown.png");
        abort();
    }
    QIcon icon4;
    icon4.addFile(themePath+QStringLiteral("resources/player_pause.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->pauseButton->setIcon(icon4);
    QIcon icon5;
    icon5.addFile(themePath+QStringLiteral("resources/player_end.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->skipButton->setIcon(icon5);
    QIcon icon6;
    icon6.addFile(themePath+QStringLiteral("resources/cancel.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->cancelButton->setIcon(icon6);
    QIcon icon7;
    icon7.addFile(themePath+QStringLiteral("resources/putOnTop.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->putOnTop->setIcon(icon7);
    QIcon icon8;
    icon8.addFile(themePath+QStringLiteral("resources/moveUp.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->pushUp->setIcon(icon8);
    QIcon icon9;
    icon9.addFile(themePath+QStringLiteral("resources/putOnBottom.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->putOnBottom->setIcon(icon9);
    QIcon icon10;
    icon10.addFile(themePath+QStringLiteral("resources/remove.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->del->setIcon(icon10);
    QIcon icon11;
    icon11.addFile(themePath+QStringLiteral("resources/search.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->searchButton->setIcon(icon11);
    QIcon icon12;
    icon12.addFile(themePath+QStringLiteral("resources/export-transfer-list.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->exportTransferList->setIcon(icon12);
    QIcon icon13;
    icon13.addFile(themePath+QStringLiteral("resources/import-transfer-list.png"), QSize(), QIcon::Normal, QIcon::Off);
    ui->importTransferList->setIcon(icon13);
    QIcon icon14;
    icon14.addFile(themePath+QStringLiteral("resources/main.png"), QSize(), QIcon::Normal, QIcon::Off);
    this->setWindowIcon(icon14);
    #endif

    shutdown=facilityEngine->haveFunctionality("shutdown");
    ui->shutdown->setVisible(shutdown);

    selectionModel=ui->TransferList->selectionModel();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(&transferModel,&TransferModel::debugInformation,this,&Themes::debugInformation);
    #endif

    updateSpeed();
    alwaysOnTop_clicked(false);
    /*#ifdef Q_OS_WIN32
    uiOptions->labelAlwaysOnTop->hide();
    uiOptions->alwaysOnTop->hide();
    #endif*/
    #ifdef ULTRACOPIER_VERSION_ULTIMATE
    #ifdef SUPERCOPIER
    ui->ad_ultimate->setText(tr("%1 is deprecated, Use %2").arg("<span style=\"color:#ee0000\">Super</span><span style=\"color:#0000cc\">Copier</span>").arg("<a href=\"http://ultracopier.first-world.info/\">Ultracopier</a>"));
    #else
    ui->ad_ultimate->hide();
    #endif
    #else
    QString ultimateUrl=QString::fromStdString(facilityEngine->ultimateUrl());
    if(ultimateUrl.isEmpty())
        ui->ad_ultimate->hide();
    else
        ui->ad_ultimate->setText(
                    #ifdef SUPERCOPIER
                    tr("%1 is deprecated, Use %2").arg("<span style=\"color:#ee0000\">Super</span><span style=\"color:#0000cc\">copier</span>").arg("<a href=\"http://ultracopier.first-world.info/\">Ultracopier</a><br />")+
                    #endif
                    QStringLiteral("<a href=\"%1\">%2</a>").arg(ultimateUrl).arg(tr("Buy the Ultimate version to fund development")));
    #endif

    #ifdef SUPERCOPIER
    uiOptions->labelDualProgression->hide();
    uiOptions->showDualProgression->hide();
    ui->progressBar_all->setMaximumHeight(17);
    ui->progressBar_file->setMaximumHeight(17);
    ui->progressBarCurrentSpeed->setMaximumHeight(17);
    ui->progressBar_all->setMinimumHeight(17);
    ui->progressBar_file->setMinimumHeight(17);
    ui->progressBarCurrentSpeed->setMinimumHeight(17);
    ui->progressBar_all->setStyleSheet(QStringLiteral("QProgressBar{color:#fff;font-weight:bold;border:1px solid black;text-align:center;background-image:url(:/Themes/Supercopier/resources/progressbarright.png);}QProgressBar::chunk{background-image: url(:/Themes/Supercopier/resources/progressbarleft.png);}"));
    ui->progressBar_file->setStyleSheet(QStringLiteral("QProgressBar{color:#fff;font-weight:bold;border:1px solid black;text-align:center;background-image:url(:/Themes/Supercopier/resources/progressbarright.png);}QProgressBar::chunk{background-image: url(:/Themes/Supercopier/resources/progressbarleft.png);}"));
    ui->progressBarCurrentSpeed->setStyleSheet(QStringLiteral("QProgressBar{color:#fff;font-weight:bold;border:1px solid black;text-align:center;background-image:url(:/Themes/Supercopier/resources/progressbarright.png);}QProgressBar::chunk{background-image: url(:/Themes/Supercopier/resources/progressbarleft.png);}"));
    this->setWindowTitle("Supercopier");
    #endif

    show();

    sysTrayIcon = new QSystemTrayIcon(this);
    connect(sysTrayIcon,&QSystemTrayIcon::activated,this,&Themes::catchAction);
}

Themes::~Themes()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //disconnect(ui->actionAddFile);
    //disconnect(ui->actionAddFolder);
    delete selectionModel;
    delete menu;
    delete sysTrayIcon;
}

QWidget * Themes::getOptionsEngineWidget()
{
    return &optionEngineWidget;
}

void Themes::getOptionsEngineEnabled(const bool &isEnabled)
{
    if(isEnabled)
    {
        QScrollArea *scrollArea=new QScrollArea(ui->tabWidget);
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(&optionEngineWidget);
        ui->tabWidget->addTab(scrollArea,QString::fromStdString(facilityEngine->translateText("Copy engine")));
    }
}

void Themes::closeEvent(QCloseEvent *event)
{
    event->ignore();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->hide();
    if(uiOptions->minimizeToSystray->isChecked())
    {
        updateSysTrayIcon();
        sysTrayIcon->show();
    }
    else
        emit cancel();
}

void Themes::updateSysTrayIcon()
{
    if(totalSize==0)
    {
        sysTrayIcon->setIcon(dynaIcon(0,"-"));
        return;
    }
    quint64 currentNew=currentSize*100;
    //update systray icon
    quint16 getVarProgression=currentNew/totalSize;
    if(getOldProgression!=getVarProgression)
    {
        getOldProgression=getVarProgression;
        sysTrayIcon->setIcon(dynaIcon(getVarProgression));
    }
}

void Themes::updateOverallInformation()
{
    if(uiOptions->showProgressionInTheTitle->isChecked())
        updateTitle();
    ui->overall->setText(tr("File %1/%2, size: %3/%4")
                         .arg(currentFile)
                         .arg(totalFile)
                         .arg(QString::fromStdString(facilityEngine->sizeToString(currentSize)))
                         .arg(QString::fromStdString(facilityEngine->sizeToString(totalSize)))
                         );
}

void Themes::actionInProgess(const Ultracopier::EngineActionInProgress &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"start: "+std::to_string(action));
    this->action=action;
    switch(action)
    {
        case Ultracopier::Copying:
        case Ultracopier::CopyingAndListing:
            ui->progressBar_all->setMaximum(65535);
            ui->progressBar_all->setMinimum(0);
        break;
        case Ultracopier::Listing:
            ui->progressBar_all->setMaximum(0);
            ui->progressBar_all->setMinimum(0);
        break;
        case Ultracopier::Idle:
            ui->progressBar_all->setMaximum(65535);
            ui->progressBar_all->setMinimum(0);
            if(haveStarted && transferModel.rowCount()<=0)
            {
                if(shutdown && ui->shutdown->isChecked())
                {
                    facilityEngine->callFunctionality("shutdown");
                    return;
                }
                switch(uiOptions->comboBox_copyEnd->currentIndex())
                {
                    case 2:
                        emit cancel();
                    break;
                    case 0:
                        if(!haveError)
                            emit cancel();
                        else
                            ui->tabWidget->setCurrentWidget(ui->tab_error);
                    break;
                    default:
                    break;
                }
                stat = status_stopped;
                if(durationStarted)
                {
                    Ultracopier::TimeDecomposition time=facilityEngine->secondsToTimeDecomposition(
                                (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()-
                                duration)
                                /1000);
                    ui->labelTimeRemaining->setText(QStringLiteral("<html><body style=\"white-space:nowrap;\">")+
                                                    QString::fromStdString(facilityEngine->translateText("Completed in %1")).arg(
                                                        QString::number(time.hour)+QStringLiteral(":")+
                                                        QString::number(time.minute).rightJustified(2,'0')+
                                                        QStringLiteral(":")+
                                                        QString::number(time.second).rightJustified(2,'0')
                                                        )+QStringLiteral("</body></html>"));
                }
            }
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Very wrong switch case!");
        break;
    }
    switch(action)
    {
        case Ultracopier::Copying:
        case Ultracopier::CopyingAndListing:
            ui->pauseButton->setEnabled(true);
            if(!durationStarted)
            {
                duration=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                durationStarted=true;
            }
            haveStarted=true;
            ui->cancelButton->setText(QString::fromStdString(facilityEngine->translateText("Quit")));
            updatePause();
        break;
        case Ultracopier::Listing:
            ui->pauseButton->setEnabled(false);
            haveStarted=true;//to close if skip at root folder collision
        break;
        case Ultracopier::Idle:
            ui->pauseButton->setEnabled(false);
        break;
        default:
        break;
    }
}

void Themes::newFolderListing(const std::string &path)
{
    QString newPath=QString::fromStdString(path);
    if(newPath.size()>(64+3))
        newPath=newPath.mid(0,32)+QStringLiteral("...")+newPath.mid(newPath.size()-32,32);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(action==Ultracopier::Listing)
        ui->from->setText(newPath);
}

void Themes::detectedSpeed(const uint64_t &speed)//in byte per seconds
{
    if(uiOptions->speedWithProgressBar->isChecked())
    {
        quint64 tempSpeed=speed;
        if(tempSpeed>999999999)
            tempSpeed=999999999;
        if(tempSpeed>(quint64)ui->progressBarCurrentSpeed->maximum())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"set max speed to: "+std::to_string(tempSpeed));
            ui->progressBarCurrentSpeed->setMaximum(tempSpeed);
        }
        ui->progressBarCurrentSpeed->setValue(tempSpeed);
        ui->progressBarCurrentSpeed->setFormat(QString::fromStdString(facilityEngine->speedToString(speed)));
    }
    else
        ui->currentSpeed->setText(QString::fromStdString(facilityEngine->speedToString(speed)));
}

void Themes::remainingTime(const int &remainingSeconds)
{
    QString labelTimeRemaining(QStringLiteral(
                                   "<html><body style=\"white-space:nowrap;\">")+
                                   QString::fromStdString(facilityEngine->translateText("Time remaining:"))+
                                   QStringLiteral(" ")
                               );
    if(remainingSeconds==-1)
        labelTimeRemaining+=QStringLiteral("&#8734;");
    else
    {
        Ultracopier::TimeDecomposition time=facilityEngine->secondsToTimeDecomposition(remainingSeconds);
        labelTimeRemaining+=QString::number(time.hour)+QStringLiteral(":")+QString::number(time.minute).rightJustified(2,'0')+QStringLiteral(":")+QString::number(time.second).rightJustified(2,'0');
    }
    labelTimeRemaining+=QStringLiteral("</body></html>");
    ui->labelTimeRemaining->setText(labelTimeRemaining);
}

void Themes::errorDetected()
{
    haveError=true;
}

/// \brief new error
void Themes::errorToRetry(const std::string &source,const std::string &destination,const std::string &error)
{
    ui->errorList->addTopLevelItem(new QTreeWidgetItem(QStringList()
                                                       << QString::fromStdString(source)
                                                       << QString::fromStdString(destination)
                                                       << QString::fromStdString(error)
                                                       ));
}

/** \brief support speed limitation */
void Themes::setSupportSpeedLimitation(const bool &supportSpeedLimitationBool)
{
    if(!supportSpeedLimitationBool)
    {
        ui->label_Slider_speed->setVisible(false);
        ui->SliderSpeed->setVisible(false);
        ui->label_SpeedMaxValue->setVisible(false);
        uiOptions->labelShowSpeedAsMain->setVisible(false);
        uiOptions->checkBoxShowSpeed->setVisible(false);
    }
    else
        emit newSpeedLimitation(currentSpeed);
}

//get information about the copy
void Themes::setGeneralProgression(const uint64_t &current,const uint64_t &total)
{
    currentSize=current;
    totalSize=total;
    if(total>0)
    {
        int newIndicator=((double)current/total)*65535;
        ui->progressBar_all->setValue(newIndicator);
    }
    else
        ui->progressBar_all->setValue(0);
    if(current>0)
        stat = status_started;
    updateOverallInformation();
    if(isHidden())
        updateSysTrayIcon();
}

void Themes::setFileProgression(const std::vector<Ultracopier::ProgressionItem> &progressionList)
{
    std::vector<Ultracopier::ProgressionItem> progressionListBis=progressionList;
    transferModel.setFileProgression(progressionListBis);
    updateCurrentFileInformation();
}

//edit the transfer list
/// \todo check and re-enable to selection
void Themes::getActionOnList(const std::vector<Ultracopier::ReturnActionOnCopyList> &returnActions)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, returnActions.size(): "+std::to_string(returnActions.size()));
    std::vector<uint64_t> returnValue=transferModel.synchronizeItems(returnActions);
    totalFile+=returnValue.front();
    totalSize+=returnValue.at(1);
    currentFile+=returnValue.back();
    if(transferModel.rowCount()==0)
    {
        ui->skipButton->setEnabled(false);
        ui->progressBar_all->setValue(65535);
        ui->progressBar_file->setValue(65535);
        currentSize=totalSize;
        if(isHidden())
            updateSysTrayIcon();
    }
    else
        ui->skipButton->setEnabled(true);
    updateOverallInformation();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"transferModel.rowCount(): "+std::to_string(transferModel.rowCount()));
}

void Themes::setCopyType(const Ultracopier::CopyType &type)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->type=type;
    updateModeAndType();
}

void Themes::forceCopyMode(const Ultracopier::CopyMode &mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    modeIsForced=true;
    this->mode=mode;
    if(mode==Ultracopier::Copy)
        ui->tabWidget->setTabText(0,tr("Copy list"));
    else
        ui->tabWidget->setTabText(0,tr("Move list"));
    updateModeAndType();
    updateTitle();
}

void Themes::setTransferListOperation(const Ultracopier::TransferListOperation &transferListOperation)
{
    ui->exportTransferList->setVisible(transferListOperation & Ultracopier::TransferListOperation_Export);
    ui->importTransferList->setVisible(transferListOperation & Ultracopier::TransferListOperation_Import);
}

void Themes::haveExternalOrder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
//	ui->moreButton->toggle();
}

void Themes::isInPause(const bool &isInPause)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"isInPause: "+std::to_string(isInPause));
    //resume in auto the pause
    storeIsInPause=isInPause;
    updatePause();
}

void Themes::updatePause()
{
    if(storeIsInPause)
    {
        ui->pauseButton->setIcon(player_play);
        if(stat == status_started)
            ui->pauseButton->setText(QString::fromStdString(facilityEngine->translateText("Resume")));
        else
            ui->pauseButton->setText(QString::fromStdString(facilityEngine->translateText("Start")));
    }
    else
    {
        ui->pauseButton->setIcon(player_pause);
        ui->pauseButton->setText(QString::fromStdString(facilityEngine->translateText("Pause")));
    }
}

void Themes::updateCurrentFileInformation()
{
    TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
    if(transfertItem.haveItem)
    {
        QString newPath=QString::fromStdString(transfertItem.from);
        if(newPath.size()>(64+3))
            newPath=newPath.mid(0,32)+QStringLiteral("...")+newPath.mid(newPath.size()-32,32);
        ui->from->setText(newPath);
        newPath=QString::fromStdString(transfertItem.to);
        if(newPath.size()>(64+3))
            newPath=newPath.mid(0,32)+QStringLiteral("...")+newPath.mid(newPath.size()-32,32);
        ui->to->setText(newPath);
        ui->current_file->setText(QString::fromStdString(transfertItem.current_file));
        if(transfertItem.progressBar_read!=-1)
        {
            ui->progressBar_file->setRange(0,65535);
            if(uiOptions->showDualProgression->isChecked())
            {
                if(transfertItem.progressBar_read!=transfertItem.progressBar_write)
                {
                    float permilleread=round((float)transfertItem.progressBar_read/65535*1000)/1000;
                    float permillewrite=permilleread-0.001;
                    ui->progressBar_file->setStyleSheet(QStringLiteral("QProgressBar{border: 1px solid grey;text-align: center;background-color: qlineargradient(spread:pad, x1:%1, y1:0, x2:%2, y2:0, stop:0 %3, stop:1 %4);}QProgressBar::chunk{background-color:%5;}")
                        .arg(permilleread)
                        .arg(permillewrite)
                        .arg(progressColorRemaining.name())
                        .arg(progressColorRead.name())
                        .arg(progressColorWrite.name())
                        );
                }
                else
                    ui->progressBar_file->setStyleSheet(QStringLiteral("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                        .arg(progressColorRemaining.name())
                        .arg(progressColorWrite.name())
                        );
                ui->progressBar_file->setValue(transfertItem.progressBar_write);
            }
            else
                ui->progressBar_file->setValue((transfertItem.progressBar_read+transfertItem.progressBar_write)/2);
        }
        else
            ui->progressBar_file->setRange(0,0);
    }
    else
    {
        ui->from->setText(QStringLiteral(""));
        ui->to->setText(QStringLiteral(""));
        ui->current_file->setText(QStringLiteral("-"));
        if(haveStarted && transferModel.rowCount()==0)
            ui->progressBar_file->setValue(65535);
        else if(!haveStarted)
            ui->progressBar_file->setValue(0);
    }
}


void Themes::on_putOnTop_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    std::vector<uint64_t> ids;
    int index=0;
    const int &loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids.push_back(transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong());
        index++;
    }
    if(ids.size()>0)
        emit moveItemsOnTop(ids);
}

void Themes::on_pushUp_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    std::vector<uint64_t> ids;
    int index=0;
    const int &loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids.push_back(transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong());
        index++;
    }
    if(ids.size()>0)
        emit moveItemsUp(ids);
}

void Themes::on_pushDown_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    std::vector<uint64_t> ids;
    int index=0;
    const int &loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids.push_back(transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong());
        index++;
    }
    if(ids.size()>0)
        emit moveItemsDown(ids);
}

void Themes::on_putOnBottom_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    std::vector<uint64_t> ids;
    int index=0;
    const int &loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids.push_back(transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong());
        index++;
    }
    if(ids.size()>0)
        emit moveItemsOnBottom(ids);
}

void Themes::on_del_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    std::vector<uint64_t> ids;
    int index=0;
    const int &loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids.push_back(transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong());
        index++;
    }
    if(ids.size()>0)
        emit removeItems(ids);
}

void Themes::on_cancelButton_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->hide();
    emit cancel();
}


void Themes::speedWithProgressBar_toggled(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    ui->progressBarCurrentSpeed->setVisible(checked);
    ui->currentSpeed->setVisible(!checked);
}

void Themes::showDualProgression_toggled(bool checked)
{
    Q_UNUSED(checked);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    updateProgressionColorBar();
}

void Themes::checkBoxShowSpeed_toggled(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    Q_UNUSED(checked);
    updateSpeed();
}

void Themes::on_SliderSpeed_valueChanged(int value)
{
    if(!uiOptions->checkBoxShowSpeed->isChecked())
        return;
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"value: "+std::to_string(value));
    emit newSpeedLimitation(currentSpeed);
    updateSpeed();
}

void Themes::uiUpdateSpeed()
{
    if(uiOptions->checkBoxShowSpeed->isChecked())
        return;
    if(!uiOptions->checkBox_limitSpeed->isChecked())
        currentSpeed=0;
    else
        currentSpeed=uiOptions->limitSpeed->value();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit newSpeedLimitation"+std::to_string(currentSpeed));
    emit newSpeedLimitation(currentSpeed);
}

void Themes::updateSpeed()
{
    ui->label_Slider_speed->setVisible(uiOptions->checkBoxShowSpeed->isChecked());
    ui->SliderSpeed->setVisible(uiOptions->checkBoxShowSpeed->isChecked());
    ui->label_SpeedMaxValue->setVisible(uiOptions->checkBoxShowSpeed->isChecked());
    uiOptions->limitSpeed->setVisible(!uiOptions->checkBoxShowSpeed->isChecked());
    uiOptions->checkBox_limitSpeed->setVisible(!uiOptions->checkBoxShowSpeed->isChecked());

    if(uiOptions->checkBoxShowSpeed->isChecked())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"checked, currentSpeed: "+std::to_string(currentSpeed));
        uiOptions->limitSpeed->setEnabled(false);
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
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(1);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*1)));
        }
        else if(currentSpeed<=1024*4)
        {
            if(currentSpeed!=1024*4)
            {
                currentSpeed=1024*4;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(2);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*4)));
        }
        else if(currentSpeed<=1024*16)
        {
            if(currentSpeed!=1024*16)
            {
                currentSpeed=1024*16;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(3);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*16)));
        }
        else if(currentSpeed<=1024*64)
        {
            if(currentSpeed!=1024*64)
            {
                currentSpeed=1024*64;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(4);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*64)));
        }
        else
        {
            if(currentSpeed!=1024*128)
            {
                currentSpeed=1024*128;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(5);
            ui->label_SpeedMaxValue->setText(QString::fromStdString(facilityEngine->speedToString((double)(1024*1024)*128)));
        }
    }
    else
    {
        uiOptions->checkBox_limitSpeed->setChecked(currentSpeed>0);
        if(currentSpeed>0)
            uiOptions->limitSpeed->setValue(currentSpeed);
        uiOptions->checkBox_limitSpeed->setEnabled(currentSpeed!=-1);
        uiOptions->limitSpeed->setEnabled(uiOptions->checkBox_limitSpeed->isChecked());
    }
}

void Themes::on_pauseButton_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(storeIsInPause)
        emit resume();
    else
        emit pause();
}

void Themes::on_skipButton_clicked()
{
    TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
    if(transfertItem.haveItem)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"skip at running: "+std::to_string(transfertItem.id));
        emit skip(transfertItem.id);
    }
    else
    {
        if(transferModel.rowCount()>1)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"skip at idle: "+std::to_string(transferModel.firstId()));
            emit skip(transferModel.firstId());
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to skip the transfer, because no transfer running");
    }
}

void Themes::updateModeAndType()
{
    menu->clear();
    if(modeIsForced)
    {
        menu->addAction(ui->actionAddFile);
        if(type==Ultracopier::FileAndFolder)
            menu->addAction(ui->actionAddFolder);
    }
    else
    {
        menu->addAction(ui->actionAddFileToCopy);
        menu->addAction(ui->actionAddFileToMove);
        if(type==Ultracopier::FileAndFolder)
        {
            menu->addAction(ui->actionAddFolderToCopy);
            menu->addAction(ui->actionAddFolderToMove);
        }
    }
}

void Themes::forcedModeAddFile()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFile(mode);
}

void Themes::forcedModeAddFolder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFolder(mode);
}

void Themes::forcedModeAddFileToCopy()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFile(Ultracopier::Copy);
}

void Themes::forcedModeAddFolderToCopy()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFolder(Ultracopier::Copy);
}

void Themes::forcedModeAddFileToMove()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFile(Ultracopier::Move);
}

void Themes::forcedModeAddFolderToMove()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFolder(Ultracopier::Move);
}

void Themes::newLanguageLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(modeIsForced)
        forceCopyMode(mode);
    ui->retranslateUi(this);
    uiOptions->retranslateUi(this);
    uiOptions->comboBox_copyEnd->setItemText(0,tr("Don't close if errors are found"));
    uiOptions->comboBox_copyEnd->setItemText(1,tr("Never close"));
    uiOptions->comboBox_copyEnd->setItemText(2,tr("Always close"));
    if(!haveStarted)
        ui->current_file->setText(tr("File Name, 0KB"));
    else
        updateCurrentFileInformation();
    updateOverallInformation();
    updateSpeed();
    if(ui->tabWidget->count()>=4)
        ui->tabWidget->setTabText(ui->tabWidget->count()-1,
                                  QString::fromStdString(facilityEngine->translateText("Copy engine")));
    on_moreButton_toggled(ui->moreButton->isChecked());
}

void Themes::on_pushButtonCloseSearch_clicked()
{
    closeTheSearchBox();
}

//close the search box
void Themes::closeTheSearchBox()
{
    currentIndexSearch = -1;
    ui->lineEditSearch->clear();
    ui->lineEditSearch->hide();
    ui->pushButtonSearchPrev->hide();
    ui->pushButtonSearchNext->hide();
    ui->pushButtonCloseSearch->hide();
    ui->searchButton->setChecked(false);
    hilightTheSearch();
}

//search box shortcut
void Themes::searchBoxShortcut()
{
/*	if(ui->lineEditSearch->isHidden())
    {*/
        ui->lineEditSearch->show();
        ui->pushButtonSearchPrev->show();
        ui->pushButtonSearchNext->show();
        ui->pushButtonCloseSearch->show();
        ui->lineEditSearch->setFocus(Qt::ShortcutFocusReason);
        ui->searchButton->setChecked(true);
/*	}
    else
        closeTheSearchBox();*/
}

//hilight the search
void Themes::hilightTheSearch(bool searchNext)
{
    int result=transferModel.search(ui->lineEditSearch->text().toStdString(),searchNext);
    if(ui->lineEditSearch->text().isEmpty())
        ui->lineEditSearch->setStyleSheet("");
    else
    {
        if(result==-1)
            ui->lineEditSearch->setStyleSheet(QStringLiteral("background-color: rgb(255, 150, 150);"));
        else
        {
            ui->lineEditSearch->setStyleSheet(QStringLiteral("background-color: rgb(193,255,176);"));
            ui->TransferList->scrollTo(transferModel.index(result,0));
        }
    }
}

void Themes::hilightTheSearchSlot()
{
    hilightTheSearch();
}

void Themes::on_pushButtonSearchPrev_clicked()
{
    int result=transferModel.searchPrev(ui->lineEditSearch->text().toStdString());
    if(ui->lineEditSearch->text().isEmpty())
        ui->lineEditSearch->setStyleSheet("");
    else
    {
        if(result==-1)
            ui->lineEditSearch->setStyleSheet(QStringLiteral("background-color: rgb(255, 150, 150);"));
        else
        {
            ui->lineEditSearch->setStyleSheet(QStringLiteral("background-color: rgb(193,255,176);"));
            ui->TransferList->scrollTo(transferModel.index(result,0));
        }
    }
}

void Themes::on_pushButtonSearchNext_clicked()
{
    hilightTheSearch(true);
}

void Themes::on_lineEditSearch_returnPressed()
{
    hilightTheSearch();
}

void Themes::on_lineEditSearch_textChanged(QString text)
{
    if(text=="")
    {
        TimerForSearch->stop();
        hilightTheSearch();
    }
    else
        TimerForSearch->start();
}

void Themes::on_moreButton_toggled(bool checked)
{
    if(checked)
        this->setMaximumHeight(16777215);
    else
        this->setMaximumHeight(130);
    // usefull under windows
    this->updateGeometry();
    this->update();
    this->adjustSize();
}

/* drag event processing

need setAcceptDrops(true); into the constructor
need implementation to accept the drop:
void dragEnterEvent(QDragEnterEvent* event);
void dragMoveEvent(QDragMoveEvent* event);
void dragLeaveEvent(QDragLeaveEvent* event);
*/
void Themes::dropEvent(QDropEvent *event)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"hasUrls");
        std::vector<std::string> urls;
        foreach (QUrl url, mimeData->urls())
            urls.push_back(url.toString().toStdString());
        emit urlDropped(urls);
        event->acceptProposedAction();
    }
}

void Themes::dragEnterEvent(QDragEnterEvent* event)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    // if some actions should not be usable, like move, this code must be adopted
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"hasUrls");
        event->acceptProposedAction();
    }
}

void Themes::dragMoveEvent(QDragMoveEvent* event)
{
    // if some actions should not be usable, like move, this code must be adopted
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
        event->acceptProposedAction();
}

void Themes::dragLeaveEvent(QDragLeaveEvent* event)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    event->accept();
}

void Themes::on_searchButton_toggled(bool checked)
{
    if(checked)
        searchBoxShortcut();
    else
        closeTheSearchBox();
}

void Themes::on_exportTransferList_clicked()
{
    emit exportTransferList();
}

void Themes::on_importTransferList_clicked()
{
    emit importTransferList();
}

void Themes::progressColorWrite_clicked()
{
    QColor color=QColorDialog::getColor(progressColorWrite,this,tr("Select a color"));
    if(!color.isValid())
        return;
    progressColorWrite=color;
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorWrite);
    uiOptions->progressColorWrite->setIcon(pixmap);
    updateProgressionColorBar();
}

void Themes::progressColorRead_clicked()
{
    QColor color=QColorDialog::getColor(progressColorRead,this,tr("Select a color"));
    if(!color.isValid())
        return;
    progressColorRead=color;
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorRead);
    uiOptions->progressColorRead->setIcon(pixmap);
    updateProgressionColorBar();
}

void Themes::progressColorRemaining_clicked()
{
    QColor color=QColorDialog::getColor(progressColorRemaining,this,tr("Select a color"));
    if(!color.isValid())
        return;
    progressColorRemaining=color;
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorRemaining);
    uiOptions->progressColorRemaining->setIcon(pixmap);
    updateProgressionColorBar();
}

void Themes::alwaysOnTop_clicked(bool reshow)
{
    Qt::WindowFlags flags = windowFlags();
    #ifdef Q_OS_WIN32
    if(uiOptions->alwaysOnTop->isChecked())
        SetWindowPos((HWND)this->winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    else
        SetWindowPos((HWND)this->winId(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    #endif
    #ifdef Q_OS_LINUX
    if(uiOptions->alwaysOnTop->isChecked())
        flags=flags | Qt::X11BypassWindowManagerHint;
    else
        flags=flags & ~Qt::X11BypassWindowManagerHint;
    #endif
    if(uiOptions->alwaysOnTop->isChecked())
        flags=flags | Qt::WindowStaysOnTopHint;
    else
        flags=flags & ~Qt::WindowStaysOnTopHint;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"uiOptions->alwaysOnTop->isChecked(): "+std::to_string(uiOptions->alwaysOnTop->isChecked())+", flags: "+std::to_string(flags));
    setWindowFlags(flags);
    if(reshow)
        show();
}

void Themes::alwaysOnTop_clickedSlot()
{
    alwaysOnTop_clicked(true);
}

void Themes::updateProgressionColorBar()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    uiOptions->labelProgressionColor->setVisible(uiOptions->showDualProgression->isChecked());
    uiOptions->frameProgressionColor->setVisible(uiOptions->showDualProgression->isChecked());
    if(!uiOptions->showDualProgression->isChecked())
    {
        ui->progressBar_all->setStyleSheet(QStringLiteral(""));
        ui->progressBar_file->setStyleSheet(QStringLiteral(""));
        ui->progressBarCurrentSpeed->setStyleSheet(QStringLiteral(""));
    }
    else
    {
        ui->progressBar_all->setStyleSheet(QStringLiteral("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                                           .arg(progressColorRemaining.name())
                                           .arg(progressColorWrite.name())
                                           );
        ui->progressBar_file->setStyleSheet(QStringLiteral("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                                            .arg(progressColorRemaining.name())
                                            .arg(progressColorWrite.name())
                                            );
        ui->progressBarCurrentSpeed->setStyleSheet(QStringLiteral("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                                           .arg(progressColorRemaining.name())
                                           .arg(progressColorWrite.name())
                                           );
    }
    if(stat==status_never_started)
        updateCurrentFileInformation();
}

void Themes::updateTitle()
{
    if(uiOptions->showProgressionInTheTitle->isChecked() && totalSize>0)
    {
        if(!modeIsForced)
            this->setWindowTitle(tr("%1 %2% of %3").arg(QString::fromStdString(facilityEngine->translateText("Transfer"))).arg((currentSize*100)/totalSize).arg(QString::fromStdString(facilityEngine->sizeToString(totalSize)))+QStringLiteral(" - ")+QString::fromStdString(facilityEngine->softwareName()));
        else
        {
            if(mode==Ultracopier::Copy)
                this->setWindowTitle(tr("%1 %2% of %3").arg(QString::fromStdString(facilityEngine->translateText("Copy"))).arg((currentSize*100)/totalSize).arg(QString::fromStdString(facilityEngine->sizeToString(totalSize)))+QStringLiteral(" - ")+QString::fromStdString(facilityEngine->softwareName()));
            else
                this->setWindowTitle(tr("%1 %2% of %3").arg(QString::fromStdString(facilityEngine->translateText("Move"))).arg((currentSize*100)/totalSize).arg(QString::fromStdString(facilityEngine->sizeToString(totalSize)))+QStringLiteral(" - ")+QString::fromStdString(facilityEngine->softwareName()));
        }
    }
    else
    {
        if(!modeIsForced)
            this->setWindowTitle(QStringLiteral("%1").arg(QString::fromStdString(facilityEngine->translateText("Transfer")))+QStringLiteral(" - ")+QString::fromStdString(facilityEngine->softwareName()));
        else
        {
            if(mode==Ultracopier::Copy)
                this->setWindowTitle(QStringLiteral("%1").arg(QString::fromStdString(facilityEngine->translateText("Copy")))+QStringLiteral(" - ")+QString::fromStdString(facilityEngine->softwareName()));
            else
                this->setWindowTitle(QStringLiteral("%1").arg(QString::fromStdString(facilityEngine->translateText("Move")))+QStringLiteral(" - ")+QString::fromStdString(facilityEngine->softwareName()));
        }
    }
}

/** \brief Create progessive icon

Do QIcon with top and bottom image mixed and percent writed on it.
The icon it be search in the style path.
Do by mongaulois, remake by alpha_one_x86.
\param percent indique how many percent need be showed, sould be between 0 and 100
\param text The showed text if needed (optionnal)
\return QIcon of the final image
\note Can be used as it: dynaIcon(75,"...")
*/
QIcon Themes::dynaIcon(int percent,std::string text) const
{
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(pixmapTop.isNull() || pixmapBottom.isNull())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error loading the icons");
    #endif
    if(percent==-1)
        percent=getOldProgression;
    if(percent<0)
        percent=0;
    if(percent>100)
        percent=100;
    //pixmap avec un fond transparent
    #ifdef Q_OS_WIN32
    quint8 imageSize=16;
    #else
    quint8 imageSize=22;
    #endif
    QPixmap resultImage(imageSize,imageSize);
    resultImage.fill(Qt::transparent);
    {
        QPainter painter(&resultImage);
        #ifndef Q_OS_WIN32
        QFont font(QStringLiteral("Courier New"),9);
        font.setBold(true);
        font.setKerning(true);
        painter.setFont(font);
        #endif
        #ifdef Q_OS_WIN32
        QFont font(QStringLiteral("Courier New"),8);
        font.setBold(true);
        font.setKerning(true);
        painter.setFont(font);
        #endif

        //preprocessing the calcul
        quint8 bottomPixel=(percent*imageSize)/100;
        quint8 topPixel=imageSize-bottomPixel;

        //top image
        if(topPixel>0)
        {
            QRect target(0, 0, imageSize, topPixel);
            QRect source(0, 0, imageSize, topPixel);
            painter.drawPixmap(target, pixmapTop, source);
        }

        //bottom image
        if(bottomPixel>0)
        {
            QRect target2(0, topPixel, imageSize, bottomPixel);
            QRect source2(0, topPixel, imageSize, bottomPixel);
            painter.drawPixmap(target2, pixmapBottom, source2);
        }

        qint8 textxOffset=0;
        qint8 textyOffset=0;
        if(text.empty())
        {
            if(percent!=100)
                text=std::to_string(percent);
            else
            {
                text=":)";
                #ifdef Q_OS_WIN32
                textyOffset-=2;
                #else
                textyOffset-=1;
                #endif
            }
        }
        if(text.size()==1)
        {
            textxOffset+=3;
            #ifdef Q_OS_WIN32
            textxOffset-=1;
            #endif
        }
        else
        {
            #ifdef Q_OS_WIN32
            textxOffset-=1;
            #endif
        }
        #ifndef Q_OS_WIN32
        textxOffset+=2;
        textyOffset+=3;
        #endif
        painter.setPen(QPen(Qt::black));
        painter.drawText(3+textxOffset,13+textyOffset,QString::fromStdString(text));
        painter.setPen(QPen(Qt::white));
        painter.drawText(2+textxOffset,12+textyOffset,QString::fromStdString(text));
    }
    return QIcon(resultImage);
}

/** \brief For catch an action on the systray icon
\param reason Why it activated
*/
void Themes::catchAction(QSystemTrayIcon::ActivationReason reason)
{
    if(reason==QSystemTrayIcon::DoubleClick || reason==QSystemTrayIcon::Trigger)
    {
        sysTrayIcon->hide();
        this->show();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Double Click detected");
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"reason: "+std::to_string(reason));
}

void Themes::on_exportErrorToTransferList_clicked()
{
    emit exportErrorIntoTransferList();
}
