#include <stdafx.h>
#include "com_error_helper.h"


namespace mozjs
{


void ReportActiveXError( JSContext* cx, HRESULT hresult, EXCEPINFO& exception, UINT& argerr )
{
    switch ( hresult )
    {
    case DISP_E_BADPARAMCOUNT:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Wrong number of parameters" );
        break;
    }
    case DISP_E_BADVARTYPE:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Bad variable type %d", argerr );
        break;
    }
    case DISP_E_EXCEPTION:
    {
        if ( exception.bstrDescription )
        {
            pfc::string8_fast descriptionStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrDescription, SysStringLen( exception.bstrDescription ) )
            );
            pfc::string8_fast sourceStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrSource, SysStringLen( exception.bstrSource ) )
            );

            JS_ReportErrorUTF8( cx, "ActiveXObject: (%s) %s", sourceStr.c_str(), descriptionStr.c_str() );
        }
        else
        {
            JS_ReportErrorUTF8( cx, "ActiveXObject: Error code %d", exception.scode );
        }
        SysFreeString( exception.bstrSource );
        SysFreeString( exception.bstrDescription );
        SysFreeString( exception.bstrHelpFile );
        break;
    }
    case DISP_E_MEMBERNOTFOUND:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Function not found" );
        break;
    }
    case DISP_E_OVERFLOW:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Can not convert variable %d", argerr );
        break;
    }
    case DISP_E_PARAMNOTFOUND:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Parameter %d not found", argerr );
        break;
    }
    case DISP_E_TYPEMISMATCH:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Parameter %d type mismatch", argerr );
        break;
    }
    case DISP_E_UNKNOWNINTERFACE:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Unknown interface" );
        break;
    }
    case DISP_E_UNKNOWNLCID:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Unknown LCID" );
        break;
    }
    case DISP_E_PARAMNOTOPTIONAL:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Parameter %d is required", argerr );
        break;
    }
    default:
    {
    }
    }
}

}
