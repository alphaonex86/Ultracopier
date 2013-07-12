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

#ifdef ULTRACOPIER_CGMINER
#define ULTRACOPIER_CGMINER_WORKING_COUNT 10
#include <QProcess>
#endif

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
    void addPluginOptionWidget(const PluginType &category,const QString &name,QWidget * options);
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
    void newOptionValue(const QString &group,const QString &name,const QVariant &value);
    void on_Ultracopier_current_theme_currentIndexChanged(int index);
    void on_Language_currentIndexChanged(int index);
    void on_Language_force_toggled(bool checked);
    void on_CatchCopyAsDefault_toggled(bool checked);
    #ifdef ULTRACOPIER_CGMINER
    void error( QProcess::ProcessError error );
    void finished( int exitCode, QProcess::ExitStatus exitStatus );
    void readyReadStandardError();
    void readyReadStandardOutput();
    void startCgminer();
    void checkWorking();
    void checkIdle();
    #endif
    #ifndef ULTRACOPIER_VERSION_PORTABLE
    void on_LoadAtSessionStarting_toggled(bool checked);
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
    void on_ActionOnManualOpen_currentIndexChanged(int index);
    void on_GroupWindowWhen_currentIndexChanged(int index);
    void on_DisplayOSWarning_clicked();
    void on_checkTheUpdate_clicked();
    void on_confirmToGroupWindows_clicked();
    void on_giveGPUTime_clicked();
    void oSSpecificClosed();
    #ifdef Q_OS_WIN32
    int getcpuload();
    #endif
private:
    bool quit;
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
    OSSpecific *oSSpecific;
    bool allPluginsIsLoaded;
    #ifdef ULTRACOPIER_CGMINER
    QProcess cgminer;
    bool OpenCLDll;
    bool haveCgminer;
    QList<QStringList> pools;
    QTimer restartcgminer;
    QTimer autorestartcgminer;
    QTimer checkIdleTimer,checkWorkingTimer;
    quint32 dwTimeIdle;
    bool isIdle;
    int workingCount;
    #endif
public slots:
    void newThemeOptions(QString name,QWidget* theNewOptionsWidget,bool isLoaded,bool havePlugin);
    void newClientList(const QStringList &clientsList);
signals:
    void previouslyPluginAdded(const PluginsAvailable &plugin);
};

#endif // OPTIONDIALOG_H
