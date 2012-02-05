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
};

#endif // AVANCEDQFILE_H
