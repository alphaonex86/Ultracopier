/** \file OptionDialog.h
\brief To have an interface to control the options
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#ifndef OPTIONDIALOG_H
#define OPTIONDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include "GlobalClass.h"

namespace Ui {
    class OptionDialog;
}

/** \brief Dialog for the options

  It's need manage the ultracopier options, plugins selection, plugin prority.
  It's need manage too the plugin options and plugins informations.
  */
class OptionDialog : public QDialog, public GlobalClass
{
	Q_OBJECT
public:
	explicit OptionDialog();
	~OptionDialog();
	/** \brief add the option widget from copy engine */
	void addCopyEngineWidget(QString name,QWidget * options);
	/** \brief remove the option widget from copy engine */
	void removeCopyEngineWidget(QString name);
protected:
	void changeEvent(QEvent *e);
private slots:
	void on_treeWidget_itemSelectionChanged();
	void on_buttonBox_clicked(QAbstractButton *button);
	//plugin management
	void onePluginAdded(PluginsAvailable plugin);
	void onePluginWillBeRemoved(PluginsAvailable plugin);
	void loadOption();
	void newOptionValue(QString group,QString name,QVariant value);
	void on_Ultracopier_current_theme_currentIndexChanged(int index);
	void on_Language_currentIndexChanged(int index);
	void on_Language_autodetect_toggled(bool checked);
	void on_CatchCopyAsDefault_toggled(bool checked);
	void on_LoadAtSessionStarting_toggled(bool checked);
	void on_CopyEngineList_itemSelectionChanged();
	void on_toolButtonDown_clicked();
	void on_toolButtonUp_clicked();
	void on_pluginList_itemSelectionChanged();
	void on_pluginRemove_clicked();
	void on_pluginInformation_clicked();
	void on_pluginAdd_clicked();
	void on_checkBox_Log_clicked();
	void on_lineEditLog_File_editingFinished();
	void on_lineEditLog_transfer_format_editingFinished();
	void on_lineEditLog_error_format_editingFinished();
	void on_checkBoxLog_transfer_clicked();
	void on_checkBoxLog_error_clicked();
	void on_pushButton_clicked();
	void on_checkBoxLog_folder_clicked();
	void on_checkBoxLog_sync_clicked();
	void on_ActionOnManualOpen_currentIndexChanged(int index);

private:
	Ui::OptionDialog *ui;
	struct pluginStore
	{
		QTreeWidgetItem * item;
		QString path;
		bool isWritable;
	};
	QList<pluginStore> pluginLink;
	struct pluginCopyEngine
	{
		QString name;
		QTreeWidgetItem * item;
		QWidget *options;
	};
	QList<pluginCopyEngine> copyEngineList;
	void addLanguage(PluginsAvailable plugin);
	void removeLanguage(PluginsAvailable plugin);
	void addTheme(PluginsAvailable plugin);
	void removeTheme(PluginsAvailable plugin);
	void removeCopyEngine(PluginsAvailable plugin);
	QStringList copyEngineStringList();
	bool ignoreCopyEngineListEdition;
	PluginsManager::ImportBackend defaultImportBackend;
	int index,loop_size;
	int loadedCopyEnginePlugin;
	QTreeWidgetItem * treeWidgetItem;
public slots:
	void newThemeOptions(QWidget* theNewOptionsWidget,bool isLoaded,bool havePlugin);
};

#endif // OPTIONDIALOG_H
