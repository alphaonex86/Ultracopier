/** \file PluginInformation.cpp
\brief Define the plugin information
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "PluginInformation.h"
#include "ui_PluginInformation.h"
#include "cpp11addition.h"

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

void PluginInformation::setPlugin(const PluginsAvailable &plugin)
{
    this->plugin=plugin;
    pluginIsLoaded=true;
    retranslateInformation();
}

void PluginInformation::setLanguage(const std::string &language)
{
    this->language=language;
}

std::string PluginInformation::categoryToTranslation(const PluginType &category) const
{
    switch(category)
    {
        case PluginType_CopyEngine:
            return tr("Copy engine").toStdString();
        break;
        case PluginType_Languages:
            return tr("Languages").toStdString();
        break;
        case PluginType_Listener:
            return tr("Listener").toStdString();
        break;
        case PluginType_PluginLoader:
            return tr("Plugin loader").toStdString();
        break;
        case PluginType_SessionLoader:
            return tr("Session loader").toStdString();
        break;
        case PluginType_Themes:
            return tr("Themes").toStdString();
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"cat translation not found");
            return tr("Unknown").toStdString();
        break;
    }
}

void PluginInformation::retranslateInformation()
{
    if(!pluginIsLoaded)
        return;
    ui->retranslateUi(this);
    this->setWindowTitle(tr("Information about %1").arg(QString::fromStdString(plugin.name)));
    ui->name->setText(QString::fromStdString(plugin.name));
    ui->title->setText(QString::fromStdString(getTranslatedText(plugin,"title",language)));
    ui->category->setText(QString::fromStdString(categoryToTranslation(plugin.category)));
    ui->author->setText(QString::fromStdString(getInformationText(plugin,"author")));
    QString website=QString::fromStdString(getTranslatedText(plugin,"website",language));
    ui->website->setText(QStringLiteral("<a href=\"")+website+QStringLiteral("\" title=\"")+website+QStringLiteral("\">")+website+QStringLiteral("</a>"));
    bool ok;
    int timeStamps=stringtoint32(getInformationText(plugin,"pubDate"),&ok);
    QDateTime date;
    date.setTime_t(timeStamps);
    ui->date->setDateTime(date);
    if(!ok || timeStamps<=0)
        ui->date->setEnabled(false);
    ui->description->setPlainText(QString::fromStdString(getTranslatedText(plugin,"description",language)));
    ui->version->setText(QString::fromStdString(getInformationText(plugin,"version")));
}

/// \brief get informations text
std::string PluginInformation::getInformationText(const PluginsAvailable &plugin,const std::string &informationName)
{
    unsigned int index=0;
    while(index<plugin.informations.size())
    {
        if(plugin.informations.at(index).size()==2 && plugin.informations.at(index).first()==informationName)
            return plugin.informations.at(index).last();
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"information not found: "+informationName+", for: "+plugin.name+", cat: "+categoryToTranslation(plugin.category));
    return std::string;
}

/// \brief get translated text
std::string PluginInformation::getTranslatedText(const PluginsAvailable &plugin,const std::string &informationName,const std::string &mainShortName)
{
    unsigned int index=0;
    std::string TextFound;
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
    if(TextFound.empty() || TextFound.empty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"text is not found or empty for: "+informationName+", with the language: "+mainShortName+", for the plugin: "+plugin.path);
    #endif // ULTRACOPIER_DEBUG
    return TextFound;
}
