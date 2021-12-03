/** \file TransferThread.h
\brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QObject>
#include <QElapsedTimer>
#include <QThread>

#include <regex>
#include <vector>
#include <string>
#include <utility>
#include <dirent.h>
#include <atomic>

#ifdef WIDESTRING
#define INTERNALTYPEPATH std::wstring
#define INTERNALTYPECHAR wchar_t
#else
#define INTERNALTYPEPATH std::string
#define INTERNALTYPECHAR char
#endif

//defore the next define
#include "CopyEngineUltracopier-SpecVariable.h"

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

#include "Environment.h"
#include "DriveManagement.h"
#include "StructEnumDefinition_CopyEngine.h"

#ifndef TRANSFERTHREAD_H
#define TRANSFERTHREAD_H

/// \brief Thread changed to manage the inode operation, the signals, canceling, pre and post operations
class TransferThread : public QThread
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
    #endif
    /// \brief get the transfer time in ms
    int64_t transferTime() const;
    /// \brief to store the transfer id
    std::atomic<uint64_t>			transferId;
    /// \brief to store the transfer size
    uint64_t			transferSize;

    //not copied size, ...
    #ifdef Q_OS_WIN32
    static std::string resolvedName(std::string inode);
    static std::wstring resolvedName(std::wstring inode);
    #else
    static std::string resolvedName(const std::string &inode);
    static std::wstring resolvedName(const std::wstring &inode);
    #endif
    INTERNALTYPEPATH getSourcePath() const;
    INTERNALTYPEPATH getDestinationPath() const;
    Ultracopier::CopyMode getMode() const;
    // \warning check mkpath() call should not exists because only existing dest is allowed now
    #ifdef Q_OS_UNIX
    static bool mkpath(const INTERNALTYPEPATH &file_path, const mode_t &mode=0755);
    static bool mkdir(const INTERNALTYPEPATH &file_path, const mode_t &mode=0755);
    #else
    static bool mkpath(const INTERNALTYPEPATH &file_path);
    static bool mkdir(const INTERNALTYPEPATH &file_path);
    #endif
    #ifdef WIDESTRING
    static INTERNALTYPEPATH stringToInternalString(const std::string& utf8);
    static std::string internalStringTostring(const INTERNALTYPEPATH& utf16);
    #else
    static std::string stringToInternalString(const std::string& utf8);
    static std::string internalStringTostring(const std::string& utf16);
    #endif
    #ifdef Q_OS_WIN32
    static std::wstring toFinalPath(std::wstring path);
    static std::string toFinalPath(std::string path);
    static bool unlink(const std::wstring &path);
    static std::string GetLastErrorStdStr();
    #else
    static bool unlink(const INTERNALTYPEPATH &path);//return true if sucess
    #endif

    static int64_t readFileMDateTime(const INTERNALTYPEPATH &source);
    static bool is_symlink(const char * const filename);
    static bool is_symlink(const INTERNALTYPEPATH &filename);
    static bool is_file(const char * const filename);
    static bool is_file(const INTERNALTYPEPATH &filename);
    static bool is_dir(const char * const filename);
    static bool is_dir(const INTERNALTYPEPATH &filename);
    static bool exists(const char * const filename);
    static bool exists(const INTERNALTYPEPATH &filename);
    static int64_t file_stat_size(const INTERNALTYPEPATH &filename);
    static int64_t file_stat_size(const char * const filename);
    static bool entryInfoList(const INTERNALTYPEPATH &path, std::vector<INTERNALTYPEPATH> &list);
    static bool rmdir(const INTERNALTYPEPATH &path);
    struct dirent_uc
    {
        #ifdef Q_OS_WIN32
        int64_t size;
        #endif
        INTERNALTYPEPATH d_name;
        bool isFolder;
    };
    static bool entryInfoList(const INTERNALTYPEPATH &path, std::vector<dirent_uc> &list);
    void setMkFullPath(const bool mkFullPath);
    /*static int fseeko64(FILE *__stream, uint64_t __off, int __whence);
    static int ftruncate64(int __fd, uint64_t __length);*/
    static bool rename(const INTERNALTYPEPATH &source, const INTERNALTYPEPATH &destination);
