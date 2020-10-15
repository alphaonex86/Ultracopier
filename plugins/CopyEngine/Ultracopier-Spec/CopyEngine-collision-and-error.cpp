/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86 */

#include "CopyEngine.h"
#include "FolderExistsDialog.h"
#include "DiskSpace.h"

//dialog message
/// \note Can be call without queue because all call will be serialized
void CopyEngine::fileAlreadyExistsSlot(INTERNALTYPEPATH source,INTERNALTYPEPATH destination,bool isSame,TransferThreadAsync * thread)
{
    fileAlreadyExists(source,destination,isSame,thread);
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::errorOnFileSlot(INTERNALTYPEPATH fileInfo,std::string errorString,TransferThreadAsync * thread,const ErrorType &errorType)
{
    errorOnFile(fileInfo,errorString,thread,errorType);
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::folderAlreadyExistsSlot(INTERNALTYPEPATH source,INTERNALTYPEPATH destination,bool isSame,ScanFileOrFolder * thread)
{
    folderAlreadyExists(source,destination,isSame,thread);
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::errorOnFolderSlot(INTERNALTYPEPATH fileInfo,std::string errorString,ScanFileOrFolder * thread,ErrorType errorType)
{
    errorOnFolder(fileInfo,errorString,thread,errorType);
}

//mkpath event
void CopyEngine::mkPathErrorOnFolderSlot(INTERNALTYPEPATH folder,std::string error,ErrorType errorType)
{
    mkPathErrorOnFolder(folder,error,errorType);
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::fileAlreadyExists(INTERNALTYPEPATH source,INTERNALTYPEPATH destination,bool isSame,TransferThreadAsync * thread)
{
    if(stopIt)
        return;
    if(thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to locate the thread");
        return;
    }
    //load the action
    if(isSame)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file is same: "+TransferThread::internalStringTostring(source));
        FileExistsAction tempFileExistsAction=alwaysDoThisActionForFileExists;
        if(tempFileExistsAction==FileExists_Overwrite || tempFileExistsAction==FileExists_OverwriteIfNewer || tempFileExistsAction==FileExists_OverwriteIfNotSameMdate || tempFileExistsAction==FileExists_OverwriteIfOlder)
            tempFileExistsAction=FileExists_NotSet;
        switch(tempFileExistsAction)
        {
            case FileExists_Skip:
            case FileExists_Rename:
                thread->setFileExistsAction(tempFileExistsAction);
            break;
            default:
                if(dialogIsOpen)
                {
                    alreadyExistsQueueItem newItem;
                    newItem.source=source;
                    newItem.destination=destination;
                    newItem.isSame=isSame;
                    newItem.transfer=thread;
                    newItem.scan=NULL;
                    alreadyExistsQueue.push_back(newItem);
                    return;
                }
                dialogIsOpen=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
                FileIsSameDialog dialog(uiinterface,source,firstRenamingRule,otherRenamingRule,facilityEngine);
                emit isInPause(true);
                dialog.exec();/// \bug crash when external close
                FileExistsAction newAction=dialog.getAction();
                emit isInPause(false);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+std::to_string(newAction));
                if(newAction==FileExists_Cancel)
                {
                    emit cancelAll();
                    return;
                }
                if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileExists)
                {
                    alwaysDoThisActionForFileExists=newAction;
                    listThread->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
                    if(uiIsInstalled)
                        switch(newAction)
                        {
                            default:
                            case FileExists_Skip:
                                ui->comboBoxFileCollision->setCurrentIndex(1);
                            break;
                            case FileExists_Rename:
                                ui->comboBoxFileCollision->setCurrentIndex(6);
                            break;
                        }
                }
                if(dialog.getAlways() || newAction!=FileExists_Rename)
                    thread->setFileExistsAction(newAction);
                else
                    thread->setFileRename(dialog.getNewName());
                dialogIsOpen=false;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit queryOneNewDialog()");
                //always call to check if pending dialog
                emit queryOneNewDialog();
                return;
            break;
        }
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file already exists: "+TransferThread::internalStringTostring(source)+
                                 ", destination: "+TransferThread::internalStringTostring(destination));
        FileExistsAction tempFileExistsAction=alwaysDoThisActionForFileExists;
        switch(tempFileExistsAction)
        {
            case FileExists_Skip:
            case FileExists_Rename:
            case FileExists_Overwrite:
            case FileExists_OverwriteIfNewer:
            case FileExists_OverwriteIfOlder:
            case FileExists_OverwriteIfNotSameMdate:
            case FileExists_OverwriteIfNotSameSize:
            case FileExists_OverwriteIfNotSameSizeAndDate:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"always do this action: "+std::to_string(tempFileExistsAction));
                thread->setFileExistsAction(tempFileExistsAction);
            break;
            default:
                if(dialogIsOpen)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"dialog open, put in queue: "+
                                             TransferThread::internalStringTostring(source)+" "+
                                             TransferThread::internalStringTostring(destination)
                                 );
                    alreadyExistsQueueItem newItem;
                    newItem.source=source;
                    newItem.destination=destination;
                    newItem.isSame=isSame;
                    newItem.transfer=thread;
                    newItem.scan=NULL;
                    alreadyExistsQueue.push_back(newItem);
                    return;
                }
                dialogIsOpen=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
                FileExistsDialog dialog(uiinterface,source,destination,firstRenamingRule,otherRenamingRule,facilityEngine);
                emit isInPause(true);
                dialog.exec();/// \bug crash when external close
                FileExistsAction newAction=dialog.getAction();
                emit isInPause(false);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+std::to_string(newAction));
                if(newAction==FileExists_Cancel)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"cancel");
                    emit cancelAll();
                    return;
                }
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: newAction!=FileExists_Cancel");
                if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileExists)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: always");
                    alwaysDoThisActionForFileExists=newAction;
                    listThread->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
                    if(uiIsInstalled)
                        switch(newAction)
                        {
                            default:
                            case FileExists_Skip:
                                ui->comboBoxFileCollision->setCurrentIndex(1);
                            break;
                            case FileExists_Rename:
                                ui->comboBoxFileCollision->setCurrentIndex(6);
                            break;
                            case FileExists_Overwrite:
                                ui->comboBoxFileCollision->setCurrentIndex(2);
                            break;
                            case FileExists_OverwriteIfNotSameMdate:
                                ui->comboBoxFileCollision->setCurrentIndex(3);
                            break;
                            case FileExists_OverwriteIfNewer:
                                ui->comboBoxFileCollision->setCurrentIndex(4);
                            break;
                            case FileExists_OverwriteIfOlder:
                                ui->comboBoxFileCollision->setCurrentIndex(5);
                            break;
                            case FileExists_OverwriteIfNotSameSize:
                                ui->comboBoxFileCollision->setCurrentIndex(7);
                            break;
                            case FileExists_OverwriteIfNotSameSizeAndDate:
                                ui->comboBoxFileCollision->setCurrentIndex(8);
                            break;
                        }
                }
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: post always");
                if(dialog.getAlways() || newAction!=FileExists_Rename)
                    thread->setFileExistsAction(newAction);
                else
                    thread->setFileRename(dialog.getNewName());
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: post set thread");
                dialogIsOpen=false;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit queryOneNewDialog()");
                //always call to check if pending dialog
                emit queryOneNewDialog();
                return;
            break;
        }
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

