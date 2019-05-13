/** \file TransferThread.h
\brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef TRANSFERTHREAD_H
#define TRANSFERTHREAD_H

#include <QObject>
#include <QTime>

#include <regex>
#include <vector>
#include <string>
#include <utility>
#include <dirent.h>

//defore the next define
#include "Variable.h"

#ifdef Q_OS_UNIX
    #include <utime.h>
    #include <time.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif
#ifdef Q_OS_WIN32
    #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
        #include <utime.h>
        #include <time.h>
        #include <unistd.h>
        #include <sys/stat.h>
    #endif
#endif

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

#include "ReadThread.h"
#include "WriteThread.h"
#include "Environment.h"
#include "DriveManagement.h"
#include "StructEnumDefinition_CopyEngine.h"

/// \brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
class TransferThread : public QObject
{
    Q_OBJECT
public:
    explicit TransferThread();
    ~TransferThread();
    /// \brief get transfer stat
    TransferStat getStat() const;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief to set the id
    void setId(int id);
    /// \brief get the reading letter
    char readingLetter() const;
    /// \brief get the writing letter
    char writingLetter() const;
    #endif
    /// \brief to store the transfer id
    uint64_t			transferId;
    /// \brief to store the transfer size
    uint64_t			transferSize;
    bool haveStartTime;
    QTime startTransferTime;

    //not copied size, ...
    uint64_t realByteTransfered() const;
    std::pair<uint64_t, uint64_t> progression() const;
    static std::string resolvedName(const std::string &inode);
    std::string getSourcePath() const;
    std::string getDestinationPath() const;
    Ultracopier::CopyMode getMode() const;
    // \warning check mkpath() call should not exists because only existing dest is allowed now
    #ifdef Q_OS_UNIX
    static bool mkpath(const std::string &file_path, const mode_t &mode=0755);
    #else
    static bool mkpath(const std::string &file_path);
    #endif
    static bool mkdir(const std::string &file_path, const mode_t &mode=0755);

    static int64_t readFileMDateTime(const std::string &source);
    static bool is_symlink(const char * const filename);
    static bool is_symlink(const std::string &filename);
    static bool is_file(const char * const filename);
    static bool is_file(const std::string &filename);
    static bool is_dir(const char * const filename);
    static bool is_dir(const std::string &filename);
    static bool exists(const char * const filename);
    static bool exists(const std::string &filename);
    static int64_t file_stat_size(const std::string &filename);
    static int64_t file_stat_size(const char * const filename);
    static bool entryInfoList(const std::string &path, std::vector<std::string> &list);
    struct dirent_uc
    {
        #ifdef Q_OS_WIN32
        size_t size;
        #endif
        bool isFolder;
        char d_name[256];
    };
    static bool entryInfoList(const std::string &path, std::vector<dirent_uc> &list);
    void setMkFullPath(const bool mkFullPath);
    static int fseeko64(FILE *__stream, uint64_t __off, int __whence);
    static int ftruncate64(int __fd, uint64_t __length);
protected:
    void run();
signals:
    //to send state
    void preOperationStopped() const;
    void checkIfItCanBeResumed() const;
    //void transferStarted();//not sended (and not used then)
    void readStopped() const;
    void writeStopped() const;
    void postOperationStopped() const;
    //get dialog
    void fileAlreadyExists(const std::string &info,const std::string &info2,const bool &isSame) const;
    void errorOnFile(const std::string &info,const std::string &string,const ErrorType &errorType=ErrorType_Normal) const;
    //internal signal
    void internalStartPostOperation() const;
    void internalStartPreOperation() const;
    void internalStartResumeAfterErrorAndSeek() const;
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,std::string fonction,std::string text,std::string file,int ligne) const;
    void tryPutAtBottom() const;
    //force into the right thread
    void internalTryStartTheTransfer() const;
    /// \brief update the transfer stat
    void pushStat(const TransferStat &stat,const uint64_t &pos) const;
public slots:
    /// \brief to start the transfer of data
    void startTheTransfer();
    /// \brief to set files to transfer
    bool setFiles(const std::string& source,const int64_t &size,const std::string& destination,const Ultracopier::CopyMode &mode);
    /// \brief to set file exists action to do
    void setFileExistsAction(const FileExistsAction &action);
    /// \brief to set the new name of the destination
    void setFileRename(const std::string &nameForRename);
    /// \brief to start the transfer of data
    void setAlwaysFileExistsAction(const FileExistsAction &action);
    /// \brief set the copy info and options before runing
    void setRightTransfer(const bool doRightTransfer);
    /// \brief set keep date
    void setKeepDate(const bool keepDate);
    /// \brief stop the copy
    void stop();
    /// \brief skip the copy
    void skip();
    /// \brief retry after error
    void retryAfterError();
    /// \brief return info about the copied size
    int64_t copiedSize();
    /// \brief put the current file at bottom
    void putAtBottom();

    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    void setRsync(const bool rsync);
    #endif

    void setRenamingRules(const std::string &firstRenamingRule,const std::string &otherRenamingRule);

    void setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
    void setRenameTheOriginalDestination(const bool &renameTheOriginalDestination);
    void set_updateMount();
private slots:
    void preOperation();
    void readIsReady();
    void writeIsReady();
    void readIsFinish();
    void writeIsFinish();
    void readIsClosed();
    void writeIsClosed();
    void postOperation();
    void getWriteError();
    void getReadError();
    //void syncAfterErrorAndReadFinish();
    void readThreadIsSeekToZeroAndWait();
    void writeThreadIsReopened();
    void readThreadResumeAfterError();
    //to filter the emition of signal
    void readIsStopped();
    void writeIsStopped();
    //force into the right thread
    void internalStartTheTransfer();
private:
    enum MoveReturn
    {
        MoveReturn_skip=0,
        MoveReturn_moved=1,
        MoveReturn_error=2
    };
    TransferStat	transfer_stat;
    ReadThread		readThread;
    WriteThread		writeThread;

    Ultracopier::CopyMode		mode;
    bool			doRightTransfer;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    bool            rsync;
    #endif
    bool			keepDate;
    bool            mkFullPath;
    //ready = open + ready to operation (no error to resolv)
    bool			readIsReadyVariable;
    bool			writeIsReadyVariable;
    //can be open but with error
    bool            readIsOpeningVariable;//after call open() and before the end of internalOpen(), mostly to prevent internal error by open() when another is running
    bool            writeIsOpeningVariable;//after call open() and before the end of internalOpen(), mostly to prevent internal error by open() when another is running
    bool			readIsOpenVariable;
    bool			writeIsOpenVariable;
    bool			readIsFinishVariable;
    bool			writeIsFinishVariable;
    bool			readIsClosedVariable;
    bool			writeIsClosedVariable;
    bool			canBeMovedDirectlyVariable,canBeCopiedDirectlyVariable;
    DriveManagement driveManagement;
    volatile bool	stopIt;
    volatile bool	canStartTransfer;
    bool			retry;
    std::string		source;
    std::string		destination;
    int64_t			size;
    FileExistsAction	fileExistsAction;
    FileExistsAction	alwaysDoFileExistsAction;
    bool			needSkip,needRemove;
    int             id;
    bool            deletePartiallyTransferredFiles;
    std::string			firstRenamingRule;
    std::string			otherRenamingRule;
    //error management
    bool			writeError,writeError_source_seeked,writeError_destination_reopened;
    bool			readError;
    bool            renameTheOriginalDestination;
    bool			fileContentError;
    bool            doTheDateTransfer;
    int             parallelizeIfSmallerThan;
    std::regex renameRegex;
    #ifdef Q_OS_UNIX
        utimbuf butime;
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                utimbuf butime;
            #else
                uint32_t ftCreateL, ftAccessL, ftWriteL;
                uint32_t ftCreateH, ftAccessH, ftWriteH;
                std::regex regRead;
            #endif
        #endif
    #endif
    #ifdef Q_OS_UNIX
    struct stat permissions;
    #else
    PSECURITY_DESCRIPTOR PSecurityD;
    PACL dacl;
    #endif
    bool havePermission;
    //different pre-operation
    bool isSame();
    bool destinationExists();
    bool checkAlwaysRename();///< return true if has been renamed
    bool canBeMovedDirectly() const;
    bool canBeCopiedDirectly() const;
    void tryMoveDirectly();
    void tryCopyDirectly();
    void ifCanStartTransfer();
    //fonction to edit the file date time
    bool readSourceFileDateTime(const std::string &source);
    bool writeDestinationFileDateTime(const std::string &destination);
    bool readSourceFilePermissions(const std::string &source);
    bool writeDestinationFilePermissions(const std::string &destination);
    void resetExtraVariable();
    //error management function
    void resumeTransferAfterWriteError();
    //to send state
    bool sended_state_preOperationStopped;
    bool sended_state_readStopped;
    bool sended_state_writeStopped;
    //different post-operation
    bool checkIfAllIsClosedAndDoOperations();// return true if all is closed, and do some operations, don't use into condition to check if is closed!
    bool doFilePostOperation();
    //different pre-operation
    void tryOpen();
    bool remainFileOpen() const;
    bool remainSourceOpen() const;
    bool remainDestinationOpen() const;
};

#endif // TRANSFERTHREAD_H
