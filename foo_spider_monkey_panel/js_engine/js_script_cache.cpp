#include <stdafx.h>

#include "js_script_cache.h"

#include <js_utils/cached_utf8_paths_hack.h>

#include <js/experimental/JSStencil.h>
#include <qwr/file_helpers.h>

using namespace smp;

namespace mozjs
{

JsScriptCache::JsScriptCache()
{
}

JsScriptCache::~JsScriptCache()
{
}

JSScript* JsScriptCache::GetCachedScript( JSContext* pJsCtx, const std::filesystem::path& absolutePath )
{
    assert( JS::GetCurrentRealmOrNull( pJsCtx ) );
    assert( absolutePath.is_absolute() );

    // use ids instead of filepaths to work around https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/1
    // and https://bugzilla.mozilla.org/show_bug.cgi?id=1492090
    const auto pathId = hack::CacheUtf8Path( absolutePath );

    JS::CompileOptions opts( pJsCtx );
    opts.setFileAndLine( pathId.c_str(), 1 );

    auto pStencil = GetCachedStencil( pJsCtx, absolutePath, pathId, opts );
    assert( pStencil );

    JS::RootedScript jsScript( pJsCtx, JS::InstantiateGlobalStencil( pJsCtx, opts, pStencil ) );
    JsException::ExpectTrue( jsScript );

    return jsScript;
}

RefPtr<JS::Stencil> JsScriptCache::GetCachedStencil( JSContext* pJsCtx, const std::filesystem::path& absolutePath, const std::string& hackedPathId, const JS::CompileOptions& compileOpts )
{
    const auto cleanPath = absolutePath.lexically_normal();
    const auto lastWriteTime = [&absolutePath, &cleanPath] {
        try
        {
            return std::filesystem::last_write_time( absolutePath );
        }
        catch ( const std::filesystem::filesystem_error& e )
        {
            throw qwr::QwrException( "Failed to open file `{}`:\n"
                                     "  {}",
                                     cleanPath.u8string(),
                                     qwr::unicode::ToU8_FromAcpToWide( e.what() ) );
        }
    }();

    if ( auto it = scriptCache_.find( cleanPath.u8string() );
         scriptCache_.cend() != it )
    {
        if ( it->second.writeTime == lastWriteTime )
        {
            return it->second.scriptStencil;
        }
    }

    const auto scriptCode = qwr::file::ReadFile( cleanPath, CP_ACP, false );

    JS::SourceText<mozilla::Utf8Unit> source;
    if ( !source.init( pJsCtx, scriptCode.c_str(), scriptCode.length(), JS::SourceOwnership::Borrowed ) )
    {
        throw JsException();
    }

    RefPtr<JS::Stencil> scriptStencil =
        JS::CompileGlobalScriptToStencil( pJsCtx, compileOpts, source );
    JsException::ExpectTrue( scriptStencil );

    return scriptCache_.insert_or_assign( cleanPath.u8string(), CachedScriptStencil{ scriptStencil, lastWriteTime } ).first->second.scriptStencil;
}

} // namespace mozjs
