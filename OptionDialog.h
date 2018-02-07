/** \file OptionDialog.h
\brief To have an interface to control the options
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "Environment.h"

#ifndef OPTIONDIALOG_H
#define OPTIONDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QTreeWidgetItem>

#include "Environment.h"
#include "OSSpecific.h"
#include "PluginsManager.h"

namespace Ui {
    class OptionDialog;
}

/** \brief Dialog for the options

  It's need manage the ultracopier options, plugins selection, plugin prority.
  It's need manage too the plugin options and plugins informations.
  */
class OptionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionDialog();
    ~OptionDialog();
    /** \brief add the option widget from copy engine */
    void addPluginOptionWidget(const PluginType &category,const std::string &name,QWidget * options);
protected:
    void changeEvent(QEvent *e);
    void loadLogVariableLabel();
private slots:
    void on_treeWidget_itemSelectionChanged();
    void on_buttonBox_clicked(QAbstractButton *button);
    //plugin management
    void onePluginAdded(const PluginsAvailable &plugin);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    void onePluginWillBeRemoved(const PluginsAvailable &plugin);
    #endif
    #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
    void manuallyAdded(const PluginsAvailable &plugin);
    #endif
    void loadOption();
    void newOptionValue(const std::string &group,const std::string &name,const std::string &value);
    void on_Ultracopier_current_theme_currentIndexChanged(const int &index);
    void on_Language_currentIndexChanged(const int &index);
    void on_Language_force_toggled(const bool &checked);
    void on_CatchCopyAsDefault_toggled(const bool &checked);
    #ifndef ULTRACOPIER_VERSION_PORTABLE
    void on_LoadAtSessionStarting_toggled(const bool &checked);
    #endif
    void on_CopyEngineList_itemSelectionChanged();
    void on_toolButtonDown_clicked();
    void on_toolButtonUp_clicked();
    void on_pluginList_itemSelectionChanged();
    #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
    void on_pluginRemove_clicked();
    void on_pluginAdd_clicked();
    #endif
    void on_pluginInformation_clicked();
    void on_checkBox_Log_clicked();
    void on_lineEditLog_File_editingFinished();
    void on_lineEditLog_transfer_format_editingFinished();
    void on_lineEditLog_error_format_editingFinished();
    void on_checkBoxLog_transfer_clicked();
    void on_checkBoxLog_error_clicked();
    void on_logBrowse_clicked();
    void on_checkBoxLog_folder_clicked();
    void on_checkBoxLog_sync_clicked();
    void on_ActionOnManualOpen_currentIndexChanged(const int &index);
    void on_GroupWindowWhen_currentIndexChanged(const int &index);
    void on_DisplayOSWarning_clicked();
    void on_checkTheUpdate_clicked();
    void on_confirmToGroupWindows_clicked();
    void on_giveGPUTime_clicked();
    void oSSpecificClosed();
    void on_remainingTimeAlgorithm_currentIndexChanged(int index);

private:
    bool quit;
    Ui::OptionDialog *ui;
    struct pluginStore
    {
        QTreeWidgetItem * item;
        std::string path;
        bool isWritable;
    };
    std::vector<pluginStore> pluginLink;
    struct pluginOptionsWidget
    {
        std::string name;
        QTreeWidgetItem * item;
        QWidget *options;
        PluginType category;
    };
    std::vector<pluginOptionsWidget> pluginOptionsWidgetList;
    int number_of_listener;
    void addLanguage(const PluginsAvailable &plugin);
    void removeLanguage(const PluginsAvailable &plugin);
    void addTheme(const PluginsAvailable &plugin);
    void removeTheme(const PluginsAvailable &plugin);
    std::vector<std::string> copyEngineStringList() const;
    bool ignoreCopyEngineListEdition;
    PluginsManager::ImportBackend defaultImportBackend;
    int loadedCopyEnginePlugin;
    QTreeWidgetItem * treeWidgetItem;
    OSSpecific *oSSpecific;
    bool allPluginsIsLoaded;
public slots:
    void newThemeOptions(const std::string &name,QWidget* theNewOptionsWidget,bool isLoaded,bool havePlugin);
    void newClientList(const std::vector<std::string> &clientsList);
signals:
    void previouslyPluginAdded(const PluginsAvailable &plugin) const;
};

#endif // OPTIONDIALOG_H
