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

#ifdef WIDESTRING
std::wstring ScanFileOrFolder::text_slash=L"/";
std::wstring ScanFileOrFolder::text_antislash=L"\\";
std::wstring ScanFileOrFolder::text_dot=L".";
#else
std::string ScanFileOrFolder::text_slash="/";
std::string ScanFileOrFolder::text_antislash="\\";
std::string ScanFileOrFolder::text_dot=".";
#endif

ScanFileOrFolder::ScanFileOrFolder(const Ultracopier::CopyMode &mode) :
    moveTheWholeFolder(false),
    stopIt(false),
    stopped(false),
    folderExistsAction(FolderExistsAction::FolderExists_NotSet),
    fileErrorAction(FileErrorAction::FileError_NotSet),
    checkDestinationExists(false),
    copyListOrder(false),
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    rsync(false),
    #endif
    mode(Ultracopier::CopyMode::Copy),
    reloadTheNewFilters(false),
    haveFilters(false),
    ignoreBlackList(false)
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
    #ifdef Q_OS_WIN32
    QString userName;
    DWORD size=255;
    WCHAR * userNameW=new WCHAR[size];
    if(GetUserNameW(userNameW,&size))
    {
        #ifdef WIDESTRING
        blackList.push_back(INTERNALTYPEPATH(L"C:/Users/")+userNameW+L"/AppData/Roaming/");
        blackList.push_back(INTERNALTYPEPATH(L"C:\\Users\\")+userNameW+L"\\AppData\\Roaming\\");
        #else
        blackList.push_back(INTERNALTYPEPATH("C:/Users/")+userNameW+"/AppData/Roaming/");
        blackList.push_back(INTERNALTYPEPATH("C:\\Users\\")+userNameW+"\\AppData\\Roaming\\");
        #endif
    }
    delete userNameW;
    #endif
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

void ScanFileOrFolder::addToList(const std::vector<INTERNALTYPEPATH>& sources,const INTERNALTYPEPATH& destination)
{
    stopIt=false;
    this->sources=parseWildcardSources(sources);
    this->destination=destination;
    #ifdef WIDESTRING
    QFileInfo destinationInfo(QString::fromStdWString(this->destination));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"check symblink: "+destinationInfo.absoluteFilePath().toStdString()+", destination: "+TransferThread::internalStringTostring(destination));
    #ifdef WIDESTRING
    while(TransferThread::is_symlink(destinationInfo.absoluteFilePath().toStdWString()))
    #else
    while(TransferThread::is_symlink(destinationInfo.absoluteFilePath().toStdString()))
    #endif
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"resolv destination to: "+destinationInfo.symLinkTarget().toStdString());
        if(QFileInfo(destinationInfo.symLinkTarget()).isAbsolute())
            this->destination=destinationInfo.symLinkTarget().toStdWString();
        else
            this->destination=destinationInfo.absolutePath().toStdWString()+text_slash+destinationInfo.symLinkTarget().toStdWString();
        destinationInfo.setFile(QString::fromStdWString(this->destination));
    }
    if(sources.size()>1 || QFileInfo(QString::fromStdWString(destination)).isDir())
        /* Disabled because the separator transformation product bug
         * if(!destination.endsWith(QDir::separator()))
            this->destination+=QDir::separator();*/
        if(!stringEndsWith(destination,'/') && !stringEndsWith(destination,'\\'))
            this->destination+=text_slash;//put unix separator because it's transformed into that's under windows too
    #else
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
    #endif
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"addToList("+stringimplode(sources,";")+","+this->destination+")");
}


