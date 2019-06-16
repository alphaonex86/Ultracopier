/** \file factory.h
\brief Define the factory, to create instance of the interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef FACTORYTHEMEOXYGEN_H
#define FACTORYTHEMEOXYGEN_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QCloseEvent>
#include <QFile>
#include <QIcon>
#include <QColor>
#include <QPixmap>

#include "../../../interface/PluginInterface_Themes.h"
#include "ui_themesOptions.h"
#include "interface.h"
#include "Environment.h"

namespace Ui {
    class themesOptions;
}

/// \brief Define the factory, to create instance of the interface
class ThemesFactory : public PluginInterface_ThemesFactory
{
    Q_OBJECT
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
    Q_PLUGIN_METADATA(IID "first-world.info.ultracopier.PluginInterface.ThemesFactory/1.0.1.0" FILE "plugin.json")
    Q_INTERFACES(PluginInterface_ThemesFactory)
    #endif
public:
    ThemesFactory();
    ~ThemesFactory();
    /// \brief to return the instance of the copy engine
    PluginInterface_Themes * getInstance();
    /// \brief set the resources, to store options, to have facilityInterface
    void setResources(OptionInterface * optionsEngine,const std::string &
                  #ifdef ULTRACOPIER_PLUGIN_DEBUG
                  writePath
                  #endif
                  ,const std::string &
                  #ifdef ULTRACOPIER_PLUGIN_DEBUG
                  pluginPath
                  #endif
                  ,FacilityInterface * facilityEngine,const bool &portableVersion);
    /// \brief to get the default options widget
    QWidget * options();
    /// \brief to get a resource icon
    QIcon getIcon(const std::string &fileName) const;
private slots:
    void checkBoxShowSpeedHaveChanged(bool toggled);
    void checkBoxStartWithMoreButtonPushedHaveChanged(bool toggled);
    void comboBox_copyEnd(int value);
    void speedWithProgressBar(bool toggled);
    void checkBoxShowSpeed(bool checked);
    void minimizeToSystray(bool checked);
    void alwaysOnTop(bool checked);
    void showDualProgression(bool checked);
    void on_SliderSpeed_valueChanged(int value);
    void uiUpdateSpeed();
    void updateSpeed();
    void progressColorWrite_clicked();
    void progressColorRead_clicked();
    void progressColorRemaining_clicked();
    void updateProgressionColorBar();
    void setShowProgressionInTheTitle();
    void startMinimized(bool checked);
    void savePositionBeforeClose(QObject *obj);
    void savePositionHaveChanged(bool checked);
public slots:
    void resetOptions();
    void newLanguageLoaded();
private:
    OptionInterface * optionsEngine;
    Ui::themesOptions *ui;
    QWidget *tempWidget;
    FacilityInterface * facilityEngine;
    int32_t currentSpeed;///< in KB/s, assume as 0KB/s as default like every where
    QColor progressColorWrite,progressColorRead,progressColorRemaining;
signals:
    void reloadLanguage() const;
};

#endif // FACTORY_H
