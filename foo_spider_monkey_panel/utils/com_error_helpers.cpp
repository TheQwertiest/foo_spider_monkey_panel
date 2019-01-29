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
        throw SmpException( smp::string::Formatter() << "ActiveXObject: Bad variable type " << argerr );
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
            pfc::string8_fast descriptionStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrDescription, SysStringLen( exception.bstrDescription ) ) );
            pfc::string8_fast sourceStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrSource, SysStringLen( exception.bstrSource ) ) );

            std::string errorMsg = std::string{} + "ActiveXObject: (" + sourceStr.c_str() + ")" + descriptionStr.c_str();
            throw SmpException( errorMsg );
        }
        else if ( exception.scode )
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
        throw SmpException( smp::string::Formatter() << "ActiveXObject: Can not convert variable " << argerr );
    }
    case DISP_E_PARAMNOTFOUND:
    {
        throw SmpException( smp::string::Formatter() << "ActiveXObject: Parameter" << argerr << " not found" );
    }
    case DISP_E_TYPEMISMATCH:
    {
        throw SmpException( smp::string::Formatter() << "ActiveXObject: Parameter" << argerr << " type mismatch" );
    }
    case DISP_E_PARAMNOTOPTIONAL:
    {
        throw SmpException( smp::string::Formatter() << "ActiveXObject: Parameter" << argerr << " is required" );
    }
    default:
    {
        CheckHR( hresult, "ActiveXObject" );
    }
    }
}

} // namespace smp::error
