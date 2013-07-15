/** \file PluginsManager.h
\brief Define the class to manage and load the plugins
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGINS_MANAGER_H
#define PLUGINS_MANAGER_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QDomElement>
#include <QDomDocument>
#include <QDomNode>
#include <QTreeWidgetItem>
#include <QDateTime>
#include <QAction>
#include <QMenu>
#include <QCryptographicHash>
#include <QString>
#include <QSemaphore>
#include <QRegularExpression>
#include <QThread>

#include "Environment.h"
#include "OptionEngine.h"
#include "ResourcesManager.h"
#include "PluginInformation.h"
#ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
#include "QXzDecodeThread.h"
#include "QTarDecode.h"
#endif

namespace Ui {
    class PluginOptions;
}

/** \brief Define the class to manage and load the resources linked with the themes

This class provide a core load and manage the resources */
class PluginsManager : public QThread
{
    Q_OBJECT
    public:
        /// \brief to get plugins of type specific
        QList<PluginsAvailable> getPluginsByCategory(const PluginType &type) const;
        /** \brief to get plugins */
        QList<PluginsAvailable> getPlugins(bool withError=false) const;
        /// \brief get translated text
        //QString getTranslatedText(PluginsAvailable plugin,QString informationName,QString mainShortName);
        //QString getTranslatedText(PluginsAvailable plugin,QString informationName);
        /// \brief transform short plugin name into file name
        static QString getResolvedPluginName(const QString &name);
        static bool isSamePlugin(const PluginsAvailable &pluginA,const PluginsAvailable &pluginB);
        void lockPluginListEdition();
        void unlockPluginListEdition();
        bool allPluginHaveBeenLoaded() const;
        /// \brief to load the get dom specific
        QString getDomSpecific(const QDomElement &root,const QString &name,const QList<QPair<QString,QString> > &listChildAttribute) const;
        QString getDomSpecific(const QDomElement &root,const QString &name) const;
        /// \brief set current language
        void setLanguage(const QString &language);
        /// \brief Enumeration of plugin add backend
        enum ImportBackend
        {
            ImportBackend_File,		//import plugin from local file
            ImportBackend_Internet	//import plugin form internet
        };
        static PluginsManager *pluginsManager;
        /// \brief Create the manager and load the defaults variables
        PluginsManager();
        /// \brief Destroy the language manager
        ~PluginsManager();
        /// \brief To compare version
        static bool compareVersion(const QString &versionA,const QString &sign,const QString &versionB);
    private:
        /// \brief List of plugins
        QList<PluginsAvailable> pluginsList;
        QMultiMap<PluginType,PluginsAvailable> pluginsListIndexed;
        /// \brief to load the multi-language balise
        void loadBalise(const QDomElement &root,const QString &name,QList<QStringList> *informations,QString *errorString,bool needHaveOneEntryMinimum=true,bool multiLanguage=false,bool englishNeedBeFound=false);
        /// \brief get the version
        QString getPluginVersion(const QString &pluginName) const;
        /// \brief list of cat plugin type
        //QStringList catPlugin;
        QStringList englishPluginType;
        QList<QTreeWidgetItem *> catItemList;
        /// \brief store the current mainShortName
        QString mainShortName;
        /// \brief load the plugin list
        void loadPluginList();
        #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
        QAction *backendMenuFile;		///< Pointer on the file backend menu
        bool importingPlugin;
        void lunchDecodeThread(const QByteArray &data);
        QXzDecodeThread decodeThread;
        void executeTheFileBackendLoader();
        #endif
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        /// \brief check the dependencies, return number of error
        quint32 checkDependencies();
        #endif
        void loadPluginXml(PluginsAvailable * thePlugin,const QByteArray &xml);
        QStringList readPluginPath;
        bool loadPluginInformation(const QString &path);
        QSemaphore editionSemList;
        bool stopIt;
        bool pluginLoaded;
        QString language;
        QString categoryToString(const PluginType &category) const;
        QString categoryToTranslation(const PluginType &category);
        QRegularExpression regexp_to_clean_1,regexp_to_clean_2,regexp_to_clean_3,regexp_to_clean_4,regexp_to_clean_5;
        QRegularExpression regexp_to_dep_1,regexp_to_dep_2,regexp_to_dep_3,regexp_to_dep_4,regexp_to_dep_5,regexp_to_dep_6;
        PluginInformation *pluginInformation;
    private slots:
        /// \brief show the information
        void showInformationDoubleClick();
        #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
        void decodingFinished();
        #endif
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        void newAuthPath(const QString &path);
        #endif
        void post_operation();
/*	public slots:
        /// \brief to refresh the plugin list
        void refreshPluginList(QString mainShortName="en");*/
    signals:
        void pluginListingIsfinish();
        void onePluginAdded(const PluginsAvailable&);
        void onePluginInErrorAdded(const PluginsAvailable&);
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        void onePluginWillBeRemoved(const PluginsAvailable&); // when will be really removed
        void onePluginWillBeUnloaded(const PluginsAvailable&);//just unload to quit the application
        #endif
        void needLangToRefreshPluginList();
        void newLanguageLoaded();
        #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
        void manuallyAdded(const PluginsAvailable&);
        #endif
    protected:
        void run();
    public slots: //do gui action
        void showInformation(const QString &path);
        #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
        void removeThePluginSelected(const QString &path);
        void addPlugin(const ImportBackend &backend);
        void tryLoadPlugin(const QString &file);
        #endif
};

/// \brief to do structure comparaison
bool operator==(PluginsAvailable pluginA,PluginsAvailable pluginB);

#endif // PLUGINS_MANAGER_H
