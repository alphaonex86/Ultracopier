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
#include <map>
#include <regex>

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
        std::vector<PluginsAvailable> getPluginsByCategory(const PluginType &type) const;
        /** \brief to get plugins */
        std::vector<PluginsAvailable> getPlugins(bool withError=false) const;
        /// \brief get translated text
        //QString getTranslatedText(PluginsAvailable plugin,QString informationName,QString mainShortName);
        //QString getTranslatedText(PluginsAvailable plugin,QString informationName);
        /// \brief transform short plugin name into file name
        static std::string getResolvedPluginName(const std::string &name);
        static bool isSamePlugin(const PluginsAvailable &pluginA,const PluginsAvailable &pluginB);
        void lockPluginListEdition();
        void unlockPluginListEdition();
        bool allPluginHaveBeenLoaded() const;
        /// \brief to load the get dom specific
        std::string getDomSpecific(const QDomElement &root,const std::string &name,const std::vector<std::pair<std::string,std::string> > &listChildAttribute) const;
        std::string getDomSpecific(const QDomElement &root,const std::string &name) const;
        /// \brief set current language
        void setLanguage(const std::string &language);
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
        /// \brief To compare version, \return true is case of error
        static bool compareVersion(const std::string &versionA,const std::string &sign,const std::string &versionB);
    private:
        /// \brief List of plugins
        std::vector<PluginsAvailable> pluginsList;
        std::map<PluginType,std::vector<PluginsAvailable> > pluginsListIndexed;
        /// \brief to load the multi-language balise
        void loadBalise(const QDomElement &root,const std::string &name,std::vector<std::vector<std::string> > *informations,std::string *errorString,bool needHaveOneEntryMinimum=true,bool multiLanguage=false,bool englishNeedBeFound=false);
        /// \brief get the version
        std::string getPluginVersion(const std::string &pluginName) const;
        /// \brief list of cat plugin type
        //QStringList catPlugin;
        std::vector<std::string> englishPluginType;
        std::vector<QTreeWidgetItem *> catItemList;
        /// \brief store the current mainShortName
        std::string mainShortName;
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
        uint32_t checkDependencies();
        #endif
        void loadPluginXml(PluginsAvailable * thePlugin,const QByteArray &xml);
        std::vector<std::string> readPluginPath;
        bool loadPluginInformation(const std::string &path);
        QSemaphore editionSemList;
        bool stopIt;
        bool pluginLoaded;
        std::string language;
        std::string categoryToString(const PluginType &category) const;
        std::string categoryToTranslation(const PluginType &category);
        std::regex regexp_to_clean_1,regexp_to_clean_2,regexp_to_clean_3,regexp_to_clean_4,regexp_to_clean_5;
        std::regex regexp_to_dep_1,regexp_to_dep_2,regexp_to_dep_3,regexp_to_dep_4,regexp_to_dep_5,regexp_to_dep_6;
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
        void pluginListingIsfinish() const;
        void onePluginAdded(const PluginsAvailable&) const;
        void onePluginInErrorAdded(const PluginsAvailable&) const;
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        void onePluginWillBeRemoved(const PluginsAvailable&) const; // when will be really removed
        void onePluginWillBeUnloaded(const PluginsAvailable&) const;//just unload to quit the application
        #endif
        void needLangToRefreshPluginList() const;
        void newLanguageLoaded() const;
        #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
        void manuallyAdded(const PluginsAvailable&) const;
        #endif
    protected:
        void run();
    public slots: //do gui action
        void showInformation(const std::string &path);
        #ifdef ULTRACOPIER_PLUGIN_IMPORT_SUPPORT
        void removeThePluginSelected(const QString &path);
        void addPlugin(const ImportBackend &backend);
        void tryLoadPlugin(const QString &file);
        #endif
};

/// \brief to do structure comparaison
bool operator==(PluginsAvailable pluginA,PluginsAvailable pluginB);

#endif // PLUGINS_MANAGER_H
