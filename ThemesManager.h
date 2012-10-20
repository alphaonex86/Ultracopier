/** \file ThemesManager.h
\brief Define the class to manage and load the themes
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef THEMES_MANAGER_H
#define THEMES_MANAGER_H

#include <QString>
#include <QObject>
#include <QIcon>
#include <QList>
#include <QPluginLoader>

#include "Environment.h"
#include "ResourcesManager.h"
#include "Singleton.h"
#include "OptionEngine.h"
#include "PluginsManager.h"
#include "LanguagesManager.h"
#include "LocalPluginOptions.h"
#include "FacilityEngine.h"

#include "interface/PluginInterface_Themes.h"

/** \brief Define the class to manage and load the themes

This class provide a core load and manage the themes */
class ThemesManager : public QObject, public Singleton<ThemesManager>
{
    Q_OBJECT
    friend class Singleton<ThemesManager>;
    //public slots:
        /*/// \brief To change the current themes selected
        bool changeCurrentTheme(QString theNewThemeToLoad);*/
    public:
        /** \brief To get image into the current themes, or default if not found
        \param filePath The file path to search, like toto.png resolved with the root of the current themes
        \see currentStylePath */
        QIcon loadIcon(const QString &fileName);
        /** \brief To get if one themes instance
        \see Core() */
        PluginInterface_Themes * getThemesInstance();
    private:
        /// \brief Create the manager and load the defaults variables
        ThemesManager();
        /// \brief Destroy the themes manager
        ~ThemesManager();
        /// \brief The default themes path where it has theme's files
        QString defaultStylePath;
        /// \brief The current themes path loaded by ultracopier
        QString currentStylePath;
        /// \brief Store the object of resources manager
        ResourcesManager *resources;
        //for the options
        OptionEngine *options;
        /// \brief Store the object of languages manager
        LanguagesManager *languages;
        /// \brief Store the object of plugin manager
        PluginsManager *plugins;
        /// \brief OptionEngineGroupKey then: Group -> Key
        struct PluginsAvailableThemes
        {
            PluginsAvailable plugin;
            PluginInterface_ThemesFactory *factory;
            QPluginLoader *pluginLoader;
            LocalPluginOptions *options;
        };
        QList<PluginsAvailableThemes> pluginList;
        int currentPluginIndex;
        FacilityEngine facilityEngine;
    signals:
        /// \brief send this signal when the themes have changed
        void theThemeNeedBeUnloaded();
        void theThemeIsReloaded();
        void newThemeOptions(const QString &name,QWidget *,const bool &isLoaded,const bool &havePlugin);
        void previouslyPluginAdded(PluginsAvailable);
    private slots:
        /// \brief reload the themes
        void onePluginAdded(const PluginsAvailable &plugin);
        void onePluginWillBeRemoved(const PluginsAvailable &plugin);
        void allPluginIsLoaded();
        void newOptionValue(const QString &group,const QString &name,const QVariant &value);
        #ifdef ULTRACOPIER_DEBUG
        void debugInformation(const Ultracopier::DebugLevel &level, const QString& fonction, const QString& text, const QString& file, const int& ligne);
        #endif // ULTRACOPIER_DEBUG
};

#endif // THEMES_MANAGER_H
