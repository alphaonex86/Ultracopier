//presume bug linked as multple paralelle inode to resume after "overwrite"
//then do overwrite node function to not re-set the file name

#include "TransferThreadAsync.h"
#include <string>
#include <dirent.h>

#ifdef Q_OS_WIN32
#include <accctrl.h>
#include <aclapi.h>
#include <winbase.h>
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
    #ifndef Q_OS_UNIX
    PSecurityD=NULL;
    dacl=NULL;
    #endif
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] start: ")+QString::number((qint64)QThread::currentThreadId())));
    //the error push

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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start"));
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start"));
    if(transfer_stat==TransferStat_Idle)
    {
        if(mode!=Ultracopier::Move)
        {
            /// \bug can pass here because in case of direct move on same media, it return to idle stat directly
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at idle"));
        }
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start"));
    if(transfer_stat==TransferStat_PostOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at PostOperation"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start"));
    if(transfer_stat==TransferStat_Transfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at Transfer"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start"));
    if(canStartTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] canStartTransfer is already set to true"));// call for second time, first time was not ready, if blocked in preop why?
        //ifCanStartTransfer();
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] check how start the transfer"));
    canStartTransfer=true;

    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start directly the transfer"));
    ifCanStartTransfer();
}

bool TransferThreadAsync::setFiles(const INTERNALTYPEPATH& source, const int64_t &size, const INTERNALTYPEPATH& destination, const Ultracopier::CopyMode &mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start, source: "+TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination));
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+TransferThread::internalStringTostring(source)+", destination: "+TransferThread::internalStringTostring(destination));
        return false;
    }
    if(!isRunning())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] The current thread is not running");
    if(!TransferThread::setFiles(source,size,destination,mode))
        return false;
    return true;
}

void TransferThreadAsync::resetExtraVariable()
{
    transferProgression=0;
    TransferThread::resetExtraVariable();
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
    needRemove=false;
    if(isSame())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is same "+
                                 TransferThread::internalStringTostring(source)+" than "+TransferThread::internalStringTostring(destination));
        return;
    }
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] destination exists: "+TransferThread::internalStringTostring(destination));
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
}
#endif

void TransferThreadAsync::ifCanStartTransfer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start "+internalStringTostring(source)+"->"+internalStringTostring(destination));
    if(transfer_stat!=TransferStat_WaitForTheTransfer /*wait preoperation*/ || !canStartTransfer/*wait start call*/)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+
                                 "] transfer_stat:"+std::to_string(transfer_stat)+
                                 ", canStartTransfer: "+std::to_string(canStartTransfer));
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
    emit pushStat(transfer_stat,transferId);
    bool realMove=(mode==Ultracopier::Move && driveManagement.isSameDrive(
                       internalStringTostring(source),
                       internalStringTostring(destination)
                       ));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start copy");
    needRemove=true;
#ifdef Q_OS_WIN32
    BOOL successFull;
    if(realMove)
        successFull=TransferThread::rename(source,destination);
    else
    {
        DWORD flags=COPY_FILE_ALLOW_DECRYPTED_DESTINATION | 0x00000800/*COPY_FILE_COPY_SYMLINK*/;// | 0x00001000/*COPY_FILE_NO_BUFFERING*/
        if(!buffer)
            flags|=0x00001000/*COPY_FILE_NO_BUFFERING*/;
        successFull=CopyFileExW(TransferThread::toFinalPath(source).c_str(),TransferThread::toFinalPath(destination).c_str(),(LPPROGRESS_ROUTINE)progressRoutine,this,&stopItWin,flags);
    }
    if(!successFull)
#else
    bool successFull=false;
    readError=false;
    writeError=false;
    if(realMove)
        successFull=TransferThread::rename(source,destination);
    else
        successFull=copy(TransferThread::internalStringTostring(source).c_str(),TransferThread::internalStringTostring(destination).c_str());
    if(!successFull)
#endif
    {
        #ifdef Q_OS_WIN32
        const std::string &strError=TransferThread::GetLastErrorStdStr();
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error: "+
                                 GetLastErrorStdStr()+" "+TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination));
        #else
        const std::string &strError=strerror(errno);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] stop copy in error "+
                                 TransferThread::internalStringTostring(source)+"->"+TransferThread::internalStringTostring(destination));
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
        emit errorOnFile(destination,strError);
        #else
        if(readError)
            emit errorOnFile(source,strError);
        else
            emit errorOnFile(destination,strError);
        #endif
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stop copy");
    if(mode==Ultracopier::Move && !realMove)
        if(exists(destination))
            if(!unlink(source))
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] move and unable to remove: "+
                                         TransferThread::internalStringTostring(source)+
                         #ifdef Q_OS_WIN32
                                         GetLastErrorStdStr()
                         #else
                                         strerror(errno)
                         #endif
                                         );
    transfer_stat=TransferStat_PostTransfer;
    emit pushStat(transfer_stat,transferId);
    transfer_stat=TransferStat_PostOperation;
    emit pushStat(transfer_stat,transferId);
    doFilePostOperation();

    source.clear();
    destination.clear();
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
    if(transfer_stat==TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"transfer_stat==TransferStat_Idle");
        return;
    }
    exit();
    wait();
    start();
    if(!source.empty() && needRemove)
        if(exists(source) && source!=destination)
            unlink(destination);
    transfer_stat=TransferStat_Idle;
    resetExtraVariable();
}

//retry after error
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
        //writeError=false;
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
    emit internalTryStartTheTransfer();
}

//skip the copy
void TransferThreadAsync::skip()
{
    #ifdef Q_OS_WIN32
    stopItWin=1;
    #endif
    stopIt=true;
    exit();
    wait();
    start();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start with stat: "+std::to_string(transfer_stat));
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_WaitForTheTransfer:
        //needRemove=true;never put that's here, can product destruction of the file
    case TransferStat_PreOperation:
        resetExtraVariable();
        break;
    case TransferStat_Transfer:
        if(!source.empty() && needRemove)
            if(exists(source) && source!=destination)
                unlink(destination);
        break;
    case TransferStat_PostTransfer:
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
    case TransferStat_PostOperation:
    case TransferStat_PostTransfer:
        return transferProgression;
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
    switch(static_cast<TransferStat>(transfer_stat))
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
    switch(static_cast<TransferStat>(transfer_stat))
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
    case TransferStat_PostTransfer:
        return transferProgression;
    case TransferStat_PostOperation:
        return transferSize;
    default:
        return 0;
    }
}

//first is read, second is write
std::pair<uint64_t, uint64_t> TransferThreadAsync::progression() const
{
    std::pair<uint64_t,uint64_t> returnVar;
    switch(static_cast<TransferStat>(transfer_stat))
    {
    case TransferStat_Transfer:
        returnVar.first=transferProgression;
        returnVar.second=transferProgression;
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
bool TransferThreadAsync::copy(const char *from,const char *to)
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

    /* | O_DSYNC slow down*/
    /* | O_SYNC slow down*/
    // O_DIRECT Invalid argument
    int flags=O_WRONLY | O_CREAT;
    /*if(!buffer)
        flags|=??;*/
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
        /* Success! */
        return true;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return false;
}
#endif
