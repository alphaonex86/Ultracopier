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
#ifdef Q_OS_WIN32
#include <windows.h>
#endif
#include <QDesktopServices>

#define ULTRACOPIERO2_MAXREMAININGTIMECOL 10
#define ULTRACOPIERO2_MAXVALUESPEEDSTORED 5

#include "interface.h"
#include "ui_interface.h"
#include "ThemesFactory.h"
#include "ProgressBarDark.h"

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
               const bool &startMinimized,
               const quint8 &position,
               const bool &dark) :
    duration(0),
    durationStarted(false),
    ui(new Ui::interfaceCopy()),
    uiOptions(new Ui::themesOptions()),
    currentFile(0),
    totalFile(0),
    currentSize(0),
    totalSize(0),
    getOldProgression(0),
    sysTrayIcon(NULL),
    menu(NULL),
    action(Ultracopier::EngineActionInProgress::Idle),
    currentSpeed(0),
    storeIsInPause(false),
    modeIsForced(false),
    type(Ultracopier::CopyType::FileAndFolder),
    mode(Ultracopier::CopyMode::Copy),
    haveStarted(false),
    haveError(false)
    #ifdef Q_OS_WIN32
    ,winTaskbarProgress(this)
    #endif
{
    darkUi=dark;
    this->facilityEngine=facilityEngine;
    File::facilityEngine=facilityEngine;
    ui->setupUi(this);
    uiOptions->setupUi(ui->optionsTab);

    m_havePause=false;
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
    uiOptions->labelSavePosition->setVisible(false);
    uiOptions->savePosition->setVisible(false);
    uiOptions->savePosition->setCurrentIndex(position);
    uiOptions->label_Slider_speed->setVisible(false);
    uiOptions->SliderSpeed->setVisible(false);
    uiOptions->label_SpeedMaxValue->setVisible(false);
    uiOptions->comboBox_copyEnd->setCurrentIndex(comboBox_copyEnd);
    ui->progressBar->setValue(0);
    ui->progressBar_2->setValue(0);
    ui->progressBar_3->setValue(0);
    ui->progressBar_4->setValue(0);
    ui->progressBar_5->setValue(0);
    ui->progressBar_6->setValue(0);
    speedWithProgressBar_toggled(speedWithProgressBar);
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorWrite);
    uiOptions->progressColorWrite->setIcon(pixmap);
    pixmap.fill(progressColorRead);
    uiOptions->progressColorRead->setIcon(pixmap);
    pixmap.fill(progressColorRemaining);
    uiOptions->progressColorRemaining->setIcon(pixmap);
    ui->labelTimeRemaining->setText(QString());

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

    connect(uiOptions->limitSpeed,		static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,	&Themes::uiUpdateSpeed);
    connect(uiOptions->checkBox_limitSpeed,&QAbstractButton::toggled,		this,	&Themes::uiUpdateSpeed);

    connect(ui->actionAddFile,&QAction::triggered,this,&Themes::forcedModeAddFile);
    connect(ui->actionAddFileToCopy,&QAction::triggered,this,&Themes::forcedModeAddFileToCopy);
    connect(ui->actionAddFileToMove,&QAction::triggered,this,&Themes::forcedModeAddFileToMove);
    connect(ui->actionAddFolderToCopy,&QAction::triggered,this,&Themes::forcedModeAddFolderToCopy);
    connect(ui->actionAddFolderToMove,&QAction::triggered,this,&Themes::forcedModeAddFolderToMove);
    connect(ui->actionAddFolder,&QAction::triggered,this,&Themes::forcedModeAddFolder);
    connect(ui->exportErrorToTransferList,&QToolButton::triggered,this,&Themes::exportErrorIntoTransferList);

    ui->overall->hide();

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

    //remaining time
    {
        int index=0;
        while(index<ULTRACOPIERO2_MAXREMAININGTIMECOL)
        {
            RemainingTimeLogarithmicColumn newEntry;
            remainingTimeLogarithmicValue.push_back(newEntry);
            index++;
        }
    }

    //reload directly untranslatable text
    newLanguageLoaded();

    //unpush the more button
    ui->moreButton->setChecked(moreButtonPushed);
    on_moreButton_toggled(moreButtonPushed);

    /// \note important for drag and drop, \see dropEvent()
    setAcceptDrops(true);

    const QString themePath=":/Themes/Oxygen2/";

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

    shutdown=facilityEngine->haveFunctionality("shutdown");
    ui->shutdown->setVisible(shutdown);
    radial=new RadialMap::Widget(dark,this);
    ui->verticalLayouMiddle->addWidget(radial);

    chartarea=new ChartArea::Widget(facilityEngine,this);
    ui->verticalLayoutRight->insertWidget(0,chartarea);

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
    QString ultimateUrl;
    if(facilityEngine->isUltimate())
        ui->ad_ultimate->hide();
    else
    {
        ultimateUrl=QString::fromStdString(facilityEngine->ultimateUrl());
        if(ultimateUrl.isEmpty())
            ui->ad_ultimate->hide();
        else
            ui->ad_ultimate->setText(
                    QStringLiteral("<a href=\"%1\">%2</a> - <a href=\"register\">%3</a>").arg(ultimateUrl).arg(tr("Buy the Ultimate version to fund development")).arg(tr("Register your key"))+", "
                        +QStringLiteral("Follow us: ")+QStringLiteral("<a href=\"%1\"><img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABnRSTlMAAAAAAABupgeRAAAAe0lEQVR4AWOAAPesxQQRUBlCNZEIu+qIjSfPvvn45c/f//////17vLxrJrIsugan3i3HoUq/fPj46c27gwWts/FpsF1x5O2/f////z5+th0uiFNDyb3n/1HBx0+LAwsWUaIB00krj7wHqfx94HgbXJBUDaMaSE58JCdvAAioiiB5mraWAAAAAElFTkSuQmCC\"/></a>").arg("https://www.facebook.com/Ultracopier/")
                        );
    }

    sysTrayIcon = new QSystemTrayIcon(this);
    connect(sysTrayIcon,&QSystemTrayIcon::activated,this,&Themes::catchAction);
    #ifdef Q_OS_WIN32
    winTaskbarProgress.show();
    #endif

    verticalLabel=new VerticalLabel();
    verticalLabel->setText(QString::fromStdString(facilityEngine->speedToString(50*1000*1000)));
    ui->verticalLayoutVL->insertWidget(0,verticalLabel);

    if(darkUi)
    {
        if(ultimateUrl.isEmpty())
            ui->ad_ultimate->hide();
        else
            ui->ad_ultimate->setText(
                QStringLiteral("<a href=\"%1\"><span style=\"color:#cdf;\">%2</span></a> - <a href=\"register\">%3</a>").arg(ultimateUrl).arg(tr("Buy the Ultimate version to fund development")).arg(tr("Register your key"))+", "
                    +QStringLiteral("<span style=\"color:#fff;\">Follow us:</span> ")+QStringLiteral("<a href=\"%1\"><img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABnRSTlMAAAAAAABupgeRAAAAe0lEQVR4AWOAAPesxQQRUBlCNZEIu+qIjSfPvvn45c/f//////17vLxrJrIsugan3i3HoUq/fPj46c27gwWts/FpsF1x5O2/f////z5+th0uiFNDyb3n/1HBx0+LAwsWUaIB00krj7wHqfx94HgbXJBUDaMaSE58JCdvAAioiiB5mraWAAAAAElFTkSuQmCC\"/></a>").arg("https://www.facebook.com/Ultracopier/")
                    );
        //ui->frame->setStyleSheet("#frame{background-color: qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, stop:0 rgb(70, 70, 70), stop:1 rgb(40, 40, 40));}");
        ui->labelTimeRemaining->setStyleSheet("color:#fff;");
        ui->labelSPStart->setStyleSheet("color:#aaa;");
        ui->labelSPStop->setStyleSheet("color:#aaa;");
        ui->from_label->setStyleSheet("color:#aaa;");
        ui->current_file->setStyleSheet("color:#fff;");
        ui->from->setStyleSheet("color:#fff;");
        verticalLabel->setColor(QColor(160,160,160));

        //ui->ad_ultimate->setStyleSheet("color:#fff;background-color:rgb(50, 50, 50);");

        QString labelTimeRemaining;
        labelTimeRemaining+="<html><body style=\"white-space:nowrap;\"><small style=\"color:#aaa\">";
        labelTimeRemaining+=QString::fromStdString(facilityEngine->translateText("Remaining:"));
        labelTimeRemaining+="</small>";
        labelTimeRemaining+=QStringLiteral(" <b>");
        labelTimeRemaining+=QStringLiteral("&#8734;");
        labelTimeRemaining+=QStringLiteral("</b></body></html>");
        ui->labelTimeRemaining->setText(labelTimeRemaining);

        ui->frameS->setStyleSheet("#frameS{border: 1px solid #b0c0f0;} QProgressBar{background-color: rgba(160,180,240,100);border: 0 solid grey; } QProgressBar::chunk {background-color: rgba(160,180,240,200);}");

        int tempIndex=ui->verticalLayoutLeft->indexOf(ui->progressBar_all);
        progressBar_all=new ProgressBarDark(ui->frameLeft);
        progressBar_all->setMaximum(ui->progressBar_all->maximum());
        progressBar_all->setValue(ui->progressBar_all->value());
        ui->progressBar_all->hide();
        ui->verticalLayoutLeft->insertWidget(tempIndex,progressBar_all);

        tempIndex=ui->verticalLayoutRight->indexOf(ui->progressBar_file);
        progressBar_file=new ProgressBarDark(ui->frameRight);
        progressBar_file->setMaximum(ui->progressBar_file->maximum());
        progressBar_file->setValue(ui->progressBar_file->value());
        ui->progressBar_file->hide();
        ui->verticalLayoutRight->insertWidget(tempIndex,progressBar_file);

        tempIndex=ui->horizontalLayoutLeft->indexOf(ui->moreButton);
        moreButton=new DarkButton(ui->frameLeft);
        moreButton->setText(ui->moreButton->text());
        moreButton->setCheckable(ui->moreButton->isCheckable());
        moreButton->setMinimumWidth(60);
        ui->moreButton->hide();
        ui->horizontalLayoutLeft->insertWidget(tempIndex,moreButton);
        {
            QIcon i;
            i.addFile(QString::fromUtf8(":/Themes/Oxygen/resources/darkmoveUp.png"), QSize(), QIcon::Normal, QIcon::Off);
            i.addFile(QString::fromUtf8(":/Themes/Oxygen/resources/darkmoveDown.png"), QSize(), QIcon::Normal, QIcon::On);
            moreButton->setIcon(i);
        }
        connect(moreButton,&QPushButton::toggled,ui->moreButton,&QPushButton::toggled);

        tempIndex=ui->horizontalLayoutLeft->indexOf(ui->pauseButton);
        pauseButton=new DarkButton(ui->frameLeft);
        pauseButton->setText(ui->pauseButton->text());
        pauseButton->setCheckable(ui->pauseButton->isCheckable());
        pauseButton->setMinimumWidth(60);
        ui->pauseButton->hide();
        ui->horizontalLayoutLeft->insertWidget(tempIndex,pauseButton);
        {
            QIcon i;
            i.addFile(QString::fromUtf8(":/Themes/Oxygen/resources/darkplayer_pause.png"), QSize(), QIcon::Normal, QIcon::Off);
            i.addFile(QString::fromUtf8(":/Themes/Oxygen/resources/darkplayer_play.png"), QSize(), QIcon::Normal, QIcon::On);
            pauseButton->setIcon(i);
        }
        connect(pauseButton,&QPushButton::toggled,ui->pauseButton,&QPushButton::toggled);
        connect(pauseButton,&QPushButton::clicked,ui->pauseButton,&QPushButton::clicked);

        tempIndex=ui->horizontalLayoutRight->indexOf(ui->skipButton);
        skipButton=new DarkButton(ui->frameLeft);
        skipButton->setText(ui->skipButton->text());
        skipButton->setCheckable(ui->skipButton->isCheckable());
        skipButton->setMinimumWidth(60);
        ui->skipButton->hide();
        ui->horizontalLayoutRight->insertWidget(tempIndex,skipButton);
        {
            QIcon i;
            i.addFile(QString::fromUtf8(":/Themes/Oxygen/resources/darkplayer_end.png"));
            skipButton->setIcon(i);
        }
        connect(skipButton,&QPushButton::toggled,ui->skipButton,&QPushButton::toggled);
        connect(skipButton,&QPushButton::clicked,ui->skipButton,&QPushButton::clicked);

        tempIndex=ui->horizontalLayoutRight->indexOf(ui->cancelButton);
        cancelButton=new DarkButton(ui->frameLeft);
        cancelButton->setText(ui->cancelButton->text());
        cancelButton->setCheckable(ui->cancelButton->isCheckable());
        cancelButton->setMinimumWidth(60);
        ui->cancelButton->hide();
        ui->horizontalLayoutRight->insertWidget(tempIndex,cancelButton);
        {
            QIcon i;
            i.addFile(QString::fromUtf8(":/Themes/Oxygen/resources/cancelDarkE.png"));
            cancelButton->setIcon(i);
        }
        connect(cancelButton,&QPushButton::toggled,ui->cancelButton,&QPushButton::toggled);
        connect(cancelButton,&QPushButton::clicked,ui->cancelButton,&QPushButton::clicked);

        #if defined(__EMSCRIPTEN__) && defined(ULTRACOPIER_LITTLE_RANDOM)
        cancelButton->setEnabled(false);
        #endif
    }
    else
    {
        progressBar_all=nullptr;
        progressBar_file=nullptr;

        moreButton=nullptr;
        pauseButton=nullptr;
        skipButton=nullptr;
        cancelButton=nullptr;
        #if defined(__EMSCRIPTEN__) && defined(ULTRACOPIER_LITTLE_RANDOM)
        ui->cancelButton->setEnabled(false);
        #endif
    }
    connect(ui->ad_ultimate,&QLabel::linkActivated,this,&Themes::ad_ultimate_clicked);
    isInPause(false);
    showDualProgression_toggled(showDualProgression);

    show();
}

