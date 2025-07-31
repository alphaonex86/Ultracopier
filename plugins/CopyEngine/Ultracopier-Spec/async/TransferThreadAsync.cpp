//presume bug linked as multple paralelle inode to resume after "overwrite"
//then do overwrite node function to not re-set the file name

#include "TransferThreadAsync.h"
#include <string>
#include <dirent.h>

#ifdef Q_OS_WIN32
#include <accctrl.h>
#include <aclapi.h>
#include <winbase.h>

#define REPARSE_MOUNTPOINT_HEADER_SIZE   8

typedef struct _REPARSE_DATA_BUFFER {
  ULONG  ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG  Flags;
      WCHAR  PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR  PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

/*memo> HANDLE hToken = NULL;
TOKEN_PRIVILEGES tp;
try {
  if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) throw ::GetLastError();
  if (!::LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &tp.Privileges[0].Luid))  throw ::GetLastError();
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))  throw ::GetLastError();
}
catch (DWORD) {
    ok=false;
}
if (hToken)
    ::CloseHandle(hToken);*/
#endif

#include "../../../../cpp11addition.h"

#ifndef Q_OS_WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

TransferThreadAsync::TransferThreadAsync() :
    transferProgression(0)
{
    haveStartTime=false;
    #ifndef Q_OS_UNIX
    PSecurityD=NULL;
    dacl=NULL;
    #endif
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] start: ")+QString::number((qint64)QThread::currentThreadId())));
    //the error push
    readThread.setWriteThread(&writeThread);

    TransferThread::run();
    if(!connect(this,&TransferThreadAsync::internalStartPostOperation,        this,                   &TransferThreadAsync::doFilePostOperation,  Qt::QueuedConnection))
        abort();

    //the thread change operation
    if(!connect(this,&TransferThread::internalStartPreOperation,	this,					&TransferThreadAsync::preOperation,          Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThread::internalStartPostOperation,	this,					&TransferThreadAsync::postOperation,         Qt::QueuedConnection))
        abort();
    if(!connect(this,&TransferThread::internalTryStartTheTransfer,	this,					&TransferThreadAsync::internalStartTheTransfer,      Qt::QueuedConnection))
        abort();

    //the error push
    if(!connect(&readThread,&ReadThread::error,                     this,					&TransferThreadAsync::read_error,      	Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::error,                   this,					&TransferThreadAsync::write_error,         Qt::QueuedConnection))
        abort();
    //the state change operation
    if(!connect(&readThread,&ReadThread::readIsStopped,             this,					&TransferThreadAsync::read_readIsStopped,         Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::readIsStopped,             &writeThread,			&WriteThread::endIsDetected,            Qt::QueuedConnection))
        abort();
    if(!connect(&readThread,&ReadThread::closed,                    this,					&TransferThreadAsync::read_closed,          Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::closed,                  this,					&TransferThreadAsync::write_closed,         Qt::QueuedConnection))
        abort();

    if(!connect(this,					&TransferThreadAsync::openRead,&readThread,&ReadThread::openRead,                              Qt::QueuedConnection))
        abort();
    if(!connect(this,					&TransferThreadAsync::openWrite,&writeThread,&WriteThread::openWrite,                           Qt::QueuedConnection))
        abort();

    //when both is ready, startRead()
    if(!connect(&readThread,&ReadThread::opened,                    this,					&TransferThreadAsync::read_opened,          Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::opened,                  this,					&TransferThreadAsync::write_opened,         Qt::QueuedConnection))
        abort();

    //error management, just try restart from 0
    if(!connect(&readThread,&ReadThread::resumeAfterErrorByRestartAll,&writeThread,         &WriteThread::flushAndSeekToZero,               Qt::QueuedConnection))
        abort();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(!connect(&readThread,&ReadThread::debugInformation,          this,                   &TransferThreadAsync::debugInformation,  Qt::QueuedConnection))
        abort();
    if(!connect(&writeThread,&WriteThread::debugInformation,        this,                   &TransferThreadAsync::debugInformation,  Qt::QueuedConnection))
        abort();
    if(!connect(&driveManagement,&DriveManagement::debugInformation,this,                   &TransferThreadAsync::debugInformation,	Qt::QueuedConnection))
        abort();
    #endif
    if(!connect(this,&TransferThreadAsync::setFileExistsActionSend,this,                   &TransferThreadAsync::setFileExistsActionInternal,	Qt::QueuedConnection))
        abort();

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: "+std::to_string((int64_t)QThread::currentThreadId()));
    start();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop: "+std::to_string((int64_t)QThread::currentThreadId()));
}

TransferThreadAsync::~TransferThreadAsync()
{
    #ifdef Q_OS_WIN32
    stopItWin=1;
    #endif
    stopIt=true;
    readThread.stop();
    readThread.exit();
    readThread.wait();
    writeThread.stop();
    writeThread.exit();
    writeThread.wait();
    exit(0);
    wait();
    //else cash without this disconnect
    //disconnect(&readThread);
    //disconnect(&writeThread);
    #ifndef Q_OS_UNIX
    if(PSecurityD!=NULL)
    {
        //free(PSecurityD);
        PSecurityD=NULL;
    }
    if(dacl!=NULL)
    {
        //free(dacl);
        dacl=NULL;
    }
    #endif
}

