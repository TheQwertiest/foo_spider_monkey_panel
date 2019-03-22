#include <stdafx.h>
#include "global_object.h"

#include <js_engine/js_container.h>
#include <js_engine/js_compartment_inner.h>
#include <js_engine/js_engine.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_engine/js_internal_global.h>
#include <js_objects/active_x_object.h>
#include <js_objects/console.h>
#include <js_objects/enumerator.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/fb_playlist_manager.h>
#include <js_objects/fb_profiler.h>
#include <js_objects/fb_title_format.h>
#include <js_objects/fb_utils.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/gdi_utils.h>
#include <js_objects/hacks.h>
#include <js_objects/utils.h>
#include <js_objects/window.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_property_helper.h>
#include <utils/file_helpers.h>
#include <utils/scope_helpers.h>

#include <js_panel_window.h>
#include <component_paths.h>

#include <filesystem>

using namespace smp;

namespace
{

using namespace mozjs;

void JsFinalizeOpLocal( JSFreeOp* /*fop*/, JSObject* obj )
{
    auto x = static_cast<JsGlobalObject*>( JS_GetPrivate( obj ) );
    if ( x )
    {
        delete x;
        JS_SetPrivate( obj, nullptr );

        auto pJsCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( js::GetObjectCompartment( obj ) ) );
        if ( pJsCompartment )
        {
            delete pJsCompartment;
            JS_SetCompartmentPrivate( js::GetObjectCompartment( obj ), nullptr );
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

JSClass jsClass = {
    "Global",
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS( static_cast<uint32_t>( JsPrototypeId::ProrototypeCount ) ) | JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

// Defining function manually, because we want a proper logging and we can't name it `include`
// TODO: define a new macro?
bool IncludeScript( JSContext* cx, unsigned argc, JS::Value* vp )
{
    const auto wrappedFunc = []( JSContext* cx, unsigned argc, JS::Value* vp ) {
        InvokeNativeCallback<1>( cx, &JsGlobalObject::IncludeScript, &JsGlobalObject::IncludeScriptWithOpt, argc, vp );
    };
    return mozjs::error::Execute_JsSafe( cx, "include", wrappedFunc, argc, vp );
}

MJS_DEFINE_JS_FN_FROM_NATIVE( ClearInterval, JsGlobalObject::ClearInterval )
MJS_DEFINE_JS_FN_FROM_NATIVE( ClearTimeout, JsGlobalObject::ClearTimeout )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetInterval, JsGlobalObject::SetInterval )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetTimeout, JsGlobalObject::SetTimeout )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "clearInterval", ClearInterval, 1, DefaultPropsFlags() ),
    JS_FN( "clearTimeout", ClearTimeout, 1, DefaultPropsFlags() ),
    JS_FN( "include", IncludeScript, 1, DefaultPropsFlags() ),
    JS_FN( "setInterval", SetInterval, 2, DefaultPropsFlags() ),
    JS_FN( "setTimeout", SetTimeout, 2, DefaultPropsFlags() ),
    JS_FS_END
};

} // namespace

namespace mozjs
{

const JSClass& JsGlobalObject::JsClass = jsClass;

JsGlobalObject::JsGlobalObject( JSContext* cx, JsContainer& parentContainer, JsWindow* pJsWindow )
    : pJsCtx_( cx )
    , parentContainer_( parentContainer )
    , pJsWindow_( pJsWindow )
{
}

JsGlobalObject::~JsGlobalObject()
{ // No need to cleanup JS here, since it must be performed manually beforehand anyway
}

// TODO: remove js_panel_window from ctor (add a method to JsContainer instead)
JSObject* JsGlobalObject::CreateNative( JSContext* cx, JsContainer& parentContainer, smp::panel::js_panel_window& parentPanel )
{
    if ( !jsOps.trace )
    { // JS_GlobalObjectTraceHook address is only accessible after mozjs is loaded.
        jsOps.trace = JS_GlobalObjectTraceHook;
    }

    JS::CompartmentOptions options;
    JS::RootedObject jsObj( cx,
                            JS_NewGlobalObject( cx, &jsClass, nullptr, JS::DontFireOnNewGlobalHook, options ) );
    if ( !jsObj )
    {
        throw smp::JsException();
    }

    {
        JSAutoCompartment ac( cx, jsObj );
        JS_SetCompartmentPrivate( js::GetContextCompartment( cx ), new JsCompartmentInner() );

        if ( !JS_InitStandardClasses( cx, jsObj ) )
        {
            throw smp::JsException();
        }

        DefineConsole( cx, jsObj );
        CreateAndInstallObject<JsGdiUtils>( cx, jsObj, "gdi" );
        CreateAndInstallObject<JsFbPlaylistManager>( cx, jsObj, "plman" );
        CreateAndInstallObject<JsUtils>( cx, jsObj, "utils" );
        CreateAndInstallObject<JsFbUtils>( cx, jsObj, "fb" );
        CreateAndInstallObject<JsWindow>( cx, jsObj, "window", parentPanel );
        // CreateAndInstallObject<JsHacks>( cx, jsObj, "hacks" ) ;

        if ( !JS_DefineFunctions( cx, jsObj, jsFunctions ) )
        {
            throw smp::JsException();
        }

#ifdef _DEBUG
        JS::RootedObject testFuncs( cx, js::GetTestingFunctions( cx ) );
        if ( !JS_DefineProperty( cx, jsObj, "test", testFuncs, DefaultPropsFlags() ) )
        {
            throw smp::JsException();
        }
#endif

        CreateAndInstallPrototype<ActiveXObject>( cx, JsPrototypeId::ActiveX );
        CreateAndInstallPrototype<JsGdiBitmap>( cx, JsPrototypeId::GdiBitmap );
        CreateAndInstallPrototype<JsEnumerator>( cx, JsPrototypeId::Enumerator );
        CreateAndInstallPrototype<JsFbMetadbHandleList>( cx, JsPrototypeId::FbMetadbHandleList );
        CreateAndInstallPrototype<JsFbProfiler>( cx, JsPrototypeId::FbProfiler );
        CreateAndInstallPrototype<JsFbTitleFormat>( cx, JsPrototypeId::FbTitleFormat );

        auto pJsWindow = GetNativeObjectProperty<JsWindow>( cx, jsObj, "window" );
        assert( pJsWindow );

        auto pNative = std::unique_ptr<JsGlobalObject>( new JsGlobalObject( cx, parentContainer, pJsWindow ) );
        pNative->heapManager_ = GlobalHeapManager::Create( cx );

        JS_SetPrivate( jsObj, pNative.release() );

        JS_FireOnNewGlobalObject( cx, jsObj );
    }

    return jsObj;
}

void JsGlobalObject::Fail( const pfc::string8_fast& errorText )
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
    auto nativeGlobal = static_cast<JsGlobalObject*>( JS_GetInstancePrivate( cx, self, &JsGlobalObject::JsClass, nullptr ) );
    assert( nativeGlobal );

