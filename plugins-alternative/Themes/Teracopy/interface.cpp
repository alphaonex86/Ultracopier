/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QMessageBox>
#include <math.h>

#include "interface.h"
#include "ui_interface.h"

Themes::Themes(FacilityInterface * facilityEngine) :
    ui(new Ui::interfaceCopy())
{
    this->facilityEngine=facilityEngine;
    ui->setupUi(this);
    ui->TransferList->setModel(&transferModel);
    transferModel.setFacilityEngine(facilityEngine);

    currentFile		= 0;
    totalFile		= 0;
    currentSize		= 0;
    totalSize		= 0;
    storeIsInPause		= false;
    modeIsForced		= false;
    haveStarted		= false;
    speedString		= facilityEngine->speedToString(0);
    ui->toolButtonMenu->setMenu(&menu);

    connect(ui->actionAddFile,&QAction::triggered,this,&Themes::forcedModeAddFile);
    connect(ui->actionAddFileToCopy,&QAction::triggered,this,&Themes::forcedModeAddFileToCopy);
    connect(ui->actionAddFileToMove,&QAction::triggered,this,&Themes::forcedModeAddFileToMove);
    connect(ui->actionAddFolderToCopy,&QAction::triggered,this,&Themes::forcedModeAddFolderToCopy);
    connect(ui->actionAddFolderToMove,&QAction::triggered,this,&Themes::forcedModeAddFolderToMove);
    connect(ui->actionAddFolder,&QAction::triggered,this,&Themes::forcedModeAddFolder);

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(&transferModel,&TransferModel::debugInformation,this,&Themes::debugInformation);
    #endif

    progressColorWrite=QApplication::palette().color(QPalette::Highlight);
    progressColorRead=QApplication::palette().color(QPalette::AlternateBase);
    progressColorRemaining=QApplication::palette().color(QPalette::Base);

    ui->progressBar_all->setStyleSheet(QString("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                                       .arg(progressColorRemaining.name())
                                       .arg(progressColorWrite.name())
                                       );
    ui->progressBar_file->setStyleSheet(QString("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                                        .arg(progressColorRemaining.name())
                                        .arg(progressColorWrite.name())
                                        );
    show();
    isInPause(false);
}

Themes::~Themes()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
}

QWidget * Themes::getOptionsEngineWidget()
{
    return &optionEngineWidget;
}

void Themes::getOptionsEngineEnabled(const bool &isEnabled)
{
    Q_UNUSED(isEnabled);
}

/// \brief set if transfer list is exportable/importable
void Themes::setTransferListOperation(const Ultracopier::TransferListOperation &transferListOperation)
{
    Q_UNUSED(transferListOperation);
}

void Themes::closeEvent(QCloseEvent *event)
{
    event->ignore();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->hide();
    emit cancel();
}

void Themes::updateOverallInformation()
{
    ui->overall->setText(tr("Total: %1 of %2").arg(facilityEngine->sizeToString(currentSize)).arg(facilityEngine->sizeToString(totalSize)));
    ui->labelNumberFile->setText(tr("%1 of %2").arg(currentFile).arg(totalFile));
}

void Themes::actionInProgess(const Ultracopier::EngineActionInProgress &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"start: "+QString::number(action));
    this->action=action;
    ui->pauseButton->setEnabled(action!=Ultracopier::Idle);
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
            if(haveStarted)
                emit cancel();
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Very wrong switch case!");
        break;
    }
    switch(action)
    {
        case Ultracopier::Copying:
        case Ultracopier::CopyingAndListing:
            haveStarted=true;
            ui->cancelButton->setText(facilityEngine->translateText("Quit"));
            if(storeIsInPause)
                ui->pauseButton->setText(facilityEngine->translateText("Start"));
            else
                ui->pauseButton->setText(facilityEngine->translateText("Pause"));
        break;
        case Ultracopier::Idle:
            ui->cancelButton->setText(facilityEngine->translateText("Quit"));
        break;
        default:
        break;
    }
}

void Themes::newFolderListing(const QString &path)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(action==Ultracopier::Listing)
        ui->from->setText(path);
}