void CopyEngine::haveNeedPutAtBottom(bool needPutAtBottom, const INTERNALTYPEPATH &fileInfo, const std::string &errorString,TransferThreadAsync *thread,const ErrorType &errorType)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(!needPutAtBottom)
    {
        alwaysDoThisActionForFileError=FileError_NotSet;
        if(uiIsInstalled)
            ui->comboBoxFileError->setCurrentIndex(0);
        errorQueueItem newItem;
        newItem.errorString=errorString;
        newItem.inode=fileInfo;
        newItem.mkPath=false;
        newItem.rmPath=false;
        newItem.scan=NULL;
        newItem.transfer=thread;
        newItem.errorType=errorType;
        errorQueue.push_back(newItem);
        showOneNewDialog();
    }
}

void CopyEngine::missingDiskSpace(std::vector<Diskspace> list)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
    DiskSpace dialog(facilityEngine,list,uiinterface);
    emit isInPause(true);
    dialog.exec();/// \bug crash when external close
    bool ok=dialog.getAction();
    emit isInPause(false);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"cancel: "+std::to_string(ok));
    if(!ok)
        emit cancelAll();
    else
        listThread->autoStartIfNeeded();
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::errorOnFile(INTERNALTYPEPATH fileInfo, std::string errorString, TransferThreadAsync * thread, const ErrorType &errorType)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+TransferThread::internalStringTostring(fileInfo)+
                             ", error: "+errorString);
    if(thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to locate the thread");
        return;
    }
    //load the action
    FileErrorAction tempFileErrorAction=alwaysDoThisActionForFileError;
    switch(tempFileErrorAction)
    {
        case FileError_Skip:
            thread->skip();
        return;
        case FileError_Retry:
            thread->retryAfterError();
        return;
        case FileError_PutToEndOfTheList:
            errorPutAtEnd++;
            emit getNeedPutAtBottom(fileInfo,errorString,thread,errorType);
            if(errorPutAtEnd>listThread->actionToDoListInode.size() || listThread->actionToDoListInode.size()==0)
            {
                alwaysDoThisActionForFileError=FileError_NotSet;
                errorPutAtEnd=0;
            }
        return;
        case FileError_Cancel:
        return;
        default:
            if(dialogIsOpen)
            {
                errorQueueItem newItem;
                newItem.errorString=errorString;
                newItem.inode=fileInfo;
                newItem.mkPath=false;
                newItem.rmPath=false;
                newItem.scan=NULL;
                newItem.transfer=thread;
                newItem.errorType=errorType;
                errorQueue.push_back(newItem);
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");

            uint64_t size=0;
            uint64_t mdate=0;
            #ifdef Q_OS_WIN32
            WIN32_FILE_ATTRIBUTE_DATA sourceW;
            if(GetFileAttributesExW(fileInfo.c_str(),GetFileExInfoStandard,&sourceW))
            {
                mdate=sourceW.ftLastWriteTime.dwHighDateTime;
                mdate<<=32;
                mdate|=sourceW.ftLastWriteTime.dwLowDateTime;
                size=sourceW.nFileSizeHigh;
                size<<=32;
                size|=sourceW.nFileSizeLow;
            }
            #else
            struct stat source_statbuf;
            #ifdef Q_OS_UNIX
            if(lstat(TransferThread::internalStringTostring(fileInfo).c_str(), &source_statbuf)==0)
            #else
            if(stat(TransferThread::internalStringTostring(fileInfo).c_str(), &source_statbuf)==0)
            #endif
            {
                #ifdef Q_OS_UNIX
                    #ifdef Q_OS_MAC
                    mdate=source_statbuf.st_mtimespec.tv_sec;
                    #else
                    mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtim);
                    #endif
                #else
                mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtime);
                #endif
                size=source_statbuf.st_size;
            }
            #endif

            emit error(TransferThread::internalStringTostring(fileInfo),size,mdate,errorString);
            FileErrorDialog dialog(uiinterface,fileInfo,errorString,errorType,facilityEngine);
            emit isInPause(true);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            emit isInPause(false);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+std::to_string(newAction));
            if(newAction==FileError_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileError)
            {
                alwaysDoThisActionForFileError=newAction;
                if(uiIsInstalled)
                    switch(newAction)
                    {
                        default:
                        case FileError_Skip:
                            ui->comboBoxFileError->setCurrentIndex(1);
                        break;
                        case FileError_PutToEndOfTheList:
                            ui->comboBoxFileError->setCurrentIndex(2);
                        break;
                    }
            }
            switch(newAction)
            {
                case FileError_Skip:
                    thread->skip();
                break;
                case FileError_Retry:
                    thread->retryAfterError();
                break;
                case FileError_PutToEndOfTheList:
                    errorPutAtEnd++;
                    thread->putAtBottom();
                break;
                default:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"file error action wrong");
                break;
            }
            dialogIsOpen=false;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit queryOneNewDialog()");
            //always call to check if pending dialog
            emit queryOneNewDialog();
            return;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::folderAlreadyExists(INTERNALTYPEPATH source, INTERNALTYPEPATH destination, bool isSame, ScanFileOrFolder * thread)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"folder already exists: "+TransferThread::internalStringTostring(source)+
                             ", destination: "+TransferThread::internalStringTostring(destination));
    if(thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to locate the thread");
        return;
    }
    //load the always action
    FolderExistsAction tempFolderExistsAction=alwaysDoThisActionForFolderExists;
    switch(tempFolderExistsAction)
    {
        case FolderExists_Skip:
        case FolderExists_Rename:
        case FolderExists_Merge:
            thread->setFolderExistsAction(tempFolderExistsAction);
        break;
        default:
            if(dialogIsOpen)
            {
                alreadyExistsQueueItem newItem;
                newItem.source=source;
                newItem.destination=destination;
                newItem.isSame=isSame;
                newItem.transfer=NULL;
                newItem.scan=thread;
                alreadyExistsQueue.push_back(newItem);
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            FolderExistsDialog dialog(uiinterface,source,isSame,destination,firstRenamingRule,otherRenamingRule);
            dialog.exec();/// \bug crash when external close
            FolderExistsAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+std::to_string(newAction));
            if(newAction==FolderExists_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFolderExists)
                setComboBoxFolderCollision(newAction);
            if(!dialog.getAlways() && newAction==FolderExists_Rename)
                thread->setFolderExistsAction(newAction,dialog.getNewName());
            else
                thread->setFolderExistsAction(newAction);
            dialogIsOpen=false;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit queryOneNewDialog()");
            //always call to check if pending dialog
            emit queryOneNewDialog();
            return;
        break;
    }
}

