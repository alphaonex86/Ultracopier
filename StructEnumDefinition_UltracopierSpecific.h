/** \file StructEnumDefinition_UltracopierSpecific.h
\brief Define the structure and enumeration used in ultracopier only
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <string>
#include <vector>
#include <QDomElement>

#ifndef STRUCTDEF_ULTRACOPIERSPECIFIC_H
#define STRUCTDEF_ULTRACOPIERSPECIFIC_H

enum PluginType : uint8_t
{
    PluginType_Unknow=0,
    PluginType_CopyEngine=1,
    PluginType_Languages=2,
    PluginType_Listener=3,
    PluginType_PluginLoader=4,
    PluginType_SessionLoader=5,
    PluginType_Themes=6
};

/// \brief structure to store the general plugin related information
struct PluginsAvailable
{
    PluginType category;
    std::string path;
    std::string name;
    std::string writablePath;
    QDomElement categorySpecific;
    std::string version;
    std::vector<std::vector<std::string> > informations;
    std::string errorString;
    bool isWritable;
    bool isAuth;
};

enum DebugLevel_custom : uint8_t
{
    DebugLevel_custom_Information=0,
    DebugLevel_custom_Critical=1,
    DebugLevel_custom_Warning=2,
    DebugLevel_custom_Notice=3,
    DebugLevel_custom_UserNote=4
};

enum ActionOnManualOpen : uint8_t
{
    ActionOnManualOpen_Nothing=0x00,
    ActionOnManualOpen_Folder=0x01,
    ActionOnManualOpen_Files=0x02
};

#endif // STRUCTDEF_ULTRACOPIERSPECIFIC_H