std::vector<INTERNALTYPEPATH> ScanFileOrFolder::parseWildcardSources(const std::vector<INTERNALTYPEPATH> &sources) const
{
    std::regex splitFolder("[/\\\\]");
    std::vector<INTERNALTYPEPATH> returnList;
    unsigned int index=0;
    while(index<(unsigned int)sources.size())
    {
        std::string sourceAt=TransferThread::internalStringTostring(sources.at(index));
        if(sourceAt.find("*") != std::string::npos)
        {
            std::vector<std::string> toParse=stringregexsplit(sourceAt,splitFolder);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"before wildcard parse: "+sourceAt+", toParse: "+stringimplode(toParse,", "));
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
                        toParseFirst=TransferThread::internalStringTostring(text_slash);
                    std::vector<std::vector<std::string> > newRecomposedSource;
                    stringreplaceAll(toParseFirst,"*","[^/\\\\]*");
                    std::regex toResolv=std::regex(toParseFirst);
                    unsigned int index_recomposedSource=0;
                    while(index_recomposedSource<recomposedSource.size())//parse each url part
                    {
                        std::string fileInfo(stringimplode(recomposedSource.at(index_recomposedSource),TransferThread::internalStringTostring(text_slash)));
                        std::vector<TransferThread::dirent_uc> list;

                        if(TransferThread::is_dir(fileInfo.c_str()))
                        {
                            if(TransferThread::entryInfoList(TransferThread::stringToInternalString(fileInfo),list))
                            {
                                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"list the folder: "+fileInfo+", with the wildcard: "+toParseFirst);
                                unsigned int index_fileList=0;
                                while(index_fileList<list.size())
                                {
                                    const std::string &fileName=TransferThread::internalStringTostring(list.at(index_fileList).d_name);
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
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"add toParse: "+stringimplode(toParse,TransferThread::internalStringTostring(text_slash)));
                    unsigned int index_recomposedSource=0;
                    while(index_recomposedSource<recomposedSource.size())
                    {
                        recomposedSource[index_recomposedSource].push_back(toParse.front());
                        if(!QFileInfo(QString::fromStdString(stringimplode(recomposedSource.at(index_recomposedSource),TransferThread::internalStringTostring(text_slash)))).exists())
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
                returnList.push_back(TransferThread::stringToInternalString(stringimplode(recomposedSource.at(index_recomposedSource),TransferThread::internalStringTostring(text_slash))));
                index_recomposedSource++;
            }
        }
        else
            returnList.push_back(TransferThread::stringToInternalString(sourceAt));
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
    this->newName=TransferThread::stringToInternalString(newName);
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,
        "start the listing with destination: "+TransferThread::internalStringTostring(destination)+", mode: "+std::to_string(mode));
    #ifdef Q_OS_UNIX
    destination=resolvDestination(destination);
    #endif
    #ifdef WIDESTRING
    stringreplaceAll(destination,L"\\",L"/");
    #else
    stringreplaceAll(destination,"\\","/");
    #endif
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"size source to list: "+std::to_string(sourceIndex)+TransferThread::internalStringTostring(text_slash)+std::to_string(sources.size()));
        if(stopIt)
        {
            stopped=true;
            return;
        }
        INTERNALTYPEPATH source=sources.at(sourceIndex);
        #ifdef WIDESTRING
        stringreplaceAll(source,L"\\",L"/");
        #else
        stringreplaceAll(source,"\\","/");
        #endif
        if(TransferThread::is_dir(source))
        {
            /* Bad way; when you copy c:\source\folder into d:\destination, you wait it create the folder d:\destination\folder
            //listFolder(source.absoluteFilePath()+QDir::separator(),destination);
            listFolder(source.absoluteFilePath()+text_slash,destination);//put unix separator because it's transformed into that's under windows too
            */
            //put unix separator because it's transformed into that's under windows too
            INTERNALTYPEPATH tempString=destination;
            if(!stringEndsWith(tempString,text_slash) && !stringEndsWith(tempString,text_antislash))
                tempString+=text_slash;
            tempString+=TransferThread::resolvedName(source);
            if(moveTheWholeFolder && mode==Ultracopier::Move && !QFileInfo(
                        #ifdef WIDESTRING
                        QString::fromStdWString(tempString)
                        #else
                        QString::fromStdString(tempString)
                        #endif
                        ).exists() &&
                    driveManagement.isSameDrive(TransferThread::internalStringTostring(source),TransferThread::internalStringTostring(tempString)))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"tempString: move and not exists: "+TransferThread::internalStringTostring(tempString));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"do real move: "+TransferThread::internalStringTostring(source)+" to "+TransferThread::internalStringTostring(tempString));
                emit addToRealMove(source,tempString);
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"tempString: "+TransferThread::internalStringTostring(tempString)+" normal listing, blacklist size: "+std::to_string(blackList.size()));
                //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"tempString: "+TransferThread::internalStringTostring(tempString)+" normal listing");
                if(stringEndsWith(source,'/'))
                    source.erase(source.end()-1);
                if(stringEndsWith(tempString,'/'))
                    tempString.erase(tempString.end()-1);
                listFolder(source,tempString);
            }
        }
        else
        {
            INTERNALTYPEPATH destinationFinish=destination;
            if(stringEndsWith(destinationFinish,'/') || stringEndsWith(destinationFinish,'\\'))
                destinationFinish.pop_back();
            destinationFinish+=text_slash;
            do
            {
                fileErrorAction=FileError_NotSet;
                if(isBlackListed(destination))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"isBlackListed: "+TransferThread::internalStringTostring(destination));
                    emit errorOnFolder(destination,tr("Blacklisted folder").toStdString(),ErrorType_Folder);
                    waitOneAction.acquire();
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,std::string("actionNum: ")+std::to_string((int)fileErrorAction));
                }
            } while(fileErrorAction==FileError_Retry);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+TransferThread::internalStringTostring(source)+" is file or symblink, is_file: "+std::to_string(TransferThread::is_file(source)));
            emit fileTransfer(source,destinationFinish+TransferThread::resolvedName(source),mode);
        }
        sourceIndex++;
    }
    stopped=true;
    if(stopIt)
        return;
    emit finishedTheListing();
}

