/** \file copyEngine.cpp
\brief Define the copy engine
\author alpha_one_x86
\version 0.3
\date 2010 */

#include "copyEngine.h"
#include "folderExistsDialog.h"

//dialog message
/// \note Can be call without queue because all call will be serialized
void copyEngine::fileAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread)
{
    fileAlreadyExists(source,destination,isSame,thread);
}

/// \note Can be call without queue because all call will be serialized
void copyEngine::errorOnFileSlot(QFileInfo fileInfo,QString errorString,TransferThread * thread)
{
    errorOnFile(fileInfo,errorString,thread);
}

/// \note Can be call without queue because all call will be serialized
void copyEngine::folderAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,scanFileOrFolder * thread)
{
    folderAlreadyExists(source,destination,isSame,thread);
}

/// \note Can be call without queue because all call will be serialized
void copyEngine::errorOnFolderSlot(QFileInfo fileInfo,QString errorString,scanFileOrFolder * thread)
{
    errorOnFolder(fileInfo,errorString,thread);
}

//mkpath event
void copyEngine::mkPathErrorOnFolderSlot(QFileInfo folder,QString error)
{
    mkPathErrorOnFolder(folder,error);
}

//rmpath event
void copyEngine::rmPathErrorOnFolderSlot(QFileInfo folder,QString error)
{
    rmPathErrorOnFolder(folder,error);
}


/// \note Can be call without queue because all call will be serialized
void copyEngine::fileAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread,bool isCalledByShowOneNewDialog)
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
        tempFileExistsAction=alwaysDoThisActionForFileExists;
        if(tempFileExistsAction==FileExists_Overwrite || tempFileExistsAction==FileExists_OverwriteIfNewer || tempFileExistsAction==FileExists_OverwriteIfNotSameModificationDate)
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
                fileIsSameDialog dialog(interface,source,firstRenamingRule,otherRenamingRule);
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
                            emit newCollisionAction("skip");
                        break;
                        case FileExists_Rename:
                            emit newCollisionAction("rename");
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
        tempFileExistsAction=alwaysDoThisActionForFileExists;
        switch(tempFileExistsAction)
        {
            case FileExists_Skip:
            case FileExists_Rename:
            case FileExists_Overwrite:
            case FileExists_OverwriteIfNewer:
            case FileExists_OverwriteIfNotSameModificationDate:
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
                fileExistsDialog dialog(interface,source,destination,firstRenamingRule,otherRenamingRule);
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
                            emit newCollisionAction("skip");
                        break;
                        case FileExists_Rename:
                            emit newCollisionAction("rename");
                        break;
                        case FileExists_Overwrite:
                            emit newCollisionAction("overwrite");
                        break;
                        case FileExists_OverwriteIfNewer:
                            emit newCollisionAction("overwriteIfNewer");
                        break;
                        case FileExists_OverwriteIfNotSameModificationDate:
                            emit newCollisionAction("overwriteIfNotSameModificationDate");
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

void copyEngine::haveNeedPutAtBottom(bool needPutAtBottom, const QFileInfo &fileInfo, const QString &errorString,TransferThread *thread)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(!needPutAtBottom)
    {
        alwaysDoThisActionForFileError=FileError_NotSet;
        emit newErrorAction("ask");
        errorQueueItem newItem;
        newItem.errorString=errorString;
        newItem.inode=fileInfo;
        newItem.mkPath=false;
        newItem.rmPath=false;
        newItem.scan=NULL;
        newItem.transfer=thread;
        errorQueue << newItem;
        showOneNewDialog();
    }
}

