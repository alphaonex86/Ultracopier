#pragma once

#include <unknwn.h>     // For IClassFactory
#include <windows.h>


class ClassFactory : public IClassFactory
{
public:
    //interface iunknown
    virtual HRESULT __stdcall QueryInterface(const IID& iid, void **ppv);
    virtual ULONG   __stdcall AddRef();
    virtual ULONG   __stdcall Release();
    //interface iclassfactory
    virtual HRESULT __stdcall CreateInstance(IUnknown *pIUnknownOuter, const IID& iid, void **ppv);
    virtual HRESULT __stdcall LockServer(BOOL bLock);

    ClassFactory();
    ~ClassFactory();
private:
    long m_cRef;
};
