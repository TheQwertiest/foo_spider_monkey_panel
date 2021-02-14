#include <stdafx.h>

#include "com_error_helpers.h"

#include <qwr/final_action.h>
#include <qwr/string_helpers.h>
#include <qwr/winapi_error_helpers.h>

namespace qwr::error
{

void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr )
{
    switch ( hresult )
    {
    case DISP_E_BADVARTYPE:
    {
        throw qwr::QwrException( fmt::format( "ActiveXObject: Bad variable type `{}`", argerr ) );
    }
    case DISP_E_EXCEPTION:
    {
        const qwr::final_action autoCleaner( [&exception] {
            SysFreeString( exception.bstrSource );
            SysFreeString( exception.bstrDescription );
            SysFreeString( exception.bstrHelpFile );
        } );

        const auto errorSource8 = qwr::unicode::ToU8( std::wstring_view{ exception.bstrSource ? exception.bstrSource : L"<none>" } );
        const auto errorDesc8 = qwr::unicode::ToU8( std::wstring_view{ exception.bstrDescription ? exception.bstrDescription : L"<none>" } );
        throw qwr::QwrException( fmt::format( "ActiveXObject:\n"
                                              "  code: {:#x}\n"
                                              "  description: {}\n"
                                              "  source: {}",
#pragma warning( push )
#pragma warning( disable : 6217 ) // Consider using SUCCEEDED or FAILED macro
                                              static_cast<uint32_t>(
                                                  !!exception.scode
                                                      ? exception.scode
                                                      : _com_error::WCodeToHRESULT( exception.wCode ) ),
#pragma warning( pop )
                                              errorDesc8,
                                              errorSource8 ) );
    }
    case DISP_E_OVERFLOW:
    {
        throw qwr::QwrException( fmt::format( "ActiveXObject: Can not convert variable `{}`", argerr ) );
    }
    case DISP_E_PARAMNOTFOUND:
    {
        throw qwr::QwrException( fmt::format( "ActiveXObject: Parameter `{}` not found", argerr ) );
    }
    case DISP_E_TYPEMISMATCH:
    {
        throw qwr::QwrException( fmt::format( "ActiveXObject: Parameter `{}` type mismatch", argerr ) );
    }
    case DISP_E_PARAMNOTOPTIONAL:
    {
        throw qwr::QwrException( fmt::format( "ActiveXObject: Parameter `{}` is required", argerr ) );
    }
    default:
    {
        CheckHR( hresult, "ActiveXObject" );
    }
    }
}

} // namespace qwr::error
