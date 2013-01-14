/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "interface.h"
#include "factory.h"
#include "ui_interface.h"

Themes::Themes(FacilityInterface * facilityEngine) :
    ui(new Ui::interface())
{
    ui->setupUi(this);
    remainingSeconds= 0;
    speed		= 0;
    progression_current=0;
    progression_total=0;
    modeIsForced	= false;
    haveStarted	= false;
    this->facilityEngine	= facilityEngine;
    transferModel.setFacilityEngine(facilityEngine);
    menu=new QMenu(this);
    ui->toolButton->setMenu(menu);
    updateModeAndType();

    connect(ui->actionAddFile,&QAction::triggered,this,&Themes::forcedModeAddFile);
    connect(ui->actionAddFileToCopy,&QAction::triggered,this,&Themes::forcedModeAddFileToCopy);
    connect(ui->actionAddFileToMove,&QAction::triggered,this,&Themes::forcedModeAddFileToMove);
    connect(ui->actionAddFolderToCopy,&QAction::triggered,this,&Themes::forcedModeAddFolderToCopy);
    connect(ui->actionAddFolderToMove,&QAction::triggered,this,&Themes::forcedModeAddFolderToMove);
    connect(ui->actionAddFolder,&QAction::triggered,this,&Themes::forcedModeAddFolder);

    updateDetails();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(&transferModel,&TransferModel::debugInformation,this,&Themes::debugInformation);
    #endif
    #ifndef Q_OS_WIN32
    ui->widget_bottom->setStyleSheet("background-color: rgb(237, 237, 237);");
    #endif
    show();
}

Themes::~Themes()
{
    delete menu;
}

void Themes::forcedModeAddFile()
{
    emit userAddFile(mode);
}

void Themes::forcedModeAddFolder()
{
    emit userAddFolder(mode);
}

void Themes::forcedModeAddFileToCopy()
{
    emit userAddFile(Ultracopier::Copy);
}

void Themes::forcedModeAddFolderToCopy()
{
    emit userAddFolder(Ultracopier::Copy);
}

void Themes::forcedModeAddFileToMove()
{
    emit userAddFile(Ultracopier::Move);
}

