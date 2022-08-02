#include <stdafx.h>

#include "global_object.h"

#include <config/package_utils.h>
#include <js_backend/engine/js_container.h>
#include <js_backend/engine/js_engine.h>
#include <js_backend/engine/js_realm_inner.h>
#include <js_backend/engine/js_script_cache.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/core/global_heap_manager.h>
#include <js_backend/objects/dom/active_x_object.h>
#include <js_backend/objects/dom/console.h>
#include <js_backend/objects/dom/enumerator.h>
#include <js_backend/objects/dom/js_event.h>
#include <js_backend/objects/fb2k/fb_metadb_handle_list.h>
#include <js_backend/objects/fb2k/fb_playlist_manager.h>
#include <js_backend/objects/fb2k/fb_profiler.h>
#include <js_backend/objects/fb2k/fb_title_format.h>
#include <js_backend/objects/fb2k/fb_utils.h>
#include <js_backend/objects/gdi/gdi_bitmap.h>
#include <js_backend/objects/gdi/gdi_font.h>
#include <js_backend/objects/gdi/gdi_utils.h>
#include <js_backend/objects/utils.h>
#include <js_backend/objects/window.h>
#include <js_backend/utils/js_error_helper.h>
#include <js_backend/utils/js_object_helper.h>
#include <js_backend/utils/js_property_helper.h>
#include <panel/panel_window.h>
#include <utils/logging.h>

#include <component_paths.h>

#include <js/CompilationAndEvaluation.h>
#include <js/Modules.h>
#include <js/SourceText.h>
#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/string_helpers.h>

#include <filesystem>

namespace fs = std::filesystem;
using namespace smp;

namespace
{

template <typename T>
static T* GetNativeObjectProperty( JSContext* cx, JS::HandleObject self, const std::string& propName )
{
    JS::RootedValue jsPropertyValue( cx );
    if ( !JS_GetProperty( cx, self, propName.data(), &jsPropertyValue ) || !jsPropertyValue.isObject() )
    {
        return nullptr;
    }

    JS::RootedObject jsProperty( cx, &jsPropertyValue.toObject() );
    return JsObjectBase<T>::ExtractNative( cx, jsProperty );
}

template <typename T>
static void CleanupObjectProperty( JSContext* cx, JS::HandleObject self, const std::string& propName )
{
    auto pNative = GetNativeObjectProperty<T>( cx, self, propName );
    if ( pNative )
    {
        pNative->PrepareForGc();
    }
}

} // namespace

