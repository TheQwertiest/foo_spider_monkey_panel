#include <stdafx.h>

#include "js_error_reporter.h"
#include <js_objects/global_object.h>

#include <popup_msg.h>
#include <user_message.h>


namespace mozjs
{

AutoReportException::AutoReportException( JSContext* cx )
    : cx( cx )
{
}

AutoReportException::~AutoReportException()
{
    if ( !JS_IsExceptionPending( cx ) )
    {
        return;
    }

    // Get exception object before printing and clearing exception.
    JS::RootedValue excn( cx );
    (void)JS_GetPendingException( cx, &excn );

    JS_ClearPendingException( cx );   

    JS::RootedObject global( cx, JS::CurrentGlobalOrNull( cx ) );
    if ( !global )
    {
        assert( 0 );
        return;
    }

    auto globalCtx = static_cast<JsGlobalObject *>(JS_GetPrivate( global ));
    if ( !globalCtx )
    {
        assert( 0 );
        return;
    }

    struct ScopedFail
    {
        ScopedFail( JSContext* pCx, JsGlobalObject * pGlobal )
        {
            pCx_ = pCx;
            pGlobal_ = pGlobal;
        }
        ~ScopedFail()
        {
            FB2K_console_formatter() << errorText.c_str();
            pGlobal_->Fail( errorText );
            JS_ClearPendingException( pCx_ );
        }

        JSContext* pCx_;
        JsGlobalObject * pGlobal_;
        std::string errorText = "Script failed!";
    };
    ScopedFail scFail( cx, globalCtx );
    
    JS::RootedObject excnObject( cx, excn.toObjectOrNull() );
    if ( !excnObject )
    {
        assert( 0 );
        return;
    }

    JSErrorReport* report = JS_ErrorFromException( cx, excnObject );
    if ( !report )
    {
        assert( 0 );
        return;
    }

    assert( !JSREPORT_IS_WARNING( report->flags ) );

    // TODO: "Error: JScript Panel v2.1.3 (Top Panel)"
    scFail.errorText = report->message().c_str();
    if ( report->filename )
    {        
        scFail.errorText += "\n\n";
        scFail.errorText += "File: ";
        scFail.errorText += report->filename;
        scFail.errorText += "\n";
        scFail.errorText += "Line: ";
        scFail.errorText += std::to_string( report->lineno );
        scFail.errorText += ", Column: ";
        scFail.errorText += std::to_string( report->column );
        if ( report->linebufLength() )
        {
            scFail.errorText += "\n";
            // <codecvt> is deprecated in C++17...
            scFail.errorText += pfc::stringcvt::string_utf8_from_utf16( report->linebuf(), report->linebufLength() );
        }
    }

    std::string stackTrace = GetStackTraceString( cx, excnObject );
    if ( !stackTrace.empty() )
    {
        scFail.errorText += "\n\n";
        scFail.errorText += "Stack trace:\n";
        scFail.errorText += stackTrace;
    }
}

void AutoReportException::PrintError()
{
    /*
    if ( !err ) return;

    DWORD ctx = 0;
    ULONG line = 0;
    LONG charpos = 0;
    EXCEPINFO excep = { 0 };
    //WCHAR buf[512] = { 0 };
    _bstr_t sourceline;
    _bstr_t name;

    if ( FAILED( err->GetSourcePosition( &ctx, &line, &charpos ) ) )
    {
        line = 0;
        charpos = 0;
    }

    if ( FAILED( err->GetSourceLineText( sourceline.GetAddress() ) ) )
    {
        sourceline = L"<source text only available at compile time>";
    }

    if ( FAILED( err->GetExceptionInfo( &excep ) ) )
    {
        return;
    }

    // Do a deferred fill-in if necessary
    if ( excep.pfnDeferredFillIn )
    {
        (*excep.pfnDeferredFillIn)(&excep);
    }

    pfc::string_formatter formatter;
    formatter << "Error: " JSP_NAME " v" JSP_VERSION " (" << m_host->ScriptInfo().build_info_string().get_ptr() << ")\n";

    if ( excep.bstrSource && excep.bstrDescription )
    {
        formatter << pfc::stringcvt::string_utf8_from_wide( excep.bstrSource ) << ":\n";
        formatter << pfc::stringcvt::string_utf8_from_wide( excep.bstrDescription ) << "\n";
    }
    else
    {
        pfc::string8_fast errorMessage;

        if ( uFormatSystemErrorMessage( errorMessage, excep.scode ) )
            formatter << errorMessage;
        else
            formatter << "Unknown error code: 0x" << pfc::format_hex_lowercase( (unsigned)excep.scode );
    }

    if ( m_contextToPathMap.exists( ctx ) )
    {
        formatter << "File: " << m_contextToPathMap[ctx] << "\n";
    }

    formatter << "Line: " << (t_uint32)(line + 1) << ", Col: " << (t_uint32)(charpos + 1) << "\n";
    formatter << pfc::stringcvt::string_utf8_from_wide( sourceline );
    if ( name.length() > 0 ) formatter << "\nAt: " << name;

    if ( excep.bstrSource ) SysFreeString( excep.bstrSource );
    if ( excep.bstrDescription ) SysFreeString( excep.bstrDescription );
    if ( excep.bstrHelpFile ) SysFreeString( excep.bstrHelpFile );

    FB2K_console_formatter() << formatter;
    popup_msg::g_show( formatter, JSP_NAME );
    MessageBeep( MB_ICONASTERISK );
    SendMessage( m_host->GetHWND(), UWM_SCRIPT_ERROR, 0, 0 );
    */
}

std::string AutoReportException::GetStackTraceString( JSContext* cx, JS::HandleObject exn )
{
    // Exceptions thrown while compiling top-level script have no stack.
    JS::RootedObject stackObj( cx, JS::ExceptionStackOrNull( exn ) );
    if ( !stackObj )
    {
        return std::string();
    }

    JS::RootedString stackStr( cx );
    if ( !BuildStackString( cx, stackObj, &stackStr, 2 ) )
    {
        return std::string();
    }

    // JS::UniqueChars generates heap corruption exception in it's destructor
    const char* encodedString = JS_EncodeStringToUTF8( cx, stackStr );
    if ( !encodedString )
    {
        return std::string();
    } 

    std::string outString( encodedString );
    JS_free( cx, (void*)encodedString );

    return outString;
}

}
