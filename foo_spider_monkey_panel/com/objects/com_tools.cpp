#include <stdafx.h>

#include "com_tools.h"

#include <span>

namespace smp::com::internal
{

TypeInfoCacheHolder::TypeInfoCacheHolder()
{
}

bool TypeInfoCacheHolder::Empty()
{
    return !typeInfo_;
}

void TypeInfoCacheHolder::InitFromTypelib( ITypeLib* p_typeLib, const GUID& guid )
{
    p_typeLib->GetTypeInfoOfGuid( guid, &typeInfo_ );
}

HRESULT TypeInfoCacheHolder::GetTypeInfo( UINT iTInfo, LCID /*lcid*/, ITypeInfo** ppTInfo )
{
    if ( Empty() )
    {
        return E_UNEXPECTED;
    }
    if ( !ppTInfo )
    {
        return E_POINTER;
    }
    if ( iTInfo != 0 )
    {
        return DISP_E_BADINDEX;
    }
    typeInfo_->AddRef();
    *ppTInfo = typeInfo_.GetInterfacePtr();
    return S_OK;
}

HRESULT TypeInfoCacheHolder::GetIDsOfNames( LPOLESTR* rgszNames, UINT cNames, MEMBERID* pMemId )
{
    assert( typeInfo_ != NULL );

    std::span<LPOLESTR> names( rgszNames, cNames );
    std::span<MEMBERID> memIds( pMemId, cNames );

    for ( auto&& [name, memId]: ranges::views::zip( names, memIds ) )
    {
        const auto hash = LHashValOfName( LANG_NEUTRAL, name );
        if ( const auto it = cache_.find( hash );
             cache_.cend() != it )
        {
            memId = it->second;
        }
        else
        {
            HRESULT hr = typeInfo_->GetIDsOfNames( &name, 1, &memId );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            cache_.emplace( hash, memId );
        }
    }

    return S_OK;
}

HRESULT TypeInfoCacheHolder::Invoke( PVOID pvInstance, MEMBERID memid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    assert( typeInfo_ != NULL );
    HRESULT hr = typeInfo_->Invoke( pvInstance, memid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
    assert( hr != RPC_E_WRONG_THREAD );
    return hr;
}

} // namespace smp::com::internal
