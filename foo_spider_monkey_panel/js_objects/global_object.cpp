#include <stdafx.h>
#include "global_object.h"

#include <js_engine/js_container.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/active_x.h>
#include <js_objects/console.h>
#include <js_objects/fb_playlist_manager.h>
#include <js_objects/gdi_utils.h>
#include <js_objects/gdi_font.h>
#include <js_objects/utils.h>
#include <js_objects/fb_utils.h>
#include <js_objects/window.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_error_helper.h>

#include <js_panel_window.h>

#include <js/TracingAPI.h>

#include <filesystem>
#include <fstream>  

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFinalizeOp<JsGlobalObject>,
    nullptr,
    nullptr,
    nullptr,
    nullptr // set in runtime to JS_GlobalObjectTraceHook
};

// TODO: remove HWND and HAS_PRIVATE after creating Window class

JSClass jsClass = {
     "Global",
     JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(40) | DefaultClassFlags(),
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


JsGlobalObject::JsGlobalObject( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel )
    : pJsCtx_( cx )
    , parentContainer_( parentContainer )
    , parentPanel_( parentPanel )
{    
}


JsGlobalObject::~JsGlobalObject()
{
    RemoveHeapTracer();
}

JSObject* JsGlobalObject::Create( JSContext* cx, JsContainer &parentContainer, js_panel_window& parentPanel )
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

        if ( !JS_InitStandardClasses( cx, jsObj ) )
        {
            return nullptr;
        }

        if ( !DefineConsole( cx, jsObj ) )
        {
            return nullptr;
        }

        if ( !CreateAndInstallObject( cx, jsObj, "gdi", JsGdiUtils::Create ) 
             || !CreateAndInstallObject( cx, jsObj, "plman", JsFbPlaylistManager::Create )
             || !CreateAndInstallObject( cx, jsObj, "utils", JsUtils::Create ) 
             || !CreateAndInstallObject( cx, jsObj, "fb", JsFbUtils::Create ) 
             || !CreateAndInstallObject( cx, jsObj, "window", JsWindow::Create, parentPanel ) )
        {
            return nullptr;
        }

        if ( !JS_DefineFunctions( cx, jsObj, jsFunctions ) )
        {
            return nullptr;
        }

        auto pNative = new JsGlobalObject( cx, parentContainer, parentPanel );
        JS_SetPrivate( jsObj, pNative );

        JS::RootedValue jsProto( cx );
        size_t protoSlotIdx = 1;

        pNative->gdiFont_protoSlot_ = protoSlotIdx;
        jsProto.setObjectOrNull( JsGdiFont::CreateProto( cx ) );
        JS_SetReservedSlot( jsObj, protoSlotIdx++, jsProto );
        
        pNative->activeX_protoSlot_ = protoSlotIdx;
        jsProto.setObjectOrNull( ActiveX::InitPrototype( cx, jsObj ) );
        JS_SetReservedSlot( jsObj, protoSlotIdx++, jsProto );
        
        if ( !JS_AddExtraGCRootsTracer( cx, JsGlobalObject::TraceHeapValue, pNative ) )
        {
            return nullptr;
        }

        JS_FireOnNewGlobalObject( cx, jsObj );
    }

    return jsObj;
}

const JSClass& JsGlobalObject::GetClass()
{
    return jsClass;
}

void JsGlobalObject::Fail( pfc::string8_fast errorText )
{
    parentContainer_.Fail();
    parentPanel_.JsEngineFail( errorText );
}

void JsGlobalObject::RegisterHeapUser( IHeapUser* heapUser )
{
    std::scoped_lock sl( heapUsersLock_ );

    assert( !heapUsers_.count( heapUser ) );
    heapUsers_.emplace( heapUser, heapUser );
}

void JsGlobalObject::UnregisterHeapUser( IHeapUser* heapUser )
{
    std::scoped_lock sl( heapUsersLock_ );

    assert( heapUsers_.count( heapUser ) );
    heapUsers_.erase( heapUser );
}

uint32_t JsGlobalObject::StoreToHeap( JS::HandleValue valueToStore )
{
    std::scoped_lock sl( heapElementsLock_ );

    while( heapElements_.count( currentHeapId_ ))
    {
        ++currentHeapId_;
    }

    heapElements_[currentHeapId_] = std::make_shared<HeapElement>( valueToStore );
    return currentHeapId_++;
}

JS::Heap<JS::Value>& JsGlobalObject::GetFromHeap( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );

    assert( heapElements_.count( id ) );
    return heapElements_[id]->value;
}

void JsGlobalObject::RemoveFromHeap( uint32_t id )
{
    std::scoped_lock sl( heapElementsLock_ );
    
    assert( heapElements_.count(id) );    
    heapElements_[id]->inUse = false;    
}

void JsGlobalObject::RemoveHeapTracer()
{
    JS_RemoveExtraGCRootsTracer( pJsCtx_, JsGlobalObject::TraceHeapValue, this );
    
    std::scoped_lock sl( heapUsersLock_ );

    for ( auto heapUser : heapUsers_ )
    {
        heapUser.second->DisableHeapCleanup();
    }

    heapUsers_.clear();
}

