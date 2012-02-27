/** \file factory.h
\brief Define the factory
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include <QProcess>

#include "../../../interface/PluginInterface_CopyEngine.h"
#include "StructEnumDefinition.h"
#include "ui_options.h"
#include "copyEngine.h"
#include "Environment.h"

#ifndef FACTORY_H
#define FACTORY_H

namespace Ui {
	class options;
}

class Factory : public PluginInterface_CopyEngineFactory
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_CopyEngineFactory)
public:
	Factory();
	~Factory();
	PluginInterface_CopyEngine * getInstance();
	void setResources(OptionInterface * options,QString writePath,QString pluginPath,FacilityInterface * facilityInterface,bool portableVersion);
	//get mode allowed
	/// \brief define if can copy file, folder or both
	CopyType getCopyType();
	TransferListOperation getTransferListOperation();
	bool canDoOnlyCopy();
	QStringList supportedProtocolsForTheSource();
	QStringList supportedProtocolsForTheDestination();
	QWidget * options();
private:
	Ui::options *ui;
	QWidget* tempWidget;
	OptionInterface * optionsEngine;
	QStringList mountSysPoint;
	QProcess mount;
	QString StandardError;
	QString StandardOutput;
	bool errorFound;
	FacilityInterface * facilityEngine;
private slots:
	void error(QProcess::ProcessError error);
	void finished(int exitCode, QProcess::ExitStatus exitStatus);
	void readyReadStandardError();
	void readyReadStandardOutput();
	void setDoRightTransfer(bool doRightTransfer);
	void setKeepDate(bool keepDate);
	void setBlockSize(int blockSize);
	void setAutoStart(bool autoStart);
public slots:
	void resetOptions();
	void newLanguageLoaded();
signals:
	void reloadLanguage();
	#ifdef ULTRACOPIER_PLUGIN_DEBUG
	/// \brief To debug source
	void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
	#endif
};

#endif // FACTORY_H