#ifdef Q_OS_UNIX
INTERNALTYPEPATH ScanFileOrFolder::resolvDestination(const INTERNALTYPEPATH &destination)
{
    INTERNALTYPEPATH temp(destination);
    std::vector<char> buf(512);
    ssize_t nbytes=0;
    do {
      buf.resize(buf.size()*2);
      nbytes=readlink(TransferThread::internalStringTostring(destination).c_str(), buf.data(), buf.size());
    } while (nbytes == (ssize_t)buf.size());
    if (nbytes!=-1)
      buf.resize(nbytes);
    while(nbytes!=-1) {
        temp=FSabsolutePath(temp);
        if(!stringEndsWith(destination,'/')
            #ifdef Q_OS_WIN32
                && !stringEndsWith(destination,'\\')
            #endif
                )
            temp+=TransferThread::stringToInternalString("/");
        temp+=TransferThread::stringToInternalString(std::string(buf.data(), buf.size()));
        /// \todo change for pure c++ code
        #ifdef WIDESTRING
        temp=QFileInfo(QString::fromStdWString(temp)).absoluteFilePath().toStdWString();
        #else
        temp=QFileInfo(QString::fromStdString(temp)).absoluteFilePath().toStdString();
        #endif
        do {
          buf.resize(buf.size() * 2);
          nbytes=readlink(TransferThread::internalStringTostring(temp).c_str(), buf.data(), buf.size());
        } while (nbytes == (ssize_t)buf.size());
        if (nbytes!=-1)
          buf.resize(nbytes);
    }
    return temp;
}
#endif

bool ScanFileOrFolder::isBlackListed(const INTERNALTYPEPATH &path)
{
    if(ignoreBlackList)
        return false;
    int index=0;
    int size=blackList.size();
    INTERNALTYPEPATH path2=path;
    while(index<size)
    {
        #ifdef WIDESTRING
        stringreplaceAll(path2,L"\\",L"/");
        #else
        stringreplaceAll(path2,"\\","/");
        #endif
        if(stringStartWith(path2,blackList.at(index)))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,TransferThread::internalStringTostring(path)+" start with: "+TransferThread::internalStringTostring(blackList.at(index)));
            return true;
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,TransferThread::internalStringTostring(path)+" not start with: "+TransferThread::internalStringTostring(blackList.at(index)));
        index++;
    }
    return false;
}

