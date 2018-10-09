#pragma once

#include "script_interface_impl.h"

#include <OleIdl.h>

namespace smp::com
{

class HostExternal : public IDispatchImpl3<IHostExternal>
{
protected:
    HostExternal( _variant_t data )
        : data_( data )
    {
    }

    virtual ~HostExternal() = default;

    void FinalRelease() override
    {
    }

public:
    STDMETHODIMP get_dialogArguments( VARIANT* pData )
    {
        if ( pData )
        {
            *pData = data_;
        }

        return S_OK;
    }

private:
    _variant_t data_;
};

} // namespace smp::com