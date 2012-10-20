/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QMessageBox>
#include <QMimeData>

#include "interface.h"
#include "ui_interface.h"

Themes::Themes(bool checkBoxShowSpeed,FacilityInterface * facilityEngine,bool moreButtonPushed) :
    ui(new Ui::interfaceCopy())
{
    this->facilityEngine=facilityEngine;
    ui->setupUi(this);
    ui->TransferList->setModel(&transferModel);
    transferModel.setFacilityEngine(facilityEngine);
    ui->tabWidget->setCurrentIndex(0);
    ui->checkBoxShowSpeed->setChecked(checkBoxShowSpeed);
    currentFile	= 0;
    totalFile	= 0;
    currentSize	= 0;
    totalSize	= 0;
    haveError	= false;
    this->show();
    menu=new QMenu(this);
    ui->add->setMenu(menu);
    on_checkBoxShowSpeed_toggled(ui->checkBoxShowSpeed->isChecked());
    currentSpeed	= 0;
    storeIsInPause	= false;
    isInPause(false);
    modeIsForced	= false;
    haveStarted	= false;
    connect(ui->limitSpeed,		static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,	&Themes::uiUpdateSpeed);
    connect(ui->checkBox_limitSpeed,&QAbstractButton::toggled,		this,	&Themes::uiUpdateSpeed);

    connect(ui->actionAddFile,&QAction::triggered,this,&Themes::forcedModeAddFile);
    connect(ui->actionAddFileToCopy,&QAction::triggered,this,&Themes::forcedModeAddFileToCopy);
    connect(ui->actionAddFileToMove,&QAction::triggered,this,&Themes::forcedModeAddFileToMove);
    connect(ui->actionAddFolderToCopy,&QAction::triggered,this,&Themes::forcedModeAddFolderToCopy);
    connect(ui->actionAddFolderToMove,&QAction::triggered,this,&Themes::forcedModeAddFolderToMove);
    connect(ui->actionAddFolder,&QAction::triggered,this,&Themes::forcedModeAddFolder);

    //setup the search part
    closeTheSearchBox();
    TimerForSearch  = new QTimer(this);
    TimerForSearch->setInterval(500);
    TimerForSearch->setSingleShot(true);
    searchShortcut  = new QShortcut(QKeySequence("Ctrl+F"),this);
    searchShortcut2 = new QShortcut(QKeySequence("F3"),this);
    searchShortcut3 = new QShortcut(QKeySequence("Escape"),this);//Qt::Key_Escape

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
    on_moreButton_toggled(false);

    /// \note important for drag and drop, \see dropEvent()
    setAcceptDrops(true);

    // try set the OS icon
    QIcon tempIcon;

    tempIcon=QIcon::fromTheme("application-exit");
    if(!tempIcon.isNull())
    {
        ui->cancelButton->setIcon(tempIcon);
        ui->pushButtonCloseSearch->setIcon(tempIcon);
        ui->shutdown->setIcon(tempIcon);
    }

    tempIcon=QIcon::fromTheme("edit-delete");
    if(!tempIcon.isNull())
        ui->del->setIcon(tempIcon);

    tempIcon=QIcon::fromTheme("media-playback-pause");
    if(!tempIcon.isNull())
    {
        player_pause=tempIcon;
        ui->pauseButton->setIcon(tempIcon);
    }
    else
        player_pause=QIcon(":/resources/player_pause.png");

    tempIcon=QIcon::fromTheme("media-playback-play");
    if(!tempIcon.isNull())
        player_play=tempIcon;
    else
        player_play=QIcon(":/resources/player_play.png");

    tempIcon=QIcon::fromTheme("media-skip-forward");
    if(!tempIcon.isNull())
        ui->skipButton->setIcon(tempIcon);

    tempIcon=QIcon::fromTheme("edit-find");
    if(!tempIcon.isNull())
        ui->searchButton->setIcon(tempIcon);

    tempIcon=QIcon::fromTheme("document-open");
    if(!tempIcon.isNull())
        ui->importTransferList->setIcon(tempIcon);

    tempIcon=QIcon::fromTheme("document-save");
    if(!tempIcon.isNull())
        ui->exportTransferList->setIcon(tempIcon);

    tempIcon=QIcon::fromTheme("list-add");
    if(!tempIcon.isNull())
    {
        ui->add->setIcon(tempIcon);
        ui->actionAddFile->setIcon(tempIcon);
        ui->actionAddFileToCopy->setIcon(tempIcon);
        ui->actionAddFileToMove->setIcon(tempIcon);
        ui->actionAddFolder->setIcon(tempIcon);
        ui->actionAddFolderToCopy->setIcon(tempIcon);
        ui->actionAddFolderToMove->setIcon(tempIcon);
    }

    shutdown=facilityEngine->haveFunctionality("shutdown");
    ui->shutdown->setVisible(shutdown);

    selectionModel=ui->TransferList->selectionModel();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(&transferModel,&TransferModel::debugInformation,this,&Themes::debugInformation);
    #endif

    updateSpeed();
}

