/** \file Variable.h
\brief Define the environment variable
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef VARIABLE_H
#define VARIABLE_H

/// \brief Un-comment this next line to put ultracopier in debug mode
#define ULTRACOPIER_DEBUG
#define ULTRACOPIER_DEBUG_MAX_GUI_LINE 5000 ///< \brief Max number of ligne show on the GUI
#define ULTRACOPIER_DEBUG_MAX_ALL_SIZE 32 ///< \brief Max size (in MB) after the console/file output is dropped
#define ULTRACOPIER_DEBUG_MAX_IMPORTANT_SIZE 64 ///< \brief Max size (in MB) after the console/file important output is dropped
/// \brief the version
#define ULTRACOPIER_VERSION		"0.4.0.5"
/// \brief the windows version
#define ULTRACOPIER_WINDOWS_VERSION	0,4,0,5
// define if the version is portable or not
//#define ULTRACOPIER_VERSION_PORTABLE
//#define ULTRACOPIER_VERSION_PORTABLEAPPS
// define if the version is ultimate, need change only the name in the code
//#define ULTRACOPIER_VERSION_ULTIMATE
/// \brief define time to update the speed detection update ont the interface (in ms)
#define ULTRACOPIER_TIME_INTERFACE_UPDATE 500

/** \brief How many value store to calculate the average value
 * 5*ULTRACOPIER_TIME_INTERFACE_UPDATE = 5*500 to get 2.5s
 * */
#define ULTRACOPIER_MAXVALUESPEEDSTORED 5

/// \brief the socket name, to have unique instance of ultracopier, and pass arguments between the instance
#define ULTRACOPIER_SOCKETNAME "ultracopier-0.3"

/// \brief to have internet support, to communicate with the web site for update and plugins
#define ULTRACOPIER_INTERNET_SUPPORT

/// \brief to disable plugin support to do all in one version with static Qt for the protable version
#define ULTRACOPIER_PLUGIN_SUPPORT

#endif // VARIABLE_H
