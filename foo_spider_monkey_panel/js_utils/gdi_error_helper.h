#pragma once


#define IF_GDI_FAILED_RETURN_WITH_REPORT(cx,gdiRet,retValue,funcName) \
    do \
    {\
        if ( gdiRet > 0 )\
        {\
            JS_ReportErrorUTF8( cx, "GDI error: '%s' failed: %s(0x%X)", #funcName, GdiErrorCodeToText( gdiRet ), gdiRet ); \
            return retValue;\
        }\
    } while(false)

namespace mozjs
{

const char* GdiErrorCodeToText( Gdiplus::Status errorCode );

}