/** \file StructEnumDefinition.h
\brief Define the structure and enumeration used in ultracopier or into the plugin
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <string>
#include <stdint.h>

#ifndef STRUCTDEF_H
#define STRUCTDEF_H

namespace Ultracopier {
/// \brief Define the mode of the copy window/request, if need be copy or move
enum CopyMode : uint8_t
{
    Copy=0x00,
    Move=0x01
};

enum RemainingTimeAlgo : uint8_t
{
    RemainingTimeAlgo_Traditional=0x00,
    RemainingTimeAlgo_Logarithmic=0x01
};

/// \brief Define the catching state, if the copy is totally catch of the explorer, partially or nothing
enum CatchState : uint8_t
{
    Uncaught=0x00,
    Semiuncaught=0x01,
    Caught=0x02
};

/// \brief Define the listening state
enum ListeningState : uint8_t
{
    NotListening=0x00,///< 0 listener is listening
    SemiListening=0x01,///< only part of listeners are listening
    FullListening=0x02///< all the listeners are listening
};

/// \brief Define the copy type, if folder, file or both
enum CopyType : uint8_t
{
    File			= 0x01,
    FileAndFolder		= 0x02
};

/// \brief transfer list operation, can define nothing, the import/export or both
enum TransferListOperation : uint8_t
{
    TransferListOperation_None		= 0x00,
    TransferListOperation_Import		= 0x01,
    TransferListOperation_Export		= 0x02,
    TransferListOperation_ImportExport	= TransferListOperation_Import | TransferListOperation_Export
};

enum EngineActionInProgress : uint8_t
{
    Idle			= 0x00,
    Listing			= 0x01,
    Copying			= 0x02,
    CopyingAndListing	= Listing | Copying
};

/// \brief the level of information
enum DebugLevel : uint8_t
{
    DebugLevel_Information=0x01,	///< Information like the compiler, OS, Qt version, all to know in witch condition ultracopier is launched
    DebugLevel_Critical=0x02,		///< Critical error, where it don't know how skip it
    DebugLevel_Warning=0x03,		///< Error, but have way to skip it
    DebugLevel_Notice=0x04		///< General information to debug, what file is open, what event is received, ...
};

enum SizeUnit : uint8_t
{
    SizeUnit_byte=0x00,
    SizeUnit_KiloByte=0x01,
    SizeUnit_MegaByte=0x02,
    SizeUnit_GigaByte=0x03,
    SizeUnit_TeraByte=0x04,
    SizeUnit_PetaByte=0x05,
    SizeUnit_ExaByte=0x06,
    SizeUnit_ZettaByte=0x07,
    SizeUnit_YottaByte=0x08
};

/// \brief structure for decompossed time
struct TimeDecomposition
{
    uint16_t second;
    uint16_t minute;
    uint16_t hour;
};

//////////////////////////// Return list //////////////////////////////
enum ActionTypeCopyList : uint8_t
{
    //playlist action
    MoveItem=0x00,
    RemoveItem=0x01,
    AddingItem=0x02,
    //Item action, to inform the stat of one entry
    PreOperation=0x03,
    Transfer=0x04,
    PostOperation=0x05,
    CustomOperation=0x06 /// \note this need be used after preoperation and before postoperation
};

/// \brief structure for progression item
struct ProgressionItem
{
    uint64_t id;
    uint64_t currentRead;
    uint64_t currentWrite;
    uint64_t total;
};

/// \brief item to insert item in the interface
struct ItemOfCopyList
{
    uint64_t id;
    // if type == CustomOperation, then is the translated name of the operation
    std::string sourceFullPath;///< full path with file name: /foo/foo.txt
    std::string sourceFileName;///< full path with file name: foo.txt
    std::string destinationFullPath;///< full path with file name: /foo/foo.txt
    std::string destinationFileName;///< full path with file name: foo.txt
    // if type == CustomOperation, then 0 = without progression, 1 = with progression
    uint64_t size;
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
