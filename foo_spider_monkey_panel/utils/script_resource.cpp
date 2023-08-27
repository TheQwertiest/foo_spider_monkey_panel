#include <stdafx.h>

#include "script_resource.h"

namespace smp::utils
{

qwr::u8string LoadScriptResource( int resourceId )
{
    puResource puRes = uLoadResource( core_api::get_my_instance(), uMAKEINTRESOURCE( resourceId ), "SCRIPT" );
    qwr::QwrException::ExpectTrue( puRes, "Failed to load script resource: {}", resourceId );

    return qwr::u8string{ static_cast<const char*>( puRes->GetPointer() ), puRes->GetSize() };
}

} // namespace smp::utils