Themes::~Themes()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //disconnect(ui->actionAddFile);
    //disconnect(ui->actionAddFolder);
    if(progressBar_all!=nullptr)
        delete progressBar_all;
    if(progressBar_file!=nullptr)
        delete progressBar_file;
    if(moreButton!=nullptr)
        delete moreButton;
    if(pauseButton!=nullptr)
        delete pauseButton;
    if(skipButton!=nullptr)
        delete skipButton;
    if(cancelButton!=nullptr)
        delete cancelButton;
    delete radial;
    delete selectionModel;
    delete menu;
    delete sysTrayIcon;
}

void Themes::ad_ultimate_clicked(const QString &link)
{
    if(link.startsWith("http://") || link.startsWith("https://"))
        QDesktopServices::openUrl(QUrl(link));
    else
        emit askProductKey();
}

void Themes::changeToUltimate()
{
    ui->ad_ultimate->hide();
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
            if(darkUi)
            {
                progressBar_all->setMaximum(65535);
                progressBar_all->setMinimum(0);
            }
            else
            {
                ui->progressBar_all->setMaximum(65535);
                ui->progressBar_all->setMinimum(0);
            }
            #ifdef Q_OS_WIN32
            winTaskbarProgress.setMaximum(65535);
            winTaskbarProgress.setMinimum(0);
            #endif
        break;
        case Ultracopier::Listing:
            if(darkUi)
            {
                progressBar_all->setMaximum(0);
                progressBar_all->setMinimum(0);
            }
            else
            {
                ui->progressBar_all->setMaximum(0);
                ui->progressBar_all->setMinimum(0);
            }
            #ifdef Q_OS_WIN32
            winTaskbarProgress.setMaximum(0);
            winTaskbarProgress.setMinimum(0);
            #endif
        break;
        case Ultracopier::Idle:
            #ifdef Q_OS_WIN32
            winTaskbarProgress.setMaximum(65535);
            winTaskbarProgress.setMinimum(0);
            #endif
            if(darkUi)
            {
                progressBar_all->setMaximum(65535);
                progressBar_all->setMinimum(0);
            }
            else
            {
                ui->progressBar_all->setMaximum(65535);
                ui->progressBar_all->setMinimum(0);
            }
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
            if(m_havePause)
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
            if(m_havePause)
                ui->pauseButton->setEnabled(false);
            haveStarted=true;//to close if skip at root folder collision
        break;
        case Ultracopier::Idle:
            if(m_havePause)
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
    /*if(uiOptions->speedWithProgressBar->isChecked())
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
        ui->currentSpeed->setText(QString::fromStdString(facilityEngine->speedToString(speed)));*/
    chartarea->addValue(speed);
}

