#pragma once

#include <utils/string_helpers.h>

#define IF_GDI_FAILED_RETURN_WITH_REPORT( cx, gdiRet, retValue, funcName )                                                 \
    do                                                                                                                     \
    {                                                                                                                      \
        if ( gdiRet > 0 )                                                                                                  \
        {                                                                                                                  \
            JS_ReportErrorUTF8( cx, "GDI error: '%s' failed: %s(0x%X)", #funcName, GdiErrorCodeToText( gdiRet ), gdiRet ); \
            return retValue;                                                                                               \
        }                                                                                                                  \
    } while ( false )

#define IF_GDI_FAILED_THROW_SMP( gdiRet, funcName )                                                                                                           \
    do                                                                                                                                                        \
    {                                                                                                                                                         \
        if ( gdiRet > 0 )                                                                                                                                     \
        {                                                                                                                                                     \
            throw smp::SmpException(                                                                                                                          \
                smp::string::Formatter() << "GDI error: " << funcName << " failed: " << GdiErrorCodeToText( gdiRet ) << "(0x" << std::hex << gdiRet << ")" ); \
        }                                                                                                                                                     \
    } while ( false )

namespace mozjs
{

template <typename T>
void ValidateGdiPlusObject( const std::unique_ptr<T>& obj ) noexcept( false )
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
            smp::string::Formatter() << "Failed to create GdiPlus object: " << GdiErrorCodeToText( obj->GetLastStatus() ) << "(0x" << std::hex << obj->GetLastStatus() << ")" );
    }
}

const char* GdiErrorCodeToText( Gdiplus::Status errorCode );

} // namespace mozjs
