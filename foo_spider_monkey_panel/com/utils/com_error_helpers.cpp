#include <stdafx.h>

#include "com_error_helpers.h"

#include <qwr/final_action.h>
#include <qwr/string_helpers.h>
#include <qwr/winapi_error_helpers.h>

namespace smp::com
{

void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr )
{
    switch ( hresult )
    {
    case DISP_E_BADVARTYPE:
    {
        throw qwr::QwrException( "ActiveXObject: Bad variable type `{}`", argerr );
    }
    case DISP_E_EXCEPTION:
    {
        const qwr::final_action autoCleaner( [&exception] {
            SysFreeString( exception.bstrSource );
            SysFreeString( exception.bstrDescription );
            SysFreeString( exception.bstrHelpFile );
        } );

#pragma warning( push )
#pragma warning( disable : 6217 ) // Consider using SUCCEEDED or FAILED macro
        const auto hr = ( !!exception.scode
                              ? exception.scode
                              : _com_error::WCodeToHRESULT( exception.wCode ) );
#pragma warning( pop )

        if ( exception.bstrDescription )
        {
            const auto errorDesc8 = qwr::unicode::ToU8( std::wstring_view{ exception.bstrDescription ? exception.bstrDescription : L"<none>" } );
            const auto errorSource8 = qwr::unicode::ToU8( std::wstring_view{ exception.bstrSource ? exception.bstrSource : L"<none>" } );
            throw qwr::QwrException( "ActiveXObject:\n"
                                     "  code: {:#x}\n"
                                     "  description: {}\n"
                                     "  source: {}",
                                     static_cast<uint32_t>( hr ),
                                     errorDesc8,
                                     errorSource8 );
        }
        else
        {
            qwr::error::CheckHR( hr, "ActiveXObject call" );
            throw qwr::QwrException( "ActiveXObject: <no info> (malformed DISP_E_EXCEPTION)", argerr );
        }
    }
    case DISP_E_OVERFLOW:
    {
        throw qwr::QwrException( "ActiveXObject: Can not convert variable `{}`", argerr );
    }
    case DISP_E_PARAMNOTFOUND:
    {
        throw qwr::QwrException( "ActiveXObject: Parameter `{}` not found", argerr );
    }
    case DISP_E_TYPEMISMATCH:
    {
        throw qwr::QwrException( "ActiveXObject: Parameter `{}` type mismatch", argerr );
    }
    case DISP_E_PARAMNOTOPTIONAL:
    {
        throw qwr::QwrException( "ActiveXObject: Parameter `{}` is required", argerr );
    }
    default:
    {
        qwr::error::CheckHR( hresult, "ActiveXObject" );
    }
    }
}

} // namespace smp::com