void Themes::remainingTime(const int &remainingSeconds)
{
    QString labelTimeRemaining;
    if(darkUi)
        labelTimeRemaining+="<html><body style=\"white-space:nowrap;\"><small style=\"color:#aaa\">";
    else
        labelTimeRemaining+="<html><body style=\"white-space:nowrap;\">";
    labelTimeRemaining+=QString::fromStdString(facilityEngine->translateText("Remaining:"));
    if(darkUi)
        labelTimeRemaining+="</small>";
    labelTimeRemaining+=QStringLiteral(" <b>");
    if(remainingSeconds==-1)
        labelTimeRemaining+=QStringLiteral("&#8734;");
    else
    {
        Ultracopier::TimeDecomposition time=facilityEngine->secondsToTimeDecomposition(remainingSeconds);
        labelTimeRemaining+=QString::number(time.hour)+QStringLiteral(":")+QString::number(time.minute).rightJustified(2,'0')+QStringLiteral(":")+QString::number(time.second).rightJustified(2,'0');
    }
    labelTimeRemaining+=QStringLiteral("</b></body></html>");
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
        if(darkUi)
            progressBar_all->setValue(newIndicator);
        else
            ui->progressBar_all->setValue(newIndicator);
        #ifdef Q_OS_WIN32
        winTaskbarProgress.setValue(newIndicator);
        #endif
    }
    else
    {
        if(darkUi)
            progressBar_all->setValue(0);
        else
            ui->progressBar_all->setValue(0);
        #ifdef Q_OS_WIN32
        winTaskbarProgress.setValue(0);
        #endif
    }
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
    if(transferModel.tree==NULL)
        transferModel.tree=new Folder(std::string());
    std::vector<uint64_t> returnValue=transferModel.synchronizeItems(returnActions);
    totalFile+=returnValue.front();
    totalSize+=returnValue.at(1);
    currentFile+=returnValue.back();
    if(transferModel.rowCount()==0)
    {
        ui->skipButton->setEnabled(false);
        if(darkUi)
        {
            progressBar_all->setValue(65535);
            progressBar_file->setValue(65535);
        }
        else
        {
            ui->progressBar_all->setValue(65535);
            ui->progressBar_file->setValue(65535);
        }
        #ifdef Q_OS_WIN32
        winTaskbarProgress.setValue(65535);
        #endif
        currentSize=totalSize;
        if(isHidden())
            updateSysTrayIcon();
    }
    else
        ui->skipButton->setEnabled(true);
    updateOverallInformation();
    radial->invalidate();
    radial->create(transferModel.tree);
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

