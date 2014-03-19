#include "ScanFileOrFolder.h"
#include "TransferThread.h"
#include <QtGlobal>

#ifdef Q_OS_WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

QString ScanFileOrFolder::text_slash=QLatin1Literal("/");
QString ScanFileOrFolder::text_antislash=QLatin1Literal("\\");
QString ScanFileOrFolder::text_dot=QLatin1Literal(".");

ScanFileOrFolder::ScanFileOrFolder(const Ultracopier::CopyMode &mode)
{
    moveTheWholeFolder  = true;
    stopped             = true;
    stopIt              = false;
    this->mode          = mode;
    folder_isolation    = QRegularExpression(QStringLiteral("^(.*/)?([^/]+)/$"));
    setObjectName(QStringLiteral("ScanFileOrFolder"));
    #ifdef Q_OS_WIN32
    QString userName;
    DWORD size=255;
    WCHAR * userNameW=new WCHAR[size];
    if(GetUserNameW(userNameW,&size))
    {
        userName=QString::fromWCharArray(userNameW,size-1);
        blackList << QFileInfo(QStringLiteral("C:/Users/%1/AppData/Roaming/").arg(userName)).absoluteFilePath();
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

void ScanFileOrFolder::addToList(const QStringList& sources,const QString& destination)
{
    stopIt=false;
    this->sources=parseWildcardSources(sources);
    this->destination=destination;
    QFileInfo destinationInfo(this->destination);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("check symblink: %1").arg(destinationInfo.absoluteFilePath()));
    while(destinationInfo.isSymLink())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("resolv destination to: %1").arg(destinationInfo.symLinkTarget()));
        if(QFileInfo(destinationInfo.symLinkTarget()).isAbsolute())
            this->destination=destinationInfo.symLinkTarget();
        else
            this->destination=destinationInfo.absolutePath()+text_slash+destinationInfo.symLinkTarget();
        destinationInfo.setFile(this->destination);
    }
    if(sources.size()>1 || QFileInfo(destination).isDir())
        /* Disabled because the separator transformation product bug
         * if(!destination.endsWith(QDir::separator()))
            this->destination+=QDir::separator();*/
        if(!destination.endsWith(text_slash) && !destination.endsWith(text_antislash))
            this->destination+=text_slash;//put unix separator because it's transformed into that's under windows too
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"addToList("+sources.join(";")+","+this->destination+")");
}


QStringList ScanFileOrFolder::parseWildcardSources(const QStringList &sources) const
{
    QRegularExpression splitFolder(QStringLiteral("[/\\\\]"));
    QStringList returnList;
    int index=0;
    while(index<sources.size())
    {
        if(sources.at(index).contains(QStringLiteral("*")))
        {
            QStringList toParse=sources.at(index).split(splitFolder);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("before wildcard parse: %1, toParse: %2, is valid: %3").arg(sources.at(index)).arg(toParse.join(", ")).arg(splitFolder.isValid()));
            QList<QStringList> recomposedSource;
            recomposedSource << (QStringList() << QStringLiteral(""));
            while(toParse.size()>0)
            {
                if(toParse.first().contains('*'))
                {
                    QString toParseFirst=toParse.first();
                    if(toParseFirst.isEmpty())
                        toParseFirst=text_slash;
                    QList<QStringList> newRecomposedSource;
                    QRegularExpression toResolv=QRegularExpression(toParseFirst.replace('*',QStringLiteral("[^/\\\\]*")));
                    int index_recomposedSource=0;
                    while(index_recomposedSource<recomposedSource.size())//parse each url part
                    {
                        QFileInfo info(recomposedSource.at(index_recomposedSource).join(text_slash));
                        if(info.isDir() && !info.isSymLink())
                        {
                            QDir folder(info.absoluteFilePath());
                            QFileInfoList fileFile=folder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System);//QStringList() << toResolv
                            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("list the folder: %1, with the wildcard: %2").arg(info.absoluteFilePath()).arg(toResolv.pattern()));
                            int index_fileList=0;
                            while(index_fileList<fileFile.size())
                            {
                                if(fileFile.at(index_fileList).fileName().contains(toResolv))
                                {
                                    QStringList tempList=recomposedSource.at(index_recomposedSource);
                                    tempList << fileFile.at(index_fileList).fileName();
                                    newRecomposedSource << tempList;
                                }
                                index_fileList++;
                            }
                        }
                        index_recomposedSource++;
                    }
                    recomposedSource=newRecomposedSource;
                }
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("add toParse: %1").arg(toParse.join(text_slash)));
                    int index_recomposedSource=0;
                    while(index_recomposedSource<recomposedSource.size())
                    {
                        recomposedSource[index_recomposedSource] << toParse.first();
                        if(!QFileInfo(recomposedSource.at(index_recomposedSource).join(text_slash)).exists())
                            recomposedSource.removeAt(index_recomposedSource);
                        else
                            index_recomposedSource++;
                    }
                }
                toParse.removeFirst();
            }
            int index_recomposedSource=0;
            while(index_recomposedSource<recomposedSource.size())
            {
                returnList<<recomposedSource.at(index_recomposedSource).join(text_slash);
                index_recomposedSource++;
            }
        }
        else
            returnList << sources.at(index);
        index++;
    }
    return returnList;
}

