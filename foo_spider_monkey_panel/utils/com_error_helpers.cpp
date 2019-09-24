#include <stdafx.h>
#include "com_error_helpers.h"

#include <utils/string_helpers.h>
#include <utils/winapi_error_helpers.h>

#include <smp_exception.h>

namespace smp::error
{

void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr )
{
    switch ( hresult )
    {
    case DISP_E_BADVARTYPE:
    {
        throw SmpException( fmt::format( "ActiveXObject: CBad variable type {}", argerr ) );
    }
    case DISP_E_EXCEPTION:
    {
        auto autoCleaner = [&exception]
        {
            SysFreeString( exception.bstrSource );
            SysFreeString( exception.bstrDescription );
            SysFreeString( exception.bstrHelpFile );
        };

        if ( exception.bstrDescription )
        {
            const auto descriptionStr = smp::unicode::ToU8( (wchar_t*)exception.bstrDescription );
            const auto sourceStr = smp::unicode::ToU8( (wchar_t*)exception.bstrSource );
            throw SmpException( fmt::format("ActiveXObject: ({}) {}", sourceStr, descriptionStr ) );
        }
        else if ( FAILED( exception.scode ) )
        {
            CheckHR( exception.scode, "ActiveXObject" );
        }
        else
        {
            throw SmpException( "ActiveXObject: exception was thrown" );
        }
    }
    case DISP_E_OVERFLOW:
    {
        throw SmpException( fmt::format( "ActiveXObject: Can not convert variable {}", argerr ) );
    }
    case DISP_E_PARAMNOTFOUND:
    {
        throw SmpException( fmt::format( "ActiveXObject: Parameter {} not found", argerr ) );
    }
    case DISP_E_TYPEMISMATCH:
    {
        throw SmpException( fmt::format( "ActiveXObject: Parameter {} type mismatch", argerr ) );
    }
    case DISP_E_PARAMNOTOPTIONAL:
    {
        throw SmpException( fmt::format( "ActiveXObject: Parameter {} is required", argerr ) );
    }
    default:
    {
        CheckHR( hresult, "ActiveXObject" );
    }
    }
}

} // namespace smp::error