/// \note Can be call without queue because all call will be serialized
/// \todo all this part
void CopyEngine::errorOnFolder(INTERNALTYPEPATH fileInfo, std::string errorString, ScanFileOrFolder * thread, ErrorType errorType)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+TransferThread::internalStringTostring(fileInfo)+", error: "+errorString);
    if(thread==NULL)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to locate the thread");
        return;
    }
    //load the always action
    FileErrorAction tempFileErrorAction=alwaysDoThisActionForFolderError;
    switch(tempFileErrorAction)
    {
        case FileError_Skip:
        case FileError_Retry:
        case FileError_PutToEndOfTheList:
            thread->setFolderErrorAction(tempFileErrorAction);
        break;
        default:
            if(dialogIsOpen)
            {
                errorQueueItem newItem;
                newItem.errorString=errorString;
                newItem.inode=fileInfo;
                newItem.mkPath=false;
                newItem.rmPath=false;
                newItem.scan=thread;
                newItem.transfer=NULL;
                newItem.errorType=errorType;
                errorQueue.push_back(newItem);
                return;
            }
            dialogIsOpen=true;

            uint64_t size=0;
            uint64_t mdate=0;
            #ifdef Q_OS_WIN32
            WIN32_FILE_ATTRIBUTE_DATA sourceW;
            if(GetFileAttributesExW(fileInfo.c_str(),GetFileExInfoStandard,&sourceW))
            {
                mdate=sourceW.ftLastWriteTime.dwHighDateTime;
                mdate<<=32;
                mdate|=sourceW.ftLastWriteTime.dwLowDateTime;
                size=sourceW.nFileSizeHigh;
                size<<=32;
                size|=sourceW.nFileSizeLow;
            }
            #else
            struct stat source_statbuf;
            #ifdef Q_OS_UNIX
            if(lstat(TransferThread::internalStringTostring(fileInfo).c_str(), &source_statbuf)==0)
            #else
            if(stat(TransferThread::internalStringTostring(fileInfo).c_str(), &source_statbuf)==0)
            #endif
            {
                #ifdef Q_OS_UNIX
                    #ifdef Q_OS_MAC
                    mdate=source_statbuf.st_mtimespec.tv_sec;
                    #else
                    mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtim);
                    #endif
                #else
                mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtime);
                #endif
                size=source_statbuf.st_size;
            }
            #endif

            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(TransferThread::internalStringTostring(fileInfo),size,mdate,errorString);
            FileErrorDialog dialog(uiinterface,fileInfo,errorString,errorType,facilityEngine);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+std::to_string(newAction));
            if(newAction==FileError_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileError)
            {
                setComboBoxFolderError(newAction);
                alwaysDoThisActionForFolderError=newAction;
            }
            dialogIsOpen=false;
            thread->setFolderErrorAction(newAction);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit queryOneNewDialog()");
            //always call to check if pending dialog
            emit queryOneNewDialog();
            return;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

