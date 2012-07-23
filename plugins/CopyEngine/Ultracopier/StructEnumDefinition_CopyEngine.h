/** \file StructEnumDefinition_CopyEngine.h
\brief Define the structure and enumeration used in the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QString>
#include <QRegularExpression>

#ifndef STRUCTDEF_COPYENGINE_H
#define STRUCTDEF_COPYENGINE_H

/// \brief Define action if file exists
enum FileExistsAction
{
	FileExists_NotSet,
	FileExists_Cancel,
	FileExists_Skip,
	FileExists_Overwrite,
	FileExists_OverwriteIfNewer,
	FileExists_OverwriteIfNotSameModificationDate,
	FileExists_Rename
};

/// \brief Define action if file error
enum FileErrorAction
{
	FileError_NotSet,
	FileError_Cancel,
	FileError_Skip,
	FileError_Retry,
	FileError_PutToEndOfTheList
};

/// \brief to have the transfer status
enum TransferStat
{
	TransferStat_Idle=0,
	TransferStat_PreOperation=1,
	TransferStat_WaitForTheTransfer=2,
	TransferStat_Transfer=3,
	TransferStat_Checksum=4,
	TransferStat_PostTransfer=5,
	TransferStat_PostOperation=6
};

/// \brief Define overwrite mode
/*enum OverwriteMode
{
	OverwriteMode_None,
	OverwriteMode_Overwrite,
	OverwriteMode_OverwriteIfNewer,
	OverwriteMode_OverwriteIfNotSameModificationDate
};*/

/// \brief Define action if file exists
enum FolderExistsAction
{
	FolderExists_NotSet,
	FolderExists_Cancel,
	FolderExists_Merge,
	FolderExists_Skip,
	FolderExists_Rename
};

enum SearchType
{
	SearchType_rawText,
	SearchType_simpleRegex,
	SearchType_perlRegex,
};

enum ApplyOn
{
	ApplyOn_file,
	ApplyOn_fileAndFolder,
	ApplyOn_folder,
};

/** to store into different way the filter rules to be exported */
struct Filters_rules
{
	QString search_text;
	SearchType search_type;
	ApplyOn apply_on;
	bool need_match_all;
	QRegularExpression regex;
};

#endif // STRUCTDEF_COPYENGINE_H
