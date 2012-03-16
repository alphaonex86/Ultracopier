/** \file StructEnumDefinition.h
\brief Define the structure and enumeration used in ultracopier or into the plugin
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING
\todo Add transfer state normal and custom */

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

/// \brief Define the copy type, if folder, file or both
enum CopyType
{
	File			= 0x00000001,
	FileAndFolder		= 0x00000002
};

/// \brief transfer list operation, can define nothing, the import/export or both
enum TransferListOperation
{
	TransferListOperation_None		= 0x00000000,
	TransferListOperation_Import		= 0x00000001,
	TransferListOperation_Export		= 0x00000002,
	TransferListOperation_ImportExport	= TransferListOperation_Import | TransferListOperation_Export
};

enum EngineActionInProgress
{
	Idle			= 0x00000000,
	Listing			= 0x00000001,
	Copying			= 0x00000002,
	CopyingAndListing	= Listing | Copying
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

/// \brief structure for decompossed time
struct TimeDecomposition
{
	quint16 second;
	quint16 minute;
	quint16 hour;
};

/// \brief to return file progression of previousled asked query
struct returnSpecificFileProgression
{
	quint64 copiedSize;
	quint64 totalSize;
	bool haveBeenLocated;
};

//////////////////////////// Return list //////////////////////////////
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

/// \brief item to insert item in the interface
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

/// \brief The definition of no removing action on transfer list
struct ActionOnCopyList
{
	ActionTypeCopyList type;//MoveItem or RemoveItem
	
	int current_position;
	///< if userAction.type == MoveItem
	int position;
};

/// \brief action normal or due to interface query on copy list
struct returnActionOnCopyList
{
	ReturnActionTypeCopyList type;///< is OtherAction or AddingItem
	///< used if type == AddingItem
	ItemOfCopyList addAction;
	// else
	ActionOnCopyList userAction;
};

#endif // STRUCTDEF_H
