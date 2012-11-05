/** \file factory.cpp
\brief Define the factory to create new instance
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QFileDialog>
#include <QDebug>

#include "factory.h"

Factory::Factory() :
    ui(new Ui::options())
{
    qRegisterMetaType<TransferThread *>("TransferThread *");
    qRegisterMetaType<scanFileOrFolder *>("scanFileOrFolder *");
    qRegisterMetaType<Ultracopier::EngineActionInProgress>("Ultracopier::EngineActionInProgress");
    qRegisterMetaType<Ultracopier::DebugLevel>("Ultracopier::DebugLevel");
    qRegisterMetaType<FileExistsAction>("FileExistsAction");
    qRegisterMetaType<FolderExistsAction>("FolderExistsAction");
    qRegisterMetaType<QList<Filters_rules> >("QList<Filters_rules>");
    qRegisterMetaType<QList<int> >("QList<int>");
    qRegisterMetaType<Ultracopier::CopyMode>("Ultracopier::CopyMode");
    qRegisterMetaType<QList<Ultracopier::ReturnActionOnCopyList> >("QList<Ultracopier::ReturnActionOnCopyList>");
    qRegisterMetaType<QList<Ultracopier::ProgressionItem> >("QList<Ultracopier::ProgressionItem>");

    tempWidget=new QWidget();
    ui->setupUi(tempWidget);
    errorFound=false;
    optionsEngine=NULL;
    filters=new Filters(tempWidget);
    renamingRules=new RenamingRules(tempWidget);

    QStringList temp=storageInfo.allLogicalDrives();
    for (int i = 0; i < temp.size(); ++i) {
        mountSysPoint<<temp.at(i);
    }
    connect(&storageInfo,&QStorageInfo::logicalDriveChanged,this,&Factory::logicalDriveChanged);

    connect(ui->doRightTransfer,		&QCheckBox::toggled,		this,&Factory::setDoRightTransfer);
    connect(ui->keepDate,			&QCheckBox::toggled,		this,&Factory::setKeepDate);
    connect(ui->blockSize,			static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&Factory::setBlockSize);
    connect(ui->autoStart,			&QCheckBox::toggled,		this,&Factory::setAutoStart);
    connect(ui->doChecksum,			&QCheckBox::toggled,		this,&Factory::doChecksum_toggled);
    connect(ui->checksumIgnoreIfImpossible,	&QCheckBox::toggled,		this,&Factory::checksumIgnoreIfImpossible_toggled);
    connect(ui->checksumOnlyOnError,	&QCheckBox::toggled,		this,&Factory::checksumOnlyOnError_toggled);
    connect(ui->osBuffer,			&QCheckBox::toggled,		this,&Factory::osBuffer_toggled);
    connect(ui->osBufferLimited,		&QCheckBox::toggled,		this,&Factory::osBufferLimited_toggled);
    connect(ui->osBufferLimit,		&QSpinBox::editingFinished,	this,&Factory::osBufferLimit_editingFinished);

    connect(filters,&Filters::sendNewFilters,this,&Factory::sendNewFilters);
    connect(ui->filters,&QPushButton::clicked,this,&Factory::showFilterDialog);
    connect(renamingRules,&RenamingRules::sendNewRenamingRules,this,&Factory::sendNewRenamingRules);
    connect(ui->renamingRules,&QPushButton::clicked,this,&Factory::showRenamingRules);
}

Factory::~Factory()
{
    delete renamingRules;
    delete filters;
    delete ui;
}

PluginInterface_CopyEngine * Factory::getInstance()
{
    copyEngine *realObject=new copyEngine(facilityEngine);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(realObject,&copyEngine::debugInformation,this,&Factory::debugInformation);
    #endif
    realObject->connectTheSignalsSlots();
    connect(this,&Factory::haveDrive,realObject,&copyEngine::setDrive);
    realObject->setDrive(mountSysPoint);
    PluginInterface_CopyEngine * newTransferEngine=realObject;
    connect(this,&Factory::reloadLanguage,realObject,&copyEngine::newLanguageLoaded);
    realObject->setRightTransfer(		optionsEngine->getOptionValue("doRightTransfer").toBool());
    realObject->setKeepDate(		optionsEngine->getOptionValue("keepDate").toBool());
    realObject->setBlockSize(		optionsEngine->getOptionValue("blockSize").toInt());
    realObject->setAutoStart(		optionsEngine->getOptionValue("autoStart").toBool());
    realObject->on_comboBoxFolderColision_currentIndexChanged(ui->comboBoxFolderColision->currentIndex());
    realObject->on_comboBoxFolderError_currentIndexChanged(ui->comboBoxFolderError->currentIndex());
    realObject->setCheckDestinationFolderExists(	optionsEngine->getOptionValue("checkDestinationFolder").toBool());

    realObject->set_doChecksum(optionsEngine->getOptionValue("doChecksum").toBool());
    realObject->set_checksumIgnoreIfImpossible(optionsEngine->getOptionValue("checksumIgnoreIfImpossible").toBool());
    realObject->set_checksumOnlyOnError(optionsEngine->getOptionValue("checksumOnlyOnError").toBool());
    realObject->set_osBuffer(optionsEngine->getOptionValue("osBuffer").toBool());
    realObject->set_osBufferLimited(optionsEngine->getOptionValue("osBufferLimited").toBool());
    realObject->set_osBufferLimit(optionsEngine->getOptionValue("osBufferLimit").toUInt());
    realObject->set_setFilters(optionsEngine->getOptionValue("includeStrings").toStringList(),
        optionsEngine->getOptionValue("includeOptions").toStringList(),
        optionsEngine->getOptionValue("excludeStrings").toStringList(),
        optionsEngine->getOptionValue("excludeOptions").toStringList()
    );
    realObject->setRenamingRules(optionsEngine->getOptionValue("firstRenamingRule").toString(),optionsEngine->getOptionValue("otherRenamingRule").toString());
    return newTransferEngine;
}

void Factory::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion)
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"MAX BUFFER BLOCK: "+QString::number(ULTRACOPIER_PLUGIN_MAXBUFFERBLOCK));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"MIN TIMER INTERVAL: "+QString::number(ULTRACOPIER_PLUGIN_MINTIMERINTERVAL));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"MAX TIMER INTERVAL: "+QString::number(ULTRACOPIER_PLUGIN_MAXTIMERINTERVAL));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"NUM SEM SPEED MANAGEMENT: "+QString::number(ULTRACOPIER_PLUGIN_NUMSEMSPEEDMANAGEMENT));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"MAX PARALLEL INODE OPT: "+QString::number(ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"MAX PARALLEL TRANFER: "+QString::number(ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER));
    #if defined (ULTRACOPIER_PLUGIN_CHECKLISTTYPE)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"CHECK LIST TYPE set");
    #else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"CHECK LIST TYPE not set");
    #endif
    if(options!=NULL)
    {
        optionsEngine=options;
        //load the options
        QList<QPair<QString, QVariant> > KeysList;
        KeysList.append(qMakePair(QString("doRightTransfer"),QVariant(false)));
        KeysList.append(qMakePair(QString("keepDate"),QVariant(false)));
        KeysList.append(qMakePair(QString("blockSize"),QVariant(ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE)));//4KB as default
        KeysList.append(qMakePair(QString("autoStart"),QVariant(true)));
        KeysList.append(qMakePair(QString("folderError"),QVariant(0)));
        KeysList.append(qMakePair(QString("folderColision"),QVariant(0)));
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
        #ifdef 	Q_OS_WIN32
        KeysList.append(qMakePair(QString("osBufferLimited"),QVariant(true)));
        #else
        KeysList.append(qMakePair(QString("osBufferLimited"),QVariant(false)));
        #endif
        KeysList.append(qMakePair(QString("osBufferLimit"),QVariant(512)));
        optionsEngine->addOptionGroup(KeysList);
        #if ! defined (Q_CC_GNU)
        ui->keepDate->setEnabled(false);
        ui->keepDate->setToolTip("Not supported with this compiler");
        #endif
        ui->doRightTransfer->setChecked(optionsEngine->getOptionValue("doRightTransfer").toBool());
        ui->keepDate->setChecked(optionsEngine->getOptionValue("keepDate").toBool());
        ui->blockSize->setValue(optionsEngine->getOptionValue("blockSize").toUInt());
        ui->autoStart->setChecked(optionsEngine->getOptionValue("autoStart").toBool());
        ui->comboBoxFolderError->setCurrentIndex(optionsEngine->getOptionValue("folderError").toUInt());
        ui->comboBoxFolderColision->setCurrentIndex(optionsEngine->getOptionValue("folderColision").toUInt());
        ui->checkBoxDestinationFolderExists->setChecked(optionsEngine->getOptionValue("checkDestinationFolder").toBool());

        ui->doChecksum->setChecked(optionsEngine->getOptionValue("doChecksum").toBool());
        ui->checksumIgnoreIfImpossible->setChecked(optionsEngine->getOptionValue("checksumIgnoreIfImpossible").toBool());
        ui->checksumOnlyOnError->setChecked(optionsEngine->getOptionValue("checksumOnlyOnError").toBool());

        ui->osBuffer->setChecked(optionsEngine->getOptionValue("osBuffer").toBool());
        ui->osBufferLimited->setChecked(optionsEngine->getOptionValue("osBufferLimited").toBool());
        ui->osBufferLimit->setValue(optionsEngine->getOptionValue("osBufferLimit").toUInt());
        //ui->autoStart->setChecked(optionsEngine->getOptionValue("autoStart").toBool());//moved from options(), wrong previous place
        filters->setFilters(optionsEngine->getOptionValue("includeStrings").toStringList(),
            optionsEngine->getOptionValue("includeOptions").toStringList(),
            optionsEngine->getOptionValue("excludeStrings").toStringList(),
            optionsEngine->getOptionValue("excludeOptions").toStringList()
        );
        renamingRules->setRenamingRules(optionsEngine->getOptionValue("firstRenamingRule").toString(),optionsEngine->getOptionValue("otherRenamingRule").toString());

        ui->checksumOnlyOnError->setEnabled(ui->doChecksum->isChecked());
        ui->checksumIgnoreIfImpossible->setEnabled(ui->doChecksum->isChecked());

        connect(ui->osBufferLimited,&QAbstractButton::toggled,this,&Factory::updateBufferCheckbox);
        connect(ui->osBuffer,&QAbstractButton::toggled,this,&Factory::updateBufferCheckbox);
        updateBufferCheckbox();
    }
}

QStringList Factory::supportedProtocolsForTheSource()
{
    return QStringList() << "file";
}

QStringList Factory::supportedProtocolsForTheDestination()
{
    return QStringList() << "file";
}

Ultracopier::CopyType Factory::getCopyType()
{
    return Ultracopier::FileAndFolder;
}

Ultracopier::TransferListOperation Factory::getTransferListOperation()
{
    return Ultracopier::TransferListOperation_ImportExport;
}

bool Factory::canDoOnlyCopy()
{
    return false;
}

void Factory::resetOptions()
{
}

QWidget * Factory::options()
{

    return tempWidget;
}

void Factory::setDoRightTransfer(bool doRightTransfer)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("doRightTransfer",doRightTransfer);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::setKeepDate(bool keepDate)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("keepDate",keepDate);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::setBlockSize(int blockSize)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("blockSize",blockSize);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::setAutoStart(bool autoStart)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("autoStart",autoStart);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::newLanguageLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, retranslate the widget options");
    ui->retranslateUi(tempWidget);
    if(optionsEngine!=NULL)
    {
        filters->newLanguageLoaded();
        renamingRules->newLanguageLoaded();
    }
    emit reloadLanguage();
}

void Factory::doChecksum_toggled(bool doChecksum)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("doChecksum",doChecksum);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::checksumOnlyOnError_toggled(bool checksumOnlyOnError)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checksumOnlyOnError",checksumOnlyOnError);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::osBuffer_toggled(bool osBuffer)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("osBuffer",osBuffer);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void Factory::osBufferLimited_toggled(bool osBufferLimited)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("osBufferLimited",osBufferLimited);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void Factory::osBufferLimit_editingFinished()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the spinbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("osBufferLimit",ui->osBufferLimit->value());
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::showFilterDialog()
{
    if(optionsEngine==NULL)
    {
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded, can't access to the filters"));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"options not loaded");
        return;
    }
    filters->exec();
}

void Factory::sendNewFilters(const QStringList &includeStrings,const QStringList &includeOptions,const QStringList &excludeStrings,const QStringList &excludeOptions)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    if(optionsEngine!=NULL)
    {
        optionsEngine->setOptionValue("includeStrings",includeStrings);
        optionsEngine->setOptionValue("includeOptions",includeOptions);
        optionsEngine->setOptionValue("excludeStrings",excludeStrings);
        optionsEngine->setOptionValue("excludeOptions",excludeOptions);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    if(optionsEngine!=NULL)
    {
        optionsEngine->setOptionValue("firstRenamingRule",firstRenamingRule);
        optionsEngine->setOptionValue("otherRenamingRule",otherRenamingRule);
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::showRenamingRules()
{
    if(optionsEngine==NULL)
    {
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded, can't access to the filters"));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"options not loaded");
        return;
    }
    renamingRules->exec();
}

void Factory::updateBufferCheckbox()
{
    ui->osBufferLimited->setEnabled(ui->osBuffer->isChecked());
    ui->osBufferLimit->setEnabled(ui->osBuffer->isChecked() && ui->osBufferLimited->isChecked());
}

void Factory::checksumIgnoreIfImpossible_toggled(bool checksumIgnoreIfImpossible)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the checkbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checksumIgnoreIfImpossible",checksumIgnoreIfImpossible);
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::logicalDriveChanged(const QString &,bool)
{
    QStringList temp=storageInfo.allLogicalDrives();
    for (int i = 0; i < temp.size(); ++i) {
        mountSysPoint<<QDir::toNativeSeparators(temp.at(i));
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"mountSysPoint with Qt: "+mountSysPoint.join(";"));
    emit haveDrive(mountSysPoint);
}