void TransferThreadAsync::run()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: "+std::to_string((int64_t)QThread::currentThreadId()));
    moveToThread(this);
    exec();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop: "+std::to_string((int64_t)QThread::currentThreadId()));
}

void TransferThreadAsync::startTheTransfer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start")+", transfert id: "+std::to_string(transferId));
    if(transferId==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert if transferId==0"));
        return;
    }
    startTransferTime.restart();
    haveTransferTime=true;
    emit internalTryStartTheTransfer();
}

void TransferThreadAsync::internalStartTheTransfer()
{
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(QThread::currentThread()!=this)
        abort();
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start")+", transfert id: "+std::to_string(transferId)+" readError: "+std::to_string(writeError)+" writeError: "+std::to_string(writeError)+" canStartTransfer: "+std::to_string(canStartTransfer));
    if(transfer_stat==TransferStat_Idle)
    {
        if(mode!=Ultracopier::Move)
        {
            /// \bug can pass here because in case of direct move on same media, it return to idle stat directly
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at idle"));
        }
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start")+", transfert id: "+std::to_string(transferId));
    if(transfer_stat==TransferStat_PostOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at PostOperation"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start")+", transfert id: "+std::to_string(transferId));
    if(transfer_stat==TransferStat_Transfer && !readError && !writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at Transfer due to transfer_stat==TransferStat_Transfer (double start?)"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start")+", transfert id: "+std::to_string(transferId));
    if(canStartTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] canStartTransfer is already set to true")+", transfert id: "+std::to_string(transferId)+", transfer_stat: "+std::to_string((int)transfer_stat));
        // call for second time, first time was not ready, if blocked in preop why?
        //ifCanStartTransfer();
        //return;-> try call again, protected by if(transfer_stat!=TransferStat_WaitForTheTransfer /*wait preoperation*/ || !canStartTransfer/*wait start call*/) into ifCanStartTransfer()
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] check how start the transfer"));
    canStartTransfer=true;

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start directly the transfer"));
    ifCanStartTransfer();
}

bool TransferThreadAsync::setFiles(const INTERNALTYPEPATH& source, const int64_t &size, const INTERNALTYPEPATH& destination, const Ultracopier::CopyMode &mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start, source: "+TransferThread::internalStringTostring(source)+
                             ", destination: "+TransferThread::internalStringTostring(destination));
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+TransferThread::internalStringTostring(source)+
                                 ", destination: "+TransferThread::internalStringTostring(destination));
        return false;
    }
    if(!isRunning())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] The current thread is not running");
    if(!TransferThread::setFiles(source,size,destination,mode))
        return false;
    transferProgression=0;
    //transferSize=0;//set by ListThread at currentTransferThread->transferSize=currentActionToDoTransfer.size; very important to do parallel small file
    sended_state_readStopped=false;
    readIsClosedVariable=false;
    writeIsClosedVariable=false;
    readIsOpenVariable=false;
    writeIsOpenVariable=false;
    realMove=false;
    return true;
}

void TransferThreadAsync::resetExtraVariable()
{
    transferProgression=0;
    readIsOpenVariable=false;
    writeIsOpenVariable=false;
    TransferThread::resetExtraVariable();
}

bool TransferThreadAsync::remainFileOpen() const
{
    return remainSourceOpen() || remainDestinationOpen();
}

bool TransferThreadAsync::remainSourceOpen() const
{
    return !readIsClosedVariable;
}

bool TransferThreadAsync::remainDestinationOpen() const
{
    return !writeIsClosedVariable;
}

void TransferThreadAsync::preOperation()
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+
                                 TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination)
                                 +" transfer_stat: "+std::to_string(transfer_stat));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: source: "+
                             TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination));
    haveStartTime=true;
    needRemove=false;
    if(isSame())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is same "+
                                 TransferThread::internalStringTostring(source)+" than "+TransferThread::internalStringTostring(destination));
        return;
    }

    //this case is used only on retry after error
    readThread.stop();
    writeThread.stop();

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after is same");
    /*Why this code?
    if(readError)
    {
        readError=false;
        return;
    }*/
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before destination exists");
    if(destinationExists())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] destination exists: "+TransferThread::internalStringTostring(destination)+", alwaysDoFileExistsAction: "+std::to_string((int)alwaysDoFileExistsAction));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after destination exists");
    /*Why this code?
    if(readError)
    {
        readError=false;
        return;
    }*/
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before keep date");
    #ifdef Q_OS_WIN32
    doTheDateTransfer=!is_symlink(source);
    #else
    doTheDateTransfer=true;
    #endif
    if(doTheDateTransfer)
    {
        doTheDateTransfer=readSourceFileDateTime(source);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after keep date");
        #ifdef Q_OS_MAC
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] read the source time: "+std::to_string(butime.modtime));
        #endif
        if(!doTheDateTransfer)
        {
            //will have the real error at source open
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] unable to read the source time: "+TransferThread::internalStringTostring(source));
            if(keepDate)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"!doTheDateTransfer && keepDate");
                emit errorOnFile(source,tr("Wrong modification date or unable to get it, you can disable time transfer to do it").toStdString());
                return;
            }
        }
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before perm");
    if(doRightTransfer)
        havePermission=readSourceFilePermissions(source);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after perm");
    transfer_stat=TransferStat_WaitForTheTransfer;
    ifCanStartTransfer();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit preOperationStopped()");
    emit preOperationStopped();
}

