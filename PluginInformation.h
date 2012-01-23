/** \file PluginInformation.h
\brief Define the plugin information
\author alpha_one_x86
\version 0.3
\date 2010 */

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
		QString categoryToTranslation(PluginType category);
		/** \brief to get the new plugin informations */
		void setPlugin(PluginsAvailable plugin);
		/** \brief to set the language */
		void setLanguage(QString language);
	public slots:
		void retranslateInformation();
	private:
		bool pluginIsLoaded;
		PluginsAvailable plugin;
		Ui::PluginInformation *ui;
		QString language;
		QString getInformationText(PluginsAvailable plugin,QString informationName);
		QString getTranslatedText(PluginsAvailable plugin,QString informationName,QString mainShortName);
};

#endif // PLUGININFORMATION_H
