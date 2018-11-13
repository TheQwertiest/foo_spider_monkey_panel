#include <stdafx.h>
#include "com_error_helper.h"

#include <utils/string_helpers.h>
#include <utils/winapi_error_helper.h>

#include <smp_exception.h>

namespace smp::error
{

void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr )
{
    switch ( hresult )
    {
    case DISP_E_BADPARAMCOUNT:
    {
        throw SmpException( "ActiveXObject: Wrong number of parameters" );
    }
    case DISP_E_BADVARTYPE:
    {
        throw SmpException( smp::string::Formatter() << "ActiveXObject: Bad variable type " << argerr );
    }
    case DISP_E_EXCEPTION:
    {
        std::string errorMsg;
        if ( exception.bstrDescription )
        {
            pfc::string8_fast descriptionStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrDescription, SysStringLen( exception.bstrDescription ) ) );
            pfc::string8_fast sourceStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrSource, SysStringLen( exception.bstrSource ) ) );

            errorMsg = std::string{} + "ActiveXObject: (" + sourceStr.c_str() + ")" + descriptionStr.c_str();
        }
        else
        {
            errorMsg = smp::string::Formatter() << "ActiveXObject: Error code 0x" << std::hex << exception.scode;
        }
        SysFreeString( exception.bstrSource );
        SysFreeString( exception.bstrDescription );
        SysFreeString( exception.bstrHelpFile );

        throw SmpException( errorMsg );
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
        smp::error::CheckHR( hresult, "ActiveXObject" );
    }
    }
}

} // namespace smp::error
