#include <stdafx.h>
#include "js_error_helper.h"

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
    if ( isDisabled_ )
    {
        return;
    }

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
        pfc::string8_fast errorText = "Script failed!";
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
        scFail.errorText.add_string(std::to_string( report->lineno ).c_str());
        scFail.errorText += ", Column: ";
        scFail.errorText.add_string( std::to_string( report->column ).c_str() );
        if ( report->linebufLength() )
        {
            scFail.errorText += "\n";
            // <codecvt> is deprecated in C++17...
            scFail.errorText += pfc::stringcvt::string_utf8_from_utf16( report->linebuf(), report->linebufLength() );
        }
    }

    pfc::string8_fast stackTrace = GetStackTraceString( cx, excnObject );
    if ( !stackTrace.is_empty() )
    {
        scFail.errorText += "\n\n";
        scFail.errorText += "Stack trace:\n";
        scFail.errorText += stackTrace;
    }
}

void AutoReportException::Disable()
{
    isDisabled_ = true;
}

pfc::string8_fast AutoReportException::GetStackTraceString( JSContext* cx, JS::HandleObject exn )
{
    // Exceptions thrown while compiling top-level script have no stack.
    JS::RootedObject stackObj( cx, JS::ExceptionStackOrNull( exn ) );
    if ( !stackObj )
    {
        return pfc::string8_fast();
    }

    JS::RootedString stackStr( cx );
    if ( !BuildStackString( cx, stackObj, &stackStr, 2 ) )
    {
        return pfc::string8_fast();
    }

    // JS::UniqueChars generates heap corruption exception in it's destructor
    const char* encodedString = JS_EncodeStringToUTF8( cx, stackStr );
    if ( !encodedString )
    {
        return pfc::string8_fast();
    } 

    pfc::string8_fast outString( encodedString );
    JS_free( cx, (void*)encodedString );

    return outString;
}


pfc::string8_fast GetCurrentExceptionText( JSContext* cx )
{
    if ( !JS_IsExceptionPending( cx ) )
    {
        return pfc::string8_fast();
    }

    // Get exception object before printing and clearing exception.
    JS::RootedValue excn( cx );
    (void)JS_GetPendingException( cx, &excn );

    JS::RootedObject global( cx, JS::CurrentGlobalOrNull( cx ) );
    if ( !global )
    {
        return pfc::string8_fast();
    }

    JS::RootedObject excnObject( cx, excn.toObjectOrNull() );
    if ( !excnObject )
    {
        return pfc::string8_fast();
    }

    JSErrorReport* report = JS_ErrorFromException( cx, excnObject );
    if ( !report )
    {
        return pfc::string8_fast();
    }

    if ( JSMSG_USER_DEFINED_ERROR != report->errorNumber )
    {
        return pfc::string8_fast();
    }

    JS_ClearPendingException( cx );
    return report->message().c_str();
}

}
