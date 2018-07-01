#include <stdafx.h>
#include "winapi_error_helper.h"

DWORD Win32FromHResult( HRESULT hr )
{
    if ( (hr & 0xFFFF0000) == MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, 0 ) )
    {
        return HRESULT_CODE( hr );
    }

    if ( hr == S_OK )
    {
        return ERROR_SUCCESS;
    }

    // Not a Win32 HRESULT so return a generic error code.
    return ERROR_CAN_NOT_COMPLETE;
}