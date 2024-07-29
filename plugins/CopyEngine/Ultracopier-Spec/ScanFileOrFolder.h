/** \file scanFileOrFolder.h
\brief Thread changed to list recursively the folder
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QThread>
#include <QFileInfo>
#include <QDir>
#include <QSemaphore>
#include <QEventLoop>
#include <QCoreApplication>
#include <QMutexLocker>
#include <regex>
#include <string>
#include <vector>

#include "Environment.h"
#include "DriveManagement.h"
#include "TransferThread.h"

#ifndef SCANFILEORFOLDER_H
#define SCANFILEORFOLDER_H

#ifdef WIDESTRING
#define INTERNALTYPEPATH std::wstring
#else
#define INTERNALTYPEPATH std::string
#endif

/// \brief Thread changed to list recursively the folder
class ScanFileOrFolder : public QThread
{
    Q_OBJECT
public:
    explicit ScanFileOrFolder(const Ultracopier::CopyMode &mode);
    ~ScanFileOrFolder();
    /// \brief to the a folder listing
    void stop();
    /// \brief to get if is finished
    bool isFinished() const;
    /// \brief set action if Folder are same or exists
    void setFolderExistsAction(const FolderExistsAction &action, const std::string &newName=std::string());
    /// \brief set action if error
    void setFolderErrorAction(const FileErrorAction &action);
    /// \brief set if need check if the destination exists
    void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
    void setRenamingRules(const std::string &firstRenamingRule,const std::string &otherRenamingRule);
    void setMoveTheWholeFolder(const bool &moveTheWholeFolder);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    void setRsync(const bool rsync);
    #endif
signals:
    void fileTransfer(const INTERNALTYPEPATH &source,const INTERNALTYPEPATH &destination,const Ultracopier::CopyMode &mode) const;
    void fileTransferWithInode(const INTERNALTYPEPATH &source,const INTERNALTYPEPATH &destination,const Ultracopier::CopyMode &mode,const TransferThread::dirent_uc &inode) const;
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
    void folderAlreadyExists(const INTERNALTYPEPATH &source,const INTERNALTYPEPATH &destination,const bool &isSame) const;
    void errorOnFolder(const INTERNALTYPEPATH &fileInfo,const std::string &errorString,const ErrorType &errorType=ErrorType_FolderWithRety) const;
    void finishedTheListing() const;

    void newFolderListing(const std::string &path) const;
    void addToMkPath(const INTERNALTYPEPATH& source,const INTERNALTYPEPATH& destination, const int& inode) const;
    void addToMovePath(const INTERNALTYPEPATH& source,const INTERNALTYPEPATH& destination, const int& inodeToRemove) const;
    void addToKeepAttributePath(const INTERNALTYPEPATH& source,const INTERNALTYPEPATH& destination, const int& inodeToRemove) const;
    void addToRealMove(const INTERNALTYPEPATH& source,const INTERNALTYPEPATH& destination) const;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    void addToRmForRsync(const INTERNALTYPEPATH& destination) const;
    #endif
public slots:
    void addToList(const std::vector<INTERNALTYPEPATH>& sources,const INTERNALTYPEPATH& destination);
    void setFilters(const std::vector<Filters_rules> &include,const std::vector<Filters_rules> &exclude);
    void setFollowTheStrictOrder(const bool &order);
    void setignoreBlackList(const bool &ignoreBlackList);
    void set_updateMount();
protected:
    void run();
private:
    DriveManagement     driveManagement;
    bool                moveTheWholeFolder;
    std::vector<INTERNALTYPEPATH>         sources;
    INTERNALTYPEPATH             destination;
    volatile bool		stopIt;
    std::vector<INTERNALTYPEPATH> blackList;
    bool isBlackListed(const INTERNALTYPEPATH &path);
    void                listFolder(INTERNALTYPEPATH source, INTERNALTYPEPATH destination);
    #ifdef Q_OS_UNIX
    INTERNALTYPEPATH           resolvDestination(const INTERNALTYPEPATH &destination);
    #endif
    volatile bool		stopped;
    QSemaphore          waitOneAction;
    FolderExistsAction	folderExistsAction;
    FileErrorAction		fileErrorAction;
    volatile bool		checkDestinationExists;
    INTERNALTYPEPATH             newName;
    bool                copyListOrder;
    std::regex	folder_isolation;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    bool                rsync;
    #endif
    Ultracopier::CopyMode	mode;
    std::vector<Filters_rules>	include,exclude;
    std::vector<Filters_rules>	include_send,exclude_send;
    bool			reloadTheNewFilters;
    bool			haveFilters;
    bool ignoreBlackList;
    QMutex			filtersMutex;
    std::string			firstRenamingRule;
    std::string			otherRenamingRule;
    //std::vector<std::string>     blackList;
    /** Parse the multiple wildcard source, it allow resolv multiple wildcard with Qt into their path
     * The string: /toto/f*a/yy*a/toto.mp3
     * Will give: /toto/f1a/yy*a/toto.mp3, /toto/f2a/yy*a/toto.mp3
     * Will give: /toto/f2a/yy1a/toto.mp3, /toto/f2a/yy2a/toto.mp3
    */
    std::vector<INTERNALTYPEPATH>		parseWildcardSources(const std::vector<INTERNALTYPEPATH> &sources) const;

    static INTERNALTYPEPATH text_slash;
    static INTERNALTYPEPATH text_antislash;
    static INTERNALTYPEPATH text_dot;
};

#endif // SCANFILEORFOLDER_H