namespace
{

using namespace mozjs;

void JsFinalizeOpLocal( JSFreeOp* /*fop*/, JSObject* obj )
{
    auto x = static_cast<JsGlobalObject*>( JS::GetPrivate( obj ) );
    if ( x )
    {
        delete x;
        JS::SetPrivate( obj, nullptr );

        auto pJsRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( js::GetNonCCWObjectRealm( obj ) ) );
        if ( pJsRealm )
        {
            delete pJsRealm;
            JS::SetRealmPrivate( js::GetNonCCWObjectRealm( obj ), nullptr );
        }
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

constexpr JSClass jsClass = {
    "Global",
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS( static_cast<uint32_t>( JsPrototypeId::ProrototypeCount ) ) | JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT_FULL( IncludeScript, "include", JsGlobalObject::IncludeScript, JsGlobalObject::IncludeScriptWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( clearInterval, JsGlobalObject::ClearInterval )
MJS_DEFINE_JS_FN_FROM_NATIVE( clearTimeout, JsGlobalObject::ClearTimeout )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( setInterval, JsGlobalObject::SetInterval, JsGlobalObject::SetIntervalWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( setTimeout, JsGlobalObject::SetTimeout, JsGlobalObject::SetTimeoutWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>( {
    JS_FN( "clearInterval", clearInterval, 1, kDefaultPropsFlags ),
    JS_FN( "clearTimeout", clearTimeout, 1, kDefaultPropsFlags ),
    JS_FN( "include", IncludeScript, 1, kDefaultPropsFlags ),
    JS_FN( "setInterval", setInterval, 2, kDefaultPropsFlags ),
    JS_FN( "setTimeout", setTimeout, 2, kDefaultPropsFlags ),
    JS_FS_END,
} );

} // namespace

namespace mozjs
{

const JSClass& JsGlobalObject::JsClass = jsClass;

JsGlobalObject::JsGlobalObject( JSContext* cx, JsContainer& parentContainer, JsWindow* pJsWindow )
    : pJsCtx_( cx )
    , parentContainer_( parentContainer )
    , pJsWindow_( pJsWindow )
    , scriptLoader_( cx )
{
}

JSObject* JsGlobalObject::CreateNative( JSContext* cx, JsContainer& parentContainer )
{
    if ( !jsOps.trace )
    { // JS_GlobalObjectTraceHook address is only accessible after mozjs is loaded.
        jsOps.trace = JS_GlobalObjectTraceHook;
    }

    JS::RealmCreationOptions creationOptions;
    creationOptions.setTrace( JsGlobalObject::Trace );
    JS::RealmOptions options( creationOptions, JS::RealmBehaviors{} );
    JS::RootedObject jsObj( cx,
                            JS_NewGlobalObject( cx, &jsClass, nullptr, JS::DontFireOnNewGlobalHook, options ) );
    if ( !jsObj )
    {
        throw JsException();
    }

    {
        JSAutoRealm ac( cx, jsObj );
        JS::SetRealmPrivate( js::GetContextRealm( cx ), new JsRealmInner() );

        if ( !JS::InitRealmStandardClasses( cx ) )
        {
            throw JsException();
        }

        DefineConsole( cx, jsObj );

#ifdef SMP_V2
        utils::CreateAndInstallPrototype<JsEvent>( cx, JsPrototypeId::Event );
        utils::CreateAndInstallPrototype<JsEventTarget>( cx, JsPrototypeId::EventTarget );
#endif

        CreateAndInstallObject<JsGdiUtils>( cx, jsObj, "gdi" );
        CreateAndInstallObject<JsFbPlaylistManager>( cx, jsObj, "plman" );
        CreateAndInstallObject<JsUtils>( cx, jsObj, "utils" );
        CreateAndInstallObject<JsFbUtils>( cx, jsObj, "fb" );
        CreateAndInstallObject<JsWindow>( cx, jsObj, "window", parentContainer.GetParentPanel() );

        if ( !JS_DefineFunctions( cx, jsObj, jsFunctions.data() ) )
        {
            throw JsException();
        }

#ifdef _DEBUG
        JS::RootedObject testFuncs( cx, js::GetTestingFunctions( cx ) );
        if ( !JS_DefineProperty( cx, jsObj, "test", testFuncs, kDefaultPropsFlags ) )
        {
            throw JsException();
        }
#endif

        utils::CreateAndInstallPrototype<JsActiveXObject>( cx, JsPrototypeId::ActiveX );
        utils::CreateAndInstallPrototype<JsGdiBitmap>( cx, JsPrototypeId::GdiBitmap );
        utils::CreateAndInstallPrototype<JsGdiFont>( cx, JsPrototypeId::GdiFont );
        utils::CreateAndInstallPrototype<JsEnumerator>( cx, JsPrototypeId::Enumerator );
        utils::CreateAndInstallPrototype<JsFbMetadbHandleList>( cx, JsPrototypeId::FbMetadbHandleList );
        utils::CreateAndInstallPrototype<JsFbProfiler>( cx, JsPrototypeId::FbProfiler );
        utils::CreateAndInstallPrototype<JsFbTitleFormat>( cx, JsPrototypeId::FbTitleFormat );

        auto pJsWindow = GetNativeObjectProperty<JsWindow>( cx, jsObj, "window" );
        assert( pJsWindow );

        auto pNative = std::unique_ptr<JsGlobalObject>( new JsGlobalObject( cx, parentContainer, pJsWindow ) );
        pNative->heapManager_ = GlobalHeapManager::Create( cx );
        assert( pNative->heapManager_ );

        JS::SetPrivate( jsObj, pNative.release() );

        JS_FireOnNewGlobalObject( cx, jsObj );
    }

    return jsObj;
}

JsGlobalObject* JsGlobalObject::ExtractNative( JSContext* cx, JS::HandleObject jsObject )
{
    return static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsObject, &mozjs::JsGlobalObject::JsClass, nullptr ) );
}

void JsGlobalObject::Fail( const qwr::u8string& errorText )
{
    parentContainer_.Fail( errorText );
}

GlobalHeapManager& JsGlobalObject::GetHeapManager() const
{
    assert( heapManager_ );
    return *heapManager_;
}

void JsGlobalObject::PrepareForGc( JSContext* cx, JS::HandleObject self )
{
    auto pNativeGlobal = JsGlobalObject::ExtractNative( cx, self );
    assert( pNativeGlobal );

    pNativeGlobal->scriptLoader_.PrepareForGc();

    CleanupObjectProperty<JsWindow>( cx, self, "window" );
    CleanupObjectProperty<JsFbPlaylistManager>( cx, self, "plman" );

    if ( pNativeGlobal->heapManager_ )
    {
        pNativeGlobal->heapManager_->PrepareForGc();
        pNativeGlobal->heapManager_.reset();
    }
}

ScriptLoader& JsGlobalObject::GetScriptLoader()
{
    return scriptLoader_;
}

HWND JsGlobalObject::GetPanelHwnd() const
{
    assert( pJsWindow_ );
    return pJsWindow_->GetHwnd();
}

void JsGlobalObject::ClearInterval( uint32_t intervalId )
{
    assert( pJsWindow_ );
    pJsWindow_->ClearInterval( intervalId );
}

void JsGlobalObject::ClearTimeout( uint32_t timeoutId )
{
    assert( pJsWindow_ );
    pJsWindow_->ClearInterval( timeoutId );
}

void JsGlobalObject::IncludeScript( const qwr::u8string& path, JS::HandleValue options )
{
    const auto parsedOptions = ParseIncludeOptions( options );
    scriptLoader_.IncludeScript( path, parentContainer_.GetParentPanel().GetSettings(), parsedOptions.alwaysEvaluate );
}

void JsGlobalObject::IncludeScriptWithOpt( size_t optArgCount, const qwr::u8string& path, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return IncludeScript( path, options );
    case 1:
        return IncludeScript( path );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint32_t JsGlobalObject::SetInterval( JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs )
{
    return pJsWindow_->SetInterval( func, delay, funcArgs );
}

uint32_t JsGlobalObject::SetIntervalWithOpt( size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs )
{
    return pJsWindow_->SetIntervalWithOpt( optArgCount, func, delay, funcArgs );
}

uint32_t JsGlobalObject::SetTimeout( JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs )
{
    return pJsWindow_->SetTimeout( func, delay, funcArgs );
}

uint32_t JsGlobalObject::SetTimeoutWithOpt( size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs )
{
    return pJsWindow_->SetTimeoutWithOpt( optArgCount, func, delay, funcArgs );
}

JsGlobalObject::IncludeOptions JsGlobalObject::ParseIncludeOptions( JS::HandleValue options )
{
    IncludeOptions parsedOptions;
    if ( !options.isNullOrUndefined() )
    {
        qwr::QwrException::ExpectTrue( options.isObject(), "options argument is not an object" );
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );

        if ( const auto propOpt = utils::GetOptionalProperty<bool>( pJsCtx_, jsOptions, "always_evaluate" ) )
        {
            parsedOptions.alwaysEvaluate = *propOpt;
        }
    }

    return parsedOptions;
}

void JsGlobalObject::Trace( JSTracer* trc, JSObject* obj )
{
    auto pNative = static_cast<JsGlobalObject*>( JS::GetPrivate( obj ) );
    if ( !pNative )
    {
        return;
    }

    if ( pNative->heapManager_ )
    {
        pNative->heapManager_->Trace( trc );
    }

    pNative->scriptLoader_.Trace( trc );
}

} // namespace mozjs
