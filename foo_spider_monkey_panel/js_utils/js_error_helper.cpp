#include <stdafx.h>
#include "js_error_helper.h"

#include <convert/native_to_js.h>
#include <convert/js_to_native.h>
#include <js_objects/global_object.h>
#include <js_utils/scope_helper.h>

#include <smp_exception.h>

namespace
{

using namespace mozjs;

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

bool PrependTextToJsStringException( JSContext* cx, JS::HandleValue excn, const pfc::string8_fast& text )
{
    pfc::string8_fast currentMessage;
    try
    { // Must not throw errors in error handler
        currentMessage = convert::to_native::ToValue<pfc::string8_fast>( cx, excn );
    }
    catch ( const smp::JsException& )
    {
        return false;
    }
    catch ( const smp::SmpException& )
    {
        return false;
    }

    if ( currentMessage == pfc::string8_fast( "out of memory" ) )
    { // Can't modify the message since we're out of memory
        return true;
    }

    const pfc::string8_fast newMessage = [&text, &currentMessage] {
        pfc::string8_fast newMessage;
        newMessage += text;
        if ( currentMessage.length() )
        {
            newMessage += ( ":\n" );
            newMessage += currentMessage;
        }

        return newMessage;
    }();

    JS::RootedValue jsMessage( cx );
    try
    { // Must not throw errors in error handler
        convert::to_js::ToValue<pfc::string8_fast>( cx, newMessage, &jsMessage );
    }
    catch ( const smp::JsException& )
    {
        return false;
    }
    catch ( const smp::SmpException& )
    {
        return false;
    }

    JS_SetPendingException( cx, jsMessage );
    return true;
}

bool PrependTextToJsObjectException( JSContext* cx, JS::HandleValue excn, const pfc::string8_fast& text )
{
    JS::RootedObject excnObject( cx, &excn.toObject() );

    JSErrorReport* report = JS_ErrorFromException( cx, excnObject );
    if ( !report )
    { // Sometimes happens with custom JS errors
        return false;
    }

    pfc::string8_fast currentMessage = report->message().c_str();
    const pfc::string8_fast newMessage = [&text, &currentMessage] {
        pfc::string8_fast newMessage;
        newMessage += text;
        if ( currentMessage.length() )
        {
            newMessage += ( ":\n" );
            newMessage += currentMessage;
        }

        return newMessage;
    }();

    JS::RootedValue jsFilename( cx );
    JS::RootedValue jsMessage( cx );
    try
    { // Must not throw errors in error handler
        convert::to_js::ToValue<pfc::string8_fast>( cx, report->filename, &jsFilename );
        convert::to_js::ToValue<pfc::string8_fast>( cx, newMessage, &jsMessage );
    }
    catch ( const smp::JsException& )
    {
        return false;
    }
    catch ( const smp::SmpException& )
    {
        return false;
    }

    JS::RootedObject excnStack( cx, JS::ExceptionStackOrNull( excnObject ) );
    JS::RootedValue newExcn( cx );
    JS::RootedString jsFilenameStr( cx, jsFilename.toString() );
    JS::RootedString jsMessageStr( cx, jsMessage.toString() );

    if ( !JS::CreateError( cx, (JSExnType)report->exnType, excnStack, jsFilenameStr, report->lineno, report->column, nullptr, jsMessageStr, &newExcn ) )
    {
        return false;
    }

    JS_SetPendingException( cx, newExcn );
    return true;
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

    pfc::string8_fast errorText = GetFullTextFromCurrentJsError( cx );
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

pfc::string8_fast GetFullTextFromCurrentJsError( JSContext* cx )
{
    assert( JS_IsExceptionPending( cx ) );

    JS::RootedValue excn( cx );
    (void)JS_GetPendingException( cx, &excn );

    pfc::string8_fast errorText;

    if ( excn.isString() )
    {
        try
        { // Must not throw errors in error handler
            errorText = convert::to_native::ToValue<pfc::string8_fast>( cx, excn );
        }
        catch ( const smp::JsException& )
        {
        }
        catch ( const smp::SmpException& )
        {
        }
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

void SuppressException( JSContext* cx )
{
    try
    {
        throw;
    }
    catch ( const smp::JsException& )
    {
    }
    catch ( const smp::SmpException& )
    {
    }
    catch ( const _com_error& )
    {
    }
    catch ( const std::bad_alloc& )
    {
    }
    // SM is not designed to handle uncaught exceptions, so we are risking here,
    // while hoping that this exception will reach fb2k handler.

    JS_ClearPendingException( cx );
}

void PrependTextToJsError( JSContext* cx, const pfc::string8_fast& text )
{
    scope::final_action autoJsReport( [cx, text]() {
        JS_ReportErrorUTF8( cx, "%s", text.c_str() );
    } );

    if ( !JS_IsExceptionPending( cx ) )
    {
        return;
    }

    // Get exception object before printing and clearing exception.
    JS::RootedValue excn( cx );
    (void)JS_GetPendingException( cx, &excn );

    if ( excn.isString() )
    {
        if ( PrependTextToJsStringException( cx, excn, text ) )
        {
            autoJsReport.cancel();
        }
        return;
    }
    else if ( excn.isObject() )
    {
        if ( PrependTextToJsObjectException( cx, excn, text ) )
        {
            autoJsReport.cancel();
        }
        return;
    }
}

} // namespace mozjs::error