void Themes::detectedSpeed(const quint64 &speed)//in byte per seconds
{
    speedString=facilityEngine->speedToString(speed);
}

/** \brief support speed limitation */
void Themes::setSupportSpeedLimitation(const bool &supportSpeedLimitationBool)
{
    Q_UNUSED(supportSpeedLimitationBool);
}

void Themes::remainingTime(const int &remainingSeconds)
{
    if(remainingSeconds==-1)
        ui->labelTimeRemaining->setText("<html><body>&#8734;</body></html>");
    else
    {
        Ultracopier::TimeDecomposition time=facilityEngine->secondsToTimeDecomposition(remainingSeconds);
        ui->labelTimeRemaining->setText(QString::number(time.hour)+":"+QString::number(time.minute)+":"+QString::number(time.second));
    }
}

void Themes::newCollisionAction(const QString &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    Q_UNUSED(action);
}

void Themes::newErrorAction(const QString &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    Q_UNUSED(action);
}

void Themes::errorDetected()
{
}

//speed limitation
bool Themes::setSpeedLimitation(const qint64 &speedLimitation)
{
    if(speedLimitation>0)
        emit newSpeedLimitation(0);
    return true;
}

//get information about the copy
void Themes::setGeneralProgression(const quint64 &current,const quint64 &total)
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
}

void Themes::setCollisionAction(const QList<QPair<QString,QString> > &list)
{
    Q_UNUSED(list);
}

void Themes::setErrorAction(const QList<QPair<QString,QString> > &list)
{
    Q_UNUSED(list);
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
        this->setWindowTitle("Ultracopier - "+facilityEngine->translateText("Copy"));
    else
        this->setWindowTitle("Ultracopier - "+facilityEngine->translateText("Move"));
    updateModeAndType();
}

void Themes::updateTitle()
{
    QString startString;
    if(action==Ultracopier::Copying || action==Ultracopier::CopyingAndListing)
        startString=tr("%1% done").arg(((double)currentSize/totalSize)*100);
    else
        startString="Ultracopier";
    startString+=" - ";
    if(mode==Ultracopier::Copy)
        this->setWindowTitle(startString+facilityEngine->translateText("Copy")+" ("+speedString+")");
    else
        this->setWindowTitle(startString+facilityEngine->translateText("Move")+" ("+speedString+")");
}

void Themes::haveExternalOrder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
//	ui->moreButton->toggle();
}

void Themes::isInPause(const bool &isInPause)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"isInPause: "+QString::number(isInPause));
    //resume in auto the pause
    storeIsInPause=isInPause;
    if(isInPause)
        ui->pauseButton->setText(facilityEngine->translateText("Resume"));
    else
        ui->pauseButton->setText(facilityEngine->translateText("Pause"));
}

void Themes::updateCurrentFileInformation()
{
    TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
    if(transfertItem.haveItem)
    {
        ui->from->setText(transfertItem.from);
        //commented because not displayed on this interface
        //ui->to->setText(transfertItem.to);
        //ui->current_file->setText(transfertItem.current_file);
        if(transfertItem.progressBar_read!=-1)
        {
            ui->progressBar_file->setRange(0,65535);
            if(transfertItem.progressBar_read!=transfertItem.progressBar_write)
            {
                float permilleread=round((float)transfertItem.progressBar_read/65535*1000)/1000;
                float permillewrite=permilleread-0.001;
                ui->progressBar_file->setStyleSheet(QString("QProgressBar{border: 1px solid grey;text-align: center;background-color: qlineargradient(spread:pad, x1:%1, y1:0, x2:%2, y2:0, stop:0 %3, stop:1 %4);}QProgressBar::chunk{background-color:%5;}")
                    .arg(permilleread)
                    .arg(permillewrite)
                    .arg(progressColorRemaining.name())
                    .arg(progressColorRead.name())
                    .arg(progressColorWrite.name())
                    );
            }
            else
                ui->progressBar_file->setStyleSheet(QString("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                    .arg(progressColorRemaining.name())
                    .arg(progressColorWrite.name())
                    );
            ui->progressBar_file->setValue(transfertItem.progressBar_write);
        }
        else
            ui->progressBar_file->setRange(0,0);
    }
    else
    {
        ui->from->setText("");
        //commented because not displayed on this interface
        //ui->to->setText("");
        //ui->current_file->setText("-");
        if(haveStarted && transferModel.rowCount()==0)
            ui->progressBar_file->setValue(65535);
        else if(!haveStarted)
            ui->progressBar_file->setValue(0);
    }
}


void Themes::on_cancelButton_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->hide();
    emit cancel();
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("skip at running: %1").arg(transfertItem.id));
        emit skip(transfertItem.id);
    }
    else
    {
        if(transferModel.rowCount()>1)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("skip at idle: %1").arg(transferModel.firstId()));
            emit skip(transferModel.firstId());
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to skip the transfer, because no transfer running");
    }
}

