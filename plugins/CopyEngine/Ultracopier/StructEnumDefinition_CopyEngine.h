/** \file StructEnumDefinition_CopyEngine.h
\brief Define the structure and enumeration used in the copy engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QString>
#include <QRegularExpression>

#ifndef STRUCTDEF_COPYENGINE_H
#define STRUCTDEF_COPYENGINE_H

/// \brief Define action if file exists
enum FileExistsAction
{
    FileExists_NotSet=0,
    FileExists_Cancel=1,
    FileExists_Skip=2,
    FileExists_Overwrite=3,
    FileExists_OverwriteIfNotSame=4,
    FileExists_OverwriteIfNewer=5,
    FileExists_OverwriteIfOlder=6,
    FileExists_Rename=7
};

/// \brief Define action if file error
enum FileErrorAction
{
    FileError_NotSet=1,
    FileError_Cancel=2,
    FileError_Skip=3,
    FileError_Retry=4,
    FileError_PutToEndOfTheList=5
};

enum TransferAlgorithm
{
    TransferAlgorithm_Automatic=0,
    TransferAlgorithm_Sequential=1,
    TransferAlgorithm_Parallel=2
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
    FolderExists_NotSet=0,
    FolderExists_Cancel=1,
    FolderExists_Merge=2,
    FolderExists_Skip=3,
    FolderExists_Rename=4
};

enum SearchType
{
    SearchType_rawText=0,
    SearchType_simpleRegex=1,
    SearchType_perlRegex=2
};

enum ApplyOn
{
    ApplyOn_file=0,
    ApplyOn_fileAndFolder=1,
    ApplyOn_folder=2
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

/// \brief get action type
enum ActionType
{
    ActionType_MkPath=1,
    ActionType_MovePath=2,
    ActionType_RealMove=3
};

#endif // STRUCTDEF_COPYENGINE_H
