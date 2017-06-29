/** \file LanguagesManager.h
\brief Define the class to manage and load the languages
\author alpha_one_x86
\licence GPL3, see the file COPYING */

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
#include "OptionEngine.h"
#include "ResourcesManager.h"
#include "PluginsManager.h"

/** \brief Define the class to manage and load the resources linked with the themes

This class provide a core load and manage the resources */
class LanguagesManager : public QObject
{
    Q_OBJECT
    //public:
    //	QString getMainShortName();
    public:
        const QString autodetectedLanguage() const;
        static LanguagesManager *languagesManager;
        /// \brief Create the manager and load the defaults variables
        LanguagesManager();
        /// \brief Destroy the language manager
        ~LanguagesManager();
    private:
        /** \brief To set the current language
        \param newLanguage Should be short name code found into informations.xml of language file */
        void setCurrentLanguage(const std::string &newLanguage);
        /// \brief Structure of language
        struct LanguagesAvailable
        {
            std::string path;
            std::string fullName;
            std::string mainShortName;
            std::vector<std::string> shortName;
        };
        /// \brief To store the language path
        std::vector<std::string> languagePath;
        /// \brief To store the language detected
        std::vector<LanguagesAvailable> LanguagesAvailableList;
        /// \brief check if short name is found into language
        std::string getMainShortName(const QString &shortName) const;
        /// \brief list of installed translator
        std::vector<QTranslator *> installedTranslator;
        std::string currentLanguage;
        /// \brief load the language selected
        std::string getTheRightLanguage() const;
    private slots:
        /// \brief load the language in languagePath
        void allPluginIsLoaded();
        //plugin management
        void onePluginAdded(const PluginsAvailable &plugin);
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        void onePluginWillBeRemoved(const PluginsAvailable &plugin);
        #endif
        void newOptionValue(const std::string &group);
    signals:
        //send the language is loaded or the new language is loaded
        void newLanguageLoaded(const std::string &mainShortName) const;
        void previouslyPluginAdded(PluginsAvailable) const;
};

#endif // LANGUAGES_MANAGER_H