/// \brief set have pause
void Themes::havePause(const bool &havePause)
{
    if(darkUi)
        pauseButton->setEnabled(havePause);
    else
        ui->pauseButton->setEnabled(havePause);
    m_havePause=havePause;
}

void Themes::updatePause()
{
    QPushButton *tempPauseButton=ui->pauseButton;
    if(darkUi)
        tempPauseButton=pauseButton;
    if(storeIsInPause)
    {
        if(!darkUi)
            ui->pauseButton->setIcon(player_play);
        if(stat == status_started)
            tempPauseButton->setText(QString::fromStdString(facilityEngine->translateText("Resume")));
        else
            tempPauseButton->setText(QString::fromStdString(facilityEngine->translateText("Start")));
    }
    else
    {
        if(!darkUi)
            tempPauseButton->setIcon(player_pause);
        tempPauseButton->setText(QString::fromStdString(facilityEngine->translateText("Pause")));
    }
}

void Themes::updateCurrentFileInformation()
{
    TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
    if(transfertItem.haveItem)
    {
        std::string from=transfertItem.from;
        std::string::size_type pos=from.rfind('/');
        if(pos == std::string::npos)
        {
            #ifdef Q_OS_WIN32
            std::string::size_type pos=from.rfind('\\');
            if(pos != std::string::npos)
                if(pos < from.size()-1)
                    from=from.substr(0,pos);
            #endif
        }
        else if(pos < from.size()-1)
        {
            #ifdef Q_OS_WIN32
            std::string::size_type pos2=from.rfind('\\');
            if(pos2 != std::string::npos)
            {
                std::string::size_type pos=from.rfind('\\');
                if(pos != std::string::npos)
                {
                    if(pos2 < from.size()-1)
                    {
                        if(pos<pos2)
                            from=from.substr(0,pos2);
                        else
                            from=from.substr(0,pos);
                    }
                }
                else
                    from=from.substr(0,pos);
            }
            else
            #endif
            from=from.substr(0,pos);
        }
        QString newPath=QString::fromStdString(from);
        if(newPath.size()>(64+3))
            newPath=newPath.mid(0,32)+QStringLiteral("...")+newPath.mid(newPath.size()-32,32);
        ui->from->setText(newPath);
        newPath=QString::fromStdString(transfertItem.to);
        if(newPath.size()>(64+3))
            newPath=newPath.mid(0,32)+QStringLiteral("...")+newPath.mid(newPath.size()-32,32);
        //ui->to->setText(newPath);
        ui->current_file->setText(QString::fromStdString(transfertItem.current_file));
        if(transfertItem.progressBar_read!=-1)
        {
            if(darkUi)
                progressBar_file->setRange(0,65535);
            else
                ui->progressBar_file->setRange(0,65535);
            if(uiOptions->showDualProgression->isChecked())
            {
                if(!darkUi)
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
                }
                if(darkUi)
                    progressBar_file->setValue(transfertItem.progressBar_write);
                else
                    ui->progressBar_file->setValue(transfertItem.progressBar_write);
            }
            else
            {
                if(darkUi)
                    progressBar_file->setValue((transfertItem.progressBar_read+transfertItem.progressBar_write)/2);
                else
                    ui->progressBar_file->setValue((transfertItem.progressBar_read+transfertItem.progressBar_write)/2);
            }
        }
        else
        {
            if(darkUi)
                progressBar_file->setRange(0,0);
            else
                ui->progressBar_file->setRange(0,0);
        }
    }
    else
    {
        ui->from->setText(QStringLiteral(""));
        //ui->to->setText(QStringLiteral(""));
        ui->current_file->setText(QStringLiteral("-"));
        if(haveStarted && transferModel.rowCount()==0)
        {
            if(darkUi)
                progressBar_file->setValue(65535);
            else
                ui->progressBar_file->setValue(65535);
        }
        else if(!haveStarted)
        {
            if(darkUi)
                progressBar_file->setValue(0);
            else
                ui->progressBar_file->setValue(0);
        }
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
    (void)checked;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    /*ui->progressBarCurrentSpeed->setVisible(checked);
    ui->currentSpeed->setVisible(!checked);*/
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
    Q_UNUSED(checked);
    /*if(checked)
        this->setMaximumHeight(16777215);
    else
        this->setMaximumHeight(130);*/
    // usefull under windows
    #if ! defined(__ANDROID__) && ! defined(ANDROID) && ! defined(__ANDROID_API__)
    this->adjustSize();
    #endif
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
        if(event->dropAction()!=Qt::CopyAction)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"event->dropAction()!=Qt::CopyAction ignore");
            //drag'n'drop with shift pressed send the file to trash
            event->ignore();
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"hasUrls");
        std::vector<std::string> urls;
        unsigned int index=0;
        foreach (QUrl url, mimeData->urls())
        {
            //const std::string &urlString=url.path(QUrl::FullyDecoded).toStdString();->genera crash in windows 32Bits after successfull copy
            const std::string &urlString=QUrl::fromPercentEncoding(url.toString().toUtf8()).toStdString();
            if(index<99)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,urlString);
            urls.push_back(urlString);
            index++;
        }
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
        if(event->dropAction()==Qt::CopyAction)
            event->acceptProposedAction();
        else
            event->ignore();//drag'n'drop with shift pressed send the file to trash
    }
}

