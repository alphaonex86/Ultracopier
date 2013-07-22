/** \file PluginInformation.h
\brief Define the plugin information
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGININFORMATION_H
#define PLUGININFORMATION_H

#include <QDialog>
#include <QDateTime>

#include "Environment.h"

namespace Ui {
    class PluginInformation;
}

/** \brief to show the plugin information */
class PluginInformation : public QDialog
{
    Q_OBJECT
    public:
        explicit PluginInformation();
        ~PluginInformation();
        /** \brief get translated categorie */
        QString categoryToTranslation(const PluginType &category) const;
        /** \brief to get the new plugin informations */
        void setPlugin(const PluginsAvailable &plugin);
        /** \brief to set the language */
        void setLanguage(const QString &language);
    public slots:
        void retranslateInformation();
    private:
        bool pluginIsLoaded;
        PluginsAvailable plugin;
        Ui::PluginInformation *ui;
        QString language;
        QString getInformationText(const PluginsAvailable &plugin,const QString &informationName);
        QString getTranslatedText(const PluginsAvailable &plugin,const QString &informationName,const QString &mainShortName);
};

#endif // PLUGININFORMATION_H