void ScanFileOrFolder::setFilters(const QList<Filters_rules> &include, const QList<Filters_rules> &exclude)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    QMutexLocker lock(&filtersMutex);
    this->include_send=include;
    this->exclude_send=exclude;
    reloadTheNewFilters=true;
    haveFilters=include_send.size()>0 || exclude_send.size()>0;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("haveFilters: %1, include_send.size(): %2, exclude_send.size(): %3").arg(haveFilters).arg(include_send.size()).arg(exclude_send.size()));
}

//set action if Folder are same or exists
void ScanFileOrFolder::setFolderExistsAction(const FolderExistsAction &action, const QString &newName)
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start the listing with destination: "+destination+", mode: "+QString::number(mode));
    destination=resolvDestination(destination).absoluteFilePath();
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
    int sourceIndex=0;
    while(sourceIndex<sources.size())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"size source to list: "+QString::number(sourceIndex)+text_slash+QString::number(sources.size()));
        if(stopIt)
        {
            stopped=true;
            return;
        }
        QFileInfo source=sources.at(sourceIndex);
        if(source.isDir() && !source.isSymLink())
        {
            /* Bad way; when you copy c:\source\folder into d:\destination, you wait it create the folder d:\destination\folder
            //listFolder(source.absoluteFilePath()+QDir::separator(),destination);
            listFolder(source.absoluteFilePath()+text_slash,destination);//put unix separator because it's transformed into that's under windows too
            */
            //put unix separator because it's transformed into that's under windows too
            QString tempString=QFileInfo(destination).absoluteFilePath();
            if(!tempString.endsWith(text_slash) && !tempString.endsWith(text_antislash))
                tempString+=text_slash;
            tempString+=TransferThread::resolvedName(source);
            if(moveTheWholeFolder && mode==Ultracopier::Move && !QFileInfo(tempString).exists() && driveManagement.isSameDrive(source.absoluteFilePath(),tempString))
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("tempString: %1 move and not exists").arg(tempString));
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("do real move: %1 to %2").arg(source.absoluteFilePath()).arg(tempString));
                emit addToRealMove(source.absoluteFilePath(),tempString);
            }
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("tempString: %1 normal listing, blacklist size: %2").arg(tempString).arg(blackList.size()));
                listFolder(source.absoluteFilePath(),tempString);
            }
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("source: %1 is file or symblink").arg(source.absoluteFilePath()));
            emit fileTransfer(source,destination+text_slash+source.fileName(),mode);
        }
        sourceIndex++;
    }
    stopped=true;
    if(stopIt)
        return;
    emit finishedTheListing();
}

QFileInfo ScanFileOrFolder::resolvDestination(const QFileInfo &destination)
{
    QFileInfo newDestination=destination;
    while(newDestination.isSymLink())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("resolv destination to: %1").arg(newDestination.symLinkTarget()));
        if(QFileInfo(newDestination.symLinkTarget()).isAbsolute())
            newDestination.setFile(newDestination.symLinkTarget());
        else
            newDestination.setFile(newDestination.absolutePath()+text_slash+newDestination.symLinkTarget());
    }
    do
    {
        fileErrorAction=FileError_NotSet;
        if(isBlackListed(destination))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("isBlackListed: %1").arg(destination.absoluteFilePath()));
            emit errorOnFolder(destination,tr("Blacklisted folder"),ErrorType_Folder);
            waitOneAction.acquire();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionNum: "+QString::number(fileErrorAction));
        }
    } while(fileErrorAction==FileError_Retry || fileErrorAction==FileError_PutToEndOfTheList);
    return newDestination;
}

