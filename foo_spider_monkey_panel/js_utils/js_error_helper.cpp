#include <stdafx.h>
#include "js_error_helper.h"

#include <convert/native_to_js.h>
#include <convert/js_to_native.h>
#include <js_objects/global_object.h>
#include <js_utils/scope_helper.h>

#include <smp_exception.h>

namespace
{

pfc::string8_fast GetStackTraceString( JSContext* cx, JS::HandleObject exn )
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

} // namespace

namespace mozjs::error
{

AutoJsReport::AutoJsReport( JSContext* cx )
    : cx( cx )
{
}

AutoJsReport::~AutoJsReport()
{
    if ( isDisabled_ )
    {
        return;
    }

    if ( !JS_IsExceptionPending( cx ) )
    {
        return;
    }

    pfc::string8_fast errorText = GetTextFromCurrentJsError( cx );
    JS_ClearPendingException( cx );

    JS::RootedObject global( cx, JS::CurrentGlobalOrNull( cx ) );
    if ( !global )
    {
        assert( 0 );
        return;
    }

    auto globalCtx = static_cast<JsGlobalObject*>( JS_GetPrivate( global ) );
    if ( !globalCtx )
    {
        assert( 0 );
        return;
    }

    globalCtx->Fail( errorText );
    JS_ClearPendingException( cx );
}

void AutoJsReport::Disable()
{
    isDisabled_ = true;
}

pfc::string8_fast GetTextFromCurrentJsError( JSContext* cx )
{
    assert( JS_IsExceptionPending( cx ) );
    
    JS::RootedValue excn( cx );
    (void)JS_GetPendingException( cx, &excn );

    pfc::string8_fast errorText;

    if ( excn.isString() )
    {
        auto retVal = convert::to_native::ToValue<pfc::string8_fast>( cx, excn );
        if ( !retVal )
        {
            return errorText;
        }

        errorText = retVal.value();
    }
    else if ( excn.isObject() )
    {
        JS::RootedObject excnObject( cx, &excn.toObject() );

        JSErrorReport* report = JS_ErrorFromException( cx, excnObject );
        if ( !report )
        { // Can sometimes happen :/
            return errorText;
        }

        assert( !JSREPORT_IS_WARNING( report->flags ) );

        errorText = report->message().c_str();
        if ( report->filename )
        {
            errorText += "\n\n";
            errorText += "File: ";
            errorText += report->filename;
            errorText += "\n";
            errorText += "Line: ";
            errorText += std::to_string( report->lineno ).c_str();
            errorText += ", Column: ";
            errorText += std::to_string( report->column ).c_str();
            if ( report->linebufLength() )
            {
                errorText += "\n";
                errorText += pfc::stringcvt::string_utf8_from_utf16( report->linebuf(), report->linebufLength() );
            }
        }

        pfc::string8_fast stackTrace = GetStackTraceString( cx, excnObject );
        if ( !stackTrace.is_empty() )
        {
            errorText += "\n\n";
            errorText += "Stack trace:\n";
            errorText += stackTrace;
        }
    }

    return errorText;
}

void ExceptionToJsError( JSContext* cx )
{
    try
    {
        throw;
    }
    catch ( const smp::JsException& )
    {
        assert( JS_IsExceptionPending( cx ) );
    }
    catch ( const smp::SmpException& e )
    {
        JS_ClearPendingException( cx );
        JS_ReportErrorUTF8( cx, e.what() );
    }
    catch ( const _com_error& e )
    {
        JS_ClearPendingException( cx );

        pfc::string8_fast errorMsg8 = pfc::stringcvt::string_utf8_from_wide( e.ErrorMessage() ? (const wchar_t*)e.ErrorMessage() : L"<none>" );
        pfc::string8_fast errorSource8 = pfc::stringcvt::string_utf8_from_wide( e.Source().length() ? (const wchar_t*)e.Source() : L"<none>" );
        pfc::string8_fast errorDesc8 = pfc::stringcvt::string_utf8_from_wide( e.Description().length() ? (const wchar_t*)e.Description() : L"<none>" );
        JS_ReportErrorUTF8( cx, "COM error: message %s; source: %s; description: %s", errorMsg8.c_str(), errorSource8.c_str(), errorDesc8.c_str() );
    }
    catch ( const std::bad_alloc& )
    {
        JS_ClearPendingException( cx );
        JS_ReportAllocationOverflow( cx );
    }
    // SM is not designed to handle uncaught exceptions, so we are risking here,
    // while hoping that this exception will reach fb2k handler.
}

void ReportJsErrorWithFunctionName( JSContext* cx, const char* functionName )
{
    scope::final_action autoJsReport( [cx, functionName]() {
        JS_ReportErrorUTF8( cx, "'%s' failed", functionName );
    } );

    if ( !JS_IsExceptionPending( cx ) )
    {
        return;
    }

    // Get exception object before printing and clearing exception.
    JS::RootedValue excn( cx );
    (void)JS_GetPendingException( cx, &excn );

    auto makeErrorString = [&functionName]( const pfc::string8_fast& currentMessage ) {
        pfc::string8_fast newMessage;
        newMessage += "'";
        newMessage += functionName;
        newMessage += "'";
        newMessage += " failed";
        if ( currentMessage.length() )
        {
            newMessage += ( ":\n" );
            newMessage += currentMessage;
        }

        return newMessage;
    };

    if ( excn.isString() )
    {
        auto retVal = convert::to_native::ToValue<pfc::string8_fast>( cx, excn );
        if ( !retVal )
        {
            return;
        }

        if ( retVal.value() == pfc::string8_fast( "out of memory" ) )
        { // Can't modify the message since we're out of memory
            autoJsReport.cancel();
            return;
        }

        const pfc::string8_fast newMessage = makeErrorString( retVal.value() );
        JS::RootedValue jsMessage( cx );
        if ( !convert::to_js::ToValue<pfc::string8_fast>( cx, newMessage, &jsMessage ) )
        {
            return;
        }

        autoJsReport.cancel();
        JS_SetPendingException( cx, jsMessage );
        return;
    }
    else if ( excn.isObject() )
    {
        JS::RootedObject excnObject( cx, &excn.toObject() );

        JSErrorReport* report = JS_ErrorFromException( cx, excnObject );
        if ( !report )
        { // Sometimes happens with custom JS errors
            return;
        }

        pfc::string8_fast currentMessage = report->message().c_str();
        pfc::string8_fast newMessage;
        newMessage += "'";
        newMessage += functionName;
        newMessage += "'";
        newMessage += " failed";
        if ( currentMessage.length() )
        {
            newMessage += ( ":\n" );
            newMessage += currentMessage;
        }

        JS::RootedValue jsFilename( cx );
        JS::RootedValue jsMessage( cx );
        if ( !convert::to_js::ToValue<pfc::string8_fast>( cx, report->filename, &jsFilename )
             || !convert::to_js::ToValue<pfc::string8_fast>( cx, newMessage, &jsMessage ) )
        {
            return;
        }

        JS::RootedObject excnStack( cx, JS::ExceptionStackOrNull( excnObject ) );
        JS::RootedValue newExcn( cx );
        JS::RootedString jsFilenameStr( cx, jsFilename.toString() );
        JS::RootedString jsMessageStr( cx, jsMessage.toString() );

        if ( !JS::CreateError( cx, (JSExnType)report->exnType, excnStack, jsFilenameStr, report->lineno, report->column, nullptr, jsMessageStr, &newExcn ) )
        {
            return;
        }

        autoJsReport.cancel();
        JS_SetPendingException( cx, newExcn );
    }
}

} // namespace mozjs::error