void TransferThreadAsync::postOperation()
{
    doFilePostOperation();
}

#ifdef Q_OS_WIN32
DWORD CALLBACK progressRoutine(
  LARGE_INTEGER TotalFileSize,
  LARGE_INTEGER TotalBytesTransferred,
  LARGE_INTEGER StreamSize,
  LARGE_INTEGER StreamBytesTransferred,
  DWORD dwStreamNumber,
  DWORD dwCallbackReason,
  HANDLE hSourceFile,
  HANDLE hDestinationFile,
  LPVOID lpData
)
{
    (void)TotalFileSize;
    (void)TotalBytesTransferred;
    (void)StreamSize;
    (void)StreamBytesTransferred;
    (void)dwStreamNumber;
    (void)dwCallbackReason;
    (void)hSourceFile;
    (void)hDestinationFile;
    (void)lpData;

    static_cast<TransferThreadAsync *>(lpData)->setProgression(TotalBytesTransferred.QuadPart,TotalFileSize.QuadPart);
    return PROGRESS_CONTINUE;
}

void TransferThreadAsync::setProgression(const uint64_t &pos, const uint64_t &size)
{
    if(transfer_stat==TransferStat_Transfer)
    {
        if(pos==size)
            emit readStopped();
        transferProgression=pos;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+
                             "] transfer_stat:"+std::to_string(transfer_stat)+
                             ", canStartTransfer: "+std::to_string(canStartTransfer)+", transfert id: "+std::to_string(transferId));
    }
}
#endif

/// \brief set buffer
void TransferThreadAsync::setBuffer(const bool buffer)
{
    writeThread.buffer=buffer;
}

