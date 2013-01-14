/** \file AvancedQFile.h
\brief Define the QFile herited class to set file date/time
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef AVANCEDQFILE_H
#define AVANCEDQFILE_H

#include <QFile>
#include <QDateTime>
#include <QFileInfo>

/// \brief devired class from QFile to set time/date on file
class AvancedQFile : public QFile
{
    Q_OBJECT
public:
	/// \brief set created date, not exists in unix world
	bool setCreated(QDateTime time);
	/// \brief set last modification date
	bool setLastModified(QDateTime time);
	/// \brief set last read date
	bool setLastRead(QDateTime time);
	
/*	//fileName
	void close();
	bool	open ( FILE * fh, OpenMode mode )
	bool	open ( int fd, OpenMode mode )*/
};

#endif // AVANCEDQFILE_H
