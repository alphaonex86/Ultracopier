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

#ifndef SCANFILEORFOLDER_H
#define SCANFILEORFOLDER_H

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
    void setFolderExistsAction(const FolderExistsAction &action, const std::string &newName="");
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
    void fileTransfer(const std::string &source,const std::string &destination,const Ultracopier::CopyMode &mode) const;
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
    void folderAlreadyExists(const std::string &source,const std::string &destination,const bool &isSame) const;
    void errorOnFolder(const std::string &fileInfo,const std::string &errorString,const ErrorType &errorType=ErrorType_FolderWithRety) const;
    void finishedTheListing() const;

    void newFolderListing(const std::string &path) const;
    void addToMkPath(const std::string& source,const std::string& destination, const int& inode) const;
    void addToMovePath(const std::string& source,const std::string& destination, const int& inodeToRemove) const;
    void addToRealMove(const std::string& source,const std::string& destination) const;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    void addToRmForRsync(const std::string& destination) const;
    #endif
public slots:
    void addToList(const std::vector<std::string>& sources,const std::string& destination);
    void setFilters(const std::vector<Filters_rules> &include,const std::vector<Filters_rules> &exclude);
    void setCopyListOrder(const bool &order);
    void set_updateMount();
protected:
    void run();
private:
    DriveManagement     driveManagement;
    bool                moveTheWholeFolder;
    std::vector<std::string>         sources;
    std::string             destination;
    volatile bool		stopIt;
    void                listFolder(std::string source, std::string destination);
    //std::string           resolvDestination(const std::string &destination);
    volatile bool		stopped;
    QSemaphore          waitOneAction;
    FolderExistsAction	folderExistsAction;
    FileErrorAction		fileErrorAction;
    volatile bool		checkDestinationExists;
    std::string             newName;
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
    QMutex			filtersMutex;
    std::string			firstRenamingRule;
    std::string			otherRenamingRule;
    std::vector<std::string>     blackList;
    /** Parse the multiple wildcard source, it allow resolv multiple wildcard with Qt into their path
     * The string: /toto/f*a/yy*a/toto.mp3
     * Will give: /toto/f1a/yy*a/toto.mp3, /toto/f2a/yy*a/toto.mp3
     * Will give: /toto/f2a/yy1a/toto.mp3, /toto/f2a/yy2a/toto.mp3
    */
    std::vector<std::string>		parseWildcardSources(const std::vector<std::string> &sources) const;

    static std::string text_slash;
    static std::string text_antislash;
    static std::string text_dot;
};

#endif // SCANFILEORFOLDER_H
