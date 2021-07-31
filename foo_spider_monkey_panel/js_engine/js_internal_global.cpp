#include <stdafx.h>

#include "js_internal_global.h"

#include <js_engine/js_realm_inner.h>
#include <js_utils/cached_utf8_paths_hack.h>

#include <qwr/file_helpers.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

using namespace smp;

namespace
{

using namespace mozjs;

void JsFinalizeOpLocal( JSFreeOp* /*fop*/, JSObject* obj )
{
    auto pJsRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( js::GetNonCCWObjectRealm( obj ) ) );
    if ( pJsRealm )
    {
        delete pJsRealm;
        JS::SetRealmPrivate( js::GetNonCCWObjectRealm( obj ), nullptr );
    }
}

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFinalizeOpLocal,
    nullptr,
    nullptr,
    nullptr,
    nullptr // set in runtime to JS_GlobalObjectTraceHook
};

JSClass jsClass = {
    "InternalGlobal",
    JSCLASS_GLOBAL_FLAGS | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};
} // namespace

namespace mozjs
{

JsInternalGlobal::JsInternalGlobal( JSContext* cx, JS::HandleObject global )
    : pJsCtx_( cx )
    , jsGlobal_( cx, global )
    , scriptCache_( cx )
{
    assert( global );
}

JsInternalGlobal::~JsInternalGlobal()
{
    scriptCache_.reset();
    jsGlobal_.reset();
}

std::unique_ptr<JsInternalGlobal> JsInternalGlobal::Create( JSContext* cx )
{
    if ( !jsOps.trace )
    { // JS_GlobalObjectTraceHook address is only accessible after mozjs is loaded.
        jsOps.trace = JS_GlobalObjectTraceHook;
    }

    JS::RealmOptions options;
    JS::RootedObject jsObj( cx,
                            JS_NewGlobalObject( cx, &jsClass, nullptr, JS::FireOnNewGlobalHook, options ) );
    JsException::ExpectTrue( jsObj );

    return std::unique_ptr<JsInternalGlobal>( new JsInternalGlobal( cx, jsObj ) );
}

JSScript* JsInternalGlobal::GetCachedScript( const std::filesystem::path& absolutePath )
{
    assert( absolutePath.is_absolute() );

    // Shared scripts must be saved in the shared global realm (vs their respective realms)
    // to prevent GC issues
    JSAutoRealm ac( pJsCtx_, jsGlobal_ );

    auto& scriptDataMap = scriptCache_.get().data;
    const auto cleanPath = absolutePath.lexically_normal();
    const auto lastWriteTime = [&absolutePath, &cleanPath]
    {
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

    if ( auto it = scriptDataMap.find( cleanPath.u8string() );
         scriptDataMap.cend() != it )
    {
        if ( it->second.writeTime == lastWriteTime )
        {
            return it->second.script;
        }
    }

    const std::wstring scriptCode = qwr::file::ReadFileW( cleanPath, CP_ACP, false );

    JS::SourceText<char16_t> source;
    if ( !source.init( pJsCtx_, reinterpret_cast<const char16_t*>( scriptCode.c_str() ), scriptCode.length(), JS::SourceOwnership::Borrowed ) )
    {
        throw JsException();
    }

    JS::CompileOptions opts( pJsCtx_ );
    const auto pathId = hack::CacheUtf8Path( absolutePath );
    // use ids instead of filepaths to work around https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/1
    // and https://bugzilla.mozilla.org/show_bug.cgi?id=1492090
    opts.setFileAndLine( pathId.c_str(), 1 );

    JS::RootedScript parsedScript( pJsCtx_, JS::Compile( pJsCtx_, opts, source ) );
    JsException::ExpectTrue( parsedScript );

    return scriptDataMap.insert_or_assign( cleanPath.u8string(), JsHashMap::ValueType{ parsedScript, lastWriteTime } ).first->second.script;
}

} // namespace mozjs
