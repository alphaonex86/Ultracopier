/** \file LanguagesManager.h
\brief Define the class to manage and load the languages
\author alpha_one_x86
\version 0.3
\date 2010 */ 

#ifndef LANGUAGES_MANAGER_H
#define LANGUAGES_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QLocale>
#include <QTranslator>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>

#include "Environment.h"
#include "Singleton.h"
#include "OptionEngine.h"
#include "ResourcesManager.h"
#include "PluginsManager.h"

/** \brief Define the class to manage and load the resources linked with the themes

This class provide a core load and manage the resources */
class LanguagesManager : public QObject, public Singleton<LanguagesManager>
{
	Q_OBJECT
	friend class Singleton<LanguagesManager>;
	//public:
	//	QString getMainShortName();
	private:
		/// \brief Create the manager and load the defaults variables
		LanguagesManager();
		/// \brief Destroy the language manager
		~LanguagesManager();
		//for the options
		OptionEngine *options;
		/** \brief To set the current language
		\param newLanguage Should be short name code found into informations.xml of language file */
		void setCurrentLanguage(QString newLanguage);
		/// \brief Structure of language
		struct LanguagesAvailable
		{
			QString path;
			QString fullName;
			QString mainShortName;
			QStringList shortName;
		};
		/// \brief To store the language path
		QStringList languagePath;
		/// \brief To store the language detected
		QList<LanguagesAvailable> LanguagesAvailableList;
		/// \brief check if short name is found into language
		QString getMainShortName(QString shortName);
		/// \brief Store the object of resources manager
		ResourcesManager *resources;
		/// \brief Store the object of plugin manager
		PluginsManager *plugins;
		/// \brief list of installed translator
		QList<QTranslator *> installedTranslator;
		QString currentLanguage;
		/// \brief load the language selected
		QString getTheRightLanguage();
	private slots:
		/// \brief load the language in languagePath
		void allPluginIsLoaded();
		//plugin management
		void onePluginAdded(PluginsAvailable plugin);
		void onePluginWillBeRemoved(PluginsAvailable plugin);
		void newOptionValue(QString group);
	signals:
		//send the language is loaded or the new language is loaded
		void newLanguageLoaded(QString mainShortName);
};

#endif // LANGUAGES_MANAGER_H
