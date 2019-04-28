/** \file MkPath.h
\brief Make the path given as queued mkpath
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef MKPATH_H
#define MKPATH_H

#include <QThread>
#include <string>
#include <QSemaphore>
#include <vector>
#include <QDir>
#include <QDateTime>

#include "Environment.h"

#ifdef Q_OS_UNIX
    #include <utime.h>
    #include <time.h>
    #include <unistd.h>
    #include <sys/stat.h>
#else
    #ifdef Q_OS_WIN32
        #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
            #include <utime.h>
            #include <time.h>
            #include <unistd.h>
            #include <sys/stat.h>
        #endif
    #endif
#endif

/// \brief Make the path given as queued mkpath
class MkPath : public QThread
{
    Q_OBJECT
public:
    explicit MkPath();
    ~MkPath();
    /// \brief add path to make
    void addPath(const std::string& source,const std::string& destination,const ActionType &actionType);
    void setRightTransfer(const bool doRightTransfer);
    void setKeepDate(const bool keepDate);
    void setMkFullPath(const bool mkFullPath);
signals:
    void errorOnFolder(const std::string &,const std::string &,const ErrorType &errorType=ErrorType_FolderWithRety) const;
    void firstFolderFinish();
    void internalStartAddPath(const std::string& source,const std::string& destination, const ActionType &actionType) const;
    void internalStartDoThisPath() const;
    void internalStartSkip() const;
    void internalStartRetry() const;
    void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
public slots:
    /// \brief skip after creation error
    void skip();
    /// \brief retry after creation error
    void retry();
private:
    void run();
    bool waitAction;
    bool stopIt;
    bool skipIt;
    struct Item
    {
        std::string source;
        std::string destination;
        ActionType actionType;
    };
    std::vector<Item> pathList;
    void checkIfCanDoTheNext();
    bool doRightTransfer;
    bool keepDate;
    bool mkFullPath;
    bool doTheDateTransfer;
    #ifdef Q_OS_UNIX
            utimbuf butime;
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                utimbuf butime;
            #else
                uint64_t ftCreateL, ftAccessL, ftWriteL;
                uint64_t ftCreateH, ftAccessH, ftWriteH;
                std::regex regRead;
            #endif
        #endif
    #endif
    //fonction to edit the file date time
    bool readFileDateTime(const std::string &source);
    bool writeFileDateTime(const std::string &destination);
    static std::string text_slash;
private slots:
    void internalDoThisPath();
    void internalAddPath(const std::string& source, const std::string& destination,const ActionType &actionType);
    void internalSkip();
    void internalRetry();
    bool rmpath(const std::string &dir
                #ifdef ULTRACOPIER_PLUGIN_RSYNC
                , const bool &toSync=false
                #endif
            );
};

#endif // MKPATH_H
