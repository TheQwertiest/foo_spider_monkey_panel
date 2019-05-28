#include <stdafx.h>
#include "host_external.h"

namespace smp::com
{

HostExternal::HostExternal( _variant_t data )
    : data_( data )
{
}

STDMETHODIMP HostExternal::get_dialogArguments( VARIANT* pData )
{
    if ( pData )
    {
        VariantCopy( pData, &static_cast<VARIANT>(data_) );
    }

    return S_OK;
}

} // namespace smp::com
