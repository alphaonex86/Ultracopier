/** \file ListThread_InodeAction.cpp
\brief To be included into ListThread.cpp, to optimize and prevent code duplication
\see ListThread.cpp */

#ifdef LISTTHREAD_H

//do the inode action
ActionToDoInode& currentActionToDoInode=actionToDoListInode[int_for_internal_loop];
switch(currentActionToDoInode.type)
{
    case ActionType_RealMove:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"launch real move, source: "+currentActionToDoInode.source+", destination: "+currentActionToDoInode.destination);
        mkPathQueue.addPath(currentActionToDoInode.source,currentActionToDoInode.destination,currentActionToDoInode.type);
        currentActionToDoInode.isRunning=true;
        numberOfInodeOperation++;
        if(numberOfInodeOperation>=inodeThreads)
            return;
    break;
    case ActionType_MkPath:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"launch mkpath, source: "+currentActionToDoInode.source+", destination: "+currentActionToDoInode.destination);
        mkPathQueue.addPath(currentActionToDoInode.source,currentActionToDoInode.destination,currentActionToDoInode.type);
        currentActionToDoInode.isRunning=true;
        numberOfInodeOperation++;
        if(numberOfInodeOperation>=inodeThreads)
            return;
    break;
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    case ActionType_RmSync:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"launch rmsync, destination: "+currentActionToDoInode.destination);
        mkPathQueue.addPath(currentActionToDoInode.destination,currentActionToDoInode.destination,currentActionToDoInode.type);
        currentActionToDoInode.isRunning=true;
        numberOfInodeOperation++;
        if(numberOfInodeOperation>=inodeThreads)
            return;
    break;
    #endif
    case ActionType_MovePath:
        //then empty (no file), can try remove it
        if(currentActionToDoInode.size==0 || actionToDoListTransfer.empty())//don't put afterTheTransfer because actionToDoListInode_afterTheTransfer -> already afterTheTransfer
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"launch rmpath: "+currentActionToDoInode.source);
            mkPathQueue.addPath(currentActionToDoInode.source,currentActionToDoInode.destination,currentActionToDoInode.type);
            currentActionToDoInode.isRunning=true;
            numberOfInodeOperation++;
            if(numberOfInodeOperation>=inodeThreads)
                return;
        }
        else //have do the destination, put the remove to after
        {
            currentActionToDoInode.size=0;
            actionToDoListInode_afterTheTransfer.push_back(currentActionToDoInode);
            actionToDoListInode.erase(actionToDoListInode.cbegin()+int_for_internal_loop);
            int_for_internal_loop--;
            actionToDoListInode_count--;
            if(numberOfInodeOperation>=inodeThreads)
                return;
        }
    break;
    default:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Wrong type at inode action");
    return;
}

#endif
