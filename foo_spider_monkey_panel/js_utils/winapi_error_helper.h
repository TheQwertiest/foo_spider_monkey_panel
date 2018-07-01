#pragma once

#define WINAPI_RETURN_WITH_REPORT(cx, retValue, funcName) \
    do \
    {\
        LPVOID lpMsgBuf;\
        DWORD errorCode = GetLastError();\
\
        FormatMessage(\
            FORMAT_MESSAGE_ALLOCATE_BUFFER |\
            FORMAT_MESSAGE_FROM_SYSTEM |\
            FORMAT_MESSAGE_IGNORE_INSERTS,\
            nullptr,\
            errorCode,\
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),\
            (LPTSTR)&lpMsgBuf,\
            0, nullptr );\
\
        std::string errorStr = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)lpMsgBuf );\
        JS_ReportErrorASCII( cx, "WinAPI error: '%s' failed with error (0x%X): %s", #funcName, errorCode, errorStr.c_str() );\
\
        LocalFree( lpMsgBuf );\
        return retValue;\
    } while(false)

#define IF_WINAPI_FAILED_RETURN_WITH_REPORT(cx, bRet, retValue, funcName) \
    do \
    {\
        if ( !bRet )\
        {\
            WINAPI_RETURN_WITH_REPORT(cx, retValue, funcName);\
        }\
    } while(false)

#define IF_HR_FAILED_RETURN_WITH_REPORT(cx, hr, retValue, funcName) \
    do \
    {\
        int iRet_internal = Win32FromHResult(hr);\
        if ( ERROR_SUCCESS != iRet_internal )\
        {\
            SetLastError(iRet_internal);\
            WINAPI_RETURN_WITH_REPORT(cx, retValue, funcName);\
        }\
    } while(false)

DWORD Win32FromHResult( HRESULT hr );