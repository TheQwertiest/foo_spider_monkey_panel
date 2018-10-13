#include <stdafx.h>
#include "js_error_helper.h"

#include <convert/native_to_js.h>
#include <convert/js_to_native.h>
#include <js_objects/global_object.h>
#include <js_utils/scope_helper.h>

#include <smp_exception.h>

namespace
{

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

    auto globalCtx = static_cast<JsGlobalObject*>( JS_GetPrivate( global ) );
    if ( !globalCtx )
    {
        assert( 0 );
        return;
    }

    pfc::string8_fast errorText;
    scope::final_action autoFail( [cx=cx, &globalCtx, &errorText]
    {
        globalCtx->Fail( errorText );
        JS_ClearPendingException( cx );
    } );

    if ( excn.isString() )
    {
        auto retVal = convert::to_native::ToValue<pfc::string8_fast>( cx, excn );
        if ( !retVal )
        {
            return;
        }

        errorText = retVal.value();
    }
    else if ( excn.isObject() )
    {
        JS::RootedObject excnObject( cx, &excn.toObject() );

        JSErrorReport* report = JS_ErrorFromException( cx, excnObject );
        if ( !report )
        { // Can sometimes happen :/
            return;
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
}

void AutoJsReport::Disable()
{
    isDisabled_ = true;
}

pfc::string8_fast AutoJsReport::GetStackTraceString( JSContext* cx, JS::HandleObject exn )
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

void ExceptionToJsError( JSContext* cx )
{ // TODO: ask in IRC how to handle OOM errors and if it it's alright to throw C++ exceptions (remove catch(...) after that)
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

        pfc::string8_fast errorMsg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.ErrorMessage() );
        pfc::string8_fast errorSource8 = pfc::stringcvt::string_utf8_from_wide( e.Source().length() ? (const wchar_t*)e.Source() : L"" );
        pfc::string8_fast errorDesc8 = pfc::stringcvt::string_utf8_from_wide( e.Description().length() ? (const wchar_t*)e.Description() : L"" );
        JS_ReportErrorUTF8( cx, "COM error: message %s; source: %s; description: %s", errorMsg8.c_str(), errorSource8.c_str(), errorDesc8.c_str() );
    }
    catch ( const std::bad_alloc& )
    {
        JS_ClearPendingException( cx );
        JS_ReportAllocationOverflow( cx );
    }
    catch ( ... )
    {
        JS_ClearPendingException( cx );
        JS_ReportErrorUTF8( cx, "Uncaught internal exception" );
    }
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
