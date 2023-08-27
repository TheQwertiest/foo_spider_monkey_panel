#include <stdafx.h>

#include "error_handler.h"

namespace smp
{

const char* GdiPlusStatusToText( Gdiplus::Status errorCode )
{
    switch ( errorCode )
    {
    case Gdiplus::Ok:
        return "No error";
    case Gdiplus::GenericError:
        return "GenericError";
    case Gdiplus::InvalidParameter:
        return "InvalidParameter";
    case Gdiplus::OutOfMemory:
        return "OutOfMemory";
    case Gdiplus::ObjectBusy:
        return "ObjectBusy";
    case Gdiplus::InsufficientBuffer:
        return "InsufficientBuffer";
    case Gdiplus::NotImplemented:
        return "NotImplemented";
    case Gdiplus::Win32Error:
        return "Win32Error";
    case Gdiplus::WrongState:
        return "WrongState";
    case Gdiplus::Aborted:
        return "Aborted";
    case Gdiplus::FileNotFound:
        return "FileNotFound";
    case Gdiplus::ValueOverflow:
        return "ValueOverflow";
    case Gdiplus::AccessDenied:
        return "AccessDenied";
    case Gdiplus::UnknownImageFormat:
        return "UnknownImageFormat";
    case Gdiplus::FontFamilyNotFound:
        return "FontFamilyNotFound";
    case Gdiplus::FontStyleNotFound:
        return "FontStyleNotFound";
    case Gdiplus::NotTrueTypeFont:
        return "NotTrueTypeFont";
    case Gdiplus::UnsupportedGdiplusVersion:
        return "UnsupportedGdiplusVersion";
    case Gdiplus::GdiplusNotInitialized:
        return "GdiplusNotInitialized";
    case Gdiplus::PropertyNotFound:
        return "PropertyNotFound";
    case Gdiplus::PropertyNotSupported:
        return "PropertyNotSupported";
    case Gdiplus::ProfileNotFound:
        return "ProfileNotFound";
    default:
        return "UnknownErrorCode";
    }
}

void CheckGdiPlusStatus( Gdiplus::Status gdiStatus, std::string_view functionName )
{
    if ( gdiStatus > 0 )
    {
        throw qwr::QwrException( "GdiPlus error: {} failed with error ({:#x}): {}", functionName, static_cast<int>( gdiStatus ), GdiPlusStatusToText( gdiStatus ) );
    }
}

} // namespace smp