void ScanFileOrFolder::listFolder(INTERNALTYPEPATH source,INTERNALTYPEPATH destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+TransferThread::internalStringTostring(source)+
                             ", destination: "+TransferThread::internalStringTostring(destination)
                             );
    if(stopIt)
        return;
    #ifdef Q_OS_UNIX
    destination=resolvDestination(destination);
    #endif
    if(stopIt)
        return;
    if(fileErrorAction==FileError_Skip)
        return;
    //if is same
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
        #ifdef WIDESTRING
        if(destination.find(L"//") != std::wstring::npos)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path destination contains error");
        if(source.find(L"//") != std::wstring::npos)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path source contains error");
        #else
        if(destination.find("//") != std::string::npos)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path destination contains error");
        if(source.find("//") != std::string::npos)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path source contains error");
        #endif
    #endif
    if(source==destination)
    {
        emit folderAlreadyExists(source,destination,true);
        waitOneAction.acquire();
        INTERNALTYPEPATH destinationSuffixPath;
        switch(folderExistsAction)
        {
            case FolderExists_Merge:
            break;
            case FolderExists_Skip:
                return;
            break;
            case FolderExists_Rename:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination before rename: "+TransferThread::internalStringTostring(destination));
                if(newName.empty())
                {
                    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"pattern: "+folder_isolation.str());
                    //resolv the new name
                    destinationSuffixPath=TransferThread::resolvedName(destination);
                    int num=1;
                    do
                    {
                        if(num==1)
                        {
                            if(firstRenamingRule.empty())
                                #ifdef WIDESTRING
                                destinationSuffixPath=tr("%1 - copy").arg(QString::fromStdWString(TransferThread::resolvedName(destination))).toStdWString();
                                #else
                                destinationSuffixPath=tr("%1 - copy").arg(QString::fromStdString(TransferThread::resolvedName(destination))).toStdString();
                                #endif
                            else
                                destinationSuffixPath=TransferThread::stringToInternalString(firstRenamingRule);
                        }
                        else
                        {
                            if(otherRenamingRule.empty())
                                #ifdef WIDESTRING
                                destinationSuffixPath=tr("%1 - copy (%2)").arg(QString::fromStdWString(TransferThread::resolvedName(destination))).arg(num).toStdWString();
                                #else
                                destinationSuffixPath=tr("%1 - copy (%2)").arg(QString::fromStdString(TransferThread::resolvedName(destination))).arg(num).toStdString();
                                #endif
                            else
                            {
                                destinationSuffixPath=TransferThread::stringToInternalString(otherRenamingRule);
                                #ifdef WIDESTRING
                                stringreplaceAll(destinationSuffixPath,L"%number%",std::to_wstring(num));
                                #else
                                stringreplaceAll(destinationSuffixPath,"%number%",std::to_string(num));
                                #endif
                            }
                        }
                        #ifdef WIDESTRING
                        stringreplaceAll(destinationSuffixPath,L"%name%",TransferThread::resolvedName(destination));
                        #else
                        stringreplaceAll(destinationSuffixPath,"%name%",TransferThread::resolvedName(destination));
                        #endif
                        num++;
                        {
                            std::string::size_type n=destination.rfind('/');
                            if(n == std::string::npos)
                                n=destination.rfind('.');
                            else
                                n=destination.rfind(n,'.');
                            if(n == std::string::npos)
                            {
                                destination=FSabsolutePath(destination);
                                if(!stringEndsWith(destination,'/')
                                    #ifdef Q_OS_WIN32
                                        && !stringEndsWith(destination,'\\')
                                    #endif
                                        )
                                    destination+=text_slash;
                                destination+=destinationSuffixPath;
                            }
                            else
                            {
                                destination=FSabsolutePath(destination);
                                if(!stringEndsWith(destination,'/')
                                    #ifdef Q_OS_WIN32
                                        && !stringEndsWith(destination,'\\')
                                    #endif
                                        )
                                    destination+=text_slash;
                                destination+=destinationSuffixPath+TransferThread::stringToInternalString(".")+destination.substr(n);
                            }
                        }
                    }
                    while(TransferThread::exists(destination));
                }
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use new name: "+TransferThread::internalStringTostring(newName));
                    destinationSuffixPath = newName;
                }
                destination=FSabsolutePath(destination);
                if(!stringEndsWith(destination,'/')
                    #ifdef Q_OS_WIN32
                        && !stringEndsWith(destination,'\\')
                    #endif
                        )
                    destination+=text_slash;
                destination+=destinationSuffixPath;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination after rename: "+TransferThread::internalStringTostring(destination));
            break;
            default:
                return;
            break;
        }
    }
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
        #ifdef WIDESTRING
        if(destination.find(L"//") != std::wstring::npos)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path destination contains error");
        if(source.find(L"//") != std::wstring::npos)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path source contains error");
        #else
        if(destination.find("//") != std::string::npos)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path destination contains error");
        if(source.find("//") != std::string::npos)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path source contains error");
        #endif
    #endif
    //check if destination exists
    if(checkDestinationExists)
    {
        if(TransferThread::exists(destination))
        {
            emit folderAlreadyExists(source,destination,false);
            waitOneAction.acquire();
            INTERNALTYPEPATH destinationSuffixPath;
            switch(folderExistsAction)
            {
                case FolderExists_Merge:
                break;
                case FolderExists_Skip:
                    return;
                break;
                case FolderExists_Rename:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination before rename: "+TransferThread::internalStringTostring(destination));
                    if(newName.empty())
                    {
                        //resolv the new name
                        int num=1;
                        INTERNALTYPEPATH tempdestination;
                        do
                        {
                            if(num==1)
                            {
                                if(firstRenamingRule.empty())
                                    #ifdef WIDESTRING
                                    destinationSuffixPath=tr("%name% - copy").toStdWString();
                                    #else
                                    destinationSuffixPath=tr("%name% - copy").toStdString();
                                    #endif
                                else
                                    destinationSuffixPath=TransferThread::stringToInternalString(firstRenamingRule);
                            }
                            else
                            {
                                if(otherRenamingRule.empty())
                                    #ifdef WIDESTRING
                                    destinationSuffixPath=tr("%name% - copy (%number%)").toStdWString();
                                    #else
                                    destinationSuffixPath=tr("%name% - copy (%number%)").toStdString();
                                    #endif
                                else
                                    destinationSuffixPath=TransferThread::stringToInternalString(otherRenamingRule);
                                #ifdef WIDESTRING
                                stringreplaceAll(destinationSuffixPath,L"%number%",std::to_wstring(num));
                                #else
                                stringreplaceAll(destinationSuffixPath,"%number%",std::to_string(num));
                                #endif
                            }
                            #ifdef WIDESTRING
                            stringreplaceAll(destinationSuffixPath,L"%name%",TransferThread::resolvedName(destination));
                            #else
                            stringreplaceAll(destinationSuffixPath,"%name%",TransferThread::resolvedName(destination));
                            #endif
                            tempdestination=FSabsolutePath(destination);
                            if(!stringEndsWith(destination,'/')
                                #ifdef Q_OS_WIN32
                                    && !stringEndsWith(destination,'\\')
                                #endif
                                    )
                                tempdestination+=text_slash;
                            tempdestination+=destinationSuffixPath;
                            num++;
                        }
                        while(TransferThread::exists(tempdestination));
                    }
                    else
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use new name: "+TransferThread::internalStringTostring(newName));
                        destinationSuffixPath = newName;
                    }
                    {
                        std::string::size_type n=destination.rfind('/');
                        if(n == std::string::npos)
                            n=destination.rfind('.');
                        else
                            n=destination.rfind(n,'.');
                        if(n == std::string::npos)
                        {
                            destination=FSabsolutePath(destination);
                            if(!stringEndsWith(destination,'/')
                                #ifdef Q_OS_WIN32
                                    && !stringEndsWith(destination,'\\')
                                #endif
                                    )
                                destination+=text_slash;
                            destination+=destinationSuffixPath;
                        }
                        else
                        {
                            destination=FSabsolutePath(destination);
                            if(!stringEndsWith(destination,'/')
                                #ifdef Q_OS_WIN32
                                    && !stringEndsWith(destination,'\\')
                                #endif
                                    )
                                destination+=text_slash;
                            destination+=destinationSuffixPath+
                                    TransferThread::stringToInternalString(".")+destination.substr(n);
                        }
                    }
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination after rename: "+TransferThread::internalStringTostring(destination));
                break;
                default:
                    return;
                break;
            }
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"checkDestinationExists but stat failed");
    }
    if(stopIt)
        return;
    std::vector<TransferThread::dirent_uc> entryList;
    do
    {
        fileErrorAction=FileError_NotSet;
        if(isBlackListed(destination))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"isBlackListed: "+TransferThread::internalStringTostring(destination));
            emit errorOnFolder(destination,tr("Blacklisted folder").toStdString(),ErrorType_Folder);
            waitOneAction.acquire();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,std::string("actionNum: ")+std::to_string((int)fileErrorAction));
        }
        else if(!TransferThread::entryInfoList(source,entryList))
        {
            #ifdef Q_OS_UNIX
            int saveerrno=errno;
            const std::string &errorStr=strerror(saveerrno);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Problem with the folder "+TransferThread::internalStringTostring(source)+" to read: "+std::to_string(saveerrno));
            emit errorOnFolder(source,tr("Problem with folder read").toStdString()+": "+errorStr);
            #else
            const std::string &errorStr=TransferThread::GetLastErrorStdStr();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Problem with the folder "+TransferThread::internalStringTostring(source)+" to read: "+errorStr);
            emit errorOnFolder(source,tr("Problem with folder read").toStdString()+": "+errorStr);
            #endif
            waitOneAction.acquire();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionNum: "+std::to_string(fileErrorAction));
        }
    } while(fileErrorAction==FileError_Retry);

    if(copyListOrder)
        std::sort(entryList.begin(), entryList.end(), [](TransferThread::dirent_uc a, TransferThread::dirent_uc b) {
                return a.d_name<b.d_name;
        });
    if(stopIt)
        return;
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
        #ifdef WIDESTRING
            if(destination.find(L"//") != std::wstring::npos)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path destination contains error");
            if(source.find(L"//") != std::wstring::npos)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path source contains error");
        #else
            if(destination.find("//") != std::string::npos)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path destination contains error");
            if(source.find("//") != std::string::npos)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"path source contains error");
        #endif
    #endif
    const unsigned int sizeEntryList=entryList.size();
    emit newFolderListing(TransferThread::internalStringTostring(source));
    if(mode!=Ultracopier::Move)
        emit addToMkPath(source,destination,sizeEntryList);
    for(unsigned int index=0;index<sizeEntryList;++index)
    {
        const TransferThread::dirent_uc &fileInfo=entryList.at(index);
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
            const INTERNALTYPEPATH &fileName=fileInfo.d_name;
            if(fileInfo.isFolder)
            {
                bool excluded=false,included=(include.size()==0);
                unsigned int filters_index=0;
                while(filters_index<exclude.size())
                {
                    if(exclude.at(filters_index).apply_on==ApplyOn_folder || exclude.at(filters_index).apply_on==ApplyOn_fileAndFolder)
                    {
                        if(std::regex_match(TransferThread::internalStringTostring(fileName),exclude.at(filters_index).regex))
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
                            if(std::regex_match(TransferThread::internalStringTostring(fileName),include.at(filters_index).regex))
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
                    {
                        INTERNALTYPEPATH fullSource=source;
                        if(!stringEndsWith(fullSource,'/'))
                            fullSource+='/';
                        fullSource+=fileName;
                        INTERNALTYPEPATH fullDestination=destination;
                        if(!stringEndsWith(fullDestination,'/'))
                            fullDestination+='/';
                        fullDestination+=fileName;
                        listFolder(fullSource,fullDestination);
                    }
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
                        if(std::regex_match(TransferThread::internalStringTostring(fileName),exclude.at(filters_index).regex))
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
                            if(std::regex_match(TransferThread::internalStringTostring(fileName),include.at(filters_index).regex))
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
                    {
                        INTERNALTYPEPATH fullSource=source;
                        if(!stringEndsWith(fullSource,'/'))
                            fullSource+='/';
                        fullSource+=fileName;
                        #ifndef ULTRACOPIER_PLUGIN_RSYNC
                            #ifdef Q_OS_WIN32
                            emit fileTransferWithInode(fullSource,destination+TransferThread::stringToInternalString("/")+fileName,mode,fileInfo);
                            #else
                            emit fileTransfer(fullSource,destination+TransferThread::stringToInternalString("/")+fileName,mode);
                            #endif
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
                                #ifdef Q_OS_WIN32
                                emit fileTransferWithInode(fileInfo.absoluteFilePath(),destination.absoluteFilePath()+"/"+fileInfo.fileName(),mode,fileInfo);
                                #else
                                emit fileTransfer(fileInfo.absoluteFilePath(),destination.absoluteFilePath()+"/"+fileInfo.fileName(),mode);
                                #endif
                        }
                        #endif
                    }
                }
            }
        }
        else
        {
            const INTERNALTYPEPATH fileName(fileInfo.d_name);
            if(fileInfo.isFolder)//possible wait time here
            {
                //listFolder(source,destination,suffixPath+fileInfo.fileName()+QDir::separator());
                INTERNALTYPEPATH fullSource=source;
                if(!stringEndsWith(fullSource,'/'))
                    fullSource+='/';
                fullSource+=fileName;
                INTERNALTYPEPATH fullDestination=destination;
                if(!stringEndsWith(fullDestination,'/'))
                    fullDestination+='/';
                fullDestination+=fileName;
                listFolder(fullSource,fullDestination);
            }
            else
            {
                INTERNALTYPEPATH fullSource=source;
                if(!stringEndsWith(fullSource,'/'))
                    fullSource+='/';
                fullSource+=fileName;
                #ifndef ULTRACOPIER_PLUGIN_RSYNC
                    #ifdef Q_OS_WIN32
                    emit fileTransferWithInode(fullSource,destination+TransferThread::stringToInternalString("/")+fileName,mode,fileInfo);
                    #else
                    emit fileTransfer(fullSource,destination+TransferThread::stringToInternalString("/")+fileName,mode);
                    #endif
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
                        #ifdef Q_OS_WIN32
                        emit fileTransferWithInode(fileInfo.absoluteFilePath(),destination.absoluteFilePath()+"/"+fileInfo.fileName(),mode,fileInfo);
                        #else
                        emit fileTransfer(fileInfo.absoluteFilePath(),destination.absoluteFilePath()+"/"+fileInfo.fileName(),mode);
                        #endif
                }
                #endif
            }
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+TransferThread::internalStringTostring(source)+", sizeEntryList: "+std::to_string(sizeEntryList));
        emit addToMovePath(source,destination,sizeEntryList);
    }
    else// if(keepDate or keep permition, perfer alwasy send it)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+TransferThread::internalStringTostring(source)+", sizeEntryList: "+std::to_string(sizeEntryList));
        emit addToKeepAttributePath(source,destination,sizeEntryList);
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

void ScanFileOrFolder::setFollowTheStrictOrder(const bool &order)
{
    this->copyListOrder=order;
}

void ScanFileOrFolder::setignoreBlackList(const bool &ignoreBlackList)
{
    this->ignoreBlackList=ignoreBlackList;
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
