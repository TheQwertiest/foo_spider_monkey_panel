#include <stdafx.h>
#include "com_tools.h"

#include <nonstd/span.hpp>

namespace internal
{

type_info_cache_holder::type_info_cache_holder()
    : m_type_info( nullptr )
{
}

bool type_info_cache_holder::empty()
{
    return !m_type_info;
}

void type_info_cache_holder::init_from_typelib( ITypeLib* p_typeLib, const GUID& guid )
{
    p_typeLib->GetTypeInfoOfGuid( guid, &m_type_info );
}

HRESULT type_info_cache_holder::GetTypeInfo( UINT iTInfo, [[maybe_unused]] LCID lcid, ITypeInfo** ppTInfo )
{
    if ( empty() )
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
    m_type_info->AddRef();
    *ppTInfo = m_type_info.GetInterfacePtr();
    return S_OK;
}

HRESULT type_info_cache_holder::GetIDsOfNames( LPOLESTR* rgszNames, UINT cNames, MEMBERID* pMemId )
{
    assert( m_type_info != NULL );

    nonstd::span<LPOLESTR> names( rgszNames, cNames );
    nonstd::span<MEMBERID> memIds( pMemId, cNames );

    for ( auto&& [name, memId] : ranges::view::zip( names, memIds ) )
    {
        const auto hash = LHashValOfName( LANG_NEUTRAL, name );
        if ( const auto it = m_cache.find( hash );
             m_cache.cend() != it )
        {
            memId = it->second;
        }
        else
        {
            HRESULT hr = m_type_info->GetIDsOfNames( &name, 1, &memId );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            m_cache.emplace( hash, memId );
        }
    }

    return S_OK;
}

HRESULT type_info_cache_holder::Invoke( PVOID pvInstance, MEMBERID memid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    assert( m_type_info != NULL );
    HRESULT hr = m_type_info->Invoke( pvInstance, memid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
    assert( hr != RPC_E_WRONG_THREAD );
    return hr;
}

} // namespace internal
