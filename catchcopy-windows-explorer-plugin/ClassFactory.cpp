
#include "ClassFactory.h"
#include "DDShellExt.h"
#include <new>
#include <Shlwapi.h>

extern long g_cDllRef;

ClassFactory::ClassFactory() : m_cRef(1)
{
    InterlockedIncrement(&g_cDllRef);
}

ClassFactory::~ClassFactory()
{
    InterlockedDecrement(&g_cDllRef);
}

// IUnknown
HRESULT __stdcall ClassFactory::QueryInterface(const IID& iid, void **ppv)
{
   if(iid == IID_IUnknown || iid == IID_IClassFactory)
    {
        *ppv = static_cast<IClassFactory*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}
ULONG __stdcall ClassFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG __stdcall ClassFactory::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
    }
    return cRef;
}

//CreateInstance
HRESULT __stdcall ClassFactory::CreateInstance(IUnknown *pIUnknownOuter, const IID& iid, void **ppv)
{
    HRESULT hr = CLASS_E_NOAGGREGATION;

    // pIUnknownOuter is used for aggregation. We do not support it in the sample.
    if (pIUnknownOuter == NULL)
    {
        hr = E_OUTOFMEMORY;

        //// Create the COM component.
        CDDShellExt *pExt = new (std::nothrow) CDDShellExt();
        if (pExt)
        {
            // Query the specified interface.
            hr = pExt->QueryInterface(iid, ppv);
            pExt->Release();
        }
    }

    return hr;
}

HRESULT __stdcall ClassFactory::LockServer(BOOL bLock)
{
    if(bLock == TRUE)
    {
        InterlockedIncrement(&g_cDllRef);
    }
    else
    {
        InterlockedDecrement(&g_cDllRef);
    }
    return S_OK;
}

