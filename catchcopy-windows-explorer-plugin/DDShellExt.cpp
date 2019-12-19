// DDShellExt.cpp : Implementation of CDDShellExt

#include "tchar.h"
#include "DDShellExt.h"
#include "ClientCatchcopy.h"

#include <Shlwapi.h>

extern HINSTANCE g_hInst;
extern long g_cDllRef;

CDDShellExt::CDDShellExt(): m_cRef(1)
{
	InterlockedIncrement(&g_cDllRef);
}

CDDShellExt::~CDDShellExt()
{
    InterlockedDecrement(&g_cDllRef);
}

// Query to the interface the component supported.
IFACEMETHODIMP CDDShellExt::QueryInterface(REFIID riid, void **ppv)
{
  if(riid == IID_IUnknown || riid == IID_IContextMenu)
    {
        *ppv = static_cast<IContextMenu*>(this);
    }
    else if (riid == IID_IShellExtInit)
    {
        *ppv = static_cast<IShellExtInit*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

// Increase the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) CDDShellExt::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// Decrease the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) CDDShellExt::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return cRef;
}

STDMETHODIMP CDDShellExt::Initialize(LPCITEMIDLIST pidlFolder,LPDATAOBJECT pDO,HKEY hProgID)
{
	(void)hProgID;
	if(!connected)
	{
		bool b = m_ac.connectToServer();

		if (b==true)
		{
			connected=true;
		}
		else
			return E_FAIL;
	}

	FORMATETC fmt={CF_HDROP,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	STGMEDIUM stg={TYMED_HGLOBAL};
	HDROP hDrop;

	fDestDir[0]=0;
	if (!SHGetPathFromIDList(pidlFolder,fDestDir))
	{
		#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		MessageBox(NULL,L"Initialize",L"E_FAIL 1",MB_OK);
		#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		return E_FAIL;
	}

	// Detect if it's explorer that started the operation by enumerating available
	// clipboard formats and searching for one that only explorer uses
	IEnumFORMATETC *en;
	FORMATETC fmt2;
	WCHAR fmtName[65535]=L"\0";
	fFromExplorer=false;
	pDO->EnumFormatEtc(DATADIR_GET,&en);
	while(en->Next(1,&fmt2,NULL)==S_OK){
		GetClipboardFormatName(fmt2.cfFormat,fmtName,sizeof(fmtName));
		if (!wcscmp(fmtName,CFSTR_SHELLIDLIST)) fFromExplorer=true;
	}
	en->Release();

	// Look for CF_HDROP data in the data object. If there
	// is no such data, return an error back to Explorer.
	if (FAILED(pDO->GetData(&fmt,&stg)))
	{
		#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		MessageBox(NULL,L"Initialize",L"E_INVALIDARG 2",MB_OK);
		#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		return E_INVALIDARG;
	}

	// Get a pointer to the actual data.
	hDrop=(HDROP)GlobalLock(stg.hGlobal);

	// Make sure it worked.
	if (hDrop==NULL)
	{
		#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		MessageBox(NULL,L"Initialize",L"E_INVALIDARG 1",MB_OK);
		#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		return E_INVALIDARG;
	}

	UINT numFiles,i;
	WCHAR fn[MAX_PATH]=L"";

	numFiles=DragQueryFile(hDrop,0xFFFFFFFF,NULL,0);

	if (numFiles)
	{
		for(i=0;i<numFiles;++i)
		{
			if(DragQueryFile(hDrop,i,fn,MAX_PATH))
				sources.push_back(fn);
		}
	}

	GlobalUnlock(stg.hGlobal);
	ReleaseStgMedium(&stg);

	return S_OK;
}

STDMETHODIMP CDDShellExt::QueryContextMenu(HMENU hmenu,UINT uMenuIndex,UINT uidFirstCmd,UINT uidLastCmd,UINT uFlags)
{
	(void)uidLastCmd;
	if(!m_ac.isConnected())
	{
		if(!m_ac.connectToServer())
			return E_FAIL;
	}
	if (uFlags&CMF_DEFAULTONLY)
		return MAKE_HRESULT(SEVERITY_SUCCESS,FACILITY_NULL,0);

	int x=uidFirstCmd;

    InsertMenu(hmenu,uMenuIndex++,MF_STRING|MF_BYPOSITION,x++,L"Copy with advanced software");
    InsertMenu(hmenu,uMenuIndex++,MF_STRING|MF_BYPOSITION,x++,L"Move with advanced software");

	int defItem=GetMenuDefaultItem(hmenu,false,0);
	if (defItem==1) // 1: Copy
	{
		if (fFromExplorer)
			SetMenuDefaultItem(hmenu,uidFirstCmd,false);
	}
	else if (defItem==2) //2: Move
	{
		SetMenuDefaultItem(hmenu,uidFirstCmd+1,false);
	}
	return MAKE_HRESULT(SEVERITY_SUCCESS,FACILITY_NULL,2);
}


STDMETHODIMP CDDShellExt::InvokeCommand ( LPCMINVOKECOMMANDINFO pInfo )
{
	if(HIWORD(pInfo->lpVerb))
		return E_INVALIDARG;
	switch(LOWORD(pInfo->lpVerb))
	{
		case 0:// copy
			if(!m_ac.addCopyWithDestination(sources,fDestDir))
				return E_FAIL;
		break;
		case 1:// move
			if(!m_ac.addMoveWithDestination(sources,fDestDir))
				return E_FAIL;
		break;
		default :
			return S_OK;
	}
	return S_OK;
}
