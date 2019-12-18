#pragma once

#include <com_objects/com_interface.h>
#include <com_objects/com_tools.h>

#include <OleIdl.h>

_COM_SMARTPTR_TYPEDEF( IHostExternal, __uuidof( IHostExternal ) );

namespace smp::com
{

class HostExternal : public IDispatchImpl3<IHostExternal>
{
protected:
    HostExternal( _variant_t data );
    ~HostExternal() override = default;

public:
    STDMETHODIMP get_dialogArguments( VARIANT* pData ) override;

private:
    _variant_t data_;
};

} // namespace smp::com
