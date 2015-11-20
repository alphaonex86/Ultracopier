/** \file factory.cpp
\brief Define the factory to create new instance
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QFileDialog>
#include <QList>
#include <QDebug>
#include <cmath>
#include <QStorageInfo>

#include "CopyEngineFactory.h"

// The cmath header from MSVC does not contain round()
#if (defined(_WIN64) || defined(_WIN32)) && defined(_MSC_VER)
inline double round(double d) {
    return floor( d + 0.5 );
}
#endif

CopyEngineFactory::CopyEngineFactory() :
    ui(new Ui::copyEngineOptions())
{
    qRegisterMetaType<FolderExistsAction>("FolderExistsAction");
    qRegisterMetaType<FileExistsAction>("FileExistsAction");
    qRegisterMetaType<QList<Filters_rules> >("QList<Filters_rules>");
    qRegisterMetaType<TransferStat>("TransferStat");
    qRegisterMetaType<QList<QByteArray> >("QList<QByteArray>");
    qRegisterMetaType<TransferAlgorithm>("TransferAlgorithm");
    qRegisterMetaType<ActionType>("ActionType");
    qRegisterMetaType<ErrorType>("ErrorType");
    qRegisterMetaType<Diskspace>("Diskspace");
    qRegisterMetaType<QList<Diskspace> >("QList<Diskspace>");
    qRegisterMetaType<QFileInfo>("QFileInfo");
    qRegisterMetaType<Ultracopier::CopyMode>("Ultracopier::CopyMode");

    tempWidget=new QWidget();
    ui->setupUi(tempWidget);
    ui->toolBox->setCurrentIndex(0);
    ui->blockSize->setMaximum(ULTRACOPIER_PLUGIN_MAX_BLOCK_SIZE);
    errorFound=false;
    optionsEngine=NULL;
    filters=new Filters(tempWidget);
    renamingRules=new RenamingRules(tempWidget);

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
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    connect(ui->rsync,                      &QCheckBox::toggled,                this,&CopyEngineFactory::setRsync);
    #endif
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
    connect(ui->copyListOrder,              &QCheckBox::toggled,                this,&CopyEngineFactory::copyListOrder);

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
}

PluginInterface_CopyEngine * CopyEngineFactory::getInstance()
{
    CopyEngine *realObject=new CopyEngine(facilityEngine);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(realObject,&CopyEngine::debugInformation,this,&CopyEngineFactory::debugInformation);
    #endif
    realObject->connectTheSignalsSlots();
    PluginInterface_CopyEngine * newTransferEngine=realObject;
    connect(this,&CopyEngineFactory::reloadLanguage,realObject,&CopyEngine::newLanguageLoaded);
    realObject->setRightTransfer(ui->doRightTransfer->isChecked());
    realObject->setKeepDate(ui->keepDate->isChecked());
    realObject->setBlockSize(ui->blockSize->value());
    realObject->setAutoStart(ui->autoStart->isChecked());
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    realObject->setRsync(ui->rsync->isChecked());
    #endif
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
    realObject->setCopyListOrder(ui->copyListOrder->isChecked());
    return newTransferEngine;
}

void CopyEngineFactory::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start, writePath: ")+writePath+QStringLiteral(", pluginPath:")+pluginPath);
    this->facilityEngine=facilityInterface;
    Q_UNUSED(portableVersion);
    #ifndef ULTRACOPIER_PLUGIN_DEBUG
        Q_UNUSED(writePath);
        Q_UNUSED(pluginPath);
    #endif
    #if ! defined (Q_CC_GNU)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("Unable to change date time of files, only gcc is supported"));
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,COMPILERINFO);
    #if defined (ULTRACOPIER_PLUGIN_CHECKLISTTYPE)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("CHECK LIST TYPE set"));
    #else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("CHECK LIST TYPE not set"));
    #endif
    if(options!=NULL)
    {
        //load the options
        QList<QPair<QString, QVariant> > KeysList;
        KeysList.append(qMakePair(QStringLiteral("doRightTransfer"),QVariant(true)));
        #ifndef Q_OS_LINUX
        KeysList.append(qMakePair(QStringLiteral("keepDate"),QVariant(false)));
        #else
        KeysList.append(qMakePair(QStringLiteral("keepDate"),QVariant(true)));
        #endif
        KeysList.append(qMakePair(QStringLiteral("blockSize"),QVariant(ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE)));
        quint32 sequentialBuffer=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*ULTRACOPIER_PLUGIN_DEFAULT_SEQUENTIAL_NUMBER_OF_BLOCK;
        quint32 parallelBuffer=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
        //to prevent swap and other bad effect, only under windows and unix for now
        #if defined(Q_OS_WIN32) or (defined(Q_OS_LINUX) and defined(_SC_PHYS_PAGES))
        size_t max_memory=getTotalSystemMemory()/1024;
        if(max_memory>0)
        {
            if(sequentialBuffer>(max_memory/10))
                    sequentialBuffer=max_memory/10;
            if(parallelBuffer>(max_memory/100))
                    parallelBuffer=max_memory/100;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("detected memory: %1MB").arg(max_memory/1024));
        #endif
        KeysList.append(qMakePair(QStringLiteral("sequentialBuffer"),QVariant(sequentialBuffer)));
        KeysList.append(qMakePair(QStringLiteral("parallelBuffer"),QVariant(parallelBuffer)));
        KeysList.append(qMakePair(QStringLiteral("parallelizeIfSmallerThan"),QVariant(1)));
        KeysList.append(qMakePair(QStringLiteral("autoStart"),QVariant(true)));
        #ifdef ULTRACOPIER_PLUGIN_RSYNC
        KeysList.append(qMakePair(QStringLiteral("rsync"),QVariant(true)));
        #endif
        KeysList.append(qMakePair(QStringLiteral("folderError"),QVariant(0)));
        KeysList.append(qMakePair(QStringLiteral("folderCollision"),QVariant(0)));
        KeysList.append(qMakePair(QStringLiteral("fileError"),QVariant(2)));
        KeysList.append(qMakePair(QStringLiteral("fileCollision"),QVariant(0)));
        KeysList.append(qMakePair(QStringLiteral("transferAlgorithm"),QVariant(0)));
        KeysList.append(qMakePair(QStringLiteral("checkDestinationFolder"),QVariant(true)));
        KeysList.append(qMakePair(QStringLiteral("includeStrings"),QVariant(QStringList())));
        KeysList.append(qMakePair(QStringLiteral("includeOptions"),QVariant(QStringList())));
        KeysList.append(qMakePair(QStringLiteral("excludeStrings"),QVariant(QStringList())));
        KeysList.append(qMakePair(QStringLiteral("excludeOptions"),QVariant(QStringList())));
        KeysList.append(qMakePair(QStringLiteral("doChecksum"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("checksumIgnoreIfImpossible"),QVariant(true)));
        KeysList.append(qMakePair(QStringLiteral("checksumOnlyOnError"),QVariant(true)));
        KeysList.append(qMakePair(QStringLiteral("osBuffer"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("firstRenamingRule"),QVariant("")));
        KeysList.append(qMakePair(QStringLiteral("otherRenamingRule"),QVariant("")));
        KeysList.append(qMakePair(QStringLiteral("osBufferLimited"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("osBufferLimit"),QVariant(512)));
        KeysList.append(qMakePair(QStringLiteral("deletePartiallyTransferredFiles"),QVariant(true)));
        KeysList.append(qMakePair(QStringLiteral("moveTheWholeFolder"),QVariant(true)));
        KeysList.append(qMakePair(QStringLiteral("followTheStrictOrder"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("renameTheOriginalDestination"),QVariant(false)));
        KeysList.append(qMakePair(QStringLiteral("checkDiskSpace"),QVariant(true)));
        KeysList.append(qMakePair(QStringLiteral("defaultDestinationFolder"),QVariant(QString())));
        KeysList.append(qMakePair(QStringLiteral("inodeThreads"),QVariant(1)));
        KeysList.append(qMakePair(QStringLiteral("copyListOrder"),QVariant(false)));
        options->addOptionGroup(KeysList);
        #if ! defined (Q_CC_GNU)
        ui->keepDate->setEnabled(false);
        ui->keepDate->setToolTip(QStringLiteral("Not supported with this compiler"));
        #endif
        ui->doRightTransfer->setChecked(options->getOptionValue(QStringLiteral("doRightTransfer")).toBool());
        ui->keepDate->setChecked(options->getOptionValue(QStringLiteral("keepDate")).toBool());
        ui->blockSize->setValue(options->getOptionValue(QStringLiteral("blockSize")).toUInt());//keep before sequentialBuffer and parallelBuffer
        ui->autoStart->setChecked(options->getOptionValue(QStringLiteral("autoStart")).toBool());
        #ifdef ULTRACOPIER_PLUGIN_RSYNC
        ui->rsync->setChecked(options->getOptionValue(QStringLiteral("rsync")).toBool());
        #else
        ui->label_rsync->setVisible(false);
        ui->rsync->setVisible(false);
        #endif
        ui->comboBoxFolderError->setCurrentIndex(options->getOptionValue(QStringLiteral("folderError")).toUInt());
        ui->comboBoxFolderCollision->setCurrentIndex(options->getOptionValue(QStringLiteral("folderCollision")).toUInt());
        ui->comboBoxFileError->setCurrentIndex(options->getOptionValue(QStringLiteral("fileError")).toUInt());
        ui->comboBoxFileCollision->setCurrentIndex(options->getOptionValue(QStringLiteral("fileCollision")).toUInt());
        ui->transferAlgorithm->setCurrentIndex(options->getOptionValue(QStringLiteral("transferAlgorithm")).toUInt());
        ui->checkBoxDestinationFolderExists->setChecked(options->getOptionValue(QStringLiteral("checkDestinationFolder")).toBool());
        ui->parallelizeIfSmallerThan->setValue(options->getOptionValue(QStringLiteral("parallelizeIfSmallerThan")).toUInt());
        ui->sequentialBuffer->setValue(options->getOptionValue(QStringLiteral("sequentialBuffer")).toUInt());
        ui->parallelBuffer->setValue(options->getOptionValue(QStringLiteral("parallelBuffer")).toUInt());
        ui->sequentialBuffer->setSingleStep(ui->blockSize->value());
        ui->parallelBuffer->setSingleStep(ui->blockSize->value());
        ui->deletePartiallyTransferredFiles->setChecked(options->getOptionValue(QStringLiteral("deletePartiallyTransferredFiles")).toBool());
        ui->moveTheWholeFolder->setChecked(options->getOptionValue(QStringLiteral("moveTheWholeFolder")).toBool());
        ui->followTheStrictOrder->setChecked(options->getOptionValue(QStringLiteral("followTheStrictOrder")).toBool());
        ui->inodeThreads->setValue(options->getOptionValue(QStringLiteral("inodeThreads")).toUInt());
        ui->renameTheOriginalDestination->setChecked(options->getOptionValue(QStringLiteral("renameTheOriginalDestination")).toBool());
        ui->checkDiskSpace->setChecked(options->getOptionValue(QStringLiteral("checkDiskSpace")).toBool());
        ui->defaultDestinationFolder->setText(options->getOptionValue(QStringLiteral("defaultDestinationFolder")).toString());

        ui->doChecksum->setChecked(options->getOptionValue(QStringLiteral("doChecksum")).toBool());
        ui->checksumIgnoreIfImpossible->setChecked(options->getOptionValue(QStringLiteral("checksumIgnoreIfImpossible")).toBool());
        ui->checksumOnlyOnError->setChecked(options->getOptionValue(QStringLiteral("checksumOnlyOnError")).toBool());

        ui->osBuffer->setChecked(options->getOptionValue(QStringLiteral("osBuffer")).toBool());
        ui->osBufferLimited->setChecked(options->getOptionValue(QStringLiteral("osBufferLimited")).toBool());
        ui->osBufferLimit->setValue(options->getOptionValue(QStringLiteral("osBufferLimit")).toUInt());
        //ui->autoStart->setChecked(options->getOptionValue(QStringLiteral("autoStart")).toBool());//moved from options(), wrong previous place
        includeStrings=options->getOptionValue(QStringLiteral("includeStrings")).toStringList();
        includeOptions=options->getOptionValue(QStringLiteral("includeOptions")).toStringList();
        excludeStrings=options->getOptionValue(QStringLiteral("excludeStrings")).toStringList();
        excludeOptions=options->getOptionValue(QStringLiteral("excludeOptions")).toStringList();
        filters->setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
        firstRenamingRule=options->getOptionValue(QStringLiteral("firstRenamingRule")).toString();
        otherRenamingRule=options->getOptionValue(QStringLiteral("otherRenamingRule")).toString();
        renamingRules->setRenamingRules(firstRenamingRule,otherRenamingRule);

        ui->checksumOnlyOnError->setEnabled(ui->doChecksum->isChecked());
        ui->checksumIgnoreIfImpossible->setEnabled(ui->doChecksum->isChecked());
        ui->copyListOrder->setChecked(options->getOptionValue(QStringLiteral("copyListOrder")).toBool());

        updateBufferCheckbox();
        optionsEngine=options;

        updatedBlockSize();
    }
}

QStringList CopyEngineFactory::supportedProtocolsForTheSource() const
{
    return QStringList() << QStringLiteral("file");
}

QStringList CopyEngineFactory::supportedProtocolsForTheDestination() const
{
    return QStringList() << QStringLiteral("file");
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
        optionsEngine->setOptionValue(QStringLiteral("doRightTransfer"),doRightTransfer);
}

void CopyEngineFactory::setKeepDate(bool keepDate)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("keepDate"),keepDate);
}

void CopyEngineFactory::setBlockSize(int blockSize)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("blockSize"),blockSize);
    updatedBlockSize();
}

void CopyEngineFactory::setParallelBuffer(int parallelBuffer)
{
    if(optionsEngine!=NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
        parallelBuffer=round((float)parallelBuffer/(float)ui->blockSize->value())*ui->blockSize->value();
        ui->parallelBuffer->setValue(parallelBuffer);
        optionsEngine->setOptionValue(QStringLiteral("parallelBuffer"),parallelBuffer);
    }
}

void CopyEngineFactory::setSequentialBuffer(int sequentialBuffer)
{
    if(optionsEngine!=NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("the value have changed"));
        sequentialBuffer=round((float)sequentialBuffer/(float)ui->blockSize->value())*ui->blockSize->value();
        ui->sequentialBuffer->setValue(sequentialBuffer);
        optionsEngine->setOptionValue(QStringLiteral("sequentialBuffer"),sequentialBuffer);
    }
}

void CopyEngineFactory::setParallelizeIfSmallerThan(int parallelizeIfSmallerThan)
{
    if(optionsEngine!=NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
        optionsEngine->setOptionValue(QStringLiteral("parallelizeIfSmallerThan"),parallelizeIfSmallerThan);
    }
}

void CopyEngineFactory::setAutoStart(bool autoStart)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("autoStart"),autoStart);
}

void CopyEngineFactory::setFolderCollision(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("folderCollision"),index);
}

void CopyEngineFactory::setFolderError(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("folderError"),index);
}

void CopyEngineFactory::setTransferAlgorithm(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("transferAlgorithm"),index);
}

void CopyEngineFactory::setCheckDestinationFolder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("checkDestinationFolder"),ui->checkBoxDestinationFolderExists->isChecked());
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
        optionsEngine->setOptionValue(QStringLiteral("doChecksum"),doChecksum);
}

void CopyEngineFactory::checksumOnlyOnError_toggled(bool checksumOnlyOnError)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("checksumOnlyOnError"),checksumOnlyOnError);
}

void CopyEngineFactory::osBuffer_toggled(bool osBuffer)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("osBuffer"),osBuffer);
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void CopyEngineFactory::osBufferLimited_toggled(bool osBufferLimited)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("osBufferLimited"),osBufferLimited);
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void CopyEngineFactory::osBufferLimit_editingFinished()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the spinbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("osBufferLimit"),ui->osBufferLimit->value());
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("new filter"));
    this->includeStrings=includeStrings;
    this->includeOptions=includeOptions;
    this->excludeStrings=excludeStrings;
    this->excludeOptions=excludeOptions;
    if(optionsEngine!=NULL)
    {
        optionsEngine->setOptionValue(QStringLiteral("includeStrings"),includeStrings);
        optionsEngine->setOptionValue(QStringLiteral("includeOptions"),includeOptions);
        optionsEngine->setOptionValue(QStringLiteral("excludeStrings"),excludeStrings);
        optionsEngine->setOptionValue(QStringLiteral("excludeOptions"),excludeOptions);
    }
}

void CopyEngineFactory::sendNewRenamingRules(const QString &firstRenamingRule,const QString &otherRenamingRule)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    if(optionsEngine!=NULL)
    {
        optionsEngine->setOptionValue(QStringLiteral("firstRenamingRule"),firstRenamingRule);
        optionsEngine->setOptionValue(QStringLiteral("otherRenamingRule"),otherRenamingRule);
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
        optionsEngine->setOptionValue(QStringLiteral("checksumIgnoreIfImpossible"),checksumIgnoreIfImpossible);
}

void CopyEngineFactory::setFileCollision(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("action index: %1").arg(index));
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
            optionsEngine->setOptionValue(QStringLiteral("fileCollision"),index);
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
        break;
    }
}

void CopyEngineFactory::setFileError(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("action index: %1").arg(index));
    if(optionsEngine==NULL)
        return;
    switch(index)
    {
        case 0:
        case 1:
        case 2:
            optionsEngine->setOptionValue(QStringLiteral("fileError"),index);
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
        optionsEngine->setOptionValue(QStringLiteral("deletePartiallyTransferredFiles"),checked);
}

void CopyEngineFactory::renameTheOriginalDestination(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("renameTheOriginalDestination"),checked);
}

void CopyEngineFactory::checkDiskSpace(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("checkDiskSpace"),checked);
}

void CopyEngineFactory::defaultDestinationFolderBrowse()
{
    QString destination = QFileDialog::getExistingDirectory(ui->defaultDestinationFolder,facilityEngine->translateText(QStringLiteral("Select destination directory")),QStringLiteral(""),QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(destination.isEmpty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Canceled by the user");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    ui->defaultDestinationFolder->setText(destination);
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("defaultDestinationFolder"),destination);
}

void CopyEngineFactory::defaultDestinationFolder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("defaultDestinationFolder"),ui->defaultDestinationFolder->text());
}

void CopyEngineFactory::followTheStrictOrder(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("followTheStrictOrder"),checked);
}

void CopyEngineFactory::moveTheWholeFolder(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("moveTheWholeFolder"),checked);
}

void CopyEngineFactory::on_inodeThreads_editingFinished()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the spinbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("inodeThreads"),ui->inodeThreads->value());
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

#ifdef Q_OS_LINUX
size_t CopyEngineFactory::getTotalSystemMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#endif

#ifdef ULTRACOPIER_PLUGIN_RSYNC
void CopyEngineFactory::setRsync(bool rsync)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("rsync",rsync);
}
#endif

void CopyEngineFactory::copyListOrder(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue(QStringLiteral("copyListOrder"),checked);
}
