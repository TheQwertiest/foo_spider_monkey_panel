#include <stdafx.h>
#include "gdi_error_helper.h"


namespace mozjs
{

const char* GdiErrorCodeToText( Gdiplus::Status errorCode )
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
    default:
        return "UnknownErrorCode";
    }
}

}