void TransferThreadAsync::ifCanStartTransfer()
{
    realMove=false;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start "+internalStringTostring(source)+"->"+internalStringTostring(destination));
    if(transfer_stat!=TransferStat_WaitForTheTransfer /*wait preoperation*/ || !canStartTransfer/*wait start call*/)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+
                                 "] transfer_stat:"+std::to_string(transfer_stat)+
                                 ", canStartTransfer: "+std::to_string(canStartTransfer)+", transfert id: "+std::to_string(transferId));
        //preOperationStopped();//tiger to seam maybe is can be started, maybe this generate a bug
        return;
    }
    transfer_stat=TransferStat_Transfer;

    #ifdef WIDESTRING
    const size_t destinationIndex=destination.rfind(L'/');
    if(destinationIndex!=std::string::npos && destinationIndex<destination.size())
    {
        const std::wstring &path=destination.substr(0,destinationIndex);
        if(!is_dir(path))
            if(!TransferThread::mkpath(path))
            {
                #ifdef Q_OS_WIN32
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to create the destination folder: "+internalStringTostring(path)+" "+TransferThread::GetLastErrorStdStr());
                emit errorOnFile(destination,tr("Unable to create the destination folder: ").toStdString()+TransferThread::GetLastErrorStdStr());
                #else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to create the destination folder: "+internalStringTostring(path)+" "+std::to_string(errno));
                emit errorOnFile(destination,tr("Unable to create the destination folder, errno: %1").arg(QString::number(errno)).toStdString());
                #endif
                return;
            }
    }
    #else
    const size_t destinationIndex=destination.rfind('/');
    if(destinationIndex!=std::string::npos && destinationIndex<destination.size())
    {
        const std::string &path=destination.substr(0,destinationIndex);
        if(!is_dir(path))
            if(!TransferThread::mkpath(path))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to create the destination folder: "+path);
                #ifdef Q_OS_WIN32
                emit errorOnFile(destination,tr("Unable to create the destination folder: ")+TransferThread::GetLastErrorStdStr());
                #else
                emit errorOnFile(destination,tr("Unable to create the destination folder, errno: %1").arg(QString::number(errno)).toStdString());
                #endif
                return;
            }
    }
    #endif

    /* http://www.flexhex.com/docs/articles/hard-links.phtml
     * Do here the smblink logic
     * Under windows: BOOLEAN CreateSymbolicLinkA(
  LPCSTR lpSymlinkFileName,
  LPCSTR lpTargetFileName,
  DWORD  dwFlags
  unix: int symlink(const char *target, const char *linkpath);
);

    */

    emit pushStat(transfer_stat,transferId);
    realMove=(mode==Ultracopier::Move && driveManagement.isSameDrive(
                       internalStringTostring(source),
                       internalStringTostring(destination)
                       ));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start copy");
    needRemove=true;
    bool successFull=false;
    readError=false;
    writeError=false;
    if(realMove)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start real move, source "+TransferThread::internalStringTostring(source)+" exists: "+std::to_string(TransferThread::exists(source)));
        if(TransferThread::exists(source))
            successFull=TransferThread::rename(source,destination);
        #ifdef Q_OS_UNIX
        if(!successFull && errno==18)
        {
            //try full move
            realMove=false;
            openRead(source,mode);
            openWrite(destination,0);
            return;
        }
        #endif
    }
    else
    {
        #ifdef Q_OS_WIN32
        bool isJunction=false;
        bool isSymlink=false;
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        BOOL r = GetFileAttributesExW(TransferThread::toFinalPath(source).c_str(), GetFileExInfoStandard, &fileInfo);
        if(r != FALSE)
        {
            /*isJunction = fileInfo.dwFileAttributes != INVALID_FILE_ATTRIBUTES &&
               (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);*/
            isSymlink = fileInfo.dwFileAttributes != INVALID_FILE_ATTRIBUTES &&
               (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
        }
        if(isSymlink)
        {
            BYTE buf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
            REPARSE_DATA_BUFFER& ReparseBuffer = (REPARSE_DATA_BUFFER&)buf;
            DWORD dwRet=0;
            HANDLE hDir = ::CreateFile(TransferThread::toFinalPath(source).c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
            if (hDir != INVALID_HANDLE_VALUE)
            {
                BOOL ioc = ::DeviceIoControl(hDir, FSCTL_GET_REPARSE_POINT, NULL, 0, &ReparseBuffer,
                MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwRet, NULL);
                ::CloseHandle(hDir);
                if(ioc!=FALSE)
                {
                    if(ReparseBuffer.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
                    {
                        //printf("%S\n",ReparseBuffer.MountPointReparseBuffer.PathBuffer);
                        isJunction=true;
                    }
                    else
                    {
                        //printf("%S\n",ReparseBuffer.SymbolicLinkReparseBuffer.PathBuffer);
                    }
                }
                else
                {
                    const std::string &strError=TransferThread::GetLastErrorStdStr();
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error: "+
                                             GetLastErrorStdStr()+" "+TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                             " "+strError
                                             );
                    readError=true;
                    writeError=false;
                    emit errorOnFile(source,strError);
                    return;
                }
            }
            else
            {
                const std::string &strError=TransferThread::GetLastErrorStdStr();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error: "+
                                         GetLastErrorStdStr()+" "+TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                         " "+strError
                                         );
                readError=true;
                writeError=false;
                emit errorOnFile(source,strError);
                return;
            }
            if(isJunction)//junction
            {
                std::wstring cleanPath(ReparseBuffer.MountPointReparseBuffer.PathBuffer);
                if(cleanPath.substr(0,4)==L"\\??\\")
                    cleanPath=cleanPath.substr(4);
                successFull=TransferThreadAsync::mkJunction(
                            TransferThread::toFinalPath(destination).c_str(),
                            cleanPath.c_str()
                            );
                if(successFull==FALSE)
                {
                    const std::string &strError=TransferThread::GetLastErrorStdStr();
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error: "+
                                             GetLastErrorStdStr()+" "+TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                             " "+strError
                                             );
                    readError=true;
                    writeError=true;
                    emit errorOnFile(destination,strError);
                }
            }
            else//symlink or symlinkD
            {
                successFull=CopyFileExW(TransferThread::toFinalPath(source).c_str(),TransferThread::toFinalPath(destination).c_str(),
                    (LPPROGRESS_ROUTINE)progressRoutine,this,&stopItWin,COPY_FILE_ALLOW_DECRYPTED_DESTINATION | 0x00000800);//0x00000800 is COPY_FILE_COPY_SYMLINK
                if(successFull==FALSE)
                {
                    const std::string &strError=TransferThread::GetLastErrorStdStr();
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error: "+
                                             GetLastErrorStdStr()+" "+TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                             " "+strError
                                             );
                    readError=true;
                    writeError=true;
                    emit errorOnFile(destination,strError);
                }
            }
        }
        else
        #else
        if(TransferThread::is_symlink(source))
        {
            realMove=true;
            bool isFileOrSymlink=false;
            {
                struct stat p_statbuf;
                if (lstat(TransferThread::internalStringTostring(destination).c_str(), &p_statbuf) < 0)
                {}
                else if (S_ISLNK(p_statbuf.st_mode))
                    isFileOrSymlink=true;
                else if (S_ISREG(p_statbuf.st_mode))
                    isFileOrSymlink=true;
            }
            if(isFileOrSymlink)
                if(!unlink(destination))
                {
                    const int terr=errno;
                    if(terr!=2)
                    {
                        const std::string &strError=strerror(terr);
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error "+
                                                 TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                                 " "+strError+"("+std::to_string(terr)+")"
                                                 );
                        readError=false;
                        writeError=true;
                        emit errorOnFile(destination,strError);
                        return;
                    }
                }
            std::vector<char> buf(512);
            ssize_t s=0;
            do {
              buf.resize(buf.size()*2);
              s=readlink(TransferThread::internalStringTostring(source).c_str(),buf.data(),buf.size());
            } while (s == (ssize_t)buf.size());
            if (s!=-1) {
              buf.resize(s + 1);
              buf[s]='\0';
            }
            if(s<0)
            {
                const int terr=errno;
                const std::string &strError=strerror(terr);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error "+
                                         TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                         " "+strError+"("+std::to_string(terr)+")"
                                         );
                readError=true;
                writeError=false;
                emit errorOnFile(source,strError);
                return;
            }
            else
            {
                if(symlink(buf.data(),TransferThread::internalStringTostring(destination).c_str())!=0)
                {
                    const int terr=errno;
                    const std::string &strError=strerror(terr);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error "+
                                             buf.data()+"->"+TransferThread::internalStringTostring(destination)+
                                             " "+strError+"("+std::to_string(terr)+")"
                                             );
                    readError=true;
                    writeError=false;
                    emit errorOnFile(destination,strError);
                    return;
                }
            }
            successFull=true;
        }
        else
        #endif
        {
            #ifdef Q_OS_WIN32
            if(native_copy)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,internalStringTostring(source)+" to "+internalStringTostring(destination)+": native_copy enabled");
                successFull=CopyFileExW(TransferThread::toFinalPath(source).c_str(),TransferThread::toFinalPath(destination).c_str(),
                    (LPPROGRESS_ROUTINE)progressRoutine,this,&stopItWin,COPY_FILE_ALLOW_DECRYPTED_DESTINATION | 0x00000800);//0x00000800 is COPY_FILE_COPY_SYMLINK
                if(successFull==FALSE)
                {
                    const std::string &strError=TransferThread::GetLastErrorStdStr();
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error: "+
                                             GetLastErrorStdStr()+" "+TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                             " "+strError
                                             );
                    readError=true;
                    writeError=true;
                    emit errorOnFile(destination,strError);
                }
            }
            else
            #endif
            {
                openRead(source,mode);
                openWrite(destination,0);
                return;
            }
        }
    }
    if(!successFull)
    {
        #ifdef Q_OS_WIN32
        const std::string &strError=TransferThread::GetLastErrorStdStr();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error: "+
                                 GetLastErrorStdStr()+" "+TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                 " "+strError
                                 );
        #else
        const int terr=errno;
        const std::string &strError=strerror(terr);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error "+
                                 TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination)+
                                 " "+strError+"("+std::to_string(terr)+")"
                                 );
        #endif
        if(stopIt)
        {
            if(!source.empty())
                if(exists(source) && source!=destination)
                    unlink(destination);
            resetExtraVariable();
            return;//and reset?
        }
        #ifdef Q_OS_WIN32
        readError=true;
        writeError=true;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"!successFull");
        emit errorOnFile(destination,strError);
        #else
        if(readError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"!successFull on read");
            emit errorOnFile(source,strError);
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"!successFull on write");
            emit errorOnFile(destination,strError);
        }
        #endif
        return;
    }
    readIsClosedVariable=true;
    writeIsClosedVariable=true;
    checkIfAllIsClosedAndDoOperations();
}

void TransferThreadAsync::checkIfAllIsClosedAndDoOperations()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop copy");
    if((readError || writeError) && !needSkip && !stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] resolve error before progress");
        return;
    }
    if(remainFileOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] remainFileOpen()");
        return;
    }
    if(!source.empty() && needRemove && (stopIt || needSkip))
        if(is_file(source) && source!=destination)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"remove because: is_file(source): "+std::to_string(is_file(source))+", source: "+TransferThread::internalStringTostring(source));
            unlink(destination);
        }
    transfer_stat=TransferStat_Idle;

    transferSize=readThread.getLastGoodPosition();

    if(mode==Ultracopier::Move && !realMove)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] remove mode==Ultracopier::Move && !realMove: "+
                                 TransferThread::internalStringTostring(source));
        if(exists(destination))
            if(!unlink(source))
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] move and unable to remove: "+
                                         TransferThread::internalStringTostring(source)+std::string(" ")+
                         #ifdef Q_OS_WIN32
                                         GetLastErrorStdStr()
                         #else
                                         strerror(errno)
                         #endif
                                         );
    }
    transfer_stat=TransferStat_PostTransfer;
    emit pushStat(transfer_stat,transferId);
    transfer_stat=TransferStat_PostOperation;
    emit pushStat(transfer_stat,transferId);
    doFilePostOperation();

    source.clear();
    destination.clear();
    resetExtraVariable();
    //don't need remove because have correctly finish (it's not in: have started)
    needRemove=false;
    needSkip=false;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit postOperationStopped() transfer_stat=TransferStat_Idle");
    transfer_stat=TransferStat_Idle;
    emit postOperationStopped();
}

//stop the current copy
void TransferThreadAsync::stop()
{
    #ifdef Q_OS_WIN32
    stopItWin=1;
    #endif
    stopIt=true;
    haveStartTime=false;
    if(transfer_stat==TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] transfer_stat==TransferStat_Idle");
        return;
    }
    if(transfer_stat==TransferStat_PreOperation)
    {
        transfer_stat=TransferStat_Idle;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] transfer_stat==TransferStat_PreOperation");
        return;
    }
    if(realMove)
    {
        if(readError || writeError)
            transfer_stat=TransferStat_Idle;
        return;
    }
    readThread.stop();
    writeThread.stop();
}

//skip the copy
void TransferThreadAsync::skip()
{
    #ifdef Q_OS_WIN32
    stopItWin=1;
    #endif
    stopIt=true;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start with stat: "+std::to_string(transfer_stat));
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_WaitForTheTransfer:
        //needRemove=true;never put that's here, can product destruction of the file
    case TransferStat_PreOperation:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        needSkip=true;
        //check if all is source and destination is closed
        if(remainFileOpen())
        {
            if(remainSourceOpen())
                readThread.stop();
            if(remainDestinationOpen())
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        resetExtraVariable();
        break;
    case TransferStat_Transfer:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        //needRemove=true;never put that's here, can product destruction of the file
        needSkip=true;
        if(realMove)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+
                                     "] Do the direct FS fake close, realMove: "+std::to_string(realMove));
            /*readThread.fakeReadIsStarted();
            writeThread.fakeWriteIsStarted();
            readThread.fakeReadIsStopped();
            writeThread.fakeWriteIsStopped();*/
            emit readStopped();
            emit postOperationStopped();
            transfer_stat=TransferStat_Idle;
            emit pushStat(transfer_stat,transferId);
            return;
        }
        writeThread.flushBuffer();
        if(remainFileOpen())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] remainFileOpen");
            if(remainSourceOpen())
                readThread.stop();
            if(remainDestinationOpen())
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] wait nothing, just quit");
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        if(!source.empty() && needRemove)
            if(exists(source) && source!=destination)
                unlink(destination);
        break;
    case TransferStat_PostTransfer:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        //needRemove=true;never put that's here, can product destruction of the file
        needSkip=true;
        if(realMove)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] Do the direct FS fake close, realMove: "+std::to_string(realMove));
            readThread.fakeReadIsStarted();
            writeThread.fakeWriteIsStarted();
            readThread.fakeReadIsStopped();
            writeThread.fakeWriteIsStopped();
            return;
        }
        writeThread.flushBuffer();
        if(remainFileOpen())
        {
            if(remainSourceOpen())
                readThread.stop();
            if(remainDestinationOpen())
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        break;
    case TransferStat_PostOperation:
        break;
    default:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] can skip in this state: "+std::to_string(transfer_stat));
        return;
    }
    //can be reset
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] transfer_stat=TransferStat_Idle");
    transfer_stat=TransferStat_Idle;
    //emit to manager with List Thread
    emit postOperationStopped();
}

//return info about the copied size
int64_t TransferThreadAsync::copiedSize()
{
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Transfer:
        if(transferProgression>0)//then native copy started, read/write thread not used
            return transferProgression;
        else
            return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
        break;
    case TransferStat_PostOperation:
        if(transferProgression>0)//then native copy started, read/write thread not used
            return transferSize;
        else
            return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
        break;
    case TransferStat_PostTransfer:
        if(transferProgression>0)//then native copy started, read/write thread not used
            return transferProgression;
        else
            return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
    default:
        return 0;
    }
}

//retry after error
void TransferThreadAsync::putAtBottom()
{
    emit tryPutAtBottom();
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
/// \brief set rsync
void TransferThreadAsync::setRsync(const bool rsync)
{
    this->rsync=rsync;
}
#endif

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void TransferThreadAsync::setId(int id)
{
    TransferThread::setId(id);
}

char TransferThreadAsync::readingLetter() const
{
    switch(static_cast<TransferStat>(readThread.status))
    {
    case TransferStat_Idle:
        return '_';
    break;
    case TransferStat_PreOperation:
        return 'I';
    break;
    case TransferStat_WaitForTheTransfer:
        return 'W';
    break;
    case TransferStat_Transfer:
        return 'C';
    break;
    case TransferStat_PostTransfer:
        return 'R';
    break;
    case TransferStat_PostOperation:
        return 'P';
    break;
    default:
        return '?';
    }
}

char TransferThreadAsync::writingLetter() const
{
    switch(static_cast<TransferStat>(writeThread.status))
    {
    case TransferStat_Idle:
        return '_';
    break;
    case TransferStat_PreOperation:
        return 'I';
    break;
    case TransferStat_WaitForTheTransfer:
        return 'W';
    break;
    case TransferStat_Transfer:
        return 'C';
    break;
    case TransferStat_PostTransfer:
        return 'R';
    break;
    case TransferStat_PostOperation:
        return 'P';
    break;
    default:
        return '?';
    }
}
#endif

//not copied size, ...
uint64_t TransferThreadAsync::realByteTransfered() const
{
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Transfer:
        if(transferProgression>0)//then native copy started, read/write thread not used
            return transferProgression;
        else
            return readThread.getLastGoodPosition();
    case TransferStat_PostTransfer:
        return readThread.getLastGoodPosition();
    case TransferStat_PostOperation:
        return readThread.getLastGoodPosition();
    default:
        return 0;
    }
}

//first is read, second is write, related to file
std::pair<uint64_t, uint64_t> TransferThreadAsync::progression() const
{
    std::pair<uint64_t,uint64_t> returnVar;
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Transfer:
        if(transferProgression>0)//then native copy started, read/write thread not used
        {
            returnVar.first=transferProgression;
            returnVar.second=transferProgression;
        }
        else
        {
            returnVar.first=readThread.getLastGoodPosition();
            returnVar.second=writeThread.getLastGoodPosition();
        }
        /*if(returnVar.first<returnVar.second)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+QStringLiteral("] read is smaller than write"));*/
    break;
    case TransferStat_PostTransfer:
        returnVar.first=transferSize;
        returnVar.second=transferSize;
        /*if(returnVar.first<returnVar.second)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+QStringLiteral("] read is smaller than write"));*/
    break;
    case TransferStat_PostOperation:
        returnVar.first=transferSize;
        returnVar.second=transferSize;
    break;
    default:
        returnVar.first=0;
        returnVar.second=0;
    }
    return returnVar;
}

void TransferThreadAsync::setFileExistsAction(const FileExistsAction &action)
{
    emit setFileExistsActionSend(action);
}

void TransferThreadAsync::setFileExistsActionInternal(const FileExistsAction &action)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+
                                 TransferThread::internalStringTostring(source)+(", destination: ")+TransferThread::internalStringTostring(destination));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] action: ")+std::to_string(action));
    if(action!=FileExists_Rename)
        fileExistsAction	= action;
    else
    {
        //always rename pass here
        fileExistsAction	= action;
        alwaysDoFileExistsAction=action;
    }
    if(action==FileExists_Skip)
    {
        skip();
        return;
    }
    resetExtraVariable();
    emit internalStartPreOperation();
}

#ifndef Q_OS_WIN32
/*bool TransferThreadAsync::copy(const char *from,const char *to)
{
    transferProgression=0;
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
    {
        readError=true;
        return false;
    }
    #ifdef Q_OS_LINUX
    posix_fadvise(fd_from, 0, 0, POSIX_FADV_WILLNEED);
    posix_fadvise(fd_from, 0, 0, POSIX_FADV_SEQUENTIAL);
    posix_fadvise(fd_from, 0, 0, POSIX_FADV_NOREUSE);
    #endif

    // | O_DSYNC slow down
    // | O_SYNC slow down
    // O_DIRECT Invalid argument
    int flags=O_WRONLY | O_CREAT;
    //if(!buffer)         flags|=??;
    fd_to = open(to, flags, 0666);
    if (fd_to < 0)
    {
        writeError=true;
        goto out_error;
    }
    #ifdef Q_OS_LINUX
    posix_fadvise(fd_to, 0, 0, POSIX_FADV_WILLNEED);
    posix_fadvise(fd_to, 0, 0, POSIX_FADV_SEQUENTIAL);
    posix_fadvise(fd_to, 0, 0, POSIX_FADV_NOREUSE);
    #endif

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        if(stopIt)
        {
            close(fd_to);
            close(fd_from);
            return false;
        }
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);
            if(stopIt)
            {
                close(fd_to);
                close(fd_from);
                return false;
            }

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
                transferProgression += nwritten;
            }
            else if (errno != EINTR)
            {
                writeError=true;
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        ftruncate(fd_to,transferProgression);
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            readError=true;
            goto out_error;
        }
        if (close(fd_from) < 0)
            return -1;
        if(stopIt)
            return -1;

        emit readStopped();
        return true;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return false;
}*/
#endif

//implemente to connect async
void TransferThreadAsync::read_error()
{
    if(readError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already in read error!");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    fileContentError	= true;
    readError		= true;
    //writeIsReadyVariable	= false;//wrong because write can be ready here
    if(!writeError)//already display error for the write
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"read error && !writeError");
        emit errorOnFile(source,readThread.errorString());
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"read error && writeError");
}

void TransferThreadAsync::read_readIsStopped()
{
    if(realMove)
        return;
    if(!sended_state_readStopped)
    {
        sended_state_readStopped=true;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit readIsStopped()");
        emit readStopped();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] drop dual read stopped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    canStartTransfer=false;
    //check here if need start checksuming or not
    transfer_stat=TransferStat_PostTransfer;
    emit pushStat(transfer_stat,transferId);
}

void TransferThreadAsync::read_closed()
{
    if(realMove)
        return;
    if(readIsClosedVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] double event dropped"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readIsClosedVariable=true;
    checkIfAllIsClosedAndDoOperations();
}

void TransferThreadAsync::write_error()
{
    if(writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already in write error!");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    fileContentError		= true;
    writeError			= true;
    if(!readError)//already display error for the read
        emit errorOnFile(destination,writeThread.errorString());
}

void TransferThreadAsync::write_closed()
{
    if(realMove)
        return;
    if(writeIsClosedVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    writeIsClosedVariable=true;
    if(stopIt && needRemove && source!=destination)
    {
        if(is_file(source))
            unlink(destination);
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] try destroy the destination when the source don't exists"));
    }
    checkIfAllIsClosedAndDoOperations();
}

#ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT


//set block size in Bytes for speed limitation
bool TransferThreadAsync::setBlockSize(const unsigned int blockSize)
{
    bool read=readThread.setBlockSize(blockSize);
    bool write=writeThread.setBlockSize(blockSize);
    return (read && write);
}

//set the current max speed in KB/s
void TransferThreadAsync::setMultiForBigSpeed(const int &multiForBigSpeed)
{
    readThread.setMultiForBigSpeed(multiForBigSpeed);
    writeThread.setMultiForBigSpeed(multiForBigSpeed);
}

void TransferThreadAsync::timeOfTheBlockCopyFinished()
{
    readThread.timeOfTheBlockCopyFinished();
    writeThread.timeOfTheBlockCopyFinished();
}
#endif

//pause the copy
void TransferThreadAsync::pause()
{
    //only pause/resume during the transfer of file data
    //from transfer_stat!=TransferStat_Idle because it resume at wrong order
    if(transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_PostTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] wrong stat to put in pause");
        return;
    }
    haveStartTime=false;
    readThread.pause();
    writeThread.pause();
}

