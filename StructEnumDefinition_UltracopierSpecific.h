/** \file StructEnumDefinition_UltracopierSpecific.h
\brief Define the structure and enumeration used in ultracopier only
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QString>
#include <QList>
#include <QDomElement>

#ifndef STRUCTDEF_ULTRACOPIERSPECIFIC_H
#define STRUCTDEF_ULTRACOPIERSPECIFIC_H

enum PluginType
{
	PluginType_Unknow,
	PluginType_CopyEngine,
	PluginType_Languages,
	PluginType_Listener,
	PluginType_PluginLoader,
	PluginType_SessionLoader,
	PluginType_Themes
};

/// \brief structure to store the general plugin related information
struct PluginsAvailable
{
	PluginType category;
	QString path;
	QString name;
	QString writablePath;
	QDomElement categorySpecific;
	QString version;
	QList<QStringList> informations;
	QString errorString;
	bool isWritable;
	bool isAuth;
};

enum DebugLevel_custom
{
	DebugLevel_custom_Information,
	DebugLevel_custom_Critical,
	DebugLevel_custom_Warning,
	DebugLevel_custom_Notice,
	DebugLevel_custom_UserNote
};

enum ActionOnManualOpen
{
	ActionOnManualOpen_Nothing=0x00,
	ActionOnManualOpen_Folder=0x01,
	ActionOnManualOpen_Files=0x02
};

#endif // STRUCTDEF_ULTRACOPIERSPECIFIC_H
