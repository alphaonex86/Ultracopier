/** \file ListThread_InodeAction.cpp
\brief To be included into ListThread.cpp, to optimize and prevent code duplication
\see ListThread.cpp
\author alpha_one_x86
\version 0.3
\date 2011 */

#include "ListThread.h"

//do the inode action
actionToDoInode& currentActionToDoInode=actionToDoListInode[int_for_internal_loop];
switch(currentActionToDoInode.type)
{
case ActionType_MkPath:
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("launch mkpath: %1").arg(currentActionToDoInode.source.absoluteFilePath()));
	mkPathQueue.addPath(currentActionToDoInode.source.absoluteFilePath());
	currentActionToDoInode.isRunning=true;
	numberOfInodeOperation++;
	if(numberOfInodeOperation>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
		return;
break;
case ActionType_RmPath:
/*	What is this code?
	if((int_for_loop+number_rm_path_moved)>=(loop_size-1))
	{
		if(numberOfTranferRuning)
			break;
		else
			currentActionToDoInode.size=0;
	}*/
	//then empty (no file), can try remove it
	if(currentActionToDoInode.size==0)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("launch rmpath: %1").arg(currentActionToDoInode.source.absoluteFilePath()));
		rmPathQueue.addPath(currentActionToDoInode.source.absoluteFilePath());
		currentActionToDoInode.isRunning=true;
		numberOfInodeOperation++;
		if(numberOfInodeOperation>=ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT)
			return;
	}
	else //have not finish all the transfer into it, do it later
	{
		actionToDoListInode.move(int_for_loop,loop_size-1);
		number_rm_path_moved++;
		if(!numberOfTranferRuning)
			currentActionToDoInode.size=0;
		continue;
	}
break;
default:
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,QString("Wrong type at inode action"));
	return;
}
