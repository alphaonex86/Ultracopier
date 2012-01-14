/** \file StructEnumDefinition.h
\brief Define the structure and enumeration used in the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

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

#endif // STRUCTDEF_COPYENGINE_H
