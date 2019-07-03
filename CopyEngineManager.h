/** \file CopyEngineManager.h
\brief Define the copy engine manager
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef COPYENGINEMANAGER_H
#define COPYENGINEMANAGER_H

#include <QObject>
#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
#include <QPluginLoader>
#endif
#include <QList>
#include <QWidget>
#include <QString>

#include "Environment.h"
#include "LocalPluginOptions.h"
#include "OptionDialog.h"
#include "interface/PluginInterface_CopyEngine.h"
#include "FacilityEngine.h"

namespace Ui {
    class CopyEngineOptions;
}

/** \brief Manage copy engine plugins and their instance */
class CopyEngineManager : public QObject
{
    Q_OBJECT
public:
    /** \brief internal structure to return one copy engine instance */
    struct returnCopyEngine
    {
        PluginInterface_CopyEngine * engine;	///< The copy engine instance
        bool canDoOnlyCopy;			///< true if can do only the copy (not move)
        bool havePause;
        Ultracopier::CopyType type;				///< Kind of copy what it can do
        Ultracopier::TransferListOperation transferListOperation;
    };
    explicit CopyEngineManager(OptionDialog *optionDialog);
    /** \brief return copy engine instance when know the sources and destinations
      \param mode the mode (copy/move)
      \param protocolsUsedForTheSources list of sources used
      \param protocolsUsedForTheDestination list of destination used
      \see getCopyEngine()
      */
    returnCopyEngine getCopyEngine(const Ultracopier::CopyMode &mode,const std::vector<std::string> &protocolsUsedForTheSources,const std::string &protocolsUsedForTheDestination);
    /** \brief return copy engine instance with specific engine
      \param mode the mode (copy/move)
      \param name name of the engine needed
      \see getCopyEngine()
      */
    returnCopyEngine getCopyEngine(const Ultracopier::CopyMode &mode,const std::string &name);
    //bool currentEngineCanDoOnlyCopy(std::vector<std::string> protocolsUsedForTheSources,std::string protocolsUsedForTheDestination="");
    //CopyType currentEngineGetCopyType(std::vector<std::string> protocolsUsedForTheSources,std::string protocolsUsedForTheDestination="");
    /** \brief to send all signal because all object is connected on it */
    void setIsConnected();
    /** \brief check if the protocols given is supported by the copy engine
      \see Core::newCopy()
      \see Core::newMove()
      */
    bool protocolsSupportedByTheCopyEngine(PluginInterface_CopyEngine * engine,const std::vector<std::string> &protocolsUsedForTheSources,const std::string &protocolsUsedForTheDestination);
private slots:
    void onePluginAdded(const PluginsAvailable &plugin);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    void onePluginWillBeRemoved(const PluginsAvailable &plugin);
    void onePluginWillBeUnloaded(const PluginsAvailable &plugin);
    #endif
    #ifdef ULTRACOPIER_DEBUG
    void debugInformation(const Ultracopier::DebugLevel &level, const std::string& fonction, const std::string& text, const std::string& file, const int& ligne);
    #endif // ULTRACOPIER_DEBUG
    /// \brief To notify when new value into a group have changed
    void newOptionValue(const std::string &groupName,const std::string &variableName,const std::string &value);
    void allPluginIsloaded();
private:
    /// \brief the option interface
    struct CopyEnginePlugin
    {
        std::string path;
        std::string name;
        std::string pluginPath;
        std::vector<std::string> supportedProtocolsForTheSource;
        std::vector<std::string> supportedProtocolsForTheDestination;
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
        QPluginLoader * pointer;
        #endif
        PluginInterface_CopyEngineFactory * factory;
        std::vector<PluginInterface_CopyEngine *> intances;
        bool canDoOnlyCopy;
        Ultracopier::CopyType type;
        Ultracopier::TransferListOperation transferListOperation;
        LocalPluginOptions *options;
        QWidget *optionsWidget;
    };
    std::vector<CopyEnginePlugin> pluginList;
    OptionDialog *optionDialog;
    bool isConnected;
signals:
    //void newCopyEngineOptions(std::string,std::string,QWidget *);
    void addCopyEngine(std::string name,bool canDoOnlyCopy) const;
    void removeCopyEngine(std::string name) const;
    void previouslyPluginAdded(PluginsAvailable) const;
};

#endif // COPYENGINEMANAGER_H
