#include <stdafx.h>

#include "js_script_cache.h"

#include <convert/native_to_js.h>

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

RefPtr<JS::Stencil> JsScriptCache::GetCachedStencil( JSContext* pJsCtx, const std::filesystem::path& absolutePath, const JS::CompileOptions& compileOpts, bool isModule )
{
    assert( absolutePath.is_absolute() );

    const auto cleanPath = absolutePath.lexically_normal();
    const auto lastWriteTime = [&cleanPath] {
        try
        {
            return std::filesystem::last_write_time( cleanPath );
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
