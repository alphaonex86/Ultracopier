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
    bool setCreated(const QDateTime &time);
    /// \brief set last modification date
    bool setLastModified(const QDateTime &time);
    /// \brief set last read date
    bool setLastRead(const QDateTime &time);

    #ifdef ULTRACOPIER_OVERLAPPED_FILE
    explicit AvancedQFile();
    ~AvancedQFile();
    bool open(OpenMode mode);
    void close();
    bool seek(qint64 pos);
    bool resize(qint64 size);
    QString errorString() const;
    bool isOpen() const;
    qint64 write(const QByteArray &data);
    QByteArray read(qint64 maxlen);
    FileError error() const;
    QString getLastWindowsError();
private:
    HANDLE handle;
    FileError fileError;
    QString fileErrorString;
    #endif
};

#endif // AVANCEDQFILE_H