//resume the copy
void TransferThreadAsync::resume()
{
    //only pause/resume during the transfer of file data
    //from transfer_stat!=TransferStat_Idle because it resume at wrong order
    if(transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_PostTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] wrong stat to put in pause");
        return;
    }
    readThread.resume();
    writeThread.resume();
}

//when both is ready, startRead()
void TransferThreadAsync::read_opened()
{
    if(readIsOpenVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already readIsOpenVariable=true");
        return;
    }
    readIsOpenVariable=true;
    transferSize=readThread.size();
    if(writeIsOpenVariable)
        readThread.startRead();
}

void TransferThreadAsync::write_opened()
{
    if(writeIsOpenVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already writeIsOpenVariable=true");
        return;
    }
    writeIsOpenVariable=true;
    if(readIsOpenVariable)
        readThread.startRead();
}

#ifdef Q_OS_WIN32
bool TransferThreadAsync::mkJunction(LPCWSTR szJunction, LPCWSTR szPath)
{
  BYTE buf[sizeof(REPARSE_DATA_BUFFER) + MAX_PATH * sizeof(WCHAR)];
  REPARSE_DATA_BUFFER& ReparseBuffer = (REPARSE_DATA_BUFFER&)buf;

  if (!::CreateDirectoryW(szJunction, NULL))
    return false;
  HANDLE hDir = ::CreateFileW(szJunction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if (hDir == INVALID_HANDLE_VALUE)
      return false;

  memset(buf, 0, sizeof(buf));
  ReparseBuffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
  int len = wcslen(szPath)+1;
  ReparseBuffer.MountPointReparseBuffer.PrintNameOffset = (len--) * sizeof(WCHAR);
  ReparseBuffer.MountPointReparseBuffer.SubstituteNameLength = len * sizeof(WCHAR);
  ReparseBuffer.ReparseDataLength = ReparseBuffer.MountPointReparseBuffer.SubstituteNameLength + 12;
  wcscpy(ReparseBuffer.MountPointReparseBuffer.PathBuffer, szPath);

  DWORD dwRet;
  if (!::DeviceIoControl(hDir, FSCTL_SET_REPARSE_POINT, &ReparseBuffer,
                         ReparseBuffer.ReparseDataLength+REPARSE_MOUNTPOINT_HEADER_SIZE, NULL, 0, &dwRet, NULL))
  {
    ::CloseHandle(hDir);
    ::RemoveDirectoryW(szJunction);
    return false;
  }

  ::CloseHandle(hDir);
  return true;
}
#endif

void TransferThreadAsync::setOsSpecFlags(bool os_spec_flags)
{
    readThread.setOsSpecFlags(os_spec_flags);
    writeThread.setOsSpecFlags(os_spec_flags);
}

void TransferThreadAsync::setNativeCopy(bool native_copy)
{
    this->native_copy=native_copy;
}

//////////////////////////////////////////////////////////////////
/////////////////////// Error management /////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThreadAsync::retryAfterError()
{
    /// \warning skip the resetExtraVariable(); to be more exact and resolv some bug
    if(transfer_stat==TransferStat_Idle)
    {
        if(transferId==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+
                                     ("] seam have bug, source: ")+TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination));
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+
                                 "] restart all, source: "+TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination));
        readError=false;
        //writeError=false;
        emit internalStartPreOperation();
        return;
    }
    //opening error
    if(transfer_stat==TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+
                                 "] is not idle, source: "+TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination)+
                                 ", stat: "+std::to_string(transfer_stat));
        readError=false;
        writeError=false;//why was commented?
        emit internalStartPreOperation();
        //tryOpen();-> recheck all, because can be an error into isSame(), rename(), ...
        return;
    }
    //data streaming error
    if(transfer_stat!=TransferStat_PostOperation && transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_PostTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] is not in right stat, source: ")+
                                 TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination)+", stat: "+std::to_string(transfer_stat));
        return;
    }
    if(transfer_stat==TransferStat_PostOperation)
    {
        emit internalStartPostOperation();
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+
                             "] retryAfterError, source: "+TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination));

    /*try restart from 0
    * This is simplest way
    * Simple mean less bug
    * Allow restart with native copy
    * More data check
    * Yes it's less efficient, on failed on source, if after close/reopen the size is same, can be resumed where it stop
    * */
    //emit internalTryStartTheTransfer(); -> wrong in version 2
    resetExtraVariable();
    /*included into resetExtraVariable()
    writeIsOpenVariable=false;
    readError=false;
    writeError=false;*/
    transfer_stat=TransferStat_PreOperation;
    writeThread.flushBuffer();
    emit internalStartPreOperation();
}
