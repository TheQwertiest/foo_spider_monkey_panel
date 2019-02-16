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
        *pData = data_;
    }

    return S_OK;
}

} // namespace smp::com
