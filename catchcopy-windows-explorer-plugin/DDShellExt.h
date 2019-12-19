// DDShellExt.h : Declaration of the CDDShellExt

#pragma once
#include "resource.h"       // main symbols
#include "shlobj.h"
#include "ClientCatchcopy.h"
#include "Variable.h"
#include "Deque.h"

static bool connected=false;
static ClientCatchcopy m_ac;

// CDDShellExt

extern const CLSID CLSID_DDShellExt;

class CDDShellExt :
	public IShellExtInit,
    public IContextMenu
{
private:
	static int fBaselistHandle;
	bool fFromExplorer;
	WCHAR fDestDir[MAX_PATH];
	//static ClientCatchcopy m_ac;
	CDeque sources;	
	//static bool connected;

	// Reference count of component.
    long m_cRef;
public:
	// IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

	// IShellExtInit
	STDMETHODIMP Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY);

	CDDShellExt(void);
	~CDDShellExt(void);
	// IContextMenu
    STDMETHODIMP GetCommandString(UINT_PTR idCmd,UINT uFlags,UINT* pwReserved,LPSTR pszName,UINT cchMax){(void)idCmd;(void)uFlags;(void)pwReserved;(void)pszName;(void)cchMax;(void)connected;return E_NOTIMPL;};
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO);
    STDMETHODIMP QueryContextMenu(HMENU,UINT,UINT,UINT,UINT);
};
