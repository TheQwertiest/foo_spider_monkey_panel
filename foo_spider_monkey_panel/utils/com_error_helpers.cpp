#include <stdafx.h>

#include "com_error_helpers.h"

#include <utils/scope_helpers.h>
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
        const smp::utils::final_action autoCleaner( [&exception] {
            SysFreeString( exception.bstrSource );
            SysFreeString( exception.bstrDescription );
            SysFreeString( exception.bstrHelpFile );
        } );

        if ( exception.bstrDescription )
        {
            const auto errorSource8 = smp::unicode::ToU8( std::wstring_view{ exception.bstrSource ? exception.bstrSource : L"<none>" } );
            const auto errorDesc8 = smp::unicode::ToU8( std::wstring_view{ exception.bstrDescription ? exception.bstrDescription : L"<none>" } );
            throw SmpException( fmt::format( "ActiveXObject: source: {}; description: {}", errorSource8, errorDesc8 ) );
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
