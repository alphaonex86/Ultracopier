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