void Themes::dragMoveEvent(QDragMoveEvent* event)
{
    // if some actions should not be usable, like move, this code must be adopted
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        if(event->dropAction()==Qt::CopyAction)
            event->acceptProposedAction();
        else
            event->ignore();//drag'n'drop with shift pressed send the file to trash
    }
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
    if(!darkUi)
    {
        if(!uiOptions->showDualProgression->isChecked())
        {
            ui->progressBar_all->setStyleSheet(QStringLiteral(""));
            ui->progressBar_file->setStyleSheet(QStringLiteral(""));
            //ui->progressBarCurrentSpeed->setStyleSheet(QStringLiteral(""));
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
            /*ui->progressBarCurrentSpeed->setStyleSheet(QStringLiteral("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                                               .arg(progressColorRemaining.name())
                                               .arg(progressColorWrite.name())
                                               );*/
        }
    }
    if(stat==status_never_started)
        updateCurrentFileInformation();
}

QString Themes::simplifiedBigNum(const uint64_t &num)
{
    if(num<1000)
        return QString::number(num);
    else if(num<1000000)
        return QString::number(num/1000)+QStringLiteral("k");
    else
        return QString::number(num/1000000)+QStringLiteral("M");
}

void Themes::updateTitle()
{
    if(uiOptions->showProgressionInTheTitle->isChecked() && totalSize>0)
    {
        if(!modeIsForced)
            this->setWindowTitle(tr("%1 %2% of %3 into %4 files")
                                .arg(QString::fromStdString(facilityEngine->translateText("Transfer")))
                                .arg((currentSize*100)/totalSize)
                                .arg(QString::fromStdString(facilityEngine->sizeToString(totalSize)))
                                .arg(simplifiedBigNum(totalFile))+
                                 QStringLiteral(" - ")+
                                 QString::fromStdString(facilityEngine->softwareName())
                                 );
        else
        {
            if(mode==Ultracopier::Copy)
                this->setWindowTitle(tr("%1 %2% of %3 into %4 files")
                                     .arg(QString::fromStdString(facilityEngine->translateText("Copy")))
                                     .arg((currentSize*100)/totalSize)
                                     .arg(QString::fromStdString(facilityEngine->sizeToString(totalSize)))
                                     .arg(simplifiedBigNum(totalFile))+
                                      QStringLiteral(" - ")+
                                      QString::fromStdString(facilityEngine->softwareName())
                                     );
            else
                this->setWindowTitle(tr("%1 %2% of %3 into %4 files")
                                     .arg(QString::fromStdString(facilityEngine->translateText("Move")))
                                     .arg((currentSize*100)/totalSize)
                                     .arg(QString::fromStdString(facilityEngine->sizeToString(totalSize)))
                                     .arg(simplifiedBigNum(totalFile))+
                                      QStringLiteral(" - ")+
                                      QString::fromStdString(facilityEngine->softwareName())
                                     );
        }
    }
    else
    {
        if(!modeIsForced)
            this->setWindowTitle(
                    QString::fromStdString(facilityEngine->translateText("Transfer"))+
                                 QStringLiteral(" - ")+
                                 QString::fromStdString(facilityEngine->softwareName())
                        );
        else
        {
            if(mode==Ultracopier::Copy)
                this->setWindowTitle(
                        QString::fromStdString(facilityEngine->translateText("Copy"))+
                                     QStringLiteral(" - ")+
                                     QString::fromStdString(facilityEngine->softwareName())
                        );
            else
                this->setWindowTitle(
                        QString::fromStdString(facilityEngine->translateText("Move"))+
                                     QStringLiteral(" - ")+
                                     QString::fromStdString(facilityEngine->softwareName())
                        );
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

void Themes::resizeEvent(QResizeEvent*)
{
    if(!ui->moreButton->isChecked() && (moreButton==NULL || !moreButton->isChecked()))
    {
        if(width()<height())
        {
            ui->horizontalLayout_3->setDirection(QBoxLayout::TopToBottom);
            ui->frameLeft->setMaximumHeight(height()/3);
            ui->frameLeft->setMaximumWidth(1000000);
            ui->frameMiddle->setMaximumHeight(height()/3);
            ui->frameMiddle->setMaximumWidth(1000000);
            ui->frameRight->setMaximumHeight(height()/3);
            ui->frameRight->setMaximumWidth(1000000);
        }
        else
        {
            ui->horizontalLayout_3->setDirection(QBoxLayout::LeftToRight);
            ui->frameLeft->setMaximumHeight(1000000);
            ui->frameLeft->setMaximumWidth(width()/3);
            ui->frameMiddle->setMaximumHeight(1000000);
            ui->frameMiddle->setMaximumWidth(width()/3);
            ui->frameRight->setMaximumHeight(1000000);
            ui->frameRight->setMaximumWidth(width()/3);
        }
    }
    else {
        ui->frameLeft->setMaximumHeight(1000000);
        ui->frameLeft->setMaximumWidth(1000000);
        ui->frameMiddle->setMaximumHeight(1000000);
        ui->frameMiddle->setMaximumWidth(1000000);
        ui->frameRight->setMaximumHeight(1000000);
        ui->frameRight->setMaximumWidth(1000000);
    }
    if(ui->frameS->width()>300)
    {
        int space=ui->frameS->width()/20;
        ui->horizontalLayoutS->setContentsMargins(space,space/2,space,space/2);
        ui->horizontalLayoutS->setSpacing(space);
    }
    else
    {
        ui->horizontalLayoutS->setMargin(6);
        ui->horizontalLayoutS->setSpacing(6);
    }
}

void Themes::doneTime(const std::vector<std::pair<uint64_t,uint32_t> > &timeList)
{
    if(remainingTimeLogarithmicValue.size()<ULTRACOPIERO2_MAXREMAININGTIMECOL)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"bug, remainingTimeLogarithmicValue.size() "+std::to_string(remainingTimeLogarithmicValue.size())+" <ULTRACOPIERO2_MAXREMAININGTIMECOL");
    else
    {
        unsigned int sub_index=0;
        while(sub_index<timeList.size())
        {
            const std::pair<uint64_t,uint32_t> &timeUnit=timeList.at(sub_index);
            const uint8_t &col=fileCatNumber(timeUnit.first);
            RemainingTimeLogarithmicColumn &remainingTimeLogarithmicColumn=remainingTimeLogarithmicValue[col];
            if(remainingTimeLogarithmicValue.size()<=col)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"bug, remainingTimeLogarithmicValue.size() "+std::to_string(remainingTimeLogarithmicValue.size())+" < col %2"+std::to_string(col));
                break;
            }
            else
            {
                if(timeUnit.second>0)
                {
                    remainingTimeLogarithmicColumn.lastProgressionSpeed.push_back(static_cast<uint64_t>(timeUnit.first/timeUnit.second)*1000);
                    if(remainingTimeLogarithmicColumn.lastProgressionSpeed.size()>ULTRACOPIERO2_MAXVALUESPEEDSTORED)
                        remainingTimeLogarithmicColumn.lastProgressionSpeed.erase(remainingTimeLogarithmicColumn.lastProgressionSpeed.begin());

                }
            }
            sub_index++;
        }
        unsigned int max=1;
        sub_index=0;
        while(sub_index<remainingTimeLogarithmicValue.size() && sub_index<6)
        {
            const RemainingTimeLogarithmicColumn &col=remainingTimeLogarithmicValue.at(sub_index);
            unsigned int tot=0;
            unsigned int index=0;
            while(index<col.lastProgressionSpeed.size())
            {
                tot+=col.lastProgressionSpeed.at(index);
                index++;
            }
            unsigned int res=0;
            if(!col.lastProgressionSpeed.empty())
                res=tot/col.lastProgressionSpeed.size();
            if(max<res)
                max=res;
            sub_index++;
        }
        if(max>1)
            verticalLabel->setText(QString::fromStdString(facilityEngine->speedToString(max)));
        sub_index=0;
        while(sub_index<remainingTimeLogarithmicValue.size() && sub_index<6)
        {
            const RemainingTimeLogarithmicColumn &col=remainingTimeLogarithmicValue.at(sub_index);
            unsigned int tot=0;
            unsigned int index=0;
            while(index<col.lastProgressionSpeed.size())
            {
                tot+=col.lastProgressionSpeed.at(index);
                index++;
            }
            unsigned int res=0;
            if(!col.lastProgressionSpeed.empty())
                res=tot/col.lastProgressionSpeed.size();
            QProgressBar *p=nullptr;
            switch (sub_index) {
            case 0:
                p=ui->progressBar;
                break;
            case 1:
                p=ui->progressBar_2;
                break;
            case 2:
                p=ui->progressBar_3;
                break;
            case 3:
                p=ui->progressBar_4;
                break;
            case 4:
                p=ui->progressBar_5;
                break;
            case 5:
                p=ui->progressBar_6;
                break;
            default:
                break;
            }
            p->setValue(res);
            p->setMaximum(max);
            p->setToolTip(QString::fromStdString(facilityEngine->speedToString(res)));
            sub_index++;
        }
    }
}

/* return 0 to 5 */
uint8_t Themes::fileCatNumber(uint64_t size)
{
    //all is in base 10 to understand more easily
    //drop the big value
    if(size>100*1000*1000)
        size=100*1000*1000;
    size=size/100;//to group all the too small file into the value 0
    const double rlog=round(log10(size));
    if(rlog>5)
        return 5;
    return rlog;
}

void Themes::paintEvent(QPaintEvent * event)
{
    if(darkUi)
    {
        if(background.width()!=width() || background.height()!=height())
        {
            int minimal=height();
            if(width()<height())
                minimal=width();

            QPixmap temp(minimal,minimal);
            QPainter paint;
            paint.begin(&temp);

            QRadialGradient radialGrad(QPointF(minimal/2,minimal/2), minimal/2);
            radialGrad.setColorAt(0, QColor(70, 70, 70));
            radialGrad.setColorAt(1, QColor(40, 40, 40));
            QRect rect_radial(0,0,minimal,minimal);
            paint.fillRect(rect_radial, radialGrad);
            background=temp.scaled(width(),height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        }
        QPainter paint;
        paint.begin(this);
        paint.drawPixmap(0,0,background.width(),    background.height(),    background);
    }
    else
        QWidget::paintEvent(event);
}