void Themes::forcedModeAddFolderToMove()
{
    emit userAddFolder(Ultracopier::Move);
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

void Themes::closeEvent(QCloseEvent *event)
{
    event->ignore();
    this->hide();
    emit cancel();
}

void Themes::detectedSpeed(const quint64 &speed)
{
    this->speed=speed;
    if(ui->more->isChecked())
        ui->label_speed->setText(facilityEngine->speedToString(speed));
}

QWidget * Themes::getOptionsEngineWidget()
{
        return NULL;
}

void Themes::getOptionsEngineEnabled(const bool &isEnabled)
{
        Q_UNUSED(isEnabled)
}

void Themes::setCopyType(const Ultracopier::CopyType &type)
{
    this->type=type;
    updateModeAndType();
}

void Themes::forceCopyMode(const Ultracopier::CopyMode &mode)
{
    modeIsForced=true;
    this->mode=mode;
    updateModeAndType();
    updateInformations();
}

void Themes::updateTitle()
{
    remainingTime(remainingSeconds);
}

void Themes::actionInProgess(const Ultracopier::EngineActionInProgress &action)
{
    this->action=action;
    switch(action)
    {
        case Ultracopier::Copying:
        case Ultracopier::CopyingAndListing:
            ui->progressBar->setMaximum(65535);
            ui->progressBar->setMinimum(0);
        break;
        case Ultracopier::Listing:
            ui->progressBar->setMaximum(0);
            ui->progressBar->setMinimum(0);
        break;
        case Ultracopier::Idle:
            if(haveStarted)
                emit cancel();
        break;
        default:
        break;
    }
    switch(action)
    {
        case Ultracopier::Copying:
        case Ultracopier::CopyingAndListing:
            haveStarted=true;
        break;
        default:
        break;
    }
}

void Themes::newTransferStart(const Ultracopier::ItemOfCopyList &item)
{
    ui->text->setText(item.sourceFullPath);
}

void Themes::newTransferStop(const quint64 &id)
{
    Q_UNUSED(id)
}

void Themes::newFolderListing(const QString &path)
{
    if(action==Ultracopier::Listing)
        ui->text->setText(path);
}

void Themes::remainingTime(const int &remainingSeconds)
{
    this->remainingSeconds=remainingSeconds;

    QString remainingTime;
    if(remainingSeconds>=0)
        remainingTime=facilityEngine->simplifiedRemainingTime(remainingSeconds);
    else
        remainingTime=facilityEngine->translateText(tr("Unknown remaining time"));

    this->setWindowTitle(remainingTime);

    if(ui->more->isChecked())
        ui->label_remaining_time->setText(remainingTime);
    else
        updateInformations();
}

void Themes::newCollisionAction(const QString &action)
{
    Q_UNUSED(action)
}

void Themes::newErrorAction(const QString &action)
{
    Q_UNUSED(action)
}

void Themes::errorDetected()
{
}

void Themes::setTransferListOperation(const Ultracopier::TransferListOperation &transferListOperation)
{
    Q_UNUSED(transferListOperation)
}

//speed limitation
bool Themes::setSpeedLimitation(const qint64 &speedLimitation)
{
    Q_UNUSED(speedLimitation)
        return false;
}

//get information about the copy
void Themes::setGeneralProgression(const quint64 &current,const quint64 &total)
{
    progression_current=current;
    progression_total=total;
    ui->progressBar->setValue(((double)current/total)*65535);
}

void Themes::setCollisionAction(const QList<QPair<QString,QString> > &list)
{
    Q_UNUSED(list)
}

void Themes::setErrorAction(const QList<QPair<QString,QString> > &list)
{
    Q_UNUSED(list)
}

//edit the transfer list
void Themes::getActionOnList(const QList<Ultracopier::ReturnActionOnCopyList> &returnActions)
{
    transferModel.synchronizeItems(returnActions);
    updateInformations();
}

void Themes::haveExternalOrder()
{
    ui->toolButton->hide();
}

void Themes::isInPause(const bool &isInPause)
{
    //resume in auto the pause
    if(isInPause)
        emit resume();
}

void Themes::newLanguageLoaded()
{
    ui->retranslateUi(this);
    updateTitle();
    updateInformations();
}

void Themes::setFileProgression(const QList<Ultracopier::ProgressionItem> &progressionList)
{
    QList<Ultracopier::ProgressionItem> progressionListBis=progressionList;
    transferModel.setFileProgression(progressionListBis);
    updateInformations();
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


void Themes::on_more_toggled(bool checked)
{
    Q_UNUSED(checked);
    updateDetails();
    updateInformations();
}

void Themes::updateDetails()
{
    ui->text->setHidden(ui->more->isChecked());
    ui->details->setHidden(!ui->more->isChecked());
    if(ui->more->isChecked())
    {
        this->setMinimumHeight(242);
        this->setMaximumHeight(242);
        ui->more->setIcon(QIcon(":/resources/arrow-up.png"));
    }
    else
    {
        this->setMinimumHeight(168);
        this->setMaximumHeight(168);
        ui->more->setIcon(QIcon(":/resources/arrow-down.png"));
    }

    // usefull under windows
    this->updateGeometry();
    this->update();
    this->adjustSize();

    updateInformations();
}

void Themes::updateInformations()
{
    TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
    if(!modeIsForced)
    {
        if(transferModel.totalFile>1)
            ui->label_main->setText(tr("Transferring %n item(s) (%2)","",transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
        else
            ui->label_main->setText(tr("Transferring %n item(s) (%2)","",transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
    }
    else
    {
        if(mode==Ultracopier::Copy)
        {
            if(transferModel.totalFile>1)
                ui->label_main->setText(tr("Copying %n item(s) (%2)","",transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
            else
                ui->label_main->setText(tr("Copying %n item(s) (%2)","",transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
        }
        else
        {
            if(transferModel.totalFile>1)
                ui->label_main->setText(tr("Moving %n item(s) (%2)","",transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
            else
                ui->label_main->setText(tr("Moving %n item(s) (%2)","",transferModel.totalFile).arg(facilityEngine->sizeToString(progression_total)));
        }
    }

    if(transfertItem.haveItem)
    {
        if(transfertItem.progressBar_file!=-1)
        {
            ui->progressBar->setRange(0,65535);
            ui->progressBar->setValue(transfertItem.progressBar_file);
        }
        else
            ui->progressBar->setRange(0,0);
    }
    else
    {
        if(haveStarted && transferModel.rowCount()==0)
            ui->progressBar->setValue(65535);
        else if(!haveStarted)
            ui->progressBar->setValue(0);
    }

    if(ui->more->isChecked())
    {
        if(transfertItem.haveItem)
        {
            QString simplifiedFrom=transfertItem.from;
            QString simplifiedTo=transfertItem.to;
            simplifiedFrom.remove(Factory::slashEnd);
            simplifiedTo.remove(Factory::slashEnd);
            simplifiedFrom.replace('\\','/');
            simplifiedTo.replace('\\','/');
            simplifiedFrom.replace(Factory::isolateName, "\\1");
            simplifiedTo.replace(Factory::isolateName, "\\1");
            ui->label_file->setText(transfertItem.current_file);
            ui->label_from->setText(QString("<b>%1</b> (%2)").arg(simplifiedFrom).arg(transfertItem.from));
            ui->label_to->setText(QString("<b>%1</b> (%2)").arg(simplifiedTo).arg(transfertItem.to));
            ui->label_items->setText(QString("%1 (%2)").arg(transferModel.totalFile-transferModel.currentFile).arg(facilityEngine->sizeToString(progression_total-progression_current)));
        }
        else
        {
            ui->label_file->setText("");
            ui->label_from->setText("");
            ui->label_to->setText("");
            ui->label_items->setText(QString("%1 (%2)").arg(transferModel.totalFile-transferModel.currentFile).arg(facilityEngine->sizeToString(progression_total-progression_current)));
        }
    }
    else
    {
        if(transfertItem.haveItem)
        {
            QString remainingTime;
            if(remainingSeconds>=0)
                remainingTime=facilityEngine->simplifiedRemainingTime(remainingSeconds);
            else
                remainingTime=facilityEngine->translateText(tr("Unknown remaining time"));
            QString simplifiedFrom=transfertItem.from;
            QString simplifiedTo=transfertItem.to;
            simplifiedFrom.remove(Factory::slashEnd);
            simplifiedTo.remove(Factory::slashEnd);
            simplifiedFrom.replace('\\','/');
            simplifiedTo.replace('\\','/');
            simplifiedFrom.replace(Factory::isolateName, "\\1");
            simplifiedTo.replace(Factory::isolateName, "\\1");
            ui->text->setText(
			//: Sample: from <b>sources</b> (e:\folder\source) to <b>destination</b> (d:\desktop\destination)<br />About 5 Hours remaining
			tr("from <b>%1</b> (%2) to <b>%3</b> (%4)<br />%5")
                      .arg(simplifiedFrom)
                      .arg(transfertItem.from)
                      .arg(simplifiedTo)
                      .arg(transfertItem.to)
                      .arg(remainingTime)
                      );
        }
        else
            ui->text->setText(tr("In waiting"));
    }
}
