#include <stdafx.h>

#include "current_script_path_hack.h"

#include <js_utils/cached_utf8_paths_hack.h>

namespace mozjs::hack
{

std::optional<std::filesystem::path> GetCurrentScriptPath( JSContext* cx )
{
    try
    {
        JS_ReportErrorUTF8( cx, "hacking around..." );
        assert( JS_IsExceptionPending( cx ) );

        JS::RootedValue excn( cx );
        (void)JS_GetPendingException( cx, &excn );
        JS_ClearPendingException( cx );

        assert( excn.isObject() );
        JS::RootedObject excnObject( cx, &excn.toObject() );

        js::ErrorReport report( cx );
        if ( !report.init( cx, excn, js::ErrorReport::SniffingBehavior::WithSideEffects ) )
        {
            throw qwr::QwrException( "js::ErrorReport::init failed" );
        }

        JSErrorReport* pReport = report.report();
        assert( pReport );

        if ( !pReport->filename || qwr::u8string{ pReport->filename }.empty() )
        {
            return std::nullopt;
        }

        // workaround for https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/1
        // and https://bugzilla.mozilla.org/show_bug.cgi?id=1492090
        const auto pathOpt = hack::GetCachedUtf8Path( pReport->filename );
        if ( !pathOpt )
        {
            return std::nullopt;
        }

        return pathOpt;
    }
    catch ( const std::filesystem::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace mozjs::hack
