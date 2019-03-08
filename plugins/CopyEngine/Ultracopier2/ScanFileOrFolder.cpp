#include "ScanFileOrFolder.h"
#include "TransferThread.h"
#include <QtGlobal>
#include <regex>
#include "../../../cpp11addition.h"

#ifdef Q_OS_WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

std::string ScanFileOrFolder::text_slash="/";
std::string ScanFileOrFolder::text_antislash="\\";
std::string ScanFileOrFolder::text_dot=".";

ScanFileOrFolder::ScanFileOrFolder(const Ultracopier::CopyMode &mode)
{
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    rsync               = false;
    #endif
    moveTheWholeFolder  = true;
    stopped             = true;
    stopIt              = false;
    this->mode          = mode;
    folder_isolation    = std::regex("^(.*/)?([^/]+)/$");
    setObjectName(QStringLiteral("ScanFileOrFolder"));
    /*#ifdef Q_OS_WIN32
    QString userName;
    DWORD size=255;
    WCHAR * userNameW=new WCHAR[size];
    if(GetUserNameW(userNameW,&size))
    {
        userName=QString::fromWCharArray(userNameW,size-1);
        blackList.push_back(QFileInfo(QStringLiteral("C:/Users/%1/AppData/Roaming/").arg(userName)));
    }
    delete userNameW;
    #endif*/
}

ScanFileOrFolder::~ScanFileOrFolder()
{
    stop();
    quit();
    wait();
}

bool ScanFileOrFolder::isFinished() const
{
    return stopped;
}

void ScanFileOrFolder::addToList(const std::vector<std::string>& sources,const std::string& destination)
{
    stopIt=false;
    this->sources=parseWildcardSources(sources);
    this->destination=destination;
    QFileInfo destinationInfo(QString::fromStdString(this->destination));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"check symblink: "+destinationInfo.absoluteFilePath().toStdString());
    while(destinationInfo.isSymLink())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"resolv destination to: "+destinationInfo.symLinkTarget().toStdString());
        if(QFileInfo(destinationInfo.symLinkTarget()).isAbsolute())
            this->destination=destinationInfo.symLinkTarget().toStdString();
        else
            this->destination=destinationInfo.absolutePath().toStdString()+text_slash+destinationInfo.symLinkTarget().toStdString();
        destinationInfo.setFile(QString::fromStdString(this->destination));
    }
    if(sources.size()>1 || QFileInfo(QString::fromStdString(destination)).isDir())
        /* Disabled because the separator transformation product bug
         * if(!destination.endsWith(QDir::separator()))
            this->destination+=QDir::separator();*/
        if(!stringEndsWith(destination,'/') && !stringEndsWith(destination,'\\'))
            this->destination+=text_slash;//put unix separator because it's transformed into that's under windows too
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"addToList("+stringimplode(sources,";")+","+this->destination+")");
}


