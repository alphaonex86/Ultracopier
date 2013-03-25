/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "CopyEngine.h"
#include "FolderExistsDialog.h"
#include "DiskSpace.h"

//dialog message
/// \note Can be call without queue because all call will be serialized
void CopyEngine::fileAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread)
{
    fileAlreadyExists(source,destination,isSame,thread);
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::errorOnFileSlot(QFileInfo fileInfo,QString errorString,TransferThread * thread,const ErrorType &errorType)
{
    errorOnFile(fileInfo,errorString,thread,errorType);
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::folderAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,ScanFileOrFolder * thread)
{
    folderAlreadyExists(source,destination,isSame,thread);
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::errorOnFolderSlot(QFileInfo fileInfo,QString errorString,ScanFileOrFolder * thread,ErrorType errorType)
{
    errorOnFolder(fileInfo,errorString,thread,errorType);
}

//mkpath event
void CopyEngine::mkPathErrorOnFolderSlot(QFileInfo folder,QString error,ErrorType errorType)
{
    mkPathErrorOnFolder(folder,error,errorType);
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::fileAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread,bool isCalledByShowOneNewDialog)
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
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file is same: "+source.absoluteFilePath());
        FileExistsAction tempFileExistsAction=alwaysDoThisActionForFileExists;
        if(tempFileExistsAction==FileExists_Overwrite || tempFileExistsAction==FileExists_OverwriteIfNewer || tempFileExistsAction==FileExists_OverwriteIfNotSame || tempFileExistsAction==FileExists_OverwriteIfOlder)
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
                    alreadyExistsQueue << newItem;
                    return;
                }
                dialogIsOpen=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
                FileIsSameDialog dialog(interface,source,firstRenamingRule,otherRenamingRule);
                emit isInPause(true);
                dialog.exec();/// \bug crash when external close
                FileExistsAction newAction=dialog.getAction();
                emit isInPause(false);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
                if(newAction==FileExists_Cancel)
                {
                    emit cancelAll();
                    return;
                }
                if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileExists)
                {
                    alwaysDoThisActionForFileExists=newAction;
                    listThread->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
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
                if(!isCalledByShowOneNewDialog)
                    emit queryOneNewDialog();
                return;
            break;
        }
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file already exists: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
        FileExistsAction tempFileExistsAction=alwaysDoThisActionForFileExists;
        switch(tempFileExistsAction)
        {
            case FileExists_Skip:
            case FileExists_Rename:
            case FileExists_Overwrite:
            case FileExists_OverwriteIfNewer:
            case FileExists_OverwriteIfOlder:
            case FileExists_OverwriteIfNotSame:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"always do this action: "+QString::number(tempFileExistsAction));
                thread->setFileExistsAction(tempFileExistsAction);
            break;
            default:
                if(dialogIsOpen)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("dialog open, put in queue: %1 %2")
                                 .arg(source.absoluteFilePath())
                                 .arg(destination.absoluteFilePath())
                                 );
                    alreadyExistsQueueItem newItem;
                    newItem.source=source;
                    newItem.destination=destination;
                    newItem.isSame=isSame;
                    newItem.transfer=thread;
                    newItem.scan=NULL;
                    alreadyExistsQueue << newItem;
                    return;
                }
                dialogIsOpen=true;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
                FileExistsDialog dialog(interface,source,destination,firstRenamingRule,otherRenamingRule);
                emit isInPause(true);
                dialog.exec();/// \bug crash when external close
                FileExistsAction newAction=dialog.getAction();
                emit isInPause(false);
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
                if(newAction==FileExists_Cancel)
                {
                    emit cancelAll();
                    return;
                }
                if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileExists)
                {
                    alwaysDoThisActionForFileExists=newAction;
                    listThread->setAlwaysFileExistsAction(alwaysDoThisActionForFileExists);
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
                        case FileExists_OverwriteIfNotSame:
                            ui->comboBoxFileCollision->setCurrentIndex(3);
                        break;
                        case FileExists_OverwriteIfNewer:
                            ui->comboBoxFileCollision->setCurrentIndex(4);
                        break;
                        case FileExists_OverwriteIfOlder:
                            ui->comboBoxFileCollision->setCurrentIndex(5);
                        break;
                    }
                }
                if(dialog.getAlways() || newAction!=FileExists_Rename)
                    thread->setFileExistsAction(newAction);
                else
                    thread->setFileRename(dialog.getNewName());
                dialogIsOpen=false;
                if(!isCalledByShowOneNewDialog)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"emit queryOneNewDialog()");
                    emit queryOneNewDialog();
                }
                return;
            break;
        }
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

void CopyEngine::haveNeedPutAtBottom(bool needPutAtBottom, const QFileInfo &fileInfo, const QString &errorString,TransferThread *thread,const ErrorType &errorType)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(!needPutAtBottom)
    {
        alwaysDoThisActionForFileError=FileError_NotSet;
        ui->comboBoxFileError->setCurrentIndex(0);
        errorQueueItem newItem;
        newItem.errorString=errorString;
        newItem.inode=fileInfo;
        newItem.mkPath=false;
        newItem.rmPath=false;
        newItem.scan=NULL;
        newItem.transfer=thread;
        newItem.errorType=errorType;
        errorQueue << newItem;
        showOneNewDialog();
    }
}

