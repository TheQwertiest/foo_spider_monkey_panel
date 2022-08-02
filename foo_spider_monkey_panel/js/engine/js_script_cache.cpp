#include <stdafx.h>

#include "js_script_cache.h"

#include <convert/native_to_js.h>

#include <js/Modules.h>
#include <js/experimental/JSStencil.h>
#include <js/utils/cached_utf8_paths_hack.h>
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

    auto pStencil = GetCachedStencil( pJsCtx, absolutePath, pathId, opts, false );
    assert( pStencil );

    JS::RootedScript jsScript( pJsCtx, JS::InstantiateGlobalStencil( pJsCtx, opts, pStencil ) );
    JsException::ExpectTrue( jsScript );

    return jsScript;
}

JSObject* JsScriptCache::GetCachedModule( JSContext* pJsCtx, const std::filesystem::path& absolutePath )
{
    assert( JS::GetCurrentRealmOrNull( pJsCtx ) );
    assert( absolutePath.is_absolute() );

    // use ids instead of filepaths to work around https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/1
    // and https://bugzilla.mozilla.org/show_bug.cgi?id=1492090
    const auto pathId = hack::CacheUtf8Path( absolutePath );

    JS::CompileOptions opts( pJsCtx );
    opts.setFileAndLine( pathId.c_str(), 1 );

    auto pStencil = GetCachedStencil( pJsCtx, absolutePath, pathId, opts, true );
    assert( pStencil );

    JS::RootedObject jsObject( pJsCtx, JS::InstantiateModuleStencil( pJsCtx, opts, pStencil ) );
    JsException::ExpectTrue( jsObject );

    std::wstring urlPath = L"file://" + absolutePath.wstring();
    std::replace( urlPath.begin(), urlPath.end(), '\\', '/' );

    JS::RootedValue jsScriptPath( pJsCtx );
    convert::to_js::ToValue( pJsCtx, urlPath, &jsScriptPath );
    JS::SetModulePrivate( jsObject, jsScriptPath );

    return jsObject;
}

RefPtr<JS::Stencil> JsScriptCache::GetCachedStencil( JSContext* pJsCtx, const std::filesystem::path& absolutePath, const std::string& hackedPathId, const JS::CompileOptions& compileOpts, bool isModule )
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

    auto& stencilCache = ( isModule ? moduleCache_ : scriptCache_ );
    if ( auto it = stencilCache.find( cleanPath.u8string() );
         stencilCache.cend() != it )
    {
        if ( it->second.writeTime == lastWriteTime )
        {
            return it->second.stencil;
        }
    }

    const auto scriptCode = qwr::file::ReadFile( cleanPath, CP_ACP, false );

    JS::SourceText<mozilla::Utf8Unit> source;
    if ( !source.init( pJsCtx, scriptCode.c_str(), scriptCode.length(), JS::SourceOwnership::Borrowed ) )
    {
        throw JsException();
    }

    RefPtr<JS::Stencil> stencil = ( isModule
                                        ? JS::CompileModuleScriptToStencil( pJsCtx, compileOpts, source )
                                        : JS::CompileGlobalScriptToStencil( pJsCtx, compileOpts, source ) );
    JsException::ExpectTrue( stencil );

    return stencilCache.insert_or_assign( cleanPath.u8string(), CachedStencil{ stencil, lastWriteTime } ).first->second.stencil;
}

} // namespace mozjs
