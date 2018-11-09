#pragma once

#include "script_interface_impl.h"

#include <OleIdl.h>

namespace smp::com
{

class HostExternal : public IDispatchImpl3<IHostExternal>
{
protected:
    HostExternal( _variant_t data );
    virtual ~HostExternal() = default;

    void FinalRelease() override;

public:
    STDMETHODIMP get_dialogArguments( VARIANT* pData );

private:
    _variant_t data_;
};

} // namespace smp::com
