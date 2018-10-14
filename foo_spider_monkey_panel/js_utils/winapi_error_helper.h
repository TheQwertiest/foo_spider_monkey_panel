#pragma once

#include <utils/string_helpers.h>

#define WINAPI_RETURN_WITH_REPORT( cx, retValue, funcName )                                                                  \
    do                                                                                                                       \
    {                                                                                                                        \
        DWORD errorCode = GetLastError();                                                                                    \
        pfc::string8_fast errorStr = mozjs::MessageFromErrorCode( errorCode );                                               \
        JS_ReportErrorUTF8( cx, "WinAPI error: '%s' failed with error (0x%X): %s", #funcName, errorCode, errorStr.c_str() ); \
        return retValue;                                                                                                     \
    } while ( false )

#define IF_WINAPI_FAILED_RETURN_WITH_REPORT( cx, successPredicate, retValue, funcName ) \
    do                                                                                  \
    {                                                                                   \
        if ( !( successPredicate ) )                                                    \
        {                                                                               \
            WINAPI_RETURN_WITH_REPORT( cx, retValue, funcName );                        \
        }                                                                               \
    } while ( false )

// TODO: replace funcName with "funcName"

#define IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, retValue, funcName ) \
    do                                                                \
    {                                                                 \
        int iRet_internal = mozjs::Win32FromHResult( hr );            \
        if ( ERROR_SUCCESS != iRet_internal )                         \
        {                                                             \
            SetLastError( iRet_internal );                            \
            WINAPI_RETURN_WITH_REPORT( cx, retValue, funcName );      \
        }                                                             \
    } while ( false )

#define WINAPI_THROW_SMP( funcName )                                                                                                                     \
    do                                                                                                                                                   \
    {                                                                                                                                                    \
        DWORD errorCode = GetLastError();                                                                                                                \
        pfc::string8_fast errorStr = mozjs::MessageFromErrorCode( errorCode );                                                                           \
        throw smp::SmpException(                                                                                                                         \
            smp::string::Formatter() << "WinAPI error: " << funcName << " failed with error (0x" << std::hex << errorCode << ": " << errorStr.c_str() ); \
    } while ( false )

#define IF_WINAPI_FAILED_THROW_SMP( successPredicate, funcName ) \
    do                                                           \
    {                                                            \
        if ( !( successPredicate ) )                             \
        {                                                        \
            WINAPI_THROW_SMP( funcName );                        \
        }                                                        \
    } while ( false )

#define IF_HR_FAILED_THROW_SMP( hr, funcName )             \
    do                                                     \
    {                                                      \
        int iRet_internal = mozjs::Win32FromHResult( hr ); \
        if ( ERROR_SUCCESS != iRet_internal )              \
        {                                                  \
            SetLastError( iRet_internal );                 \
            WINAPI_THROW_SMP( funcName );                  \
        }                                                  \
    } while ( false )

namespace mozjs
{

DWORD Win32FromHResult( HRESULT hr );
pfc::string8_fast MessageFromErrorCode( DWORD errorCode );

} // namespace mozjs