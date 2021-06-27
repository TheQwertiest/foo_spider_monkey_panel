#include <stdafx.h>

#include "resource_helpers.h"

namespace smp
{

std::optional<qwr::u8string> LoadStringResource( int resourceId, const char* resourceType )
{
    SMP_CLANG_WARNING_PUSH
    // suppress warning from uMAKEINTRESOURCE
    SMP_CLANG_SUPPRESS_WARNING( "-Wint-to-pointer-cast" )
    puResource puRes = uLoadResource( core_api::get_my_instance(), uMAKEINTRESOURCE( resourceId ), resourceType );
    SMP_CLANG_WARNING_POP
    if ( !puRes )
    {
        return std::nullopt;
    }

    return qwr::u8string{ static_cast<const char*>( puRes->GetPointer() ), puRes->GetSize() };
}

} // namespace smp
