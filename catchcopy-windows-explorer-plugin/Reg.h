#ifndef _REG_H_
#define _REG_H_
#pragma once

//#define CATCHCOPY_ROOT_MODE

#include <windows.h>
#include <winreg.h>
//
//   FUNCTION: RegisterInprocServer
//
//   PURPOSE: Register the in-process component in the registry.
//
//   PARAMETERS:
//   * pszModule - Path of the module that contains the component
//   * clsid - Class ID of the component
//   * pszFriendlyName - Friendly name
//   * pszThreadModel - Threading model
//
//   NOTE: The function creates the HKCR\CLSID\{<CLSID>} key in the registry.
//
//   HKCR
//   {
//      NoRemove CLSID
//      {
//          ForceRemove {<CLSID>} = s '<Friendly Name>'
//          {
//              InprocServer32 = s '%MODULE%'
//              {
//                  val ThreadingModel = s '<Thread Model>'
//              }
//          }
//      }
//   }
//
HRESULT RegisterInprocServer(PCWSTR pszModule, const CLSID& clsid,
    PCWSTR pszFriendlyName, PCWSTR pszThreadModel);


//
//   FUNCTION: UnregisterInprocServer
//
//   PURPOSE: Unegister the in-process component in the registry.
//
//   PARAMETERS:
//   * clsid - Class ID of the component
//
//   NOTE: The function deletes the HKCR\CLSID\{<CLSID>} key in the registry.
//
HRESULT UnregisterInprocServer(const CLSID& clsid);


//
//   FUNCTION: RegisterShellExtContextMenuHandler
//
//   PURPOSE: Register the context menu handler.
//
//   PARAMETERS:
//   * pszFileType - The file type that the context menu handler is
//     associated with. For example, '*' means all file types; '.txt' means
//     all .txt files. The parameter must not be NULL.
//   * clsid - Class ID of the component
//   * pszFriendlyName - Friendly name
//
//   NOTE: The function creates the following key in the registry.
//
//   HKCR
//   {
//      NoRemove <File Type>
//      {
//          NoRemove shellex
//          {
//              NoRemove ContextMenuHandlers
//              {
//                  {<CLSID>} = s '<Friendly Name>'
//              }
//          }
//      }
//   }
//
HRESULT RegisterShellExtContextMenuHandler(
    PCWSTR pszFileType, const CLSID& clsid, PCWSTR pszFriendlyName);


//
//   FUNCTION: UnregisterShellExtContextMenuHandler
//
//   PURPOSE: Unregister the context menu handler.
//
//   PARAMETERS:
//   * pszFileType - The file type that the context menu handler is
//     associated with. For example, '*' means all file types; '.txt' means
//     all .txt files. The parameter must not be NULL.
//   * clsid - Class ID of the component
//
//   NOTE: The function removes the {<CLSID>} key under
//   HKCR\<File Type>\shellex\ContextMenuHandlers in the registry.
//
HRESULT UnregisterShellExtContextMenuHandler(
    PCWSTR pszFileType, const CLSID& clsid);

LONG RecursiveDeleteKey(HKEY hKeyParent,  PCWSTR szKeyChild);
#endif //_REG_H_
