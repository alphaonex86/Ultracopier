#ifndef AVANCEDQFILE_H
#define AVANCEDQFILE_H

#include <QFile>
#include <QDateTime>
#include <QFileInfo>

class AvancedQFile : public QFile
{
    Q_OBJECT
public:
	bool setCreated(QDateTime time);
	bool setLastModified(QDateTime time);
	bool setLastRead(QDateTime time);
};

#endif // AVANCEDQFILE_H
