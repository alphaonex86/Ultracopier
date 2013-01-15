/** \file Variable.h
\brief Define the environment variable
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef VARIABLE_H
#define VARIABLE_H

//Un-comment this next line to put ultracopier plugin in debug mode
#define ULTRACOPIER_PLUGIN_DEBUG
#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW
#define ULTRACOPIER_PLUGIN_DEBUG_WINDOW_TIMER		150

#define ULTRACOPIER_PLUGIN_MAXBUFFERBLOCK		16
#define ULTRACOPIER_PLUGIN_MINTIMERINTERVAL		50
#define ULTRACOPIER_PLUGIN_MAXTIMERINTERVAL		100
#define ULTRACOPIER_PLUGIN_NUMSEMSPEEDMANAGEMENT	2
#define ULTRACOPIER_PLUGIN_MAXPARALLELINODEOPT		16
#define ULTRACOPIER_PLUGIN_MAXPARALLELTRANFER		1
#define ULTRACOPIER_PLUGIN_MINIMALYEAR			1990
#define ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE   4 //in KB

//if set, check the inode type at scanFileOrFolder, deprecated into the new algorithm and not used
#define ULTRACOPIER_PLUGIN_CHECKLISTTYPE

/** \brief Need be greater than 2, but greater than 20 to be efficient */
#define ULTRACOPIER_PLUGIN_TIME_UPDATE_TRASNFER_LIST 40
#define ULTRACOPIER_PLUGIN_TIME_UPDATE_PROGRESSION 200

//#define ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY

#endif // VARIABLE_H



