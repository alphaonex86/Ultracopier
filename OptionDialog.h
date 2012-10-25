/** \file OptionDialog.h
\brief To have an interface to control the options
\author alpha_one_x86
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
    void addPluginOptionWidget(const PluginType &category,const QString &name,QWidget * options);
protected:
    void changeEvent(QEvent *e);
private slots:
    void on_treeWidget_itemSelectionChanged();
    void on_buttonBox_clicked(QAbstractButton *button);
    //plugin management
    void onePluginAdded(const PluginsAvailable &plugin);
    void onePluginWillBeRemoved(const PluginsAvailable &plugin);
    void loadOption();
    void newOptionValue(const QString &group,const QString &name,const QVariant &value);
    void on_Ultracopier_current_theme_currentIndexChanged(int index);
    void on_Language_currentIndexChanged(int index);
    void on_Language_force_toggled(bool checked);
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

    void on_GroupWindowWhen_currentIndexChanged(int index);

private:
    Ui::OptionDialog *ui;
    struct pluginStore
    {
        QTreeWidgetItem * item;
        QString path;
        bool isWritable;
    };
    QList<pluginStore> pluginLink;
    struct pluginOptionsWidget
    {
        QString name;
        QTreeWidgetItem * item;
        QWidget *options;
        PluginType category;
    };
    QList<pluginOptionsWidget> pluginOptionsWidgetList;
    int number_of_listener;
    void addLanguage(PluginsAvailable plugin);
    void removeLanguage(PluginsAvailable plugin);
    void addTheme(PluginsAvailable plugin);
    void removeTheme(PluginsAvailable plugin);
    QStringList copyEngineStringList();
    bool ignoreCopyEngineListEdition;
    PluginsManager::ImportBackend defaultImportBackend;
    int index,loop_size;
    int loadedCopyEnginePlugin;
    QTreeWidgetItem * treeWidgetItem;
    bool allPluginsIsLoaded;
public slots:
    void newThemeOptions(QString name,QWidget* theNewOptionsWidget,bool isLoaded,bool havePlugin);
signals:
    void previouslyPluginAdded(const PluginsAvailable &plugin);
};

#endif // OPTIONDIALOG_H