// -----------------------------------------------------

//mkpath event
void CopyEngine::mkPathErrorOnFolder(INTERNALTYPEPATH folder, std::string errorString, const ErrorType &errorType)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+TransferThread::internalStringTostring(folder)+", error: "+errorString);
    //load the always action
    FileErrorAction tempFileErrorAction=alwaysDoThisActionForFolderError;
    switch(tempFileErrorAction)
    {
        case FileError_Skip:
            listThread->mkPathQueue.skip();
        return;
        case FileError_Retry:
            listThread->mkPathQueue.retry();
        return;
        default:
            if(dialogIsOpen)
            {
                errorQueueItem newItem;
                newItem.errorString=errorString;
                newItem.inode=folder;
                newItem.mkPath=true;
                newItem.rmPath=false;
                newItem.scan=NULL;
                newItem.transfer=NULL;
                newItem.errorType=errorType;
                errorQueue.push_back(newItem);
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");

            uint64_t size=0;
            uint64_t mdate=0;
            #ifdef Q_OS_WIN32
            WIN32_FILE_ATTRIBUTE_DATA sourceW;
            if(GetFileAttributesExW(folder.c_str(),GetFileExInfoStandard,&sourceW))
            {
                mdate=sourceW.ftLastWriteTime.dwHighDateTime;
                mdate<<=32;
                mdate|=sourceW.ftLastWriteTime.dwLowDateTime;
                size=sourceW.nFileSizeHigh;
                size<<=32;
                size|=sourceW.nFileSizeLow;
            }
            #else
            struct stat source_statbuf;
            #ifdef Q_OS_UNIX
            if(lstat(TransferThread::internalStringTostring(folder).c_str(), &source_statbuf)==0)
            #else
            if(stat(TransferThread::internalStringTostring(folder).c_str(), &source_statbuf)==0)
            #endif
            {
                #ifdef Q_OS_UNIX
                    #ifdef Q_OS_MAC
                    mdate=source_statbuf.st_mtimespec.tv_sec;
                    #else
                    mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtim);
                    #endif
                #else
                mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtime);
                #endif
                size=source_statbuf.st_size;
            }
            #endif

            emit error(TransferThread::internalStringTostring(folder),size,mdate,errorString);
            FileErrorDialog dialog(uiinterface,folder,errorString,errorType,facilityEngine);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+std::to_string(newAction));
            if(newAction==FileError_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileError)
            {
                setComboBoxFolderError(newAction);
                alwaysDoThisActionForFolderError=newAction;
            }
            dialogIsOpen=false;
            switch(newAction)
            {
                case FileError_Skip:
                    listThread->mkPathQueue.skip();
                break;
                case FileError_Retry:
                    listThread->mkPathQueue.retry();
                break;
                default:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unknow switch case: "+std::to_string(newAction));
                break;
            }
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit queryOneNewDialog()");
            //always call to check if pending dialog
            emit queryOneNewDialog();
            return;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