protected:
    void run();
    virtual void resetExtraVariable();
    bool isSame();
    bool destinationExists();

    //different pre-operation
    bool checkAlwaysRename();///< return true if has been renamed
    bool canBeMovedDirectly() const;
    bool canBeCopiedDirectly() const;

    //fonction to edit the file date time
    bool readSourceFileDateTime(const INTERNALTYPEPATH &source);
    bool writeDestinationFileDateTime(const INTERNALTYPEPATH &destination);
    bool readSourceFilePermissions(const INTERNALTYPEPATH &source);
    bool writeDestinationFilePermissions(const INTERNALTYPEPATH &destination);
signals:
    //internal signal
    void internalStartPostOperation() const;
    void internalStartPreOperation() const;
    //force into the right thread
    void internalTryStartTheTransfer() const;
    //to send state
    void preOperationStopped() const;
    void checkIfItCanBeResumed() const;
    //void transferStarted();//not sended (and not used then)
    void readStopped() const;
    void writeStopped() const;
    void postOperationStopped() const;
    //get dialog
    void fileAlreadyExists(const INTERNALTYPEPATH &info,const INTERNALTYPEPATH &info2,const bool &isSame) const;
    void errorOnFile(const INTERNALTYPEPATH &info,const std::string &string,const ErrorType &errorType=ErrorType_Normal) const;
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,std::string fonction,std::string text,std::string file,int ligne) const;
    void tryPutAtBottom() const;
    /// \brief update the transfer stat
    void pushStat(const TransferStat &stat,const uint64_t &pos) const;

    void setFileRenameSend(const std::string &nameForRename);
    void setAlwaysFileExistsActionSend(const FileExistsAction &action);
public slots:
    /// \brief to set files to transfer
    virtual bool setFiles(const INTERNALTYPEPATH& source,const int64_t &size,const INTERNALTYPEPATH& destination,const Ultracopier::CopyMode &mode);
    /// \brief to set the new name of the destination
    void setFileRename(const std::string &nameForRename);
    /// \brief to start the transfer of data
    void setAlwaysFileExistsAction(const FileExistsAction &action);
    /// \brief set the copy info and options before runing
    void setRightTransfer(const bool doRightTransfer);
    /// \brief set buffer
    virtual void setBuffer(const bool buffer);
    /// \brief set keep date
    void setKeepDate(const bool keepDate);
    /// \brief put the current file at bottom
    void putAtBottom();

    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    void setRsync(const bool rsync);
    #endif

    void setRenamingRules(const std::string &firstRenamingRule,const std::string &otherRenamingRule);

    void setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles);
    void setRenameTheOriginalDestination(const bool &renameTheOriginalDestination);
    void set_updateMount();
protected:
    void setFileRenameInternal(const std::string &nameForRename);
    void setAlwaysFileExistsActionInternal(const FileExistsAction &action);
    enum MoveReturn
    {
        MoveReturn_skip=0,
        MoveReturn_moved=1,
        MoveReturn_error=2
    };

    Ultracopier::CopyMode		mode;
    std::atomic<TransferStat>	transfer_stat;
    bool			doRightTransfer;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    bool            rsync;
    #endif
    bool			keepDate;
    bool            mkFullPath;
    volatile bool	stopIt;
    #ifdef Q_OS_WIN32
    int stopItWin;
    #endif
    DriveManagement driveManagement;
    volatile bool	canStartTransfer;
    bool			retry;
    INTERNALTYPEPATH	source;
    INTERNALTYPEPATH		destination;
    int64_t			size;
    volatile FileExistsAction	fileExistsAction;
    volatile FileExistsAction	alwaysDoFileExistsAction;
    bool			needSkip,needRemove;
    int             id;
    bool            deletePartiallyTransferredFiles;
    std::string			firstRenamingRule;
    std::string			otherRenamingRule;
    //error management
    bool            renameTheOriginalDestination;
    bool			fileContentError;
    bool            doTheDateTransfer;
    int             parallelizeIfSmallerThan;
    //error management
    bool			writeError;
    bool			readError;
    std::regex renameRegex;
    #ifdef Q_OS_UNIX
        utimbuf butime;
    #else
        #ifdef Q_OS_WIN32
            FILETIME ftCreate, ftAccess, ftWrite;
            std::regex regRead;
        #else
            #error Not unix, not windows, fix this
        #endif
    #endif
    #ifdef Q_OS_UNIX
    struct stat permissions;
    #else
    PSECURITY_DESCRIPTOR PSecurityD;
    PACL dacl;
    #endif
    bool havePermission;
    //to send state
    bool sended_state_preOperationStopped;
    //different post-operation
    bool doFilePostOperation();
protected:
    QElapsedTimer startTransferTime;
    bool haveTransferTime;
};

#endif // TRANSFERTHREAD_H
