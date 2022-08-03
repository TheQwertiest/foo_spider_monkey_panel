#include <stdafx.h>

#include "default_script.h"

#include <resources/resource.h>

namespace smp::config
{

qwr::u8string GetDefaultScript()
{
    puResource puRes = uLoadResource( core_api::get_my_instance(), uMAKEINTRESOURCE( IDR_DEFAULT_SCRIPT ), "SCRIPT" );
    if ( puRes )
    {
        return qwr::u8string{ static_cast<const char*>( puRes->GetPointer() ), puRes->GetSize() };
    }
    else
    {
        return qwr::u8string{};
    }
}

} // namespace smp::config