std::vector<std::string> ScanFileOrFolder::parseWildcardSources(const std::vector<std::string> &sources) const
{
    std::regex splitFolder("[/\\\\]");
    std::vector<std::string> returnList;
    unsigned int index=0;
    while(index<(unsigned int)sources.size())
    {
        if(sources.at(index).find("*") != std::string::npos)
        {
            std::vector<std::string> toParse=stringregexsplit(sources.at(index),splitFolder);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"before wildcard parse: "+sources.at(index)+", toParse: "+stringimplode(toParse,", "));
            std::vector<std::vector<std::string> > recomposedSource;
            {
                std::vector<std::string> t;
                t.push_back("");
                recomposedSource.push_back(t);
            }
            while(toParse.size()>0)
            {
                if(toParse.front().find("*") != std::string::npos)
                {
                    std::string toParseFirst=toParse.front();
                    if(toParseFirst.empty())
                        toParseFirst=text_slash;
                    std::vector<std::vector<std::string> > newRecomposedSource;
                    stringreplaceAll(toParseFirst,"*","[^/\\\\]*");
                    std::regex toResolv=std::regex(toParseFirst);
                    unsigned int index_recomposedSource=0;
                    while(index_recomposedSource<recomposedSource.size())//parse each url part
                    {
                        std::string fileInfo(stringimplode(recomposedSource.at(index_recomposedSource),text_slash));
                        std::vector<dirent> list;
                        struct stat p_statbuf;
                        if(lstat(fileInfo.c_str(), &p_statbuf)==0)
                        {
                            if(S_ISDIR(p_statbuf.st_mode) && TransferThread::entryInfoList(fileInfo,list))
                            {
                                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"list the folder: "+fileInfo+", with the wildcard: "+toParseFirst);
                                unsigned int index_fileList=0;
                                while(index_fileList<list.size())
                                {
                                    const std::string &fileName=list.at(index_fileList).d_name;
                                    if(std::regex_match(fileName,toResolv))
                                    {
                                        std::vector<std::string> tempList=recomposedSource.at(index_recomposedSource);
                                        tempList.push_back(fileName);
                                        newRecomposedSource.push_back(tempList);
                                    }
                                    index_fileList++;
                                }
                            }
                        }
                        index_recomposedSource++;
                    }
                    recomposedSource=newRecomposedSource;
                }
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"add toParse: "+stringimplode(toParse,text_slash));
                    unsigned int index_recomposedSource=0;
                    while(index_recomposedSource<recomposedSource.size())
                    {
                        recomposedSource[index_recomposedSource].push_back(toParse.front());
                        if(!QFileInfo(QString::fromStdString(stringimplode(recomposedSource.at(index_recomposedSource),text_slash))).exists())
                            recomposedSource.erase(recomposedSource.cbegin()+index_recomposedSource);
                        else
                            index_recomposedSource++;
                    }
                }
                toParse.erase(toParse.cbegin());
            }
            unsigned int index_recomposedSource=0;
            while(index_recomposedSource<recomposedSource.size())
            {
                returnList.push_back(stringimplode(recomposedSource.at(index_recomposedSource),text_slash));
                index_recomposedSource++;
            }
        }
        else
            returnList.push_back(sources.at(index));
        index++;
    }
    return returnList;
}

void ScanFileOrFolder::setFilters(const std::vector<Filters_rules> &include, const std::vector<Filters_rules> &exclude)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QMutexLocker lock(&filtersMutex);
    this->include_send=include;
    this->exclude_send=exclude;
    reloadTheNewFilters=true;
    haveFilters=include_send.size()>0 || exclude_send.size()>0;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"haveFilters: "+std::to_string(haveFilters)+", include_send.size(): "+std::to_string(include_send.size())+", exclude_send.size(): "+std::to_string(exclude_send.size()));
}

//set action if Folder are same or exists
void ScanFileOrFolder::setFolderExistsAction(const FolderExistsAction &action, const std::string &newName)
{
    this->newName=newName;
    folderExistsAction=action;
    waitOneAction.release();
}

//set action if error
void ScanFileOrFolder::setFolderErrorAction(const FileErrorAction &action)
{
    fileErrorAction=action;
    waitOneAction.release();
}

void ScanFileOrFolder::stop()
{
    stopIt=true;
    waitOneAction.release();
}

void ScanFileOrFolder::run()
{
    stopped=false;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start the listing with destination: "+destination+", mode: "+std::to_string(mode));
    destination=resolvDestination(destination);
    if(stopIt)
    {
        stopped=true;
        return;
    }
    if(fileErrorAction==FileError_Skip)
    {
        stopped=true;
        return;
    }
    unsigned int sourceIndex=0;
    while(sourceIndex<sources.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"size source to list: "+std::to_string(sourceIndex)+text_slash+std::to_string(sources.size()));
        if(stopIt)
        {
            stopped=true;
            return;
        }
        std::string source=sources.at(sourceIndex);
        struct stat p_statbuf;
        if(lstat(source.c_str(), &p_statbuf)==0 && S_ISDIR(p_statbuf.st_mode))
        {
            /* Bad way; when you copy c:\source\folder into d:\destination, you wait it create the folder d:\destination\folder
            //listFolder(source.absoluteFilePath()+QDir::separator(),destination);
            listFolder(source.absoluteFilePath()+text_slash,destination);//put unix separator because it's transformed into that's under windows too
            */
            //put unix separator because it's transformed into that's under windows too
            std::string tempString=destination;
            if(!stringEndsWith(tempString,text_slash) && !stringEndsWith(tempString,text_antislash))
                tempString+=text_slash;
            tempString+=TransferThread::resolvedName(source);
            if(moveTheWholeFolder && mode==Ultracopier::Move && !QFileInfo(QString::fromStdString(tempString)).exists() &&
                    driveManagement.isSameDrive(source,tempString))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"tempString: move and not exists: "+tempString);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"do real move: "+source+" to "+tempString);
                emit addToRealMove(source,tempString);
            }
            else
            {
                //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"tempString: "+tempString+" normal listing, blacklist size: "+std::to_string(blackList.size()));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"tempString: "+tempString+" normal listing");
                listFolder(source,tempString);
            }
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+source+" is file or symblink");
            emit fileTransfer(source,destination+text_slash+TransferThread::resolvedName(source),mode);
        }
        sourceIndex++;
    }
    stopped=true;
    if(stopIt)
        return;
    emit finishedTheListing();
}

std::string ScanFileOrFolder::resolvDestination(const std::string &destination)
{
    std::string temp(destination);
    char buf[PATH_MAX];
    ssize_t nbytes=0;
    nbytes=readlink(destination.c_str(), buf, sizeof(buf));
    while(nbytes!=-1) {
        temp=FSabsolutePath(temp)+'/'+std::string(buf,nbytes);
        /// \todo change for pure c++ code
        temp=QFileInfo(QString::fromStdString(temp)).absoluteFilePath().toStdString();
        nbytes=readlink(temp.c_str(), buf, sizeof(buf));
    }
    return temp;
    /*do
    {
        fileErrorAction=FileError_NotSet;
        if(isBlackListed(destination))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"isBlackListed: "+destination);
            emit errorOnFolder(destination,tr("Blacklisted folder").toStdString(),ErrorType_Folder);
            waitOneAction.acquire();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionNum: "+std::to_string(fileErrorAction));
        }
    } while(fileErrorAction==FileError_Retry || fileErrorAction==FileError_PutToEndOfTheList);
    return newDestination;*/
}

/*bool ScanFileOrFolder::isBlackListed(const QFileInfo &destination)
{
    int index=0;
    int size=blackList.size();
    while(index<size)
    {
        if(stringStartWith(destination,blackList.at(index)))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,destination+" start with: "+blackList.at(index));
            return true;
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,destination+" not start with: "+blackList.at(index));
        index++;
    }
    return false;
}*/