void JsGlobalObject::TraceHeapValue( JSTracer *trc, void *data )
{  
    assert( data );
    auto globalObject = static_cast<JsGlobalObject*>( data );

    std::scoped_lock sl( globalObject->heapElementsLock_ );
    
    auto& heapMap = globalObject->heapElements_;
    for ( auto it = heapMap.begin(); it != heapMap.end();)
    {
        if ( !it->second->inUse )
        {
            it = heapMap.erase( it );
        }
        else
        {
            JS::TraceEdge( trc, &(it->second->value), "CustomHeap_Global" );
            it++;
        }
    }
}

std::optional<std::nullptr_t> 
JsGlobalObject::IncludeScript( const pfc::string8_fast& path )
{
    pfc::string8_fast parsedPath = path;
    t_size pos = parsedPath.find_first( "%fb2k_path%" );
    pfc::string8_fast substPath = helpers::get_fb2k_path();
    parsedPath.replace_string( "%fb2k_path%", substPath.c_str(), pos );
    substPath = helpers::get_fb2k_component_path();
    parsedPath.replace_string( "%fb2k_component_path%", substPath.c_str(), pos );
    substPath = helpers::get_profile_path();
    parsedPath.replace_string( "%fb2k_profile_path%", substPath.c_str(), pos );
    parsedPath.replace_string( "/", "\\", pos );

    namespace fs = std::filesystem;
    // TODO: catch exceptions
    fs::path fsPath = fs::u8path( parsedPath.c_str() );
    if ( !fs::exists( fsPath ) || !fs::is_regular_file( fsPath ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Path does not point to a valid script file: %s", parsedPath.c_str() );
        return std::nullopt;
    }

    std::string filename = fsPath.filename().string();

    std::wstring scriptCode;

    // TODO: extract to file_helpers

    // Start

    HANDLE hFile = CreateFile( fsPath.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( !hFile )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Failed to open script file: %s", parsedPath.c_str() );
        return std::nullopt;
    }

    HANDLE hFileMapping = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
    if ( !hFileMapping )
    {
        CloseHandle( hFile );

        JS_ReportErrorUTF8( pJsCtx_, "Internal error: CreateFileMapping failed for `%s`", parsedPath.c_str() );
        return std::nullopt;
    }

    DWORD dwFileSize = GetFileSize( hFile, NULL );
    LPCBYTE pAddr = (LPCBYTE)MapViewOfFile( hFileMapping, FILE_MAP_READ, 0, 0, 0 );
    if ( !pAddr )
    {
        CloseHandle( hFileMapping );
        CloseHandle( hFile );
        
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: MapViewOfFile failed for `%s`", parsedPath.c_str() );
        return std::nullopt;
    }

    if ( dwFileSize == INVALID_FILE_SIZE )
    {
        UnmapViewOfFile( pAddr );
        CloseHandle( hFileMapping );
        CloseHandle( hFile );
        
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: failed to read file size of `%s`", parsedPath.c_str() );
        return std::nullopt;
    }

    const unsigned char bom32Be[] = { 0x00, 0x00, 0xfe, 0xff };
    const unsigned char bom32Le[] = { 0xff, 0xfe, 0x00, 0x00 };
    const unsigned char bom16Be[] = { 0xfe, 0xff };// must be 4byte size
    const unsigned char bom16Le[] = { 0xff, 0xfe };// must be 4byte size, but not 0xff, 0xfe, 0x00, 0x00
    const unsigned char bom8[] = { 0xef, 0xbb, 0xbf };

    // TODO: handle all other BOM cases as well
    if ( dwFileSize >= 4 
            && !memcmp( bom16Le, pAddr, sizeof( bom16Le ) ) )
    {
        pAddr += sizeof( bom16Le );
        dwFileSize -= sizeof( bom16Le );

        const size_t outputSize = (dwFileSize >> 1) + 1;
        scriptCode.resize( outputSize );

        memcpy( scriptCode.data(), (const char*)pAddr, dwFileSize );
        scriptCode[outputSize] = 0;
    }
    else if ( dwFileSize >= sizeof( bom8 )
        && !memcmp( bom8, pAddr, sizeof( bom8 ) ) )
    {
        pAddr += sizeof( bom8 );
        dwFileSize -= sizeof( bom8 );            

        size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( CP_UTF8, (const char*)pAddr, dwFileSize );
        scriptCode.resize( outputSize );

        outputSize = pfc::stringcvt::convert_codepage_to_wide( CP_UTF8, scriptCode.data(), outputSize, (const char*)pAddr, dwFileSize );
        scriptCode.resize( outputSize );
    }
    else
    {
        t_size codepage = helpers::detect_text_charset( (const char*)pAddr, dwFileSize );

        size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( codepage, (const char*)pAddr, dwFileSize );
        scriptCode.resize( outputSize );

        outputSize = pfc::stringcvt::convert_codepage_to_wide( codepage, scriptCode.data(), outputSize, (const char*)pAddr, dwFileSize );
        scriptCode.resize( outputSize );
    }

    // End

    UnmapViewOfFile( pAddr );
    CloseHandle( hFileMapping );
    CloseHandle( hFile );

    JS::CompileOptions opts( pJsCtx_ );
    opts.setFileAndLine( filename.c_str(), 1 );

    JS::RootedValue rval( pJsCtx_ );

    if ( !JS::Evaluate( pJsCtx_, opts, (char16_t*)scriptCode.c_str(), scriptCode.length(), &rval ) )
    {// Report in Evaluate
        return std::nullopt;
    }
    return nullptr;
}

}
