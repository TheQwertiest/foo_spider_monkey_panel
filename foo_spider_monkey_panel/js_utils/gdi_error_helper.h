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

#define IF_GDI_FAILED_THROW_SMP( gdiRet, funcName )                                                                                                                   \
    do                                                                                                                                                                \
    {                                                                                                                                                                 \
        if ( gdiRet > 0 )                                                                                                                                             \
        {                                                                                                                                                             \
            throw smp::SmpException(                                                                                                                                  \
                smp::string::Formatter() << "GDI error: " << funcName << " failed with error (0x" << std::hex << gdiRet << ": " << GdiErrorCodeToText( gdiRet ) ); \
        }                                                                                                                                                             \
    } while ( false )

namespace mozjs
{

const char* GdiErrorCodeToText( Gdiplus::Status errorCode );
}