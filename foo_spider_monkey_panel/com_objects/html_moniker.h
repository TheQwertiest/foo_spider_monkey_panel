#pragma once

#include "script_interface_impl.h"

#include <OleIdl.h>
#include <ShlObj.h>

namespace smp::com
{

class IHtmlMoniker
    : public IMoniker
{
    BEGIN_COM_QI_IMPL()
    COM_QI_ENTRY_MULTI( IUnknown, IMoniker )
    COM_QI_ENTRY_MULTI( IPersist, IMoniker )
    COM_QI_ENTRY_MULTI( IPersistStream, IMoniker )
    COM_QI_ENTRY( IMoniker )
    END_COM_QI_IMPL()

public:
    void FinalRelease(){};

    HRESULT SetHTML( const std::wstring& htmlCode )
    {
        if ( pHtmlStream_ )
        {
            pHtmlStream_->Release();
        }

        {
            IStreamPtr pStream;
            if ( S_OK != ::CreateStreamOnHGlobal( NULL, TRUE, &pStream ) )
            {
                return E_FAIL;
            }

            ULONG written;
            ULONG dataSize = htmlCode.size() * sizeof( wchar_t );
            if ( S_OK != pStream->Write( htmlCode.data(), dataSize, &written ) 
                 || written != dataSize )
            {
                return E_FAIL;
            }

            LARGE_INTEGER zero = { 0 };
            pStream->Seek( zero, STREAM_SEEK_SET, nullptr );
            pHtmlStream_ = pStream;
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE BindToStorage(
        IBindCtx* pbc,
        IMoniker* pmkToLeft,
        REFIID riid,
        void** ppvObj ) override
    {
        LARGE_INTEGER seek = { 0 };
        pHtmlStream_->Seek( seek, STREAM_SEEK_SET, nullptr );
        return pHtmlStream_->QueryInterface( riid, ppvObj );
    }

    // IMoniker

    virtual HRESULT STDMETHODCALLTYPE BindToObject(

        _In_ IBindCtx* pbc,
        _In_opt_ IMoniker* pmkToLeft,
        _In_ REFIID riidResult,
        _Outptr_ void** ppvResult )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Reduce(
        __RPC__in_opt IBindCtx* pbc,
        DWORD dwReduceHowFar,
        __RPC__deref_opt_inout_opt IMoniker** ppmkToLeft,
        __RPC__deref_out_opt IMoniker** ppmkReduced )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE ComposeWith(
        __RPC__in_opt IMoniker* pmkRight,
        BOOL fOnlyIfNotGeneric,
        __RPC__deref_out_opt IMoniker** ppmkComposite )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Enum(
        BOOL fForward,
        __RPC__deref_out_opt IEnumMoniker** ppenumMoniker )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE IsEqual(
        __RPC__in_opt IMoniker* pmkOtherMoniker )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Hash(
        __RPC__out DWORD* pdwHash )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE IsRunning(
        __RPC__in_opt IBindCtx* pbc,
        __RPC__in_opt IMoniker* pmkToLeft,
        __RPC__in_opt IMoniker* pmkNewlyRunning )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetTimeOfLastChange(
        __RPC__in_opt IBindCtx* pbc,
        __RPC__in_opt IMoniker* pmkToLeft,
        __RPC__out FILETIME* pFileTime )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Inverse(
        __RPC__deref_out_opt IMoniker** ppmk )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE CommonPrefixWith(
        __RPC__in_opt IMoniker* pmkOther,
        __RPC__deref_out_opt IMoniker** ppmkPrefix )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE RelativePathTo(
        __RPC__in_opt IMoniker* pmkOther,
        __RPC__deref_out_opt IMoniker** ppmkRelPath )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetDisplayName(
        __RPC__in_opt IBindCtx* pbc,
        __RPC__in_opt IMoniker* pmkToLeft,
        __RPC__deref_out_opt LPOLESTR* ppszDisplayName )
    {
        if ( !ppszDisplayName )
        {
            return E_INVALIDARG;
        }

        *ppszDisplayName = nullptr;

        constexpr wchar_t displayName[] = L"about:blank";

        LPOLESTR ret = (LPOLESTR)CoTaskMemAlloc( sizeof( displayName ) );
        if ( !ret )
        {
            return E_OUTOFMEMORY;
            
        }

        memcpy( ret, displayName, sizeof( displayName ) );
        *ppszDisplayName = ret;

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ParseDisplayName(
        __RPC__in_opt IBindCtx* pbc,
        __RPC__in_opt IMoniker* pmkToLeft,
        __RPC__in LPOLESTR pszDisplayName,
        __RPC__out ULONG* pchEaten,
        __RPC__deref_out_opt IMoniker** ppmkOut )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE IsSystemMoniker(
        __RPC__out DWORD* pdwMksys )
    {
        if ( !pdwMksys )
        {
            return E_POINTER;
        }
        *pdwMksys = MKSYS_NONE;
        return S_OK;
    }

    // IPersistStream

    virtual HRESULT STDMETHODCALLTYPE IsDirty( void )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Load(
        __RPC__in_opt IStream* pStm )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Save(
        __RPC__in_opt IStream* pStm,
        BOOL fClearDirty )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(
        __RPC__out ULARGE_INTEGER* pcbSize )
    {
        return E_NOTIMPL;
    }

    // IPersist

    virtual HRESULT STDMETHODCALLTYPE GetClassID(
        /* [out] */ __RPC__out CLSID* pClassID )
    {
        return E_NOTIMPL;
    }

private:
    IStreamPtr pHtmlStream_;
};

} // namespace smp::com