void ScanFileOrFolder::listFolder(std::string source,std::string destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+source+
                             ", destination: "+destination
                             );
    if(stopIt)
        return;
    destination=resolvDestination(destination);
    if(stopIt)
        return;
    if(fileErrorAction==FileError_Skip)
        return;
    //if is same
    if(source==destination)
    {
        emit folderAlreadyExists(source,destination,true);
        waitOneAction.acquire();
        std::string destinationSuffixPath;
        switch(folderExistsAction)
        {
            case FolderExists_Merge:
            break;
            case FolderExists_Skip:
                return;
            break;
            case FolderExists_Rename:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination before rename: "+destination);
                if(newName.empty())
                {
                    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"pattern: "+folder_isolation.str());
                    //resolv the new name
                    destinationSuffixPath=TransferThread::resolvedName(destination);
                    int num=1;
                    struct stat source_statbuf;
                    do
                    {
                        if(num==1)
                        {
                            if(firstRenamingRule.empty())
                                destinationSuffixPath=tr("%1 - copy").arg(QString::fromStdString(TransferThread::resolvedName(destination))).toStdString();
                            else
                                destinationSuffixPath=firstRenamingRule;
                        }
                        else
                        {
                            if(otherRenamingRule.empty())
                                destinationSuffixPath=tr("%1 - copy (%2)").arg(QString::fromStdString(TransferThread::resolvedName(destination))).arg(num).toStdString();
                            else
                            {
                                destinationSuffixPath=otherRenamingRule;
                                stringreplaceAll(destinationSuffixPath,"%number%",std::to_string(num));
                            }
                        }
                        stringreplaceAll(destinationSuffixPath,"%name%",TransferThread::resolvedName(destination));
                        num++;
                        {
                            std::string::size_type n=destination.rfind('/');
                            if(n == std::string::npos)
                                n=destination.rfind('.');
                            else
                                n=destination.rfind(n,'.');
                            if(n == std::string::npos)
                                destination=FSabsolutePath(destination)+'/'+destinationSuffixPath;
                            else
                                destination=FSabsolutePath(destination)+'/'+destinationSuffixPath+'.'+destination.substr(n);
                        }
                    }
                    while(lstat(destination.c_str(), &source_statbuf)==0);
                }
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use new name: "+newName);
                    destinationSuffixPath = newName;
                }
                destination=FSabsolutePath(destination)+text_slash+destinationSuffixPath;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination after rename: "+destination);
            break;
            default:
                return;
            break;
        }
    }
    //check if destination exists
    if(checkDestinationExists)
    {
        struct stat source_statbuf;
        if(lstat(destination.c_str(), &source_statbuf)==0)
        {
            emit folderAlreadyExists(source,destination,false);
            waitOneAction.acquire();
            std::string destinationSuffixPath;
            switch(folderExistsAction)
            {
                case FolderExists_Merge:
                break;
                case FolderExists_Skip:
                    return;
                break;
                case FolderExists_Rename:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination before rename: "+destination);
                    if(newName.empty())
                    {
                        //resolv the new name
                        QFileInfo destinationInfo;
                        int num=1;
                        do
                        {
                            if(num==1)
                            {
                                if(firstRenamingRule.empty())
                                    destinationSuffixPath=tr("%name% - copy").toStdString();
                                else
                                    destinationSuffixPath=firstRenamingRule;
                            }
                            else
                            {
                                if(otherRenamingRule.empty())
                                    destinationSuffixPath=tr("%name% - copy (%number%)").toStdString();
                                else
                                    destinationSuffixPath=otherRenamingRule;
                                stringreplaceAll(destinationSuffixPath,"%number%",std::to_string(num));
                            }
                            stringreplaceAll(destinationSuffixPath,"%name%",TransferThread::resolvedName(destination));
                            destinationInfo.setFile(destinationInfo.absolutePath()+QString::fromStdString(text_slash)+QString::fromStdString(destinationSuffixPath));
                            num++;
                        }
                        while(destinationInfo.exists());
                    }
                    else
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use new name: "+newName);
                        destinationSuffixPath = newName;
                    }
                    {
                        std::string::size_type n=destination.rfind('/');
                        if(n == std::string::npos)
                            n=destination.rfind('.');
                        else
                            n=destination.rfind(n,'.');
                        if(n == std::string::npos)
                            destination=FSabsolutePath(destination)+'/'+destinationSuffixPath;
                        else
                            destination=FSabsolutePath(destination)+'/'+destinationSuffixPath+'.'+destination.substr(n);
                    }
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination after rename: "+destination);
                break;
                default:
                    return;
                break;
            }
        }
    }
    //do source check
    //check of source is readable
    do
    {
        fileErrorAction=FileError_NotSet;
        if(!source.isReadable() || !source.isExecutable() || !source.exists() || !source.isDir())
        {
            if(!source.isDir())
                emit errorOnFolder(source,tr("This is not a folder").toStdString());
            else if(!source.exists())
                emit errorOnFolder(source,tr("The folder does exists").toStdString());
            else
                emit errorOnFolder(source,tr("The folder is not readable").toStdString());
            waitOneAction.acquire();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionNum: "+std::to_string(fileErrorAction));
        }
    } while(fileErrorAction==FileError_Retry);
    do
    {
        QDir tempDir(source.absoluteFilePath());
        fileErrorAction=FileError_NotSet;
        if(!tempDir.isReadable() || !tempDir.exists())
        {
            emit errorOnFolder(source,tr("Problem with name encoding").toStdString());
            waitOneAction.acquire();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionNum: "+std::to_string(fileErrorAction));
        }
    } while(fileErrorAction==FileError_Retry);
    if(stopIt)
        return;
    /// \todo check here if the folder is not readable or not exists
    QFileInfoList entryList;
    if(copyListOrder)
        entryList=QDir(source.absoluteFilePath()).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);//possible wait time here
    else
        entryList=QDir(source.absoluteFilePath()).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System);//possible wait time here
    if(stopIt)
        return;
    int sizeEntryList=entryList.size();
    emit newFolderListing(source);
    if(mode!=Ultracopier::Move)
        emit addToMkPath(source,destination,sizeEntryList);
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(stopIt)
            return;
        if(haveFilters)
        {
            if(reloadTheNewFilters)
            {
                QMutexLocker lock(&filtersMutex);
                QCoreApplication::processEvents(QEventLoop::AllEvents);
                reloadTheNewFilters=false;
                this->include=this->include_send;
                this->exclude=this->exclude_send;
            }
            std::string fileName=fileInfo.fileName().toStdString();
            if(fileInfo.isDir() && !fileInfo.isSymLink())
            {
                bool excluded=false,included=(include.size()==0);
                unsigned int filters_index=0;
                while(filters_index<exclude.size())
                {
                    if(exclude.at(filters_index).apply_on==ApplyOn_folder || exclude.at(filters_index).apply_on==ApplyOn_fileAndFolder)
                    {
                        if(std::regex_match(fileName,exclude.at(filters_index).regex))
                        {
                            excluded=true;
                            break;
                        }
                    }
                    filters_index++;
                }
                if(excluded)
                {}
                else
                {
                    filters_index=0;
                    while(filters_index<include.size())
                    {
                        if(include.at(filters_index).apply_on==ApplyOn_folder || include.at(filters_index).apply_on==ApplyOn_fileAndFolder)
                        {
                            if(std::regex_match(fileName,include.at(filters_index).regex))
                            {
                                included=true;
                                break;
                            }
                        }
                        filters_index++;
                    }
                    if(!included)
                    {}
                    else
                        listFolder(fileInfo,destination.absoluteFilePath()+QString::fromStdString(text_slash)+fileInfo.fileName());
                }
            }
            else
            {
                bool excluded=false,included=(include.size()==0);
                unsigned int filters_index=0;
                while(filters_index<exclude.size())
                {
                    if(exclude.at(filters_index).apply_on==ApplyOn_file || exclude.at(filters_index).apply_on==ApplyOn_fileAndFolder)
                    {
                        if(std::regex_match(fileName,exclude.at(filters_index).regex))
                        {
                            excluded=true;
                            break;
                        }
                    }
                    filters_index++;
                }
                if(excluded)
                {}
                else
                {
                    filters_index=0;
                    while(filters_index<include.size())
                    {
                        if(include.at(filters_index).apply_on==ApplyOn_file || include.at(filters_index).apply_on==ApplyOn_fileAndFolder)
                        {
                            if(std::regex_match(fileName,include.at(filters_index).regex))
                            {
                                included=true;
                                break;
                            }
                        }
                        filters_index++;
                    }
                    if(!included)
                    {}
                    else
                        #ifndef ULTRACOPIER_PLUGIN_RSYNC
                        emit fileTransfer(fileInfo,destination.absoluteFilePath()+QString::fromStdString(text_slash)+fileInfo.fileName(),mode);
                        #else
                        {
                            bool sendToTransfer=false;
                            if(!rsync)
                                sendToTransfer=true;
                            else if(!QFile::exists(destination.absoluteFilePath()+"/"+fileInfo.fileName()))
                                sendToTransfer=true;
                            else if(fileInfo.lastModified()!=QFileInfo(destination.absoluteFilePath()+"/"+fileInfo.fileName()).lastModified())
                                sendToTransfer=true;
                            if(sendToTransfer)
                                emit fileTransfer(fileInfo.absoluteFilePath(),destination.absoluteFilePath()+"/"+fileInfo.fileName(),mode);
                        }
                        #endif
                }
            }
        }
        else
        {
            if(fileInfo.isDir() && !fileInfo.isSymLink())//possible wait time here
                //listFolder(source,destination,suffixPath+fileInfo.fileName()+QDir::separator());
                listFolder(fileInfo,destination.absoluteFilePath()+QString::fromStdString(text_slash)+fileInfo.fileName());//put unix separator because it's transformed into that's under windows too
            else
                #ifndef ULTRACOPIER_PLUGIN_RSYNC
                emit fileTransfer(fileInfo,destination.absoluteFilePath()+QString::fromStdString(text_slash)+fileInfo.fileName(),mode);
                #else
                {
                    bool sendToTransfer=false;
                    if(!rsync)
                        sendToTransfer=true;
                    else if(!QFile::exists(destination.absoluteFilePath()+"/"+fileInfo.fileName()))
                        sendToTransfer=true;
                    else if(fileInfo.lastModified()!=QFileInfo(destination.absoluteFilePath()+"/"+fileInfo.fileName()).lastModified())
                        sendToTransfer=true;
                    if(sendToTransfer)
                        emit fileTransfer(fileInfo.absoluteFilePath(),destination.absoluteFilePath()+"/"+fileInfo.fileName(),mode);
                }
                #endif
        }
    }
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    if(rsync)
    {
        //check the reverse path here
        QFileInfoList entryListDestination;
        if(copyListOrder)
            entryListDestination=QDir(destination.absoluteFilePath()).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);//possible wait time here
        else
            entryListDestination=QDir(destination.absoluteFilePath()).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System);//possible wait time here
        int sizeEntryListDestination=entryListDestination.size();
        int index=0;
        for (int indexDestination=0;indexDestination<sizeEntryListDestination;++indexDestination)
        {
            index=0;
            while(index<sizeEntryList)
            {
                if(entryListDestination.at(indexDestination).fileName()==entryList.at(index).fileName())
                    break;
                index++;
            }
            if(index==sizeEntryList)
            {
                //then not found, need be remove
                emit addToRmForRsync(entryListDestination.at(indexDestination));
            }
         }
         return;
    }
    #endif
    if(mode==Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+source+", sizeEntryList: "+std::to_string(sizeEntryList));
        emit addToMovePath(source,destination,sizeEntryList);
    }
}

//set if need check if the destination exists
void ScanFileOrFolder::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
    this->checkDestinationExists=checkDestinationFolderExists;
}

void ScanFileOrFolder::setRenamingRules(const std::string &firstRenamingRule, const std::string &otherRenamingRule)
{
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
}

void ScanFileOrFolder::setMoveTheWholeFolder(const bool &moveTheWholeFolder)
{
    this->moveTheWholeFolder=moveTheWholeFolder;
}

void ScanFileOrFolder::setCopyListOrder(const bool &order)
{
    this->copyListOrder=order;
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
/// \brief set rsync
void ScanFileOrFolder::setRsync(const bool rsync)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"set rsync: "+std::to_string(rsync));
    this->rsync=rsync;
}
#endif

void ScanFileOrFolder::set_updateMount()
{
    driveManagement.tryUpdate();
}