void Themes::updateModeAndType()
{
    menu.clear();
    if(modeIsForced)
    {
        menu.addAction(ui->actionAddFile);
        if(type==Ultracopier::FileAndFolder)
            menu.addAction(ui->actionAddFolder);
    }
    else
    {
        menu.addAction(ui->actionAddFileToCopy);
        menu.addAction(ui->actionAddFileToMove);
        if(type==Ultracopier::FileAndFolder)
        {
            menu.addAction(ui->actionAddFolderToCopy);
            menu.addAction(ui->actionAddFolderToMove);
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

//set the translate
void Themes::newLanguageLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(modeIsForced)
        forceCopyMode(mode);
    ui->retranslateUi(this);
    if(haveStarted)
        updateCurrentFileInformation();
    updateOverallInformation();
}

/*
  Return[0]: totalFile
  Return[1]: totalSize
  Return[2]: currentFile
  */
void Themes::getActionOnList(const QList<Ultracopier::ReturnActionOnCopyList>& returnActions)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, returnActions.size(): "+QString::number(returnActions.size()));
    QList<quint64> returnValue=transferModel.synchronizeItems(returnActions);
    totalFile+=returnValue[0];
    totalSize+=returnValue[1];
    currentFile+=returnValue[2];
    if(transferModel.rowCount()==0)
    {
        ui->skipButton->setEnabled(false);
        ui->progressBar_all->setValue(65535);
        ui->progressBar_file->setValue(65535);
        currentSize=totalSize;
    }
    else
        ui->skipButton->setEnabled(true);
    updateOverallInformation();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"transferModel.rowCount(): "+QString::number(transferModel.rowCount()));
}

void Themes::setFileProgression(const QList<Ultracopier::ProgressionItem> &progressionList)
{
    QList<Ultracopier::ProgressionItem> progressionListBis=progressionList;
    transferModel.setFileProgression(progressionListBis);
    updateCurrentFileInformation();
}

Themes::currentTransfertItem Themes::getCurrentTransfertItem()
{
    currentTransfertItem returnItem;
    returnItem.haveItem=InternalRunningOperation.size()>0;
    if(returnItem.haveItem)
    {
        const ItemOfCopyListWithMoreInformations &itemTransfer=InternalRunningOperation.first();
        returnItem.from=itemTransfer.generalData.sourceFullPath;
        returnItem.to=itemTransfer.generalData.destinationFullPath;
        returnItem.current_file=itemTransfer.generalData.destinationFileName+", "+facilityEngine->sizeToString(itemTransfer.generalData.size);
        switch(itemTransfer.actionType)
        {
            case Ultracopier::CustomOperation:
            if(!itemTransfer.custom_with_progression)
                returnItem.progressBar_file=0;
            else
            {
                if(itemTransfer.generalData.size>0)
                    returnItem.progressBar_file=((double)itemTransfer.currentProgression/itemTransfer.generalData.size)*65535;
                else
                    returnItem.progressBar_file=0;
            }
            break;
            case Ultracopier::Transfer:
            if(itemTransfer.generalData.size>0)
                returnItem.progressBar_file=((double)itemTransfer.currentProgression/itemTransfer.generalData.size)*65535;
            else
                returnItem.progressBar_file=0;
            break;
            case Ultracopier::PostOperation:
                returnItem.progressBar_file=65535;
            break;
            default:
                returnItem.progressBar_file=0;
        }
    }
    return returnItem;
}
