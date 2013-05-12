/** \file AvancedQFile.cpp
\brief Define the QFile herited class to set file date/time
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "AvancedQFile.h"

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <errno.h>
#endif

//source
//hSrc=CreateFile(pData->pfiSrcFile->GetFullFilePath(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | (bNoBuffer ? FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH : 0), NULL);
//destination
//hDst=CreateFile(pData->strDstFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | (bNoBuffer ? FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH : 0), NULL);

bool AvancedQFile::setCreated(QDateTime time)
{
    time_t ctime=time.toTime_t();
    #ifdef Q_CC_GNU
        //creation time not exists into unix world
        Q_UNUSED(ctime)
        return true;
    #else
        setErrorString(tr("Not supported on this platform"));
        return false;
    #endif
}

bool AvancedQFile::setLastModified(QDateTime time)
{
    time_t actime=QFileInfo(*this).lastRead().toTime_t();
    //protect to wrong actime
    if(actime<0)
        actime=0;
    time_t modtime=time.toTime_t();
    if(modtime<0)
    {
        setErrorString(tr("Last modified date is wrong"));
        return false;
    }
    #ifdef Q_CC_GNU
        //this function avalaible on unix and mingw
        utimbuf butime;
        butime.actime=actime;
        butime.modtime=modtime;
        int returnVal=utime(this->fileName().toLocal8Bit().data(),&butime);
        if(returnVal==0)
            return true;
        else
        {
            setErrorString(strerror(errno));
            return false;
        }
    #else
        setErrorString(tr("Not supported on this platform"));
        return false;
    #endif
}

bool AvancedQFile::setLastRead(QDateTime time)
{
    time_t modtime=QFileInfo(*this).lastModified().toTime_t();
    //protect to wrong actime
    if(modtime<0)
        modtime=0;
    time_t actime=time.toTime_t();
    if(actime<0)
    {
        setErrorString(tr("Last access date is wrong"));
        return false;
    }
    #ifdef Q_CC_GNU
        //this function avalaible on unix and mingw
        utimbuf butime;
        butime.actime=actime;
        butime.modtime=modtime;
        int returnVal=utime(this->fileName().toLocal8Bit().data(),&butime);
        if(returnVal==0)
            return true;
        else
        {
            setErrorString(strerror(errno));
            return false;
        }
    #else
        setErrorString(tr("Not supported on this platform"));
        return false;
    #endif
}

#ifdef ULTRACOPIER_OVERLAPPED_FILE
AvancedQFile::avancedQFile()
{
    handle=INVALID_HANDLE_VALUE;
    fileError=QFileDevice::NoError;
    fileErrorString.clear();
}

AvancedQFile::~avancedQFile()
{
    close();
}

QString AvancedQFile::getLastWindowsError()
{
    WCHAR ErrorStringW[65535];
    DWORD dw = GetLastError();

    int size=FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    dw,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    ErrorStringW,
    0, NULL );
    if(size<0)
        tr("Unknown error: %1").arg(dw);
    return QString::fromWCharArray(ErrorStringW,size);
}

bool AvancedQFile::open(OpenMode mode)
{
    fileError=QFileDevice::NoError;
    fileErrorString.clear();
    WCHAR fileNameW[fileName().size()+1];
    if(QDir::toNativeSeparators("\\\\?\\"+fileName()).toWCharArray(fileNameW)!=fileName().size())
    {
        fileError=QFileDevice::OpenError;
        fileErrorString=tr("Path conversion error");
        return false;
    }
    fileNameW[fileName().size()]='\0';

    DWORD dwDesiredAccess=0;
    if(mode & QIODevice::ReadOnly)
        dwDesiredAccess|=GENERIC_READ;
    if(mode & QIODevice::WriteOnly)
        dwDesiredAccess|=GENERIC_Write;

    DWORD dwCreationDisposition;
    if(mode & QIODevice::WriteOnly)
        dwCreationDisposition=CREATE_ALWAYS;
    else
        dwCreationDisposition=OPEN_EXISTING;

    handle=CreateFile(
      fileNameW,
      dwDesiredAccess,
      0,
      0,
      dwCreationDisposition,
      FILE_FLAG_WRITE_THROUGH | FILE_FLAG_SEQUENTIAL_SCAN,
      0
    );
    if(handle==INVALID_HANDLE_VALUE)
    {
        fileError=QFileDevice::OpenError;
        fileErrorString=getLastWindowsError();
    }
    return (handle!=INVALID_HANDLE_VALUE);
}

void AvancedQFile::close()
{
    if(handle==INVALID_HANDLE_VALUE)
        return;
    CloseHandle(handle);
}

bool AvancedQFile::seek(qint64 pos)
{
    toto
}

bool AvancedQFile::resize(qint64 size)
{
    toto
}

QString AvancedQFile::errorString() const
{
    if(fileErrorString.isEmpty())
        return tr("Unknown error");
    return fileErrorString;
}

bool AvancedQFile::isOpen() const
{
    return (handle!=INVALID_HANDLE_VALUE);
}

qint64 AvancedQFile::write(const QByteArray &data)
{
}

QByteArray AvancedQFile::read(qint64 maxlen)
{
}

QFileDevice::FileError AvancedQFile::error() const
{
    return fileError;
}
#endif
