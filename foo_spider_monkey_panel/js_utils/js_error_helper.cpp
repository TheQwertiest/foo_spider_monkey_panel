#include <stdafx.h>

#include "js_error_helper.h"

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>
#include <js_objects/global_object.h>
#include <js_utils/cached_utf8_paths_hack.h>
#include <js_utils/js_property_helper.h>

#include <qwr/final_action.h>
#include <qwr/string_helpers.h>

using namespace smp;

namespace
{

using namespace mozjs;

qwr::u8string GetStackTraceString( JSContext* cx, JS::HandleObject exn )
{
    try
    { // Must not throw errors in error handler
        // Note: exceptions thrown while compiling top-level script have no stack.
        JS::RootedObject stackObj( cx, JS::ExceptionStackOrNull( exn ) );
        if ( !stackObj )
        { // quack?
            return GetOptionalProperty<qwr::u8string>( cx, exn, "stack" ).value_or( "" );
        }

        JS::RootedString stackStr( cx );
        if ( !JS::BuildStackString( cx, nullptr, stackObj, &stackStr, 2 ) )
        {
            return "";
        }

        const auto& cachedPaths = hack::GetAllCachedUtf8Paths();
        auto stdStackStr = mozjs::convert::to_native::ToValue<qwr::u8string>( cx, stackStr );

        for ( const auto& [id, path]: cachedPaths )
        { // replace ids with paths
            const auto filename = path.filename().u8string();
            size_t pos = 0;
            while ( ( pos = stdStackStr.find( id, pos ) ) != std::string::npos )
            {
                stdStackStr.replace( pos, id.length(), filename );
                pos += filename.length();
            }
        }

        return stdStackStr;
    }
    catch ( ... )
    {
        mozjs::error::SuppressException( cx );
        return "";
    }
}

bool PrependTextToJsStringException( JSContext* cx, JS::HandleValue excn, const qwr::u8string& text )
{
    qwr::u8string currentMessage;
    try
    { // Must not throw errors in error handler
        currentMessage = convert::to_native::ToValue<qwr::u8string>( cx, excn );
    }
    catch ( const JsException& )
    {
        return false;
    }
    catch ( const qwr::QwrException& )
    {
        return false;
    }

    if ( currentMessage == qwr::u8string( "out of memory" ) )
    { // Can't modify the message since we're out of memory
        return true;
    }

    const qwr::u8string newMessage = [&text, &currentMessage] {
        qwr::u8string newMessage;
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
        convert::to_js::ToValue<qwr::u8string>( cx, newMessage, &jsMessage );
    }
    catch ( const JsException& )
    {
        return false;
    }
    catch ( const qwr::QwrException& )
    {
        return false;
    }

    JS_SetPendingException( cx, jsMessage );
    return true;
}

bool PrependTextToJsObjectException( JSContext* cx, JS::HandleValue excn, const qwr::u8string& text )
{
    qwr::final_action autoClearOnError{ [cx] { JS_ClearPendingException( cx ); } };

    JS::RootedObject excnObject( cx, &excn.toObject() );
    JS_ClearPendingException( cx ); ///< need this for js::ErrorReport::init

    js::ErrorReport report( cx );
    if ( !report.init( cx, excn, js::ErrorReport::SniffingBehavior::WithSideEffects ) )
    { // Sometimes happens with custom JS errors
        return false;
    }

    JSErrorReport* pReport = report.report();

    qwr::u8string currentMessage = pReport->message().c_str();
    const qwr::u8string newMessage = [&text, &currentMessage] {
        qwr::u8string newMessage;
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
        convert::to_js::ToValue<qwr::u8string>( cx, pReport->filename, &jsFilename );
        convert::to_js::ToValue<qwr::u8string>( cx, newMessage, &jsMessage );
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

AutoJsReport::~AutoJsReport() noexcept
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
        qwr::u8string errorText = JsErrorToText( cx );
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

qwr::u8string JsErrorToText( JSContext* cx )
{
    assert( JS_IsExceptionPending( cx ) );

    JS::RootedValue excn( cx );
    (void)JS_GetPendingException( cx, &excn );
    JS_ClearPendingException( cx ); ///< need this for js::ErrorReport::init

    qwr::final_action autoErrorClear( [cx]() { // There should be no exceptions on function exit
        JS_ClearPendingException( cx );
    } );

    qwr::u8string errorText;
    if ( excn.isString() )
    {
        try
        { // Must not throw errors in error handler
            errorText = convert::to_native::ToValue<qwr::u8string>( cx, excn );
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

        const auto additionalInfo = [&] {
            qwr::u8string additionalInfo;

            if ( pReport->filename )
            {
                const auto stdPath = [&]() -> qwr::u8string {
                    if ( qwr::u8string{ pReport->filename } == "" )
                    {
                        return "<main>";
                    }

                    const auto pathOpt = hack::GetCachedUtf8Path( pReport->filename );
                    if ( pathOpt )
                    {
                        return pathOpt->filename().u8string();
                    }

                    return pReport->filename;
                }();

                additionalInfo += "\n";
                additionalInfo += fmt::format( "File: {}\n", stdPath );
                additionalInfo += fmt::format( "Line: {}, Column: {}", std::to_string( pReport->lineno ), std::to_string( pReport->column ) );
                if ( pReport->linebufLength() )
                {
                    additionalInfo += "\n";
                    additionalInfo += "Source: ";
                    additionalInfo += [pReport] {
                        pfc::string8_fast tmpBuf = pfc::stringcvt::string_utf8_from_utf16( pReport->linebuf(), pReport->linebufLength() ).get_ptr();
                        tmpBuf.truncate_eol();
                        return tmpBuf;
                    }();
                }
            }

            if ( excn.isObject() )
            {
                JS::RootedObject excnObject( cx, &excn.toObject() );
                qwr::u8string stackTrace = GetStackTraceString( cx, excnObject );
                if ( !stackTrace.empty() )
                {
                    additionalInfo += "\n";
                    additionalInfo += "Stack trace:\n";
                    additionalInfo += stackTrace;
                }
            }

            return additionalInfo;
        }();

        if ( !additionalInfo.empty() )
        {
            errorText += "\n";
            errorText += additionalInfo;
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
    catch ( const JsException& )
    {
        assert( JS_IsExceptionPending( cx ) );
    }
    catch ( const qwr::QwrException& e )
    {
        JS_ClearPendingException( cx );
        JS_ReportErrorUTF8( cx, e.what() );
    }
    catch ( const _com_error& e )
    {
        JS_ClearPendingException( cx );

        const auto errorMsg8 = qwr::unicode::ToU8( std::wstring_view{ e.ErrorMessage() ? e.ErrorMessage() : L"<none>" } );
        const auto errorSource8 = qwr::unicode::ToU8( std::wstring_view{ e.Source().length() ? static_cast<const wchar_t*>( e.Source() ) : L"<none>" } );
        const auto errorDesc8 = qwr::unicode::ToU8( std::wstring_view{ e.Description().length() ? static_cast<const wchar_t*>( e.Description() ) : L"<none>" } );
        JS_ReportErrorUTF8( cx,
                            fmt::format( "COM error:\n"
                                         "  hresult: {:#x}\n"
                                         "  message: {}\n"
                                         "  description: {}\n"
                                         "  source: {}",
                                         static_cast<uint32_t>( e.Error() ),
                                         errorMsg8,
                                         errorDesc8,
                                         errorSource8 )
                                .c_str() );
    }
    catch ( const std::bad_alloc& )
    {
        JS_ClearPendingException( cx );
        JS_ReportAllocationOverflow( cx );
    }
    // SM is not designed to handle uncaught exceptions, so we are risking here,
    // hoping that this exception will reach fb2k handler.
}

qwr::u8string ExceptionToText( JSContext* cx )
{
    try
    {
        throw;
    }
    catch ( const JsException& )
    {
        return JsErrorToText( cx );
    }
    catch ( const qwr::QwrException& e )
    {
        JS_ClearPendingException( cx );
        return e.what();
    }
    catch ( const _com_error& e )
    {
        JS_ClearPendingException( cx );

        const auto errorMsg8 = qwr::unicode::ToU8( std::wstring_view{ e.ErrorMessage() ? e.ErrorMessage() : L"<none>" } );
        const auto errorSource8 = qwr::unicode::ToU8( std::wstring_view{ e.Source().length() ? static_cast<const wchar_t*>( e.Source() ) : L"<none>" } );
        const auto errorDesc8 = qwr::unicode::ToU8( std::wstring_view{ e.Description().length() ? static_cast<const wchar_t*>( e.Description() ) : L"<none>" } );
        return fmt::format( "COM error:\n"
                            "  hresult: {:#x}\n"
                            "  message: {}\n"
                            "  description: {}\n"
                            "  source: {}",
                            static_cast<uint32_t>( e.Error() ),
                            errorMsg8,
                            errorDesc8,
                            errorSource8 );
    }
    catch ( const std::bad_alloc& e )
    {
        JS_ClearPendingException( cx );
        return e.what();
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
    catch ( const JsException& )
    {
    }
    catch ( const qwr::QwrException& )
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

void PrependTextToJsError( JSContext* cx, const qwr::u8string& text )
{
    qwr::final_action autoJsReport( [cx, text] {
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
