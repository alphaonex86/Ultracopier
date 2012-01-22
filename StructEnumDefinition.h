/** \file StructEnumDefinition.h
\brief Define the structure and enumeration used in ultracopier or into the plugin
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QVariant>
#include <QString>
#include <QList>

#ifndef STRUCTDEF_H
#define STRUCTDEF_H

/// \brief Define the mode of the copy window/request, if need be copy or move
enum CopyMode
{
	Copy,
	Move
};

/// \brief Define the catching state, if the copy is totally catch of the explorer, partially or nothing
enum CatchState
{
	Uncaught,
	Semiuncaught,
	Caught
};

/// \brief Define the listening state
enum ListeningState
{
	NotListening,///< 0 listener is listening
	SemiListening,///< only part of listeners are listening
	FullListening///< all the listeners are listening
};

enum CopyType
{
	Folder			= 0x00000001,
	File			= 0x00000002,
	FileAndFolder		= Folder | File
};

enum EngineActionInProgress
{
	Idle			= 0x00000000,
	Listing			= 0x00000001,
	Copying			= 0x00000002,
	CopyingAndListing	= Listing | Copying
};

enum ActionTypeCopyList
{
	MoveItem,
	RemoveItem
};

enum ReturnActionTypeCopyList
{
	AddingItem,
	OtherAction
};

enum DebugLevel
{
	DebugLevel_Information,
	DebugLevel_Critical,
	DebugLevel_Warning,
	DebugLevel_Notice
};

enum SizeUnit
{
	SizeUnit_byte,
	SizeUnit_KiloByte,
	SizeUnit_MegaByte,
	SizeUnit_GigaByte,
	SizeUnit_TeraByte,
	SizeUnit_PetaByte,
	SizeUnit_ExaByte,
	SizeUnit_ZettaByte,
	SizeUnit_YottaByte
};

struct TimeDecomposition
{
	quint16 second;
	quint16 minute;
	quint16 hour;
};

struct ItemOfCopyList
{
	quint64 id;
	QString sourceFullPath;///< full path with file name: /foo/foo.txt
	QString sourceFileName;///< full path with file name: foo.txt
	QString destinationFullPath;///< full path with file name: /foo/foo.txt
	QString destinationFileName;///< full path with file name: foo.txt
	quint64 size;
	CopyMode mode;
};

struct ActionOnCopyList
{
	ActionTypeCopyList type;
	quint64 id;
};

struct returnActionOnCopyList
{
	ReturnActionTypeCopyList type;///< is OtherAction or AddingItem
	///< used if type != AddingItem
	ActionOnCopyList userAction;
	///< if userAction.type == MoveItem
	int position;
	///< used if type == AddingItem
	ItemOfCopyList addAction;
};

struct returnSpecificFileProgression
{
	quint64 copiedSize;
	quint64 totalSize;
	bool haveBeenLocated;
};

#endif // STRUCTDEF_H