/// \note Can be call without queue because all call will be serialized
void copyEngine::errorOnFile(QFileInfo fileInfo,QString errorString,TransferThread * thread,bool isCalledByShowOneNewDialog)
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
    tempFileErrorAction=alwaysDoThisActionForFileError;
    switch(tempFileErrorAction)
    {
        case FileError_Skip:
            thread->skip();
        return;
        case FileError_Retry:
            thread->retryAfterError();
        return;
        case FileError_PutToEndOfTheList:
            emit getNeedPutAtBottom(fileInfo,errorString,thread);
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
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(fileInfo.absoluteFilePath(),fileInfo.size(),fileInfo.lastModified(),errorString);
            fileErrorDialog dialog(interface,fileInfo,errorString);
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
                        emit newErrorAction("skip");
                    break;
                    case FileError_PutToEndOfTheList:
                        emit newErrorAction("putToEndOfTheList");
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
void copyEngine::folderAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,scanFileOrFolder * thread,bool isCalledByShowOneNewDialog)
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
    tempFolderExistsAction=alwaysDoThisActionForFolderExists;
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
            folderExistsDialog dialog(interface,source,isSame,destination,firstRenamingRule,otherRenamingRule);
            dialog.exec();/// \bug crash when external close
            FolderExistsAction newAction=dialog.getAction();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"close dialog: "+QString::number(newAction));
            if(newAction==FolderExists_Cancel)
            {
                emit cancelAll();
                return;
            }
            if(dialog.getAlways() && newAction!=alwaysDoThisActionForFolderExists)
                setComboBoxFolderColision(newAction);
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
void copyEngine::errorOnFolder(QFileInfo fileInfo,QString errorString,scanFileOrFolder * thread,bool isCalledByShowOneNewDialog)
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
    tempFileErrorAction=alwaysDoThisActionForFolderError;
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
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(fileInfo.absoluteFilePath(),fileInfo.size(),fileInfo.lastModified(),errorString);
            fileErrorDialog dialog(interface,fileInfo,errorString);
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
void copyEngine::mkPathErrorOnFolder(QFileInfo folder,QString errorString,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+folder.absoluteFilePath()+", error: "+errorString);
    //load the always action
    tempFileErrorAction=alwaysDoThisActionForFolderError;
    error_index=0;
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
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(folder.absoluteFilePath(),folder.size(),folder.lastModified(),errorString);
            fileErrorDialog dialog(interface,folder,errorString,false);
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

//rmpath event
void copyEngine::rmPathErrorOnFolder(QFileInfo folder,QString errorString,bool isCalledByShowOneNewDialog)
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"file have error: "+folder.absoluteFilePath()+", error: "+errorString);
    //load the always action
    tempFileErrorAction=alwaysDoThisActionForFolderError;
    error_index=0;
    switch(tempFileErrorAction)
    {
        case FileError_Skip:
            listThread->rmPathQueue.skip();
        return;
        case FileError_Retry:
            listThread->rmPathQueue.retry();
        return;
        default:
            if(dialogIsOpen)
            {
                errorQueueItem newItem;
                newItem.errorString=errorString;
                newItem.inode=folder;
                newItem.mkPath=false;
                newItem.rmPath=true;
                newItem.scan=NULL;
                newItem.transfer=NULL;
                errorQueue << newItem;
                return;
            }
            dialogIsOpen=true;
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"show dialog");
            emit error(folder.absoluteFilePath(),folder.size(),folder.lastModified(),errorString);
            fileErrorDialog dialog(interface,folder,errorString,false);
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
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"do the action");
            switch(newAction)
            {
                case FileError_Skip:
                    listThread->rmPathQueue.skip();
                break;
                case FileError_Retry:
                    listThread->rmPathQueue.retry();
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
void copyEngine::showOneNewDialog()
{
    if(stopIt)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"alreadyExistsQueue.size(): "+QString::number(alreadyExistsQueue.size()));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"errorQueue.size(): "+QString::number(errorQueue.size()));
    loop_size=alreadyExistsQueue.size();
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
            errorOnFile(errorQueue.first().inode,errorQueue.first().errorString,errorQueue.first().transfer,true);
        else if(errorQueue.first().scan!=NULL)
            errorOnFolder(errorQueue.first().inode,errorQueue.first().errorString,errorQueue.first().scan,true);
        else if(errorQueue.first().mkPath)
            mkPathErrorOnFolder(errorQueue.first().inode,errorQueue.first().errorString,true);
        else if(errorQueue.first().rmPath)
            rmPathErrorOnFolder(errorQueue.first().inode,errorQueue.first().errorString,true);
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"bug, no thread actived");
        errorQueue.removeFirst();
        loop_size--;
    }
}
