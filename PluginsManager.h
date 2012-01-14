/** \file PluginsManager.h
\brief Define the class to manage and load the plugins
\author alpha_one_x86
\version 0.3
\date 2010 */ 

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

#include "Environment.h"
#include "Singleton.h"
#include "OptionEngine.h"
#include "ResourcesManager.h"
#include "PluginInformation.h"
#include "QXzDecodeThread.h"
#include "QTarDecode.h"
#include "AuthPlugin.h"

namespace Ui {
    class PluginOptions;
}

/** \brief Define the class to manage and load the resources linked with the themes

This class provide a core load and manage the resources */
class PluginsManager : public QThread, public Singleton<PluginsManager>
{
	Q_OBJECT
	friend class Singleton<PluginsManager>;
	public:
		/// \brief to get plugins of type specific
		QList<PluginsAvailable> getPluginsByCategory(PluginType type);
		/** \brief to get plugins
		  \todo check where is used to convert to PluginsAvailable
		  */
		QList<PluginsAvailable> getPlugins();
		/// \brief get translated text
		//QString getTranslatedText(PluginsAvailable plugin,QString informationName,QString mainShortName);
		//QString getTranslatedText(PluginsAvailable plugin,QString informationName);
		/// \brief transform short plugin name into file name
		static QString getResolvedPluginName(QString name);
		static bool isSamePlugin(PluginsAvailable pluginA,PluginsAvailable pluginB);
		void lockPluginListEdition();
		void unlockPluginListEdition();
		bool allPluginHaveBeenLoaded();
		/// \brief to load the get dom specific
		QString getDomSpecific(QDomElement root,QString name,QList<QPair<QString,QString> > listChildAttribute);
		QString getDomSpecific(QDomElement root,QString name);
		/// \brief set current language
		void setLanguage(QString language);
		/// \brief Enumeration of plugin add backend
		enum ImportBackend
		{
			ImportBackend_File,		//import plugin from local file
			ImportBackend_Internet	//import plugin form internet
		};
	private:
		/// \brief Create the manager and load the defaults variables
		PluginsManager();
		/// \brief Destroy the language manager
		~PluginsManager();
		/// \brief get informations text
		//QString getInformationText(PluginsAvailable plugin,QString informationName);
		//for the options
		OptionEngine *options;
		/// \brief Store the object of resources manager
		ResourcesManager *resources;
		/// \brief List of plugins
		QList<PluginsAvailable> pluginsList;
		/// \brief to load the multi-language balise
		void loadBalise(QDomElement root,QString name,QList<QStringList> *informations,QString *errorString,bool needHaveOneEntryMinimum=true,bool multiLanguage=false,bool englishNeedBeFound=false);
		/// \brief check the dependencies
		void checkDependencies();
		/// \brief get the version
		QString getPluginVersion(QString pluginName);
		/// \brief To compare version
		bool compareVersion(QString versionA,QString sign,QString versionB);
		/// \brief plugin information windows
		PluginInformation pluginInformationWindows;
		/// \brief list of cat plugin type
		//QStringList catPlugin;
		QStringList englishPluginType;
		QList<QTreeWidgetItem *> catItemList;
		/// \brief store the current mainShortName
		QString mainShortName;
		/// \brief load the plugin list
		void loadPluginList();
		QAction *backendMenuFile;		///< Pointer on the file backend menu
		bool importingPlugin;
		void lunchDecodeThread(QByteArray data);
		QXzDecodeThread decodeThread;
		void loadPluginXml(PluginsAvailable * thePlugin,QByteArray xml);
		AuthPlugin *checkPluginThread;
		QStringList readPluginPath;
		bool loadPluginInformation(QString path);
		QSemaphore editionSemList;
		bool stopIt;
		bool pluginLoaded;
		QString language;
		void excuteTheFileBackendLoader();
		QString categoryToString(PluginType category);
		QString categoryToTranslation(PluginType category);
		//temp variable
		int index,loop_size,sub_index,loop_sub_size;
	private slots:
		/// \brief show the information
		void showInformationDoubleClick();
		void decodingFinished();
		void newAuthPath(QString path);
		void post_operation();
/*	public slots:
		/// \brief to refresh the plugin list
		void refreshPluginList(QString mainShortName="en");*/
	signals:
		void pluginListingIsfinish();
		void onePluginAdded(PluginsAvailable);
		void onePluginWillBeRemoved(PluginsAvailable); // when will be really removed
		void onePluginWillBeUnloaded(PluginsAvailable);//just unload to quit the application
		void needLangToRefreshPluginList();
		void newLanguageLoaded();
	protected:
		void run();
	public slots: //do gui action
		void showInformation(QString path);
		void removeThePluginSelected(QString path);
		void addPlugin(ImportBackend backend);
};

/// \brief to do structure comparaison
bool operator==(PluginsAvailable pluginA,PluginsAvailable pluginB);

#endif // PLUGINS_MANAGER_H