    CleanupObjectProperty<JsWindow>( cx, self, "window" );
    CleanupObjectProperty<JsFbPlaylistManager>( cx, self, "plman" );

    nativeGlobal->heapManager_.reset();
}

void JsGlobalObject::ClearInterval( uint32_t intervalId )
{
    pJsWindow_->ClearInterval( intervalId );
}

void JsGlobalObject::ClearTimeout( uint32_t timeoutId )
{
    pJsWindow_->ClearInterval( timeoutId );
}

void JsGlobalObject::IncludeScript( const pfc::string8_fast& path, JS::HandleValue options )
{
    namespace fs = std::filesystem;

    const fs::path fsPath = [&path, &parentFilepaths = parentFilepaths_] {
        try
        {
            fs::path fsPath = fs::u8path( path.c_str() );
            if ( fsPath.is_relative() )
            {
                if ( parentFilepaths.empty() )
                {
                    fsPath = fs::u8path( get_fb2k_component_path().c_str() ) / fsPath;
                }
                else
                {
                    fsPath = fs::u8path( parentFilepaths.back() ) / fsPath;
                }
            }

            SmpException::ExpectTrue( fs::exists( fsPath ) && fs::is_regular_file( fsPath ), "Path does not point to a valid file: {}", path.c_str() );

            return fsPath.lexically_normal();
        }
        catch ( const fs::filesystem_error& e )
        {
            throw SmpException( fmt::format( "Failed to open file `{}`: {}", path.c_str(), e.what() ) );
        }
    }();

    const auto parsedOptions = ParseIncludeOptions( options );

    const auto u8Path = fsPath.u8string();
    if ( !parsedOptions.alwaysEvaluate && includedFiles_.count( u8Path ) )
    {
        return;
    }

    includedFiles_.emplace( u8Path );
    parentFilepaths_.emplace_back( fsPath.parent_path().u8string() );
    smp::utils::final_action autoPath{ [&parentFilesPaths = parentFilepaths_] { parentFilesPaths.pop_back(); } };

    JS::RootedScript jsScript( pJsCtx_, JsEngine::GetInstance().GetInternalGlobal().GetCachedScript( fsPath ) );
    assert( jsScript );

    JS::RootedValue dummyRval( pJsCtx_ );
    if ( !JS::CloneAndExecuteScript( pJsCtx_, jsScript, &dummyRval ) )
    {
        throw smp::JsException();
    }
}

void JsGlobalObject::IncludeScriptWithOpt( size_t optArgCount, const pfc::string8_fast& path, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return IncludeScript( path, options );
    case 1:
        return IncludeScript( path );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}


uint32_t JsGlobalObject::SetInterval( JS::HandleValue func, uint32_t delay )
{
    return pJsWindow_->SetInterval( func, delay );
}

uint32_t JsGlobalObject::SetTimeout( JS::HandleValue func, uint32_t delay )
{
    return pJsWindow_->SetTimeout( func, delay );
}

JsGlobalObject::IncludeOptions JsGlobalObject::ParseIncludeOptions( JS::HandleValue options )
{
    IncludeOptions parsedOptions;
    if ( !options.isNullOrUndefined() )
    {
        SmpException::ExpectTrue( options.isObject(), "options argument is not an object" );
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );

        parsedOptions.alwaysEvaluate = GetOptionalProperty<bool>( pJsCtx_, jsOptions, "always_evaluate" ).value_or( false );
    }

    return parsedOptions;
}

} // namespace mozjs
