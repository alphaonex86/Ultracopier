/** \file win-shell-iids.cpp
\brief Emit the Windows shell/COM interface IIDs referenced by pluginLoader.cpp.

The Explorer PluginLoader references shell interface IIDs (IID_IShellBrowser,
IID_IPersistFolder2, IID_IFolderView, IID_IServiceProvider, SID_STopLevelBrowser,
IID_IWebBrowserApp, ...). In a separate-DLL plugin build these are pulled from the
plugin's own link; in the all-in-one build (ULTRACOPIER_PLUGIN_ALL_IN_ONE) the whole
app links as one exe and no library provides them, so they must be defined in exactly
one translation unit. Including the COM headers AFTER defining INITGUID makes the
preprocessor emit the GUID definitions here.

Compiled only on Windows; empty elsewhere. */

#if defined(_WIN32) || defined(Q_OS_WIN32)

#define INITGUID
#include <initguid.h>
#include <objbase.h>
#include <servprov.h>   // IID_IServiceProvider
#include <shlobj.h>     // IID_IShellBrowser, IID_IPersistFolder2, IID_IFolderView
#include <shlguid.h>    // SID_STopLevelBrowser and other shell GUIDs
#include <exdisp.h>     // IID_IWebBrowserApp

#endif
