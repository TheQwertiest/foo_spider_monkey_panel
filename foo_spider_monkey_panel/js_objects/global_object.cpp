#include <stdafx.h>
#include "global_object.h"

#include <js_engine/js_container.h>
#include <js_engine/js_compartment_inner.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/active_x_object.h>
#include <js_objects/console.h>
#include <js_objects/fb_playlist_manager.h>
#include <js_objects/fb_utils.h>
#include <js_objects/gdi_utils.h>
#include <js_objects/utils.h>
#include <js_objects/window.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/file_helpers.h>

#include <js_panel_window.h>
#include <helpers.h>

#include <filesystem>
#include <fstream>  

namespace
{

using namespace mozjs;

void JsFinalizeOpLocal( JSFreeOp* fop, JSObject* obj )
{
    auto x = static_cast<JsGlobalObject*>(JS_GetPrivate( obj ));
    if ( x )
    {
        delete x;
        JS_SetPrivate( obj, nullptr );

        JS_SetCompartmentPrivate( js::GetObjectCompartment( obj ), nullptr );
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
     JSCLASS_GLOBAL_FLAGS_WITH_SLOTS( static_cast<uint32_t>(JsPrototypeId::ProrototypeCount) ) | JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
     &jsOps
};

// Defining function manually, because we won't a proper logging and we can't name it `include`
// TODO: define a new macro?
bool IncludeScript( JSContext* cx, unsigned argc, JS::Value* vp )
{
    bool bRet =
        InvokeNativeCallback<0>( cx, &JsGlobalObject::IncludeScript, &JsGlobalObject::IncludeScript, argc, vp );
    if ( !bRet )
    {
        mozjs::RethrowExceptionWithFunctionName( cx, "include" );
    }
    return bRet;
}

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "include", IncludeScript, 1, DefaultPropsFlags() ),
    JS_FS_END
};


}

namespace mozjs
{

const JSClass& JsGlobalObject::JsClass = jsClass; 

JsGlobalObject::JsGlobalObject( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel )
    : pJsCtx_( cx )
    , parentContainer_( parentContainer )
    , parentPanel_( parentPanel )
{    
}

JsGlobalObject::~JsGlobalObject()
{// No need to cleanup JS here, since it must be performed manually beforehand anyway
}

JSObject* JsGlobalObject::CreateNative( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel )
{
    if ( !jsOps.trace )
    {// JS_GlobalObjectTraceHook address is only accessible after mozjs is loaded.      
        jsOps.trace = JS_GlobalObjectTraceHook;
    }

    JS::CompartmentOptions options;
    JS::RootedObject jsObj( cx,
                            JS_NewGlobalObject( cx, &jsClass, nullptr, JS::DontFireOnNewGlobalHook, options ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    {
        JSAutoCompartment ac( cx, jsObj );
        JS_SetCompartmentPrivate( js::GetContextCompartment( cx ), new JsCompartmentInner() );

        if ( !JS_InitStandardClasses( cx, jsObj ) )
        {
            return nullptr;
        }

        if ( !DefineConsole( cx, jsObj ) )
        {
            return nullptr;
        }

        if ( !CreateAndInstallObject<JsGdiUtils>( cx, jsObj, "gdi" )
             || !CreateAndInstallObject<JsFbPlaylistManager>( cx, jsObj, "plman" )
             || !CreateAndInstallObject<JsUtils>( cx, jsObj, "utils" )
             || !CreateAndInstallObject<JsFbUtils>( cx, jsObj, "fb" )
             || !CreateAndInstallObject<JsWindow>( cx, jsObj, "window", parentPanel ) )
        {
            return nullptr;
        }

        if ( !JS_DefineFunctions( cx, jsObj, jsFunctions ) )
        {
            return nullptr;
        }

        auto pNative = new JsGlobalObject( cx, parentContainer, parentPanel );
        pNative->heapManager_ = GlobalHeapManager::Create( cx );
        if ( !pNative->heapManager_ )
        {// report in Create
            return nullptr;
        }

        JS_SetPrivate( jsObj, pNative );

        // TODO: remove or replace with CreateAndInstall
        JS::RootedObject jsProto( cx, ActiveXObject::InitPrototype( cx, jsObj ) );
        if ( !jsProto )
        {// report in InitPrototype
            return nullptr;
        }
        
        JS::Value protoVal = JS::ObjectValue( *jsProto );
        JS_SetReservedSlot( jsObj, JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>(JsPrototypeId::ActiveX), protoVal );        

        JS_FireOnNewGlobalObject( cx, jsObj );
    }

    return jsObj;
}

void JsGlobalObject::Fail( pfc::string8_fast errorText )
{
    parentContainer_.Fail();
    parentPanel_.JsEngineFail( errorText );
}

GlobalHeapManager& JsGlobalObject::GetHeapManager() const
{
    assert( heapManager_ );
    return *heapManager_;
}

void JsGlobalObject::CleanupBeforeDestruction( JSContext* cx, JS::HandleObject self )
{
    auto nativeGlobal = static_cast<JsGlobalObject*>(JS_GetInstancePrivate( cx, self, &JsGlobalObject::JsClass, nullptr ));
    assert( nativeGlobal );

    JS::RootedValue jsProperty( cx );
    if ( JS_GetProperty( cx, self, "window", &jsProperty ) && jsProperty.isObject() )
    {
        JS::RootedObject jsWindow( cx, &jsProperty.toObject() );
        auto nativeWindow = static_cast<JsWindow*>(JS_GetInstancePrivate( cx, jsWindow, &JsWindow::JsClass, nullptr ));
        if ( nativeWindow )
        {
            nativeWindow->CleanupBeforeDestruction();
        }
    }

    if ( JS_GetProperty( cx, self, "plman", &jsProperty ) && jsProperty.isObject() )
    {
        JS::RootedObject jsWindow( cx, &jsProperty.toObject() );
        auto nativeWindow = static_cast<JsFbPlaylistManager*>(JS_GetInstancePrivate( cx, jsWindow, &JsFbPlaylistManager::JsClass, nullptr ));
        if ( nativeWindow )
        {
            nativeWindow->CleanupBeforeDestruction();
        }
    }

    nativeGlobal->heapManager_.reset();
}

std::optional<std::nullptr_t> 
JsGlobalObject::IncludeScript( const pfc::string8_fast& path )
{
    const auto parsedPath = [&]
    {
        pfc::string8_fast tmpPath;
        tmpPath.replace_string( "/", "\\", 0 );
        return tmpPath;
    }();

    namespace fs = std::filesystem;

    fs::path fsPath = fs::u8path( parsedPath.c_str() );
    std::error_code dummyErr;
    if ( !fs::exists( fsPath ) || !fs::is_regular_file( fsPath, dummyErr ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Path does not point to a valid script file: %s", parsedPath.c_str() );
        return std::nullopt;
    }

    auto retVal = file::ReadFromFile( pJsCtx_, parsedPath );
    if ( !retVal )
    {// report in ReadFromFile;
        return std::nullopt;
    }

    std::wstring scriptCode = retVal.value();
    std::string filename = fsPath.filename().string();

    JS::CompileOptions opts( pJsCtx_ );
    opts.setFileAndLine( filename.c_str(), 1 );

    JS::RootedValue dummyRval( pJsCtx_ );
    if ( !JS::Evaluate( pJsCtx_, opts, (char16_t*)scriptCode.c_str(), scriptCode.length(), &dummyRval ) )
    {// Report in Evaluate
        return std::nullopt;
    }

    return nullptr;
}

}
