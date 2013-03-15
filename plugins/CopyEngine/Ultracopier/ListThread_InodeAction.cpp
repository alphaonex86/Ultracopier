/** \file ListThread_InodeAction.cpp
\brief To be included into ListThread.cpp, to optimize and prevent code duplication
\see ListThread.cpp
\author alpha_one_x86
\version 0.3
\date 2011 */

#ifdef LISTTHREAD_H

//do the inode action
ActionToDoInode& currentActionToDoInode=actionToDoListInode[int_for_internal_loop];
switch(currentActionToDoInode.type)
{
    case ActionType_RealMove:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("launch real move, source: %1, destination: %2").arg(currentActionToDoInode.source.absoluteFilePath()).arg(currentActionToDoInode.destination.absoluteFilePath()));
        mkPathQueue.addPath(currentActionToDoInode.source.absoluteFilePath(),currentActionToDoInode.destination.absoluteFilePath(),ActionType_RealMove);
        currentActionToDoInode.isRunning=true;
        numberOfInodeOperation++;
        if(numberOfInodeOperation>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
            return;
    break;
    case ActionType_MkPath:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("launch mkpath, source: %1, destination: %2").arg(currentActionToDoInode.source.absoluteFilePath()).arg(currentActionToDoInode.destination.absoluteFilePath()));
        mkPathQueue.addPath(currentActionToDoInode.source.absoluteFilePath(),currentActionToDoInode.destination.absoluteFilePath(),ActionType_MkPath);
        currentActionToDoInode.isRunning=true;
        numberOfInodeOperation++;
        if(numberOfInodeOperation>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
            return;
    break;
    case ActionType_MovePath:
        //then empty (no file), can try remove it
        if(currentActionToDoInode.size==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("launch rmpath: %1").arg(currentActionToDoInode.source.absoluteFilePath()));
            mkPathQueue.addPath(currentActionToDoInode.source.absoluteFilePath(),currentActionToDoInode.destination.absoluteFilePath(),ActionType_MovePath);
            currentActionToDoInode.isRunning=true;
            numberOfInodeOperation++;
            if(numberOfInodeOperation>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
                return;
        }
        else //have do the destination, put the remove to after
        {
            currentActionToDoInode.size=0;
            actionToDoListInode_afterTheTransfer << currentActionToDoInode;
            actionToDoListInode.removeAt(int_for_internal_loop);
            int_for_internal_loop--;
            actionToDoListInode_count--;
            if(numberOfInodeOperation>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
                return;
        }
    break;
    default:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QString("Wrong type at inode action"));
    return;
}

#endif
