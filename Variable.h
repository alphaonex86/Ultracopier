/** \file Variable.h
\brief Define the environment variable
\author alpha_one_x86
\version 0.3
\date 2010 */

#ifndef VARIABLE_H
#define VARIABLE_H

//Un-comment this next line to put ultracopier in debug mode
#define ULTRACOPIER_DEBUG
/// \brief the version
#define ULTRACOPIER_VERSION		"0.3.0.2"
/// \brief the windows version
#define ULTRACOPIER_WINDOWS_VERSION	0,3,0,2
/// \brief define if the version is portable or not
//#define ULTRACOPIER_VERSION_PORTABLE
//#define ULTRACOPIER_VERSION_PORTABLEAPPS
/// \brief define time to update the interface (in ms)
#define ULTRACOPIER_TIME_INTERFACE_UPDATE 500

//5*ULTRACOPIER_TIME_INTERFACE_UPDATE = 5*500 to get 2.5s
#define ULTRACOPIER_MAXVALUESPEEDSTORED 5

//the socket name
#define ULTRACOPIER_SOCKETNAME "ultracopier-0.3"

//to have internet support
#define ULTRACOPIER_INTERNET_SUPPORT

//to disable plugin support to do all in one version with static Qt for the protable version
#define ULTRACOPIER_PLUGIN_SUPPORT

#endif // VARIABLE_H
