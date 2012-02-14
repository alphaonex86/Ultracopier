/** \file factory.cpp
\brief Define the factory
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QtCore>
#include <QFileDialog>

#include "factory.h"

/// \todo connect in global options the change

Factory::Factory() :
	ui(new Ui::options())
{
	tempWidget=new QWidget();
	ui->setupUi(tempWidget);
	errorFound=false;
	optionsEngine=NULL;
	#if defined (Q_OS_WIN32)
	QFileInfoList temp=QDir::drives();
	for (int i = 0; i < temp.size(); ++i) {
		mountSysPoint<<temp.at(i).filePath();
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"mountSysPoint: "+mountSysPoint.join(";"));
	#elif defined (Q_OS_LINUX)
	connect(&mount,SIGNAL(error(QProcess::ProcessError)),		this,SLOT(error(QProcess::ProcessError)));
	connect(&mount,SIGNAL(finished(int,QProcess::ExitStatus)),	this,SLOT(finished(int,QProcess::ExitStatus)));
	connect(&mount,SIGNAL(readyReadStandardOutput()),		this,SLOT(readyReadStandardOutput()));
	connect(&mount,SIGNAL(readyReadStandardError()),		this,SLOT(readyReadStandardError()));
	mount.start("mount");
	#endif
	connect(ui->doRightTransfer,	SIGNAL(toggled(bool)),		this,SLOT(setDoRightTransfer(bool)));
	connect(ui->keepDate,		SIGNAL(toggled(bool)),		this,SLOT(setKeepDate(bool)));
	connect(ui->blockSize,		SIGNAL(valueChanged(int)),	this,SLOT(setBlockSize(int)));
	connect(ui->autoStart,		SIGNAL(toggled(bool)),		this,SLOT(setAutoStart(bool)));
}

Factory::~Factory()
{
	delete ui;
}

PluginInterface_CopyEngine * Factory::getInstance()
{
	copyEngine *realObject=new copyEngine(facilityEngine);
	realObject->setDrive(mountSysPoint);
	PluginInterface_CopyEngine * newTransferEngine=realObject;
	connect(newTransferEngine,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)),this,SIGNAL(debugInformation(DebugLevel,QString,QString,QString,int)));
	connect(this,SIGNAL(reloadLanguage()),newTransferEngine,SLOT(newLanguageLoaded()));
	realObject->setRightTransfer(		optionsEngine->getOptionValue("doRightTransfer").toBool());
	realObject->setKeepDate(		optionsEngine->getOptionValue("keepDate").toBool());
	realObject->setBlockSize(		optionsEngine->getOptionValue("blockSize").toInt());
	realObject->setAutoStart(		optionsEngine->getOptionValue("autoStart").toBool());
	realObject->on_comboBoxFolderColision_currentIndexChanged(ui->comboBoxFolderColision->currentIndex());
	realObject->on_comboBoxFolderError_currentIndexChanged(ui->comboBoxFolderError->currentIndex());
	realObject->setCheckDestinationFolderExists(	optionsEngine->getOptionValue("checkDestinationFolder").toBool());
	return newTransferEngine;
}

void Factory::setResources(OptionInterface * optionsEngine,QString writePath,QString pluginPath,FacilityInterface * facilityInterface,bool portableVersion)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath:"+pluginPath);
	this->facilityEngine=facilityInterface;
	Q_UNUSED(portableVersion);
	#ifndef ULTRACOPIER_PLUGIN_DEBUG
		Q_UNUSED(writePath);
		Q_UNUSED(pluginPath);
	#endif
	#if ! defined (Q_CC_GNU)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"Unable to change date time of files, only gcc is supported");
	#endif
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,COMPILERINFO);
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"MAX BUFFER BLOCK: "+QString::number(ULTRACOPIER_PLUGIN_MAXBUFFERBLOCK));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"MIN TIMER INTERVAL: "+QString::number(ULTRACOPIER_PLUGIN_MINTIMERINTERVAL));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"MAX TIMER INTERVAL: "+QString::number(ULTRACOPIER_PLUGIN_MAXTIMERINTERVAL));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"NUM SEM SPEED MANAGEMENT: "+QString::number(ULTRACOPIER_PLUGIN_NUMSEMSPEEDMANAGEMENT));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"MAX PARALLEL INODE OPT: "+QString::number(ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT));
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"MAX PARALLEL TRANFER: "+QString::number(ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER));
	#if defined (ULTRACOPIER_PLUGIN_CHECKLISTTYPE)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"CHECK LIST TYPE set");
	#else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Information,"CHECK LIST TYPE not set");
	#endif
	if(optionsEngine!=NULL)
	{
		this->optionsEngine=optionsEngine;
		//load the options
		QList<QPair<QString, QVariant> > KeysList;
		KeysList.append(qMakePair(QString("doRightTransfer"),QVariant(true)));
		KeysList.append(qMakePair(QString("keepDate"),QVariant(true)));
		KeysList.append(qMakePair(QString("blockSize"),QVariant(1024)));//1024KB as default
		KeysList.append(qMakePair(QString("autoStart"),QVariant(true)));
		KeysList.append(qMakePair(QString("folderError"),QVariant(0)));
		KeysList.append(qMakePair(QString("folderColision"),QVariant(0)));
		KeysList.append(qMakePair(QString("checkDestinationFolder"),QVariant(true)));
		optionsEngine->addOptionGroup(KeysList);
		#if ! defined (Q_CC_GNU)
		ui->keepDate->setEnabled(false);
		ui->keepDate->setToolTip("Not supported with this compiler");
		#endif
		ui->doRightTransfer->setChecked(optionsEngine->getOptionValue("doRightTransfer").toBool());
		ui->keepDate->setChecked(optionsEngine->getOptionValue("keepDate").toBool());
		ui->blockSize->setValue(optionsEngine->getOptionValue("blockSize").toInt());
		ui->autoStart->setChecked(optionsEngine->getOptionValue("autoStart").toBool());
		ui->comboBoxFolderError->setCurrentIndex(optionsEngine->getOptionValue("folderError").toInt());
		ui->comboBoxFolderColision->setCurrentIndex(optionsEngine->getOptionValue("folderColision").toInt());
		ui->checkBoxDestinationFolderExists->setChecked(optionsEngine->getOptionValue("checkDestinationFolder").toBool());
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

CopyType Factory::getCopyType()
{
	return FileAndFolder;
}

bool Factory::canDoOnlyCopy()
{
	return false;
}

void Factory::error(QProcess::ProcessError error)
{
	#ifndef ULTRACOPIER_PLUGIN_DEBUG
		Q_UNUSED(error)
	#endif
	errorFound=true;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"have detected error: "+QString::number(error));
}

void Factory::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"exitCode: "+QString::number(exitCode)+", exitStatus: "+QString::number(exitStatus));
	#ifndef ULTRACOPIER_PLUGIN_DEBUG
		Q_UNUSED(exitCode)
		Q_UNUSED(exitStatus)
	#endif
	if(!StandardError.isEmpty())
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"have finished with text on error output: "+StandardError);
	else if(errorFound)
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"have finished with error and no text");
	{
		QStringList tempList=StandardOutput.split(QRegExp("[\n\r]+"));
		int index=0;
		while(index<tempList.size())
		{
			QString newString=tempList.at(index);
			newString=newString.remove(QRegExp("^.* on "));
			newString=newString.remove(QRegExp(" type .*$"));
			if(!newString.endsWith(QDir::separator()))
				newString+=QDir::separator();
			mountSysPoint<<newString;
			index++;
		}
		mountSysPoint.removeDuplicates();
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"mountSysPoint: "+mountSysPoint.join(";"));
	}
}

void Factory::readyReadStandardError()
{
	StandardError+=mount.readAllStandardError();
}

void Factory::readyReadStandardOutput()
{
	StandardOutput+=mount.readAllStandardOutput();
}

void Factory::resetOptions()
{
}

QWidget * Factory::options()
{
	ui->autoStart->setChecked(optionsEngine->getOptionValue("autoStart").toBool());
	return tempWidget;
}

void Factory::setDoRightTransfer(bool doRightTransfer)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the checkbox have changed");
	if(optionsEngine!=NULL)
		optionsEngine->setOptionValue("doRightTransfer",doRightTransfer);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::setKeepDate(bool keepDate)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the checkbox have changed");
	if(optionsEngine!=NULL)
		optionsEngine->setOptionValue("keepDate",keepDate);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::setBlockSize(int blockSize)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the checkbox have changed");
	if(optionsEngine!=NULL)
		optionsEngine->setOptionValue("blockSize",blockSize);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::setAutoStart(bool autoStart)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"the checkbox have changed");
	if(optionsEngine!=NULL)
		optionsEngine->setOptionValue("autoStart",autoStart);
	else
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Critical,"internal error, crash prevented");
}

void Factory::newLanguageLoaded()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start, retranslate the widget options");
	ui->retranslateUi(tempWidget);
	emit reloadLanguage();
}

Q_EXPORT_PLUGIN2(copyEngine, Factory);