bool ScanFileOrFolder::isBlackListed(const QFileInfo &destination)
{
    int index=0;
    int size=blackList.size();
    while(index<size)
    {
        if(destination.absoluteFilePath().startsWith(blackList.at(index)))
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("%1 start with: %2").arg(destination.absoluteFilePath()).arg(blackList.at(index)));
            return true;
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("%1 not start with: %2").arg(destination.absoluteFilePath()).arg(blackList.at(index)));
        index++;
    }
    return false;
}

void ScanFileOrFolder::listFolder(QFileInfo source,QFileInfo destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("source: %1 (%2), destination: %3 (%4)").arg(source.absoluteFilePath()).arg(source.isSymLink()).arg(destination.absoluteFilePath()).arg(destination.isSymLink()));
    if(stopIt)
        return;
    destination=resolvDestination(destination);
    if(stopIt)
        return;
    if(fileErrorAction==FileError_Skip)
        return;
    //if is same
    if(source.absoluteFilePath()==destination.absoluteFilePath())
    {
        emit folderAlreadyExists(source,destination,true);
        waitOneAction.acquire();
        QString destinationSuffixPath;
        switch(folderExistsAction)
        {
            case FolderExists_Merge:
            break;
            case FolderExists_Skip:
                return;
            break;
            case FolderExists_Rename:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination before rename: "+destination.absoluteFilePath());
                if(newName.isEmpty())
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"pattern: "+folder_isolation.pattern());
                    //resolv the new name
                    destinationSuffixPath=destination.baseName();
                    int num=1;
                    do
                    {
                        if(num==1)
                        {
                            if(firstRenamingRule.isEmpty())
                                destinationSuffixPath=tr("%1 - copy").arg(destination.baseName());
                            else
                            {
                                destinationSuffixPath=firstRenamingRule;
                                destinationSuffixPath.replace(QStringLiteral("%name%"),destination.baseName());
                            }
                        }
                        else
                        {
                            if(otherRenamingRule.isEmpty())
                                destinationSuffixPath=tr("%1 - copy (%2)").arg(destination.baseName()).arg(num);
                            else
                            {
                                destinationSuffixPath=otherRenamingRule;
                                destinationSuffixPath.replace(QStringLiteral("%name%"),destination.baseName());
                                destinationSuffixPath.replace(QStringLiteral("%number%"),QString::number(num));
                            }
                        }
                        num++;
                        if(destination.completeSuffix().isEmpty())
                            destination.setFile(destination.absolutePath()+text_slash+destinationSuffixPath);
                        else
                            destination.setFile(destination.absolutePath()+text_slash+destinationSuffixPath+text_dot+destination.completeSuffix());
                    }
                    while(destination.exists());
                }
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use new name: "+newName);
                    destinationSuffixPath = newName;
                }
                destination.setFile(destination.absolutePath()+text_slash+destinationSuffixPath);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination after rename: "+destination.absoluteFilePath());
            break;
            default:
                return;
            break;
        }
    }
    //check if destination exists
    if(checkDestinationExists)
    {
        if(destination.exists())
        {
            emit folderAlreadyExists(source,destination,false);
            waitOneAction.acquire();
            QString destinationSuffixPath;
            switch(folderExistsAction)
            {
                case FolderExists_Merge:
                break;
                case FolderExists_Skip:
                    return;
                break;
                case FolderExists_Rename:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination before rename: "+destination.absoluteFilePath());
                    if(newName.isEmpty())
                    {
                        //resolv the new name
                        QFileInfo destinationInfo;
                        int num=1;
                        do
                        {
                            if(num==1)
                            {
                                if(firstRenamingRule.isEmpty())
                                    destinationSuffixPath=tr("%1 - copy").arg(destination.baseName());
                                else
                                {
                                    destinationSuffixPath=firstRenamingRule;
                                    destinationSuffixPath.replace(QStringLiteral("%name%"),destination.baseName());
                                }
                            }
                            else
                            {
                                if(otherRenamingRule.isEmpty())
                                    destinationSuffixPath=tr("%1 - copy (%2)").arg(destination.baseName()).arg(num);
                                else
                                {
                                    destinationSuffixPath=otherRenamingRule;
                                    destinationSuffixPath.replace(QStringLiteral("%name%"),destination.baseName());
                                    destinationSuffixPath.replace(QStringLiteral("%number%"),QString::number(num));
                                }
                            }
                            destinationInfo.setFile(destinationInfo.absolutePath()+text_slash+destinationSuffixPath);
                            num++;
                        }
                        while(destinationInfo.exists());
                    }
                    else
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"use new name: "+newName);
                        destinationSuffixPath = newName;
                    }
                    if(destination.completeSuffix().isEmpty())
                        destination.setFile(destination.absolutePath()+text_slash+destinationSuffixPath);
                    else
                        destination.setFile(destination.absolutePath()+text_slash+destinationSuffixPath+QStringLiteral(".")+destination.completeSuffix());
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"destination after rename: "+destination.absoluteFilePath());
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
                emit errorOnFolder(source,tr("This is not a folder"));
            else if(!source.exists())
                emit errorOnFolder(source,tr("The folder does exists"));
            else
                emit errorOnFolder(source,tr("The folder is not readable"));
            waitOneAction.acquire();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionNum: "+QString::number(fileErrorAction));
        }
    } while(fileErrorAction==FileError_Retry);
    do
    {
        QDir tempDir(source.absoluteFilePath());
        fileErrorAction=FileError_NotSet;
        if(!tempDir.isReadable() || !tempDir.exists())
        {
            emit errorOnFolder(source,tr("Problem with name encoding"));
            waitOneAction.acquire();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"actionNum: "+QString::number(fileErrorAction));
        }
    } while(fileErrorAction==FileError_Retry);
    if(stopIt)
        return;
    /// \todo check here if the folder is not readable or not exists
    QFileInfoList entryList=QDir(source.absoluteFilePath()).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);//possible wait time here
    if(stopIt)
        return;
    int sizeEntryList=entryList.size();
    emit newFolderListing(source.absoluteFilePath());
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
            QString fileName=fileInfo.fileName();
            if(fileInfo.isDir() && !fileInfo.isSymLink())
            {
                bool excluded=false,included=(include.size()==0);
                int filters_index=0;
                while(filters_index<exclude.size())
                {
                    if(exclude.at(filters_index).apply_on==ApplyOn_folder || exclude.at(filters_index).apply_on==ApplyOn_fileAndFolder)
                    {
                        if(fileName.contains(exclude.at(filters_index).regex))
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
                            if(fileName.contains(include.at(filters_index).regex))
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
                        listFolder(fileInfo,destination.absoluteFilePath()+text_slash+fileInfo.fileName());
                }
            }
            else
            {
                bool excluded=false,included=(include.size()==0);
                int filters_index=0;
                while(filters_index<exclude.size())
                {
                    if(exclude.at(filters_index).apply_on==ApplyOn_file || exclude.at(filters_index).apply_on==ApplyOn_fileAndFolder)
                    {
                        if(fileName.contains(exclude.at(filters_index).regex))
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
                            if(fileName.contains(include.at(filters_index).regex))
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
                        emit fileTransfer(fileInfo,destination.absoluteFilePath()+text_slash+fileInfo.fileName(),mode);
                }
            }
        }
        else
        {
            if(fileInfo.isDir() && !fileInfo.isSymLink())//possible wait time here
                //listFolder(source,destination,suffixPath+fileInfo.fileName()+QDir::separator());
                listFolder(fileInfo,destination.absoluteFilePath()+text_slash+fileInfo.fileName());//put unix separator because it's transformed into that's under windows too
            else
                emit fileTransfer(fileInfo,destination.absoluteFilePath()+text_slash+fileInfo.fileName(),mode);
        }
    }
    if(mode==Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"source: "+source.absoluteFilePath()+", sizeEntryList: "+QString::number(sizeEntryList));
        emit addToMovePath(source,destination,sizeEntryList);
    }
}

//set if need check if the destination exists
void ScanFileOrFolder::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
    this->checkDestinationExists=checkDestinationFolderExists;
}

void ScanFileOrFolder::setRenamingRules(const QString &firstRenamingRule, const QString &otherRenamingRule)
{
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
}

void ScanFileOrFolder::setMoveTheWholeFolder(const bool &moveTheWholeFolder)
{
    this->moveTheWholeFolder=moveTheWholeFolder;
}

void ScanFileOrFolder::setDrive(const QStringList &mountSysPoint,const QList<QStorageInfo::DriveType> &driveType)
{
    driveManagement.setDrive(mountSysPoint,driveType);
}
