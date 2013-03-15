/** \file PluginInformation.cpp
\brief Define the plugin information
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "PluginInformation.h"
#include "ui_PluginInformation.h"

PluginInformation::PluginInformation() :
    ui(new Ui::PluginInformation)
{
    ui->setupUi(this);
    pluginIsLoaded=false;
}

PluginInformation::~PluginInformation()
{
    delete ui;
}

void PluginInformation::setPlugin(PluginsAvailable plugin)
{
    this->plugin=plugin;
    pluginIsLoaded=true;
    retranslateInformation();
}

void PluginInformation::setLanguage(QString language)
{
    this->language=language;
}

QString PluginInformation::categoryToTranslation(PluginType category) const
{
    switch(category)
    {
        case PluginType_CopyEngine:
            return tr("Copy engine");
        break;
        case PluginType_Languages:
            return tr("Languages");
        break;
        case PluginType_Listener:
            return tr("Listener");
        break;
        case PluginType_PluginLoader:
            return tr("Plugin loader");
        break;
        case PluginType_SessionLoader:
            return tr("Session loader");
        break;
        case PluginType_Themes:
            return tr("Themes");
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"cat translation not found");
            return "Unknow";
        break;
    }
}

void PluginInformation::retranslateInformation()
{
    if(!pluginIsLoaded)
        return;
    ui->retranslateUi(this);
    this->setWindowTitle(tr("Information about %1").arg(plugin.name));
    ui->name->setText(plugin.name);
    ui->title->setText(getTranslatedText(plugin,"title",language));
    ui->category->setText(categoryToTranslation(plugin.category));
    ui->author->setText(getInformationText(plugin,"author"));
    QString website=getTranslatedText(plugin,"website",language);
    ui->website->setText("<a href=\""+website+"\" title=\""+website+"\">"+website+"</a>");
    bool ok;
    int timeStamps=getInformationText(plugin,"pubDate").toInt(&ok);
    QDateTime date;
    date.setTime_t(timeStamps);
    ui->date->setDateTime(date);
    if(!ok || timeStamps<=0)
        ui->date->setEnabled(false);
    ui->description->setPlainText(getTranslatedText(plugin,"description",language));
    ui->version->setText(getInformationText(plugin,"version"));
}

/// \brief get informations text
QString PluginInformation::getInformationText(PluginsAvailable plugin,QString informationName)
{
    int index=0;
    while(index<plugin.informations.size())
    {
        if(plugin.informations.at(index).size()==2 && plugin.informations.at(index).first()==informationName)
            return plugin.informations.at(index).last();
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"information not found: "+informationName+", for: "+plugin.name+", cat: "+categoryToTranslation(plugin.category));
    return "";
}

/// \brief get translated text
QString PluginInformation::getTranslatedText(PluginsAvailable plugin,QString informationName,QString mainShortName)
{
    int index=0;
    QString TextFound;
    while(index<plugin.informations.size())
    {
        if(plugin.informations.at(index).size()==3)
        {
            if(plugin.informations.at(index).first()==informationName)
            {
                if(plugin.informations.at(index).at(1)==mainShortName)
                    return plugin.informations.at(index).last();
                else if(plugin.informations.at(index).at(1)=="en")
                    TextFound=plugin.informations.at(index).last();

            }
        }
        index++;
    }
    #ifdef ULTRACOPIER_DEBUG
    if(TextFound.isEmpty() || TextFound.isEmpty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"text is not found or empty for: "+informationName+", with the language: "+mainShortName+", for the plugin: "+plugin.path);
    #endif // ULTRACOPIER_DEBUG
    return TextFound;
}