void CopyEngine::missingDiskSpace(QList<Diskspace> list)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
    DiskSpace dialog(facilityEngine,list,interface);
    emit isInPause(true);
    dialog.exec();/// \bug crash when external close
    bool ok=dialog.getAction();
    emit isInPause(false);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"cancel: "+QString::number(ok));
    if(!ok)
        emit cancelAll();
    else
        listThread->autoStartIfNeeded();
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::errorOnFile(QFileInfo fileInfo,QString errorString,TransferThread * thread,const ErrorType &errorType,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+fileInfo.absoluteFilePath()+", error: "+errorString);
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
            emit getNeedPutAtBottom(fileInfo,errorString,thread,errorType);
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
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(fileInfo.absoluteFilePath(),fileInfo.size(),fileInfo.lastModified(),errorString);
            FileErrorDialog dialog(interface,fileInfo,errorString,errorType);
            emit isInPause(true);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            emit isInPause(false);
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
            if(newAction==FileError_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFileError)
            {
                alwaysDoThisActionForFileError=newAction;
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
                    thread->putAtBottom();
                break;
                default:
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"file error action wrong");
                break;
            }
            dialogIsOpen=false;
            if(!isCalledByShowOneNewDialog)
                emit queryOneNewDialog();
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"isCalledByShowOneNewDialog==true then not show other dial");
            return;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

/// \note Can be call without queue because all call will be serialized
void CopyEngine::folderAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,ScanFileOrFolder * thread,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"folder already exists: "+source.absoluteFilePath()+", destination: "+destination.absoluteFilePath());
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
                alreadyExistsQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            FolderExistsDialog dialog(interface,source,isSame,destination,firstRenamingRule,otherRenamingRule);
            dialog.exec();/// \bug crash when external close
            FolderExistsAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
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
            if(!isCalledByShowOneNewDialog)
                emit queryOneNewDialog();
            return;
        break;
    }
}

/// \note Can be call without queue because all call will be serialized
/// \todo all this part
void CopyEngine::errorOnFolder(QFileInfo fileInfo, QString errorString, ScanFileOrFolder * thread, ErrorType errorType, bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+fileInfo.absoluteFilePath()+", error: "+errorString);
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
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(fileInfo.absoluteFilePath(),fileInfo.size(),fileInfo.lastModified(),errorString);
            FileErrorDialog dialog(interface,fileInfo,errorString,errorType);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
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
            if(!isCalledByShowOneNewDialog)
                emit queryOneNewDialog();
            return;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"stop");
}

// -----------------------------------------------------

//mkpath event
void CopyEngine::mkPathErrorOnFolder(QFileInfo folder,QString errorString,const ErrorType &errorType,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+folder.absoluteFilePath()+", error: "+errorString);
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
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(folder.absoluteFilePath(),folder.size(),folder.lastModified(),errorString);
            FileErrorDialog dialog(interface,folder,errorString,errorType);
            dialog.exec();/// \bug crash when external close
            FileErrorAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
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
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unknow switch case: "+QString::number(newAction));
                break;
            }
            if(!isCalledByShowOneNewDialog)
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
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"alreadyExistsQueue.size(): "+QString::number(alreadyExistsQueue.size()));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"errorQueue.size(): "+QString::number(errorQueue.size()));
    int loop_size=alreadyExistsQueue.size();
    while(loop_size>0)
    {
        if(alreadyExistsQueue.first().transfer!=NULL)
        {
            fileAlreadyExists(alreadyExistsQueue.first().source,
                      alreadyExistsQueue.first().destination,
                      alreadyExistsQueue.first().isSame,
                      alreadyExistsQueue.first().transfer,
                      true);
        }
        else if(alreadyExistsQueue.first().scan!=NULL)
            folderAlreadyExists(alreadyExistsQueue.first().source,
                        alreadyExistsQueue.first().destination,
                        alreadyExistsQueue.first().isSame,
                        alreadyExistsQueue.first().scan,
                        true);
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"bug, no thread actived");
        alreadyExistsQueue.removeFirst();
        loop_size--;
    }
    loop_size=errorQueue.size();
    while(errorQueue.size()>0)
    {
        if(errorQueue.first().transfer!=NULL)
            errorOnFile(errorQueue.first().inode,errorQueue.first().errorString,errorQueue.first().transfer,errorQueue.first().errorType,true);
        else if(errorQueue.first().scan!=NULL)
            errorOnFolder(errorQueue.first().inode,errorQueue.first().errorString,errorQueue.first().scan,errorQueue.first().errorType,true);
        else if(errorQueue.first().mkPath)
            mkPathErrorOnFolder(errorQueue.first().inode,errorQueue.first().errorString,errorQueue.first().errorType,true);
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"bug, no thread actived");
        errorQueue.removeFirst();
        loop_size--;
    }
}