Themes::~Themes()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //disconnect(ui->actionAddFile);
    //disconnect(ui->actionAddFolder);
    delete selectionModel;
    delete menu;
}

void Themes::uiUpdateSpeed()
{
    if(ui->checkBoxShowSpeed->isChecked())
        return;
    if(!ui->checkBox_limitSpeed->isChecked())
        currentSpeed=0;
    else
        currentSpeed=ui->limitSpeed->value();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("emit newSpeedLimitation(%1)").arg(currentSpeed));
    emit newSpeedLimitation(currentSpeed);
}

QWidget * Themes::getOptionsEngineWidget()
{
    return &optionEngineWidget;
}

void Themes::getOptionsEngineEnabled(const bool &isEnabled)
{
    if(isEnabled)
        ui->tabWidget->addTab(&optionEngineWidget,facilityEngine->translateText("Copy engine"));
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
    ui->overall->setText(tr("File %1/%2, size: %3/%4").arg(currentFile).arg(totalFile).arg(facilityEngine->sizeToString(currentSize)).arg(facilityEngine->sizeToString(totalSize)));
}

void Themes::actionInProgess(const Ultracopier::EngineActionInProgress &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"start: "+QString::number(action));
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
            if(haveStarted)
            {
                if(shutdown && ui->shutdown->isChecked())
                {
                    facilityEngine->callFunctionality("shutdown");
                    return;
                }
                switch(ui->comboBox_copyEnd->currentIndex())
                {
                    case 2:
                        emit cancel();
                    break;
                    case 0:
                        if(!haveError)
                            emit cancel();
                    break;
                    default:
                    break;
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
            haveStarted=true;
            ui->cancelButton->setText(facilityEngine->translateText("Quit"));
            if(storeIsInPause)
                ui->pauseButton->setText(facilityEngine->translateText("Start"));
            else
                ui->pauseButton->setText(facilityEngine->translateText("Pause"));
        break;
        case Ultracopier::Listing:
            ui->pauseButton->setEnabled(false);
        break;
        case Ultracopier::Idle:
            ui->pauseButton->setEnabled(false);
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
    ui->currentSpeed->setText(facilityEngine->speedToString(speed));
}

void Themes::remainingTime(const int &remainingSeconds)
{
    QString labelTimeRemaining("<html><body style=\"white-space:nowrap;\">"+facilityEngine->translateText("Time remaining:")+" ");
    if(remainingSeconds==-1)
        labelTimeRemaining+="&#8734;";
    else
    {
        Ultracopier::TimeDecomposition time=facilityEngine->secondsToTimeDecomposition(remainingSeconds);
        labelTimeRemaining+=QString::number(time.hour)+":"+QString::number(time.minute)+":"+QString::number(time.second);
    }
    labelTimeRemaining+="</body></html>";
    ui->labelTimeRemaining->setText(labelTimeRemaining);
}

void Themes::newCollisionAction(const QString &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(ui->comboBox_fileCollisions->findData(action)!=-1)
        ui->comboBox_fileCollisions->setCurrentIndex(ui->comboBox_fileCollisions->findData(action));
}

void Themes::newErrorAction(const QString &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(ui->comboBox_copyErrors->findData(action)!=-1)
        ui->comboBox_copyErrors->setCurrentIndex(ui->comboBox_copyErrors->findData(action));
}

void Themes::errorDetected()
{
    haveError=true;
}

//speed limitation
bool Themes::setSpeedLimitation(const qint64 &speedLimitation)
{
    currentSpeed=speedLimitation;
    updateSpeed();
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

void Themes::setFileProgression(const QList<Ultracopier::ProgressionItem> &progressionList)
{
    QList<Ultracopier::ProgressionItem> progressionListBis=progressionList;
    transferModel.setFileProgression(progressionListBis);
    updateCurrentFileInformation();
}

void Themes::setCollisionAction(const QList<QPair<QString,QString> > &list)
{
    ui->comboBox_fileCollisions->clear();
    index=0;
    loop_size=list.size();
    while(index<loop_size)
    {
        ui->comboBox_fileCollisions->addItem(list.at(index).first,list.at(index).second);
        index++;
    }
}

void Themes::setErrorAction(const QList<QPair<QString,QString> > &list)
{
    ui->comboBox_fileCollisions->clear();
    index=0;
    loop_size=list.size();
    while(index<loop_size)
    {
        ui->comboBox_copyErrors->addItem(list.at(index).first,list.at(index).second);
        index++;
    }
}

//edit the transfer list
/// \todo check and re-enable to selection
void Themes::getActionOnList(const QList<Ultracopier::ReturnActionOnCopyList> &returnActions)
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"isInPause: "+QString::number(isInPause));
    //resume in auto the pause
    storeIsInPause=isInPause;
    if(isInPause)
    {
        ui->pauseButton->setIcon(player_play);
        ui->pauseButton->setText(facilityEngine->translateText("Resume"));
    }
    else
    {
        ui->pauseButton->setIcon(player_pause);
        if(haveStarted)
            ui->pauseButton->setText(facilityEngine->translateText("Pause"));
        else
            ui->pauseButton->setText(facilityEngine->translateText("Start"));
    }
}

