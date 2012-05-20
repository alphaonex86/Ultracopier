/** \file factory.h
\brief Define the factory to create new instance
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
#include "Filters.h"
#include "RenamingRules.h"

#ifndef FACTORY_H
#define FACTORY_H

namespace Ui {
	class options;
}

/** \brief to generate copy engine instance */
class Factory : public PluginInterface_CopyEngineFactory
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface_CopyEngineFactory)
public:
	Factory();
	~Factory();
	/// \brief to return the instance of the copy engine
	PluginInterface_CopyEngine * getInstance();
	/// \brief set the resources, to store options, to have facilityInterface
	void setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,FacilityInterface * facilityInterface,const bool &portableVersion);
	//get mode allowed
	/// \brief define if can copy file, folder or both
	CopyType getCopyType();
	/// \brief to return which kind of transfer list operation is supported
	TransferListOperation getTransferListOperation();
	/// \brief define if can only copy, or copy and move
	bool canDoOnlyCopy();
	/// \brief to get the supported protocols for the source
	QStringList supportedProtocolsForTheSource();
	/// \brief to get the supported protocols for the destination
	QStringList supportedProtocolsForTheDestination();
	/// \brief to get the options of the copy engine
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
	Filters *filters;
	RenamingRules *renamingRules;
private slots:
	void error(QProcess::ProcessError error);
	void finished(int exitCode, QProcess::ExitStatus exitStatus);
	void readyReadStandardError();
	void readyReadStandardOutput();
	void setDoRightTransfer(bool doRightTransfer);
	void setKeepDate(bool keepDate);
	void setBlockSize(int blockSize);
	void setAutoStart(bool autoStart);
	void showFilterDialog();
	void sendNewFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions);
	void doChecksum_toggled(bool);
	void checksumOnlyOnError_toggled(bool);
	void osBuffer_toggled(bool);
	void osBufferLimited_toggled(bool);
	void osBufferLimit_editingFinished();
	void checksumIgnoreIfImpossible_toggled(bool);
	void sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
	void showRenamingRules();
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
