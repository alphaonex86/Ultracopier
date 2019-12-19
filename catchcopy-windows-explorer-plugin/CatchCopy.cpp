// Implementation of DLL Exports.
#include "resource.h"
#include "DDShellExt.h"
#include "ClassFactory.h"
#include "Reg.h"

#ifndef _M_X64
const CLSID CLSID_DDShellExt = {0x68D44A27,0xFFB6,0x4B89,{0xA3,0xE5,0x7B,0x0E,0x50,0xA7,0xAB,0x33}};
#else
const CLSID CLSID_DDShellExt = {0x68ff37c4,0x51bc,0x4c2a,{0xa9,0x92,0x7e,0x39,0xbc,0xe,0x70,0x6f}};
#endif

HINSTANCE   g_hInst     = NULL;
long        g_cDllRef   = 0;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    (void)lpReserved;
	switch(dwReason){
		case DLL_PROCESS_ATTACH:
			//MessageBox ( NULL,L"DLL_PROCESS_ATTACH", L"DLL_PROCESS_ATTACH", MB_OK);
			// Hold the instance of this DLL module, we will use it to get the
			// path of the DLL to register the component.
			g_hInst = hInstance;
			DisableThreadLibraryCalls(hInstance);
			break;
		case DLL_PROCESS_DETACH:
			//MessageBox ( NULL,L"DLL_PROCESS_DETACH", L"DLL_PROCESS_DETACH", MB_OK);
			break;
	}

	return TRUE; //_AtlModule.DllMain(hInstance, dwReason, lpReserved,ObjectMap,NULL);
}


// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
	return g_cDllRef > 0 ? S_FALSE : S_OK;
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    if (IsEqualCLSID(CLSID_DDShellExt, rclsid))
    {
         hr = E_OUTOFMEMORY;
        ClassFactory *pClassFactory = new ClassFactory();
        if (pClassFactory)
        {
            hr = pClassFactory->QueryInterface(riid, ppv);
            pClassFactory->Release();
        }
    }
    return hr;
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
//  registers object, typelib and all interfaces in typelib

	HRESULT hr;

    wchar_t szModule[MAX_PATH];
    if (GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }
     // Register the component.
    hr = RegisterInprocServer(szModule, CLSID_DDShellExt, L"CatchCopy Class", L"Apartment");
    if (SUCCEEDED(hr))
    {
        // Register the context menu handler. The context menu handler is
        // associated with the * file class.
        hr = RegisterShellExtContextMenuHandler(L"textfile", CLSID_DDShellExt, L"CatchCopy");
		hr = RegisterShellExtContextMenuHandler(L"Drive", CLSID_DDShellExt, L"CatchCopy");
		hr = RegisterShellExtContextMenuHandler(L"Directory", CLSID_DDShellExt, L"CatchCopy");
		hr = RegisterShellExtContextMenuHandler(L"Folder", CLSID_DDShellExt, L"CatchCopy");
    }
    return hr;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	m_ac.disconnectFromServer();
    HRESULT hr = S_OK;

    wchar_t szModule[MAX_PATH];
    if (GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule)) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // Unregister the component.
    hr = UnregisterInprocServer(CLSID_DDShellExt);
    if (SUCCEEDED(hr))
    {
        // Unregister the context menu handler.
        hr = UnregisterShellExtContextMenuHandler(L"textfile", CLSID_DDShellExt);
		hr = UnregisterShellExtContextMenuHandler(L"Drive", CLSID_DDShellExt);
		hr = UnregisterShellExtContextMenuHandler(L"Directory", CLSID_DDShellExt);
		hr = UnregisterShellExtContextMenuHandler(L"Folder", CLSID_DDShellExt);
		return hr;
    }
	return S_OK;
}
