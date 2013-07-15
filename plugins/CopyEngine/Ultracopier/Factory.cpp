/** \file factory.cpp
\brief Define the factory to create new instance
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QFileDialog>
#include <QList>
#include <QDebug>
#include <math.h>

#include "Factory.h"

CopyEngineFactory::CopyEngineFactory() :
    ui(new Ui::copyEngineOptions())
{
    qRegisterMetaType<FolderExistsAction>("FolderExistsAction");
    qRegisterMetaType<FileExistsAction>("FileExistsAction");
    qRegisterMetaType<QList<Filters_rules> >("QList<Filters_rules>");
    qRegisterMetaType<TransferStat>("TransferStat");
    qRegisterMetaType<QList<QStorageInfo::DriveType> >("QList<QStorageInfo::DriveType>");
    qRegisterMetaType<TransferAlgorithm>("TransferAlgorithm");
    qRegisterMetaType<ActionType>("ActionType");
    qRegisterMetaType<ErrorType>("ErrorType");
    qRegisterMetaType<Diskspace>("Diskspace");
    qRegisterMetaType<QList<Diskspace> >("QList<Diskspace>");

    tempWidget=new QWidget();
    ui->setupUi(tempWidget);
    ui->toolBox->setCurrentIndex(0);
    ui->blockSize->setMaximum(ULTRACOPIER_PLUGIN_MAX_BLOCK_SIZE);
    errorFound=false;
    optionsEngine=NULL;
    filters=new Filters(tempWidget);
    renamingRules=new RenamingRules(tempWidget);

    connect(&storageInfo,&QStorageInfo::logicalDriveChanged,this,&CopyEngineFactory::logicalDriveChanged);

    connect(ui->doRightTransfer,            &QCheckBox::toggled,                                            this,&CopyEngineFactory::setDoRightTransfer);
    connect(ui->keepDate,                   &QCheckBox::toggled,                                            this,&CopyEngineFactory::setKeepDate);
    connect(ui->blockSize,                  static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&CopyEngineFactory::setBlockSize);
    connect(ui->sequentialBuffer,           static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&CopyEngineFactory::setSequentialBuffer);
    connect(ui->parallelBuffer,             static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&CopyEngineFactory::setParallelBuffer);
    connect(ui->parallelizeIfSmallerThan,	static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&CopyEngineFactory::setParallelizeIfSmallerThan);
    connect(ui->inodeThreads,               static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&CopyEngineFactory::on_inodeThreads_editingFinished);
    connect(ui->autoStart,                  &QCheckBox::toggled,                                            this,&CopyEngineFactory::setAutoStart);
    connect(ui->doChecksum,                 &QCheckBox::toggled,                                            this,&CopyEngineFactory::doChecksum_toggled);
    connect(ui->comboBoxFolderError,        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngineFactory::setFolderError);
    connect(ui->comboBoxFolderCollision,	static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngineFactory::setFolderCollision);
    connect(ui->comboBoxFileError,          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngineFactory::setFileError);
    connect(ui->comboBoxFileCollision,      static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngineFactory::setFileCollision);
    connect(ui->transferAlgorithm,          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngineFactory::setTransferAlgorithm);
    connect(ui->checkBoxDestinationFolderExists,	&QCheckBox::toggled,		this,&CopyEngineFactory::setCheckDestinationFolder);
    connect(ui->checksumIgnoreIfImpossible,	&QCheckBox::toggled,                this,&CopyEngineFactory::checksumIgnoreIfImpossible_toggled);
    connect(ui->checksumOnlyOnError,        &QCheckBox::toggled,                this,&CopyEngineFactory::checksumOnlyOnError_toggled);
    connect(ui->osBuffer,                   &QCheckBox::toggled,                this,&CopyEngineFactory::osBuffer_toggled);
    connect(ui->osBufferLimited,            &QCheckBox::toggled,                this,&CopyEngineFactory::osBufferLimited_toggled);
    connect(ui->osBufferLimit,              &QSpinBox::editingFinished,         this,&CopyEngineFactory::osBufferLimit_editingFinished);
    connect(ui->inodeThreads,               &QSpinBox::editingFinished,         this,&CopyEngineFactory::on_inodeThreads_editingFinished);
    connect(ui->osBufferLimited,            &QAbstractButton::toggled,          this,&CopyEngineFactory::updateBufferCheckbox);
    connect(ui->osBuffer,                   &QAbstractButton::toggled,          this,&CopyEngineFactory::updateBufferCheckbox);
    connect(ui->moveTheWholeFolder,         &QCheckBox::toggled,                this,&CopyEngineFactory::moveTheWholeFolder);
    connect(ui->followTheStrictOrder,       &QCheckBox::toggled,                this,&CopyEngineFactory::followTheStrictOrder);
    connect(ui->deletePartiallyTransferredFiles,&QCheckBox::toggled,            this,&CopyEngineFactory::deletePartiallyTransferredFiles);
    connect(ui->renameTheOriginalDestination,&QCheckBox::toggled,               this,&CopyEngineFactory::renameTheOriginalDestination);
    connect(ui->checkDiskSpace,             &QCheckBox::toggled,                this,&CopyEngineFactory::checkDiskSpace);
    connect(ui->defaultDestinationFolderBrowse,&QPushButton::clicked,           this,&CopyEngineFactory::defaultDestinationFolderBrowse);
    connect(ui->defaultDestinationFolder,&QLineEdit::editingFinished,           this,&CopyEngineFactory::defaultDestinationFolder);

    connect(filters,&Filters::sendNewFilters,this,&CopyEngineFactory::sendNewFilters);
    connect(ui->filters,&QPushButton::clicked,this,&CopyEngineFactory::showFilterDialog);
    connect(renamingRules,&RenamingRules::sendNewRenamingRules,this,&CopyEngineFactory::sendNewRenamingRules);
    connect(ui->renamingRules,&QPushButton::clicked,this,&CopyEngineFactory::showRenamingRules);

    lunchInitFunction.setInterval(0);
    lunchInitFunction.setSingleShot(true);
    connect(&lunchInitFunction,&QTimer::timeout,this,&CopyEngineFactory::init,Qt::QueuedConnection);
    lunchInitFunction.start();
}

CopyEngineFactory::~CopyEngineFactory()
{
    delete renamingRules;
    delete filters;
    delete ui;
}

void CopyEngineFactory::init()
{
    logicalDriveChanged(QString(),true);
    if(mountSysPoint.empty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"no drive found with QtSystemInformation");
}

PluginInterface_CopyEngine * CopyEngineFactory::getInstance()
{
    CopyEngine *realObject=new CopyEngine(facilityEngine);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(realObject,&CopyEngine::debugInformation,this,&CopyEngineFactory::debugInformation);
    #endif
    realObject->connectTheSignalsSlots();
    connect(this,&CopyEngineFactory::haveDrive,realObject,&CopyEngine::setDrive);
    realObject->setDrive(mountSysPoint,driveType);
    PluginInterface_CopyEngine * newTransferEngine=realObject;
    connect(this,&CopyEngineFactory::reloadLanguage,realObject,&CopyEngine::newLanguageLoaded);
    realObject->setRightTransfer(ui->doRightTransfer->isChecked());
    realObject->setKeepDate(ui->keepDate->isChecked());
    realObject->setBlockSize(ui->blockSize->value());
    realObject->setAutoStart(ui->autoStart->isChecked());
    realObject->setFolderCollision(ui->comboBoxFolderCollision->currentIndex());
    realObject->setFolderError(ui->comboBoxFolderError->currentIndex());
    realObject->setFileCollision(ui->comboBoxFileCollision->currentIndex());
    realObject->setFileError(ui->comboBoxFileError->currentIndex());
    realObject->setTransferAlgorithm(ui->transferAlgorithm->currentIndex());
    realObject->setCheckDestinationFolderExists(ui->checkBoxDestinationFolderExists->isChecked());
    realObject->set_doChecksum(ui->doChecksum->isChecked());
    realObject->set_checksumIgnoreIfImpossible(ui->checksumIgnoreIfImpossible->isChecked());
    realObject->set_checksumOnlyOnError(ui->checksumOnlyOnError->isChecked());
    realObject->set_osBuffer(ui->osBuffer->isChecked());
    realObject->set_osBufferLimited(ui->osBufferLimited->isChecked());
    realObject->set_osBufferLimit(ui->osBufferLimit->value());
    realObject->set_setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
    realObject->setRenamingRules(firstRenamingRule,otherRenamingRule);
    realObject->setSequentialBuffer(ui->sequentialBuffer->value());
    realObject->setParallelBuffer(ui->parallelBuffer->value());
    realObject->setParallelizeIfSmallerThan(ui->parallelizeIfSmallerThan->value());
    realObject->setMoveTheWholeFolder(ui->moveTheWholeFolder->isChecked());
    realObject->setFollowTheStrictOrder(ui->followTheStrictOrder->isChecked());
    realObject->setDeletePartiallyTransferredFiles(ui->deletePartiallyTransferredFiles->isChecked());
    realObject->setInodeThreads(ui->inodeThreads->value());
    realObject->setRenameTheOriginalDestination(ui->renameTheOriginalDestination->isChecked());
    realObject->setCheckDiskSpace(ui->checkDiskSpace->isChecked());
    realObject->setDefaultDestinationFolder(ui->defaultDestinationFolder->text());
    return newTransferEngine;
}

void CopyEngineFactory::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath:"+pluginPath);
    this->facilityEngine=facilityInterface;
    Q_UNUSED(portableVersion);
    #ifndef ULTRACOPIER_PLUGIN_DEBUG
        Q_UNUSED(writePath);
        Q_UNUSED(pluginPath);
    #endif
    #if ! defined (Q_CC_GNU)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Unable to change date time of files, only gcc is supported");
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,COMPILERINFO);
    #if defined (ULTRACOPIER_PLUGIN_CHECKLISTTYPE)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"CHECK LIST TYPE set");
    #else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"CHECK LIST TYPE not set");
    #endif
    if(options!=NULL)
    {
        //load the options
        QList<QPair<QString, QVariant> > KeysList;
        KeysList.append(qMakePair(QString("doRightTransfer"),QVariant(true)));
        KeysList.append(qMakePair(QString("keepDate"),QVariant(true)));
        KeysList.append(qMakePair(QString("blockSize"),QVariant(ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE)));
        quint32 sequentialBuffer=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*ULTRACOPIER_PLUGIN_DEFAULT_SEQUENTIAL_NUMBER_OF_BLOCK;
        quint32 parallelBuffer=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
        //to prevent swap and other bad effect, only under windows and unix for now
        #if defined(Q_OS_WIN32) or defined(Q_OS_UNIX)
        size_t max_memory=getTotalSystemMemory();
        if(max_memory>0)
        {
            if(sequentialBuffer>(max_memory/10))
                    sequentialBuffer=max_memory/10;
            if(parallelBuffer>(max_memory/100))
                    parallelBuffer=max_memory/100;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QString("detected memory: %1").arg(max_memory));
        #endif
        KeysList.append(qMakePair(QString("sequentialBuffer"),QVariant(sequentialBuffer)));
        KeysList.append(qMakePair(QString("parallelBuffer"),QVariant(parallelBuffer)));
        KeysList.append(qMakePair(QString("parallelizeIfSmallerThan"),QVariant(1)));
        KeysList.append(qMakePair(QString("autoStart"),QVariant(true)));
        KeysList.append(qMakePair(QString("folderError"),QVariant(0)));
        KeysList.append(qMakePair(QString("folderCollision"),QVariant(0)));
        KeysList.append(qMakePair(QString("fileError"),QVariant(0)));
        KeysList.append(qMakePair(QString("fileCollision"),QVariant(0)));
        KeysList.append(qMakePair(QString("transferAlgorithm"),QVariant(0)));
        KeysList.append(qMakePair(QString("checkDestinationFolder"),QVariant(true)));
        KeysList.append(qMakePair(QString("includeStrings"),QVariant(QStringList())));
        KeysList.append(qMakePair(QString("includeOptions"),QVariant(QStringList())));
        KeysList.append(qMakePair(QString("excludeStrings"),QVariant(QStringList())));
        KeysList.append(qMakePair(QString("excludeOptions"),QVariant(QStringList())));
        KeysList.append(qMakePair(QString("doChecksum"),QVariant(false)));
        KeysList.append(qMakePair(QString("checksumIgnoreIfImpossible"),QVariant(true)));
        KeysList.append(qMakePair(QString("checksumOnlyOnError"),QVariant(true)));
        KeysList.append(qMakePair(QString("osBuffer"),QVariant(false)));
        KeysList.append(qMakePair(QString("firstRenamingRule"),QVariant("")));
        KeysList.append(qMakePair(QString("otherRenamingRule"),QVariant("")));
        KeysList.append(qMakePair(QString("osBufferLimited"),QVariant(false)));
        KeysList.append(qMakePair(QString("osBufferLimit"),QVariant(512)));
        KeysList.append(qMakePair(QString("deletePartiallyTransferredFiles"),QVariant(true)));
        KeysList.append(qMakePair(QString("moveTheWholeFolder"),QVariant(true)));
        KeysList.append(qMakePair(QString("followTheStrictOrder"),QVariant(false)));
        KeysList.append(qMakePair(QString("renameTheOriginalDestination"),QVariant(false)));
        KeysList.append(qMakePair(QString("checkDiskSpace"),QVariant(true)));
        KeysList.append(qMakePair(QString("defaultDestinationFolder"),QVariant(QString())));
        KeysList.append(qMakePair(QString("inodeThreads"),QVariant(1)));
        options->addOptionGroup(KeysList);
        #if ! defined (Q_CC_GNU)
        ui->keepDate->setEnabled(false);
        ui->keepDate->setToolTip("Not supported with this compiler");
        #endif
        ui->doRightTransfer->setChecked(options->getOptionValue("doRightTransfer").toBool());
        ui->keepDate->setChecked(options->getOptionValue("keepDate").toBool());
        ui->blockSize->setValue(options->getOptionValue("blockSize").toUInt());//keep before sequentialBuffer and parallelBuffer
        ui->autoStart->setChecked(options->getOptionValue("autoStart").toBool());
        ui->comboBoxFolderError->setCurrentIndex(options->getOptionValue("folderError").toUInt());
        ui->comboBoxFolderCollision->setCurrentIndex(options->getOptionValue("folderCollision").toUInt());
        ui->comboBoxFileError->setCurrentIndex(options->getOptionValue("fileError").toUInt());
        ui->comboBoxFileCollision->setCurrentIndex(options->getOptionValue("fileCollision").toUInt());
        ui->transferAlgorithm->setCurrentIndex(options->getOptionValue("transferAlgorithm").toUInt());
        ui->checkBoxDestinationFolderExists->setChecked(options->getOptionValue("checkDestinationFolder").toBool());
        ui->parallelizeIfSmallerThan->setValue(options->getOptionValue("parallelizeIfSmallerThan").toUInt());
        ui->sequentialBuffer->setValue(options->getOptionValue("sequentialBuffer").toUInt());
        ui->parallelBuffer->setValue(options->getOptionValue("parallelBuffer").toUInt());
        ui->sequentialBuffer->setSingleStep(ui->blockSize->value());
        ui->parallelBuffer->setSingleStep(ui->blockSize->value());
        ui->deletePartiallyTransferredFiles->setChecked(options->getOptionValue("deletePartiallyTransferredFiles").toBool());
        ui->moveTheWholeFolder->setChecked(options->getOptionValue("moveTheWholeFolder").toBool());
        ui->followTheStrictOrder->setChecked(options->getOptionValue("followTheStrictOrder").toBool());
        ui->inodeThreads->setValue(options->getOptionValue("inodeThreads").toUInt());
        ui->renameTheOriginalDestination->setChecked(options->getOptionValue("renameTheOriginalDestination").toBool());
        ui->checkDiskSpace->setChecked(options->getOptionValue("checkDiskSpace").toBool());
        ui->defaultDestinationFolder->setText(options->getOptionValue("defaultDestinationFolder").toString());

        ui->doChecksum->setChecked(options->getOptionValue("doChecksum").toBool());
        ui->checksumIgnoreIfImpossible->setChecked(options->getOptionValue("checksumIgnoreIfImpossible").toBool());
        ui->checksumOnlyOnError->setChecked(options->getOptionValue("checksumOnlyOnError").toBool());

        ui->osBuffer->setChecked(options->getOptionValue("osBuffer").toBool());
        ui->osBufferLimited->setChecked(options->getOptionValue("osBufferLimited").toBool());
        ui->osBufferLimit->setValue(options->getOptionValue("osBufferLimit").toUInt());
        //ui->autoStart->setChecked(options->getOptionValue("autoStart").toBool());//moved from options(), wrong previous place
        includeStrings=options->getOptionValue("includeStrings").toStringList();
        includeOptions=options->getOptionValue("includeOptions").toStringList();
        excludeStrings=options->getOptionValue("excludeStrings").toStringList();
        excludeOptions=options->getOptionValue("excludeOptions").toStringList();
        filters->setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
        firstRenamingRule=options->getOptionValue("firstRenamingRule").toString();
        otherRenamingRule=options->getOptionValue("otherRenamingRule").toString();
        renamingRules->setRenamingRules(firstRenamingRule,otherRenamingRule);

        ui->checksumOnlyOnError->setEnabled(ui->doChecksum->isChecked());
        ui->checksumIgnoreIfImpossible->setEnabled(ui->doChecksum->isChecked());

        updateBufferCheckbox();
        optionsEngine=options;

        updatedBlockSize();
    }
}

QStringList CopyEngineFactory::supportedProtocolsForTheSource() const
{
    return QStringList() << "file";
}

QStringList CopyEngineFactory::supportedProtocolsForTheDestination() const
{
    return QStringList() << "file";
}

Ultracopier::CopyType CopyEngineFactory::getCopyType()
{
    return Ultracopier::FileAndFolder;
}

Ultracopier::TransferListOperation CopyEngineFactory::getTransferListOperation()
{
    return Ultracopier::TransferListOperation_ImportExport;
}

bool CopyEngineFactory::canDoOnlyCopy() const
{
    return false;
}

void CopyEngineFactory::resetOptions()
{
}

QWidget * CopyEngineFactory::options()
{
    return tempWidget;
}

void CopyEngineFactory::setDoRightTransfer(bool doRightTransfer)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("doRightTransfer",doRightTransfer);
}

void CopyEngineFactory::setKeepDate(bool keepDate)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("keepDate",keepDate);
}

void CopyEngineFactory::setBlockSize(int blockSize)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("blockSize",blockSize);
    updatedBlockSize();
}

void CopyEngineFactory::setParallelBuffer(int parallelBuffer)
{
    if(optionsEngine!=NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
        parallelBuffer=round((float)parallelBuffer/(float)ui->blockSize->value())*ui->blockSize->value();
        ui->parallelBuffer->setValue(parallelBuffer);
        optionsEngine->setOptionValue("parallelBuffer",parallelBuffer);
    }
}

void CopyEngineFactory::setSequentialBuffer(int sequentialBuffer)
{
    if(optionsEngine!=NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
        sequentialBuffer=round((float)sequentialBuffer/(float)ui->blockSize->value())*ui->blockSize->value();
        ui->sequentialBuffer->setValue(sequentialBuffer);
        optionsEngine->setOptionValue("sequentialBuffer",sequentialBuffer);
    }
}

void CopyEngineFactory::setParallelizeIfSmallerThan(int parallelizeIfSmallerThan)
{
    if(optionsEngine!=NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
        optionsEngine->setOptionValue("parallelizeIfSmallerThan",parallelizeIfSmallerThan);
    }
}

void CopyEngineFactory::setAutoStart(bool autoStart)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("autoStart",autoStart);
}

void CopyEngineFactory::setFolderCollision(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("folderCollision",index);
}

void CopyEngineFactory::setFolderError(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("folderError",index);
}

void CopyEngineFactory::setTransferAlgorithm(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("transferAlgorithm",index);
}

void CopyEngineFactory::setCheckDestinationFolder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checkDestinationFolder",ui->checkBoxDestinationFolderExists->isChecked());
}

void CopyEngineFactory::newLanguageLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, retranslate the widget options");
    OptionInterface * optionsEngine=this->optionsEngine;
    this->optionsEngine=NULL;
    ui->retranslateUi(tempWidget);
    ui->comboBoxFolderError->setItemText(0,tr("Ask"));
    ui->comboBoxFolderError->setItemText(1,tr("Skip"));

    ui->comboBoxFolderCollision->setItemText(0,tr("Ask"));
    ui->comboBoxFolderCollision->setItemText(1,tr("Merge"));
    ui->comboBoxFolderCollision->setItemText(2,tr("Skip"));
    ui->comboBoxFolderCollision->setItemText(3,tr("Rename"));

    ui->comboBoxFileError->setItemText(0,tr("Ask"));
    ui->comboBoxFileError->setItemText(1,tr("Skip"));
    ui->comboBoxFileError->setItemText(2,tr("Put at the end"));

    ui->comboBoxFileCollision->setItemText(0,tr("Ask"));
    ui->comboBoxFileCollision->setItemText(1,tr("Skip"));
    ui->comboBoxFileCollision->setItemText(2,tr("Overwrite"));
    ui->comboBoxFileCollision->setItemText(3,tr("Overwrite if different"));
    ui->comboBoxFileCollision->setItemText(4,tr("Overwrite if newer"));
    ui->comboBoxFileCollision->setItemText(5,tr("Overwrite if older"));
    ui->comboBoxFileCollision->setItemText(6,tr("Rename"));

    ui->transferAlgorithm->setItemText(0,tr("Automatic"));
    ui->transferAlgorithm->setItemText(1,tr("Sequential"));
    ui->transferAlgorithm->setItemText(2,tr("Parallel"));
    if(optionsEngine!=NULL)
    {
        filters->newLanguageLoaded();
        renamingRules->newLanguageLoaded();
    }
    emit reloadLanguage();
    this->optionsEngine=optionsEngine;
}

void CopyEngineFactory::doChecksum_toggled(bool doChecksum)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("doChecksum",doChecksum);
}

void CopyEngineFactory::checksumOnlyOnError_toggled(bool checksumOnlyOnError)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checksumOnlyOnError",checksumOnlyOnError);
}

void CopyEngineFactory::osBuffer_toggled(bool osBuffer)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("osBuffer",osBuffer);
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void CopyEngineFactory::osBufferLimited_toggled(bool osBufferLimited)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("osBufferLimited",osBufferLimited);
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void CopyEngineFactory::osBufferLimit_editingFinished()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the spinbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("osBufferLimit",ui->osBufferLimit->value());
}

void CopyEngineFactory::showFilterDialog()
{
    if(optionsEngine==NULL)
    {
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded. Unable to access the filters"));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"options not loaded");
        return;
    }
    filters->exec();
}

void CopyEngineFactory::sendNewFilters(const QStringList &includeStrings,const QStringList &includeOptions,const QStringList &excludeStrings,const QStringList &excludeOptions)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    this->includeStrings=includeStrings;
    this->includeOptions=includeOptions;
    this->excludeStrings=excludeStrings;
    this->excludeOptions=excludeOptions;
    if(optionsEngine!=NULL)
    {
        optionsEngine->setOptionValue("includeStrings",includeStrings);
        optionsEngine->setOptionValue("includeOptions",includeOptions);
        optionsEngine->setOptionValue("excludeStrings",excludeStrings);
        optionsEngine->setOptionValue("excludeOptions",excludeOptions);
    }
}

void CopyEngineFactory::sendNewRenamingRules(const QString &firstRenamingRule,const QString &otherRenamingRule)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    if(optionsEngine!=NULL)
    {
        optionsEngine->setOptionValue("firstRenamingRule",firstRenamingRule);
        optionsEngine->setOptionValue("otherRenamingRule",otherRenamingRule);
    }
}

void CopyEngineFactory::showRenamingRules()
{
    if(optionsEngine==NULL)
    {
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded, can't access to the filters"));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"options not loaded");
        return;
    }
    renamingRules->exec();
}

void CopyEngineFactory::updateBufferCheckbox()
{
    ui->osBufferLimited->setEnabled(ui->osBuffer->isChecked());
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void CopyEngineFactory::checksumIgnoreIfImpossible_toggled(bool checksumIgnoreIfImpossible)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checksumIgnoreIfImpossible",checksumIgnoreIfImpossible);
}

void CopyEngineFactory::logicalDriveChanged(const QString &,bool)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"start");
    mountSysPoint.clear();
    QStringList temp=storageInfo.allLogicalDrives();
    for (int i = 0; i < temp.size(); ++i) {
        mountSysPoint<<QDir::toNativeSeparators(temp.at(i));
    }
    if(mountSysPoint.empty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"no drive found with QtSystemInformation");
    mountSysPoint.removeDuplicates();
    driveType.clear();
    for (int i = 0; i < mountSysPoint.size(); ++i) {
        driveType<<storageInfo.driveType(mountSysPoint.at(i));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("mountSysPoint: %1 (type: %2)").arg(mountSysPoint.at(i)).arg(driveType.last()));
    }
    emit haveDrive(mountSysPoint,driveType);
}

void CopyEngineFactory::setFileCollision(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("action index: %1").arg(index));
    if(optionsEngine==NULL)
        return;
    switch(index)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            optionsEngine->setOptionValue("fileCollision",index);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
        break;
    }
}

void CopyEngineFactory::setFileError(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("action index: %1").arg(index));
    if(optionsEngine==NULL)
        return;
    switch(index)
    {
        case 0:
        case 1:
        case 2:
            optionsEngine->setOptionValue("fileError",index);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
        break;
    }
}

void CopyEngineFactory::updatedBlockSize()
{
    ui->sequentialBuffer->setMinimum(ui->blockSize->value());
    ui->sequentialBuffer->setSingleStep(ui->blockSize->value());
    ui->sequentialBuffer->setMaximum(ui->blockSize->value()*ULTRACOPIER_PLUGIN_MAX_SEQUENTIAL_NUMBER_OF_BLOCK);
    ui->parallelBuffer->setMinimum(ui->blockSize->value());
    ui->parallelBuffer->setSingleStep(ui->blockSize->value());
    ui->parallelBuffer->setMaximum(ui->blockSize->value()*ULTRACOPIER_PLUGIN_MAX_PARALLEL_NUMBER_OF_BLOCK);
    setParallelBuffer(ui->parallelBuffer->value());
    setSequentialBuffer(ui->sequentialBuffer->value());
}

void CopyEngineFactory::deletePartiallyTransferredFiles(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("deletePartiallyTransferredFiles",checked);
}

void CopyEngineFactory::renameTheOriginalDestination(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("renameTheOriginalDestination",checked);
}

void CopyEngineFactory::checkDiskSpace(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checkDiskSpace",checked);
}

void CopyEngineFactory::defaultDestinationFolderBrowse()
{
    QString destination = QFileDialog::getExistingDirectory(ui->defaultDestinationFolder,facilityEngine->translateText("Select destination directory"),"",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(destination.isEmpty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Canceled by the user");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    ui->defaultDestinationFolder->setText(destination);
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("defaultDestinationFolder",destination);
}

void CopyEngineFactory::defaultDestinationFolder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("defaultDestinationFolder",ui->defaultDestinationFolder->text());
}

void CopyEngineFactory::followTheStrictOrder(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("followTheStrictOrder",checked);
}

void CopyEngineFactory::moveTheWholeFolder(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("moveTheWholeFolder",checked);
}

void CopyEngineFactory::on_inodeThreads_editingFinished()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the spinbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("inodeThreads",ui->inodeThreads->value());
}

#ifdef Q_OS_WIN32
size_t CopyEngineFactory::getTotalSystemMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
}
#endif

#ifdef Q_OS_UNIX
size_t CopyEngineFactory::getTotalSystemMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#endif
