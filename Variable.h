/** \file Variable.h
\brief Define the environment variable
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef VARIABLE_H
#define VARIABLE_H

/// \brief Un-comment this next line to put ultracopier in debug mode
#define ULTRACOPIER_DEBUG
#define ULTRACOPIER_DEBUG_MAX_GUI_LINE 50000 ///< \brief Max number of ligne show on the GUI
#define ULTRACOPIER_DEBUG_MAX_ALL_SIZE 128 ///< \brief Max size (in MB) after the console/file output is dropped
#define ULTRACOPIER_DEBUG_MAX_IMPORTANT_SIZE 150 ///< \brief Max size (in MB) after the console/file important output is dropped
/// \brief the version
#define ULTRACOPIER_VERSION		"1.2.1.1"
/// \brief the windows version
#define ULTRACOPIER_WINDOWS_VERSION	1,2,1,1
// define if the version is portable or not
//#define ULTRACOPIER_VERSION_PORTABLE
//#define ULTRACOPIER_VERSION_PORTABLEAPPS
// define if the version is ultimate, need change only the name in the code
#define ULTRACOPIER_VERSION_ULTIMATE
//#define ULTRACOPIER_PLUGIN_ALL_IN_ONE
//#define ULTRACOPIER_CGMINER
//#define ULTRACOPIER_MODE_SUPERCOPIER
/// \brief define time to update the speed detection update ont the interface (in ms)
#define ULTRACOPIER_TIME_INTERFACE_UPDATE 500

/** \brief How many value store to calculate the average value
 * 5*ULTRACOPIER_TIME_INTERFACE_UPDATE = 5*500 to get 2.5s
 * */
#define ULTRACOPIER_MAXREMAININGTIMECOL 10
#define ULTRACOPIER_MAXVALUESPEEDSTORED 5
#define ULTRACOPIER_MINVALUESPEED 3
#define ULTRACOPIER_MAXVALUESPEEDSTOREDTOREMAININGTIME 120
#define ULTRACOPIER_MINVALUESPEEDTOREMAININGTIME 10
#define ULTRACOPIER_REMAININGTIME_BIGFILEMEGABYTEBASE10   100

/// \brief the socket name, to have unique instance of ultracopier, and pass arguments between the instance
#define ULTRACOPIER_SOCKETNAME "ultracopier"

/// \brief to have internet support, to communicate with the web site for update and plugins
#define ULTRACOPIER_INTERNET_SUPPORT

/// \brief to disable plugin support, import and remove
#define ULTRACOPIER_PLUGIN_IMPORT_SUPPORT

#define ULTRACOPIER_UPDATER_URL "http://ultracopier.first-world.info/updater.txt"

#endif // VARIABLE_H
