/** \file StructEnumDefinition.h
\brief Define the structure and enumeration used in ultracopier or into the plugin
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QVariant>
#include <QString>
#include <QList>

#ifndef STRUCTDEF_H
#define STRUCTDEF_H

namespace Ultracopier {
/// \brief Define the mode of the copy window/request, if need be copy or move
enum CopyMode
{
    Copy=0x00000000,
    Move=0x00000001
};

/// \brief Define the catching state, if the copy is totally catch of the explorer, partially or nothing
enum CatchState
{
    Uncaught=0x00000000,
    Semiuncaught=0x00000001,
    Caught=0x00000002
};

/// \brief Define the listening state
enum ListeningState
{
    NotListening=0x00000000,///< 0 listener is listening
    SemiListening=0x00000001,///< only part of listeners are listening
    FullListening=0x00000002///< all the listeners are listening
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

/// \brief the level of information
enum DebugLevel
{
    DebugLevel_Information=0x00000001,	///< Information like the compiler, OS, Qt version, all to know in witch condition ultracopier is launched
    DebugLevel_Critical=0x00000002,		///< Critical error, where it don't know how skip it
    DebugLevel_Warning=0x00000003,		///< Error, but have way to skip it
    DebugLevel_Notice=0x00000004		///< General information to debug, what file is open, what event is received, ...
};

enum SizeUnit
{
    SizeUnit_byte=0x00000000,
    SizeUnit_KiloByte=0x00000001,
    SizeUnit_MegaByte=0x00000002,
    SizeUnit_GigaByte=0x00000003,
    SizeUnit_TeraByte=0x00000004,
    SizeUnit_PetaByte=0x00000005,
    SizeUnit_ExaByte=0x00000006,
    SizeUnit_ZettaByte=0x00000007,
    SizeUnit_YottaByte=0x00000008
};

/// \brief structure for decompossed time
struct TimeDecomposition
{
    quint16 second;
    quint16 minute;
    quint16 hour;
};

//////////////////////////// Return list //////////////////////////////
enum ActionTypeCopyList
{
    //playlist action
    MoveItem=0x00000000,
    RemoveItem=0x00000001,
    AddingItem=0x00000002,
    //Item action, to inform the stat of one entry
    PreOperation=0x00000003,
    Transfer=0x00000004,
    PostOperation=0x00000005,
    CustomOperation=0x00000006 /// \note this need be used after preoperation and before postoperation
};

/// \brief structure for progression item
struct ProgressionItem
{
    quint64 id;
    quint64 currentRead;
    quint64 currentWrite;
    quint64 total;
};

/// \brief item to insert item in the interface
struct ItemOfCopyList
{
    quint64 id;
    // if type == CustomOperation, then is the translated name of the operation
    QString sourceFullPath;///< full path with file name: /foo/foo.txt
    QString sourceFileName;///< full path with file name: foo.txt
    QString destinationFullPath;///< full path with file name: /foo/foo.txt
    QString destinationFileName;///< full path with file name: foo.txt
    // if type == CustomOperation, then 0 = without progression, 1 = with progression
    quint64 size;
    CopyMode mode;
};

/// \brief The definition of no removing action on transfer list
struct ActionOnCopyList
{
    int position;
    // if type == MoveItem
    // if type == RemoveItem, then 0 = normal remove, 1 = skip
    int moveAt;
};

/// \brief action normal or due to interface query on copy list
struct ReturnActionOnCopyList
{
    ActionTypeCopyList type;
    ///< used if type == AddingItem || type == PreOperation (for interface without transfer list) || type == CustomOperation
    ItemOfCopyList addAction;
    ///< used if type != AddingItem
    ActionOnCopyList userAction;
};
}

#endif // STRUCTDEF_H
