#include <stdafx.h>

#include "js_error_helper.h"

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>
#include <js_objects/global_object.h>
#include <js_utils/js_property_helper.h>
#include <utils/scope_helpers.h>

#include <smp_exception.h>

using namespace smp;

namespace
{

using namespace mozjs;

std::u8string GetStackTraceString( JSContext* cx, JS::HandleObject exn )
{
    // Exceptions thrown while compiling top-level script have no stack.
    JS::RootedObject stackObj( cx, JS::ExceptionStackOrNull( exn ) );
    if ( !stackObj )
    { // quack?
        try
        { // Must not throw errors in error handler
            return GetOptionalProperty<std::u8string>( cx, exn, "stack" ).value_or( "" );
        }
        catch ( ... )
        {
            mozjs::error::SuppressException( cx );
            return "";
        }
    }

    JS::RootedString stackStr( cx );
    if ( !BuildStackString( cx, stackObj, &stackStr, 2 ) )
    {
        return "";
    }

    // JS::UniqueChars generates heap corruption exception in it's destructor
    const char* encodedString = JS_EncodeStringToUTF8( cx, stackStr );
    if ( !encodedString )
    {
        return "";
    }

    std::u8string outString( encodedString );
    JS_free( cx, (void*)encodedString );

    return outString;
}

bool PrependTextToJsStringException( JSContext* cx, JS::HandleValue excn, const std::u8string& text )
{
    std::u8string currentMessage;
    try
    { // Must not throw errors in error handler
        currentMessage = convert::to_native::ToValue<std::u8string>( cx, excn );
    }
    catch ( const smp::JsException& )
    {
        return false;
    }
    catch ( const SmpException& )
    {
        return false;
    }

    if ( currentMessage == std::u8string( "out of memory" ) )
    { // Can't modify the message since we're out of memory
        return true;
    }

    const std::u8string newMessage = [&text, &currentMessage] {
        std::u8string newMessage;
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
        convert::to_js::ToValue<std::u8string>( cx, newMessage, &jsMessage );
    }
    catch ( const smp::JsException& )
    {
        return false;
    }
    catch ( const SmpException& )
    {
        return false;
    }

    JS_SetPendingException( cx, jsMessage );
    return true;
}

bool PrependTextToJsObjectException( JSContext* cx, JS::HandleValue excn, const std::u8string& text )
{
    utils::final_action autoClearOnError{ [cx] { JS_ClearPendingException( cx ); } };

    JS::RootedObject excnObject( cx, &excn.toObject() );
    JS_ClearPendingException( cx ); ///< need this for js::ErrorReport::init

    js::ErrorReport report( cx );
    if ( !report.init( cx, excn, js::ErrorReport::SniffingBehavior::WithSideEffects ) )
    { // Sometimes happens with custom JS errors
        return false;
    }

    JSErrorReport* pReport = report.report();

    std::u8string currentMessage = pReport->message().c_str();
    const std::u8string newMessage = [&text, &currentMessage] {
        std::u8string newMessage;
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
        convert::to_js::ToValue<std::u8string>( cx, pReport->filename, &jsFilename );
        convert::to_js::ToValue<std::u8string>( cx, newMessage, &jsMessage );
    }
    catch ( ... )
    {
        mozjs::error::SuppressException( cx );
        return false;
    }

    JS::RootedObject excnStack( cx, JS::ExceptionStackOrNull( excnObject ) );
    JS::RootedValue newExcn( cx );
    JS::RootedString jsFilenameStr( cx, jsFilename.toString() );
    JS::RootedString jsMessageStr( cx, jsMessage.toString() );

    if ( !JS_WrapObject( cx, &excnStack ) )
    { // Need wrapping for the case when exception is thrown from internal global
        return false;
    }

    if ( !JS::CreateError( cx, static_cast<JSExnType>( pReport->exnType ), excnStack, jsFilenameStr, pReport->lineno, pReport->column, nullptr, jsMessageStr, &newExcn ) )
    {
        return false;
    }

    autoClearOnError.cancel();
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

    try
    {
        std::u8string errorText = JsErrorToText( cx );
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
    catch ( const std::exception& )
    {
    }
}

void AutoJsReport::Disable()
{
    isDisabled_ = true;
}

std::u8string JsErrorToText( JSContext* cx )
{
    assert( JS_IsExceptionPending( cx ) );

    JS::RootedValue excn( cx );
    (void)JS_GetPendingException( cx, &excn );
    JS_ClearPendingException( cx ); ///< need this for js::ErrorReport::init

    utils::final_action autoErrorClear( [cx]() { // There should be no exceptions on function exit
        JS_ClearPendingException( cx );
    } );

    std::u8string errorText;
    if ( excn.isString() )
    {
        try
        { // Must not throw errors in error handler
            errorText = convert::to_native::ToValue<std::u8string>( cx, excn );
        }
        catch ( ... )
        {
            mozjs::error::SuppressException( cx );
        }
    }
    else
    {
        js::ErrorReport report( cx );
        if ( !report.init( cx, excn, js::ErrorReport::SniffingBehavior::WithSideEffects ) )
        { // Sometimes happens with custom JS errors
            return errorText;
        }

        JSErrorReport* pReport = report.report();
        assert( !JSREPORT_IS_WARNING( pReport->flags ) );

        errorText = pReport->message().c_str();
        if ( pReport->filename )
        {
            errorText += "\n\n";
            errorText += fmt::format( "File: {}\n", pReport->filename );
            errorText += fmt::format( "Line: {}, Column: {}", std::to_string( pReport->lineno ), std::to_string( pReport->column ) );
            if ( pReport->linebufLength() )
            {
                errorText += "\n";
                errorText += "Source: ";
                errorText += [pReport] {
                    pfc::string8_fast tmpBuf = pfc::stringcvt::string_utf8_from_utf16( pReport->linebuf(), pReport->linebufLength() ).get_ptr();
                    tmpBuf.truncate_eol();
                    return tmpBuf;
                }();
            }
        }

        if ( excn.isObject() )
        {
            JS::RootedObject excnObject( cx, &excn.toObject() );
            std::u8string stackTrace = GetStackTraceString( cx, excnObject );
            if ( !stackTrace.empty() )
            {
                errorText += "\n\n";
                errorText += "Stack trace:\n";
                errorText += stackTrace;
            }
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
    catch ( const SmpException& e )
    {
        JS_ClearPendingException( cx );
        JS_ReportErrorUTF8( cx, e.what() );
    }
    catch ( const _com_error& e )
    {
        JS_ClearPendingException( cx );

        const auto errorMsg8 = smp::unicode::ToU8( std::wstring_view{ e.ErrorMessage() ? e.ErrorMessage() : L"<none>" } );
        const auto errorSource8 = smp::unicode::ToU8( std::wstring_view{ e.Source().length() ? static_cast<const wchar_t*>( e.Source() ) : L"<none>" } );
        const auto errorDesc8 = smp::unicode::ToU8( std::wstring_view{ e.Description().length() ? static_cast<const wchar_t*>( e.Description() ) : L"<none>" } );
        JS_ReportErrorUTF8( cx, fmt::format( "COM error: message {}; source: {}; description: {}", errorMsg8, errorSource8, errorDesc8 ).c_str() );
    }
    catch ( const std::bad_alloc& )
    {
        JS_ClearPendingException( cx );
        JS_ReportAllocationOverflow( cx );
    }
    // SM is not designed to handle uncaught exceptions, so we are risking here,
    // hoping that this exception will reach fb2k handler.
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
    catch ( const SmpException& )
    {
    }
    catch ( const _com_error& )
    {
    }
    catch ( const std::bad_alloc& )
    {
    }
    // SM is not designed to handle uncaught exceptions, so we are risking here,
    // hoping that this exception will reach fb2k handler.

    JS_ClearPendingException( cx );
}

void PrependTextToJsError( JSContext* cx, const std::u8string& text )
{
    utils::final_action autoJsReport( [cx, text] {
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