//show one new dialog if needed
void CopyEngine::showOneNewDialog()
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"alreadyExistsQueue.size(): "+std::to_string(alreadyExistsQueue.size()));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"errorQueue.size(): "+std::to_string(errorQueue.size()));
    if(dialogIsOpen)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"dialogIsOpen should false here");
    {
        const std::vector<alreadyExistsQueueItem> alreadyExistsQueue=this->alreadyExistsQueue;
        this->alreadyExistsQueue.clear();
        for(const alreadyExistsQueueItem &u : alreadyExistsQueue)
        {
            if(u.transfer!=NULL)
            {
                fileAlreadyExists(u.source,
                          u.destination,
                          u.isSame,
                          u.transfer);
            }
            else if(u.scan!=NULL)
                folderAlreadyExists(u.source,
                            u.destination,
                            u.isSame,
                            u.scan);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"bug, no thread actived");
        }
    }
    {
        const std::vector<errorQueueItem> errorQueue=this->errorQueue;
        this->errorQueue.clear();
        for(const errorQueueItem &u : errorQueue)
        {
            if(u.transfer!=NULL)
                errorOnFile(u.inode,u.errorString,u.transfer,u.errorType);
            else if(u.scan!=NULL)
                errorOnFolder(u.inode,u.errorString,u.scan,u.errorType);
            else if(u.mkPath)
                mkPathErrorOnFolder(u.inode,u.errorString,u.errorType);
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"bug, no thread actived");
        }
    }
}
