#pragma once

#include <js_utils/gdi_helpers.h>

namespace mozjs::error
{

template <typename T>
void CheckGdiPlusObject( const std::unique_ptr<T>& obj ) noexcept( false )
{
    if ( gdi::IsGdiPlusObjectValid( obj ) )
    {
        return;
    }

    if ( !obj )
    {
        throw smp::SmpException( "Failed to create GdiPlus object" );
    }
    else
    {
        throw smp::SmpException(
            smp::string::Formatter()
            << "Failed to create GdiPlus object (0x" << std::hex << obj->GetLastStatus() << ": " << GdiErrorCodeToText( obj->GetLastStatus() ) );
    }
}

const char* GdiErrorCodeToText( Gdiplus::Status errorCode );
void CheckGdi( Gdiplus::Status gdiStatus, std::string_view functionName );

} // namespace mozjs::error
