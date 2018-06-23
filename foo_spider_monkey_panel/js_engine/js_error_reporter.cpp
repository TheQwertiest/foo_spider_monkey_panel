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

    globalCtx->Fail();
    // TODO: create auto reporter here

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

    std::string errorText = report->message().c_str();
    if ( report->filename )
    {
        
    }

    /*

    js::ErrorReport report( cx );
    if ( !report.init( cx, exn, js::ErrorReport::WithSideEffects ) )
    {
        fprintf( stderr, "out of memory initializing ErrorReport\n" );
        fflush( stderr );
        JS_ClearPendingException( cx );
        return;
    }

    MOZ_ASSERT( !JSREPORT_IS_WARNING( report.report()->flags ) );

    FILE* fp = ErrorFilePointer();
    PrintError( cx, fp, report.toStringResult(), report.report(), reportWarnings );

    {
        JS::AutoSaveExceptionState savedExc( cx );
        if ( !PrintStackTrace( cx, exn ) )
            fputs( "(Unable to print stack trace)\n", fp );
        savedExc.restore();
    }

    if ( report.report()->errorNumber == JSMSG_OUT_OF_MEMORY )
        sc->exitCode = EXITCODE_OUT_OF_MEMORY;
    else
        sc->exitCode = EXITCODE_RUNTIME_ERROR;
        */

    JS_ClearPendingException( cx );

    popup_msg::g_show( errorText.c_str(), JSP_NAME );
    MessageBeep( MB_ICONASTERISK );
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

void AutoReportException::PrintStack( JSContext* cx, JS::HandleValue exn )
{
    /*
    if ( !exn.isObject() )
        return;

    JS::Maybe<JSAutoCompartment> ac;
    JS::RootedObject exnObj( cx, &exn.toObject() );

    // Ignore non-ErrorObject thrown by |throw| statement.
    if ( !exnObj->is<JS::ErrorObject>() )
        return;

    // Exceptions thrown while compiling top-level script have no stack.
    JS::RootedObject stackObj( cx, exnObj->as<ErrorObject>().stack() );
    if ( !stackObj )
        return;

    JS::RootedString stackStr( cx );
    if ( !BuildStackString( cx, stackObj, &stackStr, 2 ) )
        return;

    JS::UniqueChars stack( JS_EncodeStringToUTF8( cx, stackStr ) );
    if ( !stack )
        return;

    FILE* fp = ErrorFilePointer();
    fputs( "Stack:\n", fp );
    fputs( stack.get(), fp );
    */
}

}