void Themes::updateCurrentFileInformation()
{
    TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
    if(transfertItem.haveItem)
    {
        ui->from->setText(transfertItem.from);
        ui->to->setText(transfertItem.to);
        ui->current_file->setText(transfertItem.current_file);
        if(transfertItem.progressBar_file!=-1)
        {
            ui->progressBar_file->setRange(0,65535);
            ui->progressBar_file->setValue(transfertItem.progressBar_file);
        }
        else
            ui->progressBar_file->setRange(0,0);
    }
    else
    {
        ui->from->setText("");
        ui->to->setText("");
        ui->current_file->setText("-");
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
    ids.clear();
    index=0;
    loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
        index++;
    }
    if(ids.size()>0)
        emit moveItemsOnTop(ids);
}

void Themes::on_pushUp_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    ids.clear();
    index=0;
    loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
        index++;
    }
    if(ids.size()>0)
        emit moveItemsUp(ids);
}

void Themes::on_pushDown_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    ids.clear();
    index=0;
    loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
        index++;
    }
    if(ids.size()>0)
        emit moveItemsDown(ids);
}

void Themes::on_putOnBottom_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    ids.clear();
    index=0;
    loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
        index++;
    }
    if(ids.size()>0)
        emit moveItemsOnBottom(ids);
}

void Themes::on_del_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    ids.clear();
    index=0;
    loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
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

void Themes::on_checkBoxShowSpeed_toggled(bool checked)
{
    Q_UNUSED(checked);
    updateSpeed();
}

void Themes::on_SliderSpeed_valueChanged(int value)
{
    if(!ui->checkBoxShowSpeed->isChecked())
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("value: %1").arg(value));
    emit newSpeedLimitation(currentSpeed);
    updateSpeed();
}

void Themes::updateSpeed()
{
    ui->groupBoxSpeedLimit->setVisible(!ui->checkBoxShowSpeed->isChecked());
    ui->label_Slider_speed->setVisible(ui->checkBoxShowSpeed->isChecked());
    ui->SliderSpeed->setVisible(ui->checkBoxShowSpeed->isChecked());
    ui->label_SpeedMaxValue->setVisible(ui->checkBoxShowSpeed->isChecked());

    if(ui->checkBoxShowSpeed->isChecked())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"checked");
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
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(1);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*1));
        }
        else if(currentSpeed<=1024*4)
        {
            if(currentSpeed!=1024*4)
            {
                currentSpeed=1024*4;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(2);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*4));
        }
        else if(currentSpeed<=1024*16)
        {
            if(currentSpeed!=1024*16)
            {
                currentSpeed=1024*16;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(3);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*16));
        }
        else if(currentSpeed<=1024*64)
        {
            if(currentSpeed!=1024*64)
            {
                currentSpeed=1024*64;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(4);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*64));
        }
        else
        {
            if(currentSpeed!=1024*128)
            {
                currentSpeed=1024*128;
                emit newSpeedLimitation(currentSpeed);
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
    if(!haveStarted)
        ui->current_file->setText(tr("File Name, 0KB"));
    else
        updateCurrentFileInformation();
    updateOverallInformation();
    updateSpeed();
    ui->tabWidget->setTabText(4,facilityEngine->translateText("Copy engine"));
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
    int result=transferModel.search(ui->lineEditSearch->text(),searchNext);
    if(ui->lineEditSearch->text().isEmpty())
        ui->lineEditSearch->setStyleSheet("");
    else
    {
        if(result==-1)
            ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
        else
        {
            ui->lineEditSearch->setStyleSheet("background-color: rgb(193,255,176);");
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
    int result=transferModel.searchPrev(ui->lineEditSearch->text());
    if(ui->lineEditSearch->text().isEmpty())
        ui->lineEditSearch->setStyleSheet("");
    else
    {
        if(result==-1)
            ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
        else
        {
            ui->lineEditSearch->setStyleSheet("background-color: rgb(193,255,176);");
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

void Themes::on_comboBox_copyErrors_currentIndexChanged(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit sendErrorAction(ui->comboBox_copyErrors->itemData(index).toString());
}

void Themes::on_comboBox_fileCollisions_currentIndexChanged(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit sendCollisionAction(ui->comboBox_fileCollisions->itemData(index).toString());
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
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        emit urlDropped(mimeData->urls());
        event->acceptProposedAction();
    }
}

void Themes::dragEnterEvent(QDragEnterEvent* event)
{
    // if some actions should not be usable, like move, this code must be adopted
    event->acceptProposedAction();
}

void Themes::dragMoveEvent(QDragMoveEvent* event)
{
    // if some actions should not be usable, like move, this code must be adopted
    event->acceptProposedAction();
}

void Themes::dragLeaveEvent(QDragLeaveEvent* event)
{
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
