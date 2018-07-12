/// Code extracted from wrap_com.cpp from JSDB by Shanti Rao (see http://jsdb.org)

#include <stdafx.h>
#include "active_x.h"

#include <js_engine/js_engine.h>
#include <js_engine/js_to_native_converter.h>
#include <js_engine/native_to_js_converter.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/global_object.h>
#include <js_objects/prototype_ids.h>
#include <js_utils/js_object_helper.h>

#include <script_interface.h>
#include <com_tools.h>

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <oleauto.h>

#include <vector>
#include <string>

// TODO: add VT_ARRAY <> JSArray and VT_DISPATCH <> JSFunction support
// TODO: rewrite everything here!

//#define DEBUG
//#define TRACE
//dword-word-word-byte[8]
//{85BBD920-42A0-1069-A2E4-08002B30309D}
//HRESULT CLSIDFromString(LPOLESTR lpsz,LPCLSID pclsid);
//S_OK == StringFromCLSID(REFCLSID rclsid,LPOLESTR * ppsz);

#ifndef DISPID_PROPERTYPUT
#   define DISPID_PROPERTYPUT (-3)
#endif

bool VariantToJs( VARIANTARG& var, JSContext* cx, JS::MutableHandleValue rval );
bool JsToVariant( VARIANTARG& arg, JSContext* cx, JS::HandleValue rval );

/////////////////////////////////////////////////

class WrappedJs
    : public IDispatchImpl3<IWrappedJs>
    , public mozjs::IHeapUser
{
protected:
    WrappedJs( JSContext * cx, JS::HandleFunction jsFunction )
        : pJsCtx_( cx )
    {
        assert( cx );

        JS::RootedObject funcObject( cx, JS_GetFunctionObject( jsFunction ) );
        assert( funcObject );

        JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
        assert( jsGlobal );

        pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>(JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::GetClass(), nullptr));
        assert( pNativeGlobal_ );

        pNativeGlobal_->RegisterHeapUser( this );

        JS::RootedValue funcValue( cx, JS::ObjectValue( *funcObject ) );
        funcId_ = pNativeGlobal_->StoreToHeap( funcValue );
        JS::RootedValue globalValue( cx, JS::ObjectValue( *jsGlobal ) );
        globalId_ = pNativeGlobal_->StoreToHeap( globalValue );

        needsCleanup_ = true;
    }
    /// @details Might be called off main thread
    virtual ~WrappedJs()
    {
    }
    /// @details Might be called off main thread
    virtual void FinalRelease()
    {// most of the JS object might be invalid at GC time,
     // so we need to be extra careful
        if ( !needsCleanup_ )
        {
            return;
        }

        std::scoped_lock sl( cleanupLock_ );
        if ( !needsCleanup_ )
        {
            return;
        }

        pNativeGlobal_->RemoveFromHeap( globalId_ );
        pNativeGlobal_->RemoveFromHeap( funcId_ );
        pNativeGlobal_->UnregisterHeapUser( this );
    }

    virtual void DisableHeapCleanup() override
    {
        std::scoped_lock sl( cleanupLock_ );
        needsCleanup_ = false;
    }

    STDMETHODIMP ExecuteValue( VARIANT* Result )
    {
        if ( !Result )
        {
            return E_POINTER;
        }

        // Might be executed outside of main JS workflow, so we need to set request and compartment

        JSAutoRequest ar( pJsCtx_ );
        JS::RootedObject jsGlobal( pJsCtx_, pNativeGlobal_->GetFromHeap( globalId_ ).toObjectOrNull() );
        assert( jsGlobal );
        JSAutoCompartment ac( pJsCtx_, jsGlobal );

        JS::RootedValue vFunc( pJsCtx_, pNativeGlobal_->GetFromHeap( funcId_ ) );
        JS::RootedFunction rFunc( pJsCtx_, JS_ValueToFunction( pJsCtx_, vFunc ) );

        JS::RootedValue retVal( pJsCtx_ );
        if ( !JS::Call( pJsCtx_, jsGlobal, rFunc, JS::HandleValueArray::empty(), &retVal ) )
        {// TODO: set fail somehow
            JS_ClearPendingException( pJsCtx_ ); ///< can't forward exceptions inside ActiveX objects (see reasons above)...
            return E_FAIL;
        }

        if ( !JsToVariant( *Result, pJsCtx_, retVal ) )
        {
            Result->vt = VT_ERROR;
            Result->scode = 0;
            return E_FAIL;
        }

        return S_OK;
    }

private:
    JSContext * pJsCtx_ = nullptr;
    uint32_t funcId_;
    uint32_t globalId_;
    mozjs::JsGlobalObject* pNativeGlobal_ = nullptr;

    std::mutex cleanupLock_;
    bool needsCleanup_ = false;
};



namespace
{

//using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    mozjs::JsFinalizeOp<ActiveX>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ActiveXObject",
    mozjs::DefaultClassFlags(),
    &jsOps
};

bool ActiveX_Constructor_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
    if ( !argc || !args[0].isString() )
    {
        JS_ReportErrorUTF8( cx, "Argument 1 is not a string" );
        return false;
    }

    bool bRet = true;
    std::wstring name = mozjs::convert::to_native::ToValue<std::wstring>( cx, args[0], bRet );
    if ( !bRet )
    {
        JS_ReportErrorUTF8( cx, "Failed to parse name argument" );
        return false;
    }

    auto jsObject = ActiveX::Create( cx, name );
    if ( !jsObject )
    {// report in ctor
        return false;
    }

    args.rval().setObjectOrNull( jsObject );
    return true;
}

bool ActiveX_JSGet_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    // TODO: dirty hack! Think of a way to replace it
    JS::RootedString s( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    if ( !s )
    {
        JS_ReportErrorUTF8( cx, "Failed to get property name" );
        return false;
    }

    size_t strLen = JS_GetStringLength( s );
    std::wstring name( strLen, '\0' );
    mozilla::Range<char16_t> wCharStr( (char16_t*)name.data(), strLen );
    if ( !JS_CopyStringChars( cx, wCharStr, s ) )
    {
        JS_ReportOutOfMemory( cx );
        return false;
    }

    std::size_t fPos = name.find( L" " );
    if ( fPos == std::wstring::npos )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "ActiveX_JSSet error: invalid command: %s", tmpStr.c_str() );
        return false;
    }
    name = name.substr( fPos + 1 );

    ActiveX* t = static_cast<ActiveX*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !t )
    {
        JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    ActiveX::PropInfo * p = t->Find( name );
    if ( !p )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "Unknown property: %s", tmpStr.c_str() );
        return false;
    }

    DISPID dispid = 0;
    if ( !t->GetDispId( p->name, dispid ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "Failed to get dispid for `%s`", tmpStr.c_str() );
    }

    return t->Get( cx, argc, vp, dispid );
}

bool ActiveX_JSSet_Impl( JSContext *cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    // TODO: dirty hack! Think of a way to replace it
    JS::RootedString s( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    if ( !s )
    {
        JS_ReportErrorUTF8( cx, "Failed to get property name" );
        return false;
    }

    size_t strLen = JS_GetStringLength( s );
    std::wstring name( strLen, '\0' );
    mozilla::Range<char16_t> wCharStr( (char16_t*)name.data(), strLen );
    if ( !JS_CopyStringChars( cx, wCharStr, s ) )
    {
        JS_ReportOutOfMemory( cx );
        return false;
    }

    std::size_t fPos = name.find( L" " );
    if ( fPos == std::wstring::npos )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "Invalid command: %s", tmpStr.c_str() );
        return false;
    }
    name = name.substr( fPos + 1 );

    ActiveX* t = static_cast<ActiveX*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !t )
    {
        JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    ActiveX::PropInfo * p = t->Find( name );
    if ( !p )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "Unknown property: %s", tmpStr.c_str() );
        return false;
    }

    DISPID dispid = 0;
    if ( !t->GetDispId( p->name, dispid ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "Failed to get dispid for `%s`", tmpStr.c_str() );
    }

    return t->Set( cx, argc, vp, dispid, p->PutRef );
}

/*
const JSPropertySpec ActiveX_properties[] = {
    JS_PSG( "className", ActiveX_JSGet, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT ),
    JS_PS_END
};
*/

/*
WRAP_HELP( ActiveX,
           "name(index)\nextract(index)\nextract(index,string)\nsize(index)\n"
           "close()\n" );
*/

bool ActiveX_Run_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    ActiveX* t = static_cast<ActiveX*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !t )
    {
        JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    JS::RootedString s( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    if ( !s )
    {
        JS_ReportErrorUTF8( cx, "Failed to get function name" );
        return false;
    }

    size_t strLen = JS_GetStringLength( s );
    std::wstring name( strLen, '\0' );
    mozilla::Range<char16_t> wCharStr( (char16_t*)name.data(), strLen );
    if ( !JS_CopyStringChars( cx, wCharStr, s ) )
    {
        JS_ReportOutOfMemory( cx );
        return false;
    }

    DISPID dispid;
    if ( !t->GetDispId( name, dispid ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "Unknown function: %s", tmpStr.c_str() );
        return false;
    }

    if ( !t->Invoke( cx, argc, vp, dispid ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "IDispatch->Invoke failed: %s", tmpStr.c_str() );
        return false;
    }

    return true;
}

// bool ActiveX_Exec( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     /*
//     GETENV;
//     GETOBJ( ActiveX, ActiveX, t );
//     */
// 
//     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
// 
//     ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
//     if ( !t )
//     {
//         //JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
//         return false;
//     }
// 
//     if ( argc < 1 )
//     {
//         JS_ReportErrorUTF8( cx, "ActiveX_Exec error" );
//         return false;
//     }
//     if ( !args[0].isString() )
//     {
//         JS_ReportErrorUTF8( cx, "ActiveX_Exec error: argument 1 is not a string" );
//         return false;        
//     }
// 
//     JS::RootedString s( JS_GetFunctionId( ARGV( 0) ) );
//     if ( s )
//     {        
//         const wchar_t* name = (wchar_t*)JS_GetStringChars( s );
//         if ( !name )
//         {
//             JS_ReportErrorUTF8( cx, "ActiveX_Exec error: No function name" );
//             args.rval().setUndefined();
//             return false;
//         }
//         DISPID dispid;
//         if ( !t->Id( name, dispid ) )
//         {
//             pfc::string8_fast tmpStr( pfc::stringcvt::string_utf8_from_wide( name ) );
//             JS_ReportErrorUTF8( cx, "ActiveX error: This object does not have that function: %s", tmpStr.c_str() );
//             args.rval().setUndefined();
//             return false;            
//         }
//         if ( !t->Invoke( dispid, cx, argc - 1, args + 1, rval ) )
//         {
//             return false;
//             //      ERR_MSG(ActiveX,"IDispatch->Invoke failed",TStr(name));
//         }
//     }
//     return true;
// }
// 
// ///Get("property","index","index")
// bool ActiveX_ToString( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     /*
//     GETARGS;
//     GETENV;
//     GETOBJ( ActiveX, ActiveX, t );
//     */
// 
//     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
// 
//     // TODO: don't actually need this, move to class
//     ActiveX* t = static_cast<ActiveX*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );    
//     if ( !t )
//     {
//         return false;
//     }
// 
//     if ( t->variant_.vt )
//     {
//         JS::RootedString jsString( JS_NewStringCopyZ( cx, "variant_" ) );
//         if ( !jsString )
//         {
//             return false;
//         }
// 
//         args.rval().setString( jsString );
//         return true;
//     }
// 
//     DISPID dispid = 0;
//     if ( t->Id( (wchar_t*)L"toString", dispid ) )
//     {
//         t->Invoke( dispid, cx, argc, vp );
//         return true;
//     }
// 
//     //if (!t->Get(dispid, cx, 0,vp, false))
// 
//     JS::RootedString jsString( JS_NewStringCopyZ( cx, "" ) );
//     if ( !jsString )
//     {
//         return false;
//     }
// 
//     args.rval().setString( jsString );
//     return true;
// }
// 
// bool ActiveX_Get( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     if ( argc == 0 ) ERR_COUNT( ActiveX, Get );
//     if ( !ISSTR( 0 ) && !ISINT( 0 ) ) ERR_TYPE( ActiveX, Get, 1, String );
// 
//     /*GETENV;
//     GETOBJ( ActiveX, ActiveX, t );*/
// 
//     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
// 
//     ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
//     if ( !t )
//     {
//         //JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
//         return false;
//     }
// 
// 
//     DISPID dispid = 0;
//     if ( ISSTR( 0 ) )
//     {
//         JSString* s = JSVAL_TO_STRING( args[0] );
// 
//         if ( s )
//         {
//             jschar* name = JS_GetStringChars( s );
//             if ( !name )
//             {
//                 JS_ReportErrorUTF8( cx, "ActiveX_Exec error: No property name" );
//                 args.rval().setUndefined();
//                 return false;
//             }
//             if ( !t->Id( name, dispid ) )
//             {
//                 pfc::string8_fast tmpStr( pfc::stringcvt::string_utf8_from_wide( name ) );
//                 JS_ReportErrorUTF8( cx, "ActiveX error: This object does not have that property: %s", tmpStr.c_str() );
//                 args.rval().setUndefined();
//                 return false;
//             }
//         }
//     }
// 
//     if ( !t->Get( dispid, cx, argc - 1, argv + 1, rval ) )
//     {
//         //    ERR_MSG(ActiveX,"IDispatch->Invoke failed","");
//         return false;
//     }
//     return true;
// }
// 
// ///Set("property","index","index","value")
// bool ActiveX_Set( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     if ( argc < 2 ) ERR_COUNT( ActiveX, Set );
//     if ( !ISSTR( 0 ) ) ERR_TYPE( ActiveX, Set, 1, String );
// 
//     /*GETENV;
//     GETOBJ2( ActiveX, ActiveX, t );*/
// 
//     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
// 
//     ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
//     if ( !t )
//     {
//         //JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
//         return false;
//     }
// 
//     JSString* s = JSVAL_TO_STRING( args[0] );
//     if ( s )
//     {
//         jschar* name = JS_GetStringChars( s );
//         if ( !name )
//         {
//             JS_ReportErrorUTF8( cx, "ActiveX_Exec error: No property name" );
//             args.rval().setUndefined();
//             return false;
//         }
// 
//         DISPID dispid;
//         if ( !t->Id( name, dispid ) )
//         {
//             pfc::string8_fast tmpStr( pfc::stringcvt::string_utf8_from_wide( name ) );
//             JS_ReportErrorUTF8( cx, "ActiveX error: This object does not have that property: %s", tmpStr.c_str() );
//             args.rval().setUndefined();
//             return false;
//         }
// 
//         ActiveX::PropInfo *p = t->Find( name );
//         RETBOOL( t->Set( dispid, cx, argc - 2, argv + 1, argv + argc - 1, p ? p->PutRef : false ) );
//     }
// 
//     args.rval().setUndefined();
//     return true;
// }
// 
// bool ActiveX_as( JSContext* cx, unsigned argc, JS::Value* vp )
// {
//     if ( argc < 1 ) ERR_COUNT( ActiveX, as );
//     if ( !ISSTR( 0 ) ) ERR_TYPE( ActiveX, as, 1, String );
//     GETENV;
//     GETOBJ( ActiveX, ActiveX, t );
// 
//     HRESULT hresult;
//     void* specific = nullptr;
//     CLSID clsid;
//     jschar * type = JS_GetStringChars( JSVAL_TO_STRING( argv[0] ) );
// 
//     if ( type[0] == L'{' )
//         hresult = CLSIDFromString( (WCHAR*)type, &clsid );
//     else
//         hresult = CLSIDFromProgID( (WCHAR*)type, &clsid );
// 
//     if ( SUCCEEDED( hresult ) )
//     {
//         IUnknown * unk;
//         hresult = t->pUnknown_->QueryInterface( clsid, (void * *)&unk );
//         if ( SUCCEEDED( hresult ) ) RETOBJ( ActiveX_Object( cx, new ActiveX( unk ), true, nullptr ) );
// 
//     }
//     RETOBJ( nullptr );
// }
// 
bool ActiveX_Close_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    ActiveX* t = static_cast<ActiveX*>( JS_GetPrivate( args.thisv().toObjectOrNull() ) );
    if ( !t )
    {
        //JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    /* t->Close();*/
    return true;
}
/*
const JSFunctionSpec ActiveX_functions[] = {
    JS_FN( "get", ActiveX_Get, 1, DefaultPropsFlags() ),
    JS_FN( "set", ActiveX_Set, 2, DefaultPropsFlags() ),
    JS_FN( "exec", ActiveX_Exec, 2, DefaultPropsFlags() ),
    JS_FN( "at", ActiveX_Exec, 2, DefaultPropsFlags() ),
    JS_FN( "as", ActiveX_as, 2, DefaultPropsFlags() ),
    JS_FN( "close", ActiveX_Close, 0, DefaultPropsFlags() ),
    JS_FN( "toString", ActiveX_ToString, 0, DefaultPropsFlags() ),
    JS_FS_END
};
*/

MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_Constructor, ActiveX_Constructor_Impl )
MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_JSGet, ActiveX_JSGet_Impl )
MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_JSSet, ActiveX_JSSet_Impl )
MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_Run, ActiveX_Run_Impl )
MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_Close, ActiveX_Close_Impl )

static JSFunctionSpec ActiveX_functions[] = {
    /*
{ "get",     ActiveX_Get,      1 },
{ "set",    ActiveX_Set, 2 },
{ "exec",    ActiveX_Exec, 2 },
{ "at",    ActiveX_Exec, 2 },
{ "as",    ActiveX_as, 2 },
*/
{ "close",ActiveX_Close,0 },
/*
{ "toString",ActiveX_ToString,0 },
*/
{ 0 }
};

// 
/*
static JSFunctionSpec ActiveX_fnstatic[] = {
    { "help",  ActiveX_HELP,    0 },
#ifdef EXPERIMENT_COM
{ "typelib",  ActiveX_typelib,    1 },
#endif
{ 0 }
};
*/

}

/// VariantToJs assumes that the caller will call VariantClear, so call AddRef on new objects
bool VariantToJs( VARIANTARG& var, JSContext* cx, JS::MutableHandleValue rval )
{
#define FETCH(x) (ref? * (var.p ## x) : var.x)

    bool ref = false;
    int type = var.vt;
    if ( type & VT_BYREF )
    {
        ref = true;
        type &= ~VT_BYREF;
    }

    try
    {
        switch ( type )
        {
        case VT_ERROR: rval.setUndefined(); break;
        case VT_NULL: rval.setNull(); break;
        case VT_EMPTY: rval.setUndefined(); break;
        case VT_I1: rval.setInt32( static_cast<int32_t>( FETCH( cVal ) ) ); break;
        case VT_I2: rval.setInt32( static_cast<int32_t>( FETCH( iVal ) ) ); break;
        case VT_INT:
        case VT_I4: rval.setInt32( FETCH( lVal ) ); break;
        case VT_R4: rval.setNumber( FETCH( fltVal ) ); break;
        case VT_R8: rval.setNumber( FETCH( dblVal ) ); break;

        case VT_BOOL: rval.setBoolean( FETCH( boolVal ) ? true : false ); break;

        case VT_UI1: rval.setNumber( static_cast<uint32_t>( FETCH( bVal ) ) ); break;
        case VT_UI2: rval.setNumber( static_cast<uint32_t>( FETCH( uiVal ) ) ); break;
        case VT_UINT:
        case VT_UI4: rval.setNumber( static_cast<uint32_t>( FETCH( ulVal ) ) ); break;

        case VT_BSTR:
        {
            rval.setString( JS_NewUCStringCopyN( cx, (char16_t*)FETCH( bstrVal ), SysStringLen( FETCH( bstrVal ) ) ) );
            break;
        };
        case VT_DATE:
        {
            DATE d = FETCH( date );
            SYSTEMTIME time;
            VariantTimeToSystemTime( d, &time );
            rval.setObjectOrNull( JS_NewDateObject( cx, 
                                                    time.wYear, time.wMonth - 1, time.wDay,
                                                    time.wHour, time.wMinute, time.wSecond ) );

            break;
        }

        case VT_UNKNOWN:
        {
            if ( !FETCH( punkVal ) )
            {
                rval.setNull(); 
                break;
            }

            std::unique_ptr<ActiveX> x( new ActiveX( FETCH( punkVal ), true ) );
            if ( !x->pUnknown_ && !x->pDispatch_ )
            {
                return false;
            }

            rval.setObjectOrNull( ActiveX::Create( cx, x.release() ) );
            break;
        }
        case VT_DISPATCH:
        {
            if ( !FETCH( pdispVal ) )
            {
                rval.setNull(); 
                break;
            }

            std::unique_ptr<ActiveX> x( new ActiveX( FETCH( pdispVal ), true ) );
            if ( !x->pUnknown_ && !x->pDispatch_ )
            {
                return false;
            }

            rval.setObjectOrNull( ActiveX::Create( cx, x.release() ) );
            break;
        }
        case VT_VARIANT: //traverse the indirection list?
            if ( ref )
            {
                VARIANTARG* v = var.pvarVal;
                if ( v )
                {
                    return VariantToJs( *v, cx, rval );
                }
            }
            break;

        default:
            if ( type <= VT_CLSID )
            {
                std::unique_ptr<ActiveX> x( new ActiveX( var ) );
                rval.setObjectOrNull( ActiveX::Create( cx, x.release() ) );

                return true;
            }

            return false;
            //default: return false;
        }
    }
    catch ( ... )
    {
        return false;
    }
    return true;
}


void CheckReturn( JSContext* cx, JS::MutableHandleValue rval )
{
    if ( !rval.isObject() )
    {
        return;
    }

    HRESULT hresult;
    JS::RootedObject j0( cx, &rval.toObject() );

    ActiveX* x = static_cast<ActiveX*>( JS_GetInstancePrivate( cx, j0, &jsClass, nullptr ) );
    if ( !x )
    {
        return;
    }

    if ( x->pUnknown_ && !x->pDispatch_ )
    {
        hresult = x->pUnknown_->QueryInterface( IID_IDispatch, (void **)&x->pDispatch_ );
        if ( SUCCEEDED( hresult ) )
        {
            x->SetupMembers( cx, j0 );
        }
        else
        {
            x->pDispatch_ = nullptr;
        }
    }
}

bool JsToVariant( VARIANTARG& arg, JSContext* cx, JS::HandleValue rval )
{
    VariantInit( &arg );

    if ( rval.isObject() )
    {
        JS::RootedObject j0( cx, rval.toObjectOrNull() );
        if ( j0 && JS_InstanceOf( cx, j0, &jsClass, 0 ) )
        {
            ActiveX* x = static_cast<ActiveX*>( JS_GetPrivate( j0 ) );
            if ( !x )
            {
                //JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );

                // Avoid cleaning up garbage
                arg.vt = VT_EMPTY;
                return false;
            }
            if ( x->variant_.vt != VT_EMPTY )
            {
                //1.7.2.3
                VariantCopyInd( &arg, &x->variant_ );
                //VariantCopy(&arg,&x->variant_);
                //1.7.2.2 could address invalid memory if x is freed before arg
                // arg.vt = VT_VARIANT | VT_BYREF;
                // arg.pvarVal = &x->variant_;
                return true;
            }
            if ( x->pDispatch_ )
            {
                arg.vt = VT_DISPATCH;
                arg.pdispVal = x->pDispatch_;
                x->pDispatch_->AddRef();
                return true;
            }
            else if ( x->pUnknown_ )
            {
                arg.vt = VT_UNKNOWN;
                arg.punkVal = x->pUnknown_;
                x->pUnknown_->AddRef();
                return true;
            }
            else
            {
                arg.vt = VT_BYREF | VT_UNKNOWN;
                arg.ppunkVal = &x->pUnknown_;
                return true;
            }
        }

        if ( j0 && JS_ObjectIsFunction( cx, j0 ) )
        {
            JS::RootedFunction func( cx, JS_ValueToFunction( cx, rval ) );
            auto pWrappedJs = new com_object_impl_t<WrappedJs>( cx, func );

            arg.vt = VT_DISPATCH;
            arg.pdispVal = pWrappedJs;
            return true;
        }
    }
    else if ( rval.isBoolean() )
    {
        arg.vt = VT_BOOL;
        arg.boolVal = rval.toBoolean() ? -1 : 0;
        return true;
    }
    else if ( rval.isInt32() )
    {
        arg.vt = VT_I4;
        arg.lVal = rval.toInt32();
        return true;
    }
    else if ( rval.isDouble() )
    {
        arg.vt = VT_R8;
        arg.dblVal = rval.toDouble();

        return true;
    }
    else if ( rval.isNull() )
    {
        arg.vt = VT_NULL;
        arg.scode = 0;
        return true;
    }
    else if ( rval.isUndefined() )
    {
        arg.vt = VT_EMPTY;
        arg.scode = 0;
        return true;
    }
    else if ( rval.isString() )
    {
        JS::RootedString rStr( cx, rval.toString() );
        size_t strLen = JS_GetStringLength( rStr );
        std::wstring strVal( strLen, '\0' );
        mozilla::Range<char16_t> wCharStr( (char16_t*)strVal.data(), strLen );
        if ( !JS_CopyStringChars( cx, wCharStr, rStr ) )
        {
            JS_ReportOutOfMemory( cx );

            // Avoid cleaning up garbage
            arg.vt = VT_EMPTY;
            return false;
        }

        arg.vt = VT_BSTR;
        arg.bstrVal = SysAllocString( strVal.c_str() );
        return true;
    }
    /*

    // See https://hg.mozilla.org/mozilla-central/rev/d115405d4e0b

    else
    {
        bool is;
        if ( JS_IsArrayObject( cx, rval, &is ) )
        {
            return false;
        }

        if ( is )
        {
            arg.vt = VT_ARRAY | VT_VARIANT;
            arg.bstrVal = SysAllocString( strVal.c_str() );
            return true;
        }
    }
    */

    // Avoid cleaning up garbage
    arg.vt = VT_EMPTY;
    return false;
}

#ifdef EXPERIMENT_COM
IDispatch* Recast( IUnknown* unk, ITypeLib* typelib, WCHAR* type )
{
    IUnknown* result = nullptr;
    IDispatch* dispatch = nullptr;
    ITypeInfo* typeinfo = nullptr;
    void* specific = nullptr;
    HRESULT hresult;
    unsigned short found = 1;
    MEMBERID memb = 0;
    CLSID clsid;

    if ( type[0] == L'{' )
        hresult = CLSIDFromString( (WCHAR*)type, &clsid );
    else
        hresult = CLSIDFromProgID( (WCHAR*)type, &clsid );
    if ( !SUCCEEDED( hresult ) )
        return nullptr;

    hresult = typelib->GetTypeInfoOfGuid( clsid, &typeinfo );
    if ( !SUCCEEDED( hresult ) || !found )
        return nullptr;

    hresult = unk->QueryInterface( clsid, &specific );
    if ( !SUCCEEDED( hresult ) || !specific )
        return nullptr;

    hresult = CreateStdDispatch( unk, specific, typeinfo, &result );

    if ( !SUCCEEDED( hresult ) || !result )
        return nullptr;

    hresult = result->QueryInterface( IID_IDispatch, (void * *)&dispatch );

    if ( !SUCCEEDED( hresult ) )
        dispatch = 0;

    return dispatch;
}

WRAP( ActiveX, typelib )
{
    if ( argc < 1 ) ERR_COUNT( ActiveX, typelib );
    const jschar * library = WSTR( 0 );
    ITypeLib * typelib = nullptr;

    HRESULT hresult = LoadTypeLib( (WCHAR*)library, &typelib );
    if ( SUCCEEDED( hresult ) )
    {
        //     MemoryStream s;
        WStr s;

        unsigned count = typelib->GetTypeInfoCount();
        for ( unsigned i = 0; i < count; i++ )
        {
            BSTR name, docstring, helpfile;
            unsigned long helpcontext;

            typelib->GetDocumentation( i, &name, &docstring, &helpcontext, &helpfile );
            WStr w1( (wchar_t*)name, SysStringLen( name ) );
            WStr w2( (wchar_t*)docstring, SysStringLen( docstring ) );

            s << ( w1 ) << (wchar_t*)L": " << ( w2 ) << (wchar_t*)L"\n";

            SysFreeString( name );
            SysFreeString( docstring );
            SysFreeString( helpfile );
        }

        RETSTRW( s );
    }
    RETSTRW( L"" );
}

WRAP( ActiveX, as )
{
    if ( argc < 2 ) ERR_COUNT( ActiveX, as );
    if ( !ISSTR( 0 ) || !ISSTR( 1 ) ) ERR_TYPE( ActiveX, as, 1, String );
    GETENV;
    GETOBJ( ActiveX, ActiveX, t );

    jschar * library = JS_GetStringChars( JSVAL_TO_STRING( argv[0] ) );
    jschar * name = JS_GetStringChars( JSVAL_TO_STRING( argv[1] ) );
    ITypeLib * typelib = nullptr;

    HRESULT hresult = LoadTypeLib( (WCHAR*)library, &typelib );
    if ( SUCCEEDED( hresult ) )
    {
        IDispatch* d = Recast( t->pUnknown_, typelib, (WCHAR*)name );
        if ( d )
        {
            if ( t->dispatch ) t->pDispatch_->Release();
            t->pDispatch_ = d;
            t->SetupMembers( cx, obj );
        }

        typelib->Release();

        if ( d )  RETOBJ( obj );
    }
    RETOBJ( nullptr );
}
#endif

void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr, JSContext* cx )
{
    switch ( hresult )
    {
    case DISP_E_BADPARAMCOUNT:
    {
        JS_ReportErrorUTF8( cx, "ActiveX: Wrong number of parameters" );
        break;
    }
    case DISP_E_BADVARTYPE:
    {
        JS_ReportErrorUTF8( cx, "ActiveX: Bad variable type %d", argerr );
        break;
    }
    case DISP_E_EXCEPTION:
    {
        if ( exception.bstrDescription )
        {
            // <codecvt> is deprecated in C++17...
            pfc::string8_fast descriptionStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrDescription, SysStringLen( exception.bstrDescription ) )
            );
            pfc::string8_fast sourceStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrSource, SysStringLen( exception.bstrSource ) )
            );

            JS_ReportErrorUTF8( cx, "ActiveX: (%s) %s", sourceStr.c_str(), descriptionStr.c_str() );
        }
        else
        {
            JS_ReportErrorUTF8( cx, "ActiveX: Error code %d", exception.scode );
        }
        SysFreeString( exception.bstrSource );
        SysFreeString( exception.bstrDescription );
        SysFreeString( exception.bstrHelpFile );
        break;
    }
    case DISP_E_MEMBERNOTFOUND:
    {
        JS_ReportErrorUTF8( cx, "ActiveX: Function not found" );
        break;
    }
    case DISP_E_OVERFLOW:
    {
        JS_ReportErrorUTF8( cx, "ActiveX: Can not convert variable %d", argerr );
        break;
    }
    case DISP_E_PARAMNOTFOUND:
    {
        JS_ReportErrorUTF8( cx, "ActiveX: Parameter %d not found", argerr );
        break;
    }
    case DISP_E_TYPEMISMATCH:
    {
        JS_ReportErrorUTF8( cx, "ActiveX: Parameter %d type mismatch", argerr );
        break;
    }
    case DISP_E_UNKNOWNINTERFACE:
    {
        JS_ReportErrorUTF8( cx, "ActiveX: Unknown interface" );
        break;
    }
    case DISP_E_UNKNOWNLCID:
    {
        JS_ReportErrorUTF8( cx, "ActiveX: Unknown LCID" );
        break;
    }
    case DISP_E_PARAMNOTOPTIONAL:
    {
        JS_ReportErrorUTF8( cx, "ActiveX: Parameter %d is required", argerr );
        break;
    }
    default:
    {
    }
    }
}

ActiveX::ActiveX( VARIANTARG& var )
{
    pUnknown_ = nullptr;
    pTypeInfo_ = nullptr;
    pDispatch_ = nullptr;
    VariantInit( &variant_ );
    VariantCopyInd( &variant_, &var );
}

ActiveX::ActiveX( IDispatch *obj, bool addref )
{
    pUnknown_ = nullptr;
    pTypeInfo_ = nullptr;
    memset( &variant_, 0, sizeof( variant_ ) );
    pDispatch_ = obj;

    if ( !pDispatch_ )
    {
        return;
    }
    if ( addref )
    {
        pDispatch_->AddRef();
    }
}

ActiveX::ActiveX( IUnknown* obj, bool addref )
{
    pDispatch_ = nullptr;
    pTypeInfo_ = nullptr;
    memset( &variant_, 0, sizeof( variant_ ) );

    pUnknown_ = obj;
    if ( !pUnknown_ )
    {
        return;
    }

    if ( addref )
    {
        pUnknown_->AddRef();
    }

    HRESULT hresult = pUnknown_->QueryInterface( IID_IDispatch, (void * *)&pDispatch_ );
    if ( !SUCCEEDED( hresult ) )
    {
        pDispatch_ = nullptr;
    }
}

ActiveX::ActiveX( CLSID& clsid )
{
    HRESULT hresult;
    pUnknown_ = nullptr;
    pDispatch_ = nullptr;
    pTypeInfo_ = nullptr;
    memset( &variant_, 0, sizeof( variant_ ) );

    hresult = CoCreateInstance( clsid, nullptr, CLSCTX_INPROC_SERVER,
                                IID_IUnknown, (void **)&pUnknown_ );

    if ( !SUCCEEDED( hresult ) )
    {
        pUnknown_ = 0; return;
    } //throw xdb("CoCreateInstance Failure");

    hresult = pUnknown_->QueryInterface( IID_IDispatch, (void * *)&pDispatch_ );


    //maybe I don't know what to do with it, but it might get passed to
    //another COM function
    if ( !SUCCEEDED( hresult ) )
    {
        pDispatch_ = nullptr;
        //pUnknown_->Release();
        //pUnknown_=nullptr;
        //throw xdb("IDispatch interface not found");
    }
}

ActiveX::~ActiveX()
{
    if ( pDispatch_ )
    {
        pDispatch_->Release();
    }
    if ( pUnknown_ )
    {
        pUnknown_->Release();
    }
    if ( pTypeInfo_ )
    {
        pTypeInfo_->Release();
    }
    if ( variant_.vt )
    {
        VariantClear( &variant_ );
    }
    CoFreeUnusedLibraries();
}

JSObject* ActiveX::InitPrototype( JSContext *cx, JS::HandleObject parentObject )
{
    return JS_InitClass( cx, parentObject, nullptr, &jsClass,
                         ActiveX_Constructor, 0,
                         nullptr, nullptr, nullptr, nullptr );
}
JSObject* ActiveX::Create( JSContext* cx, const std::wstring& name )
{   
    CLSID clsid;
    HRESULT hresult = (name[0] == L'{')
        ? CLSIDFromString( name.c_str(), &clsid )
        : CLSIDFromProgID( name.c_str(), &clsid );
    if ( !SUCCEEDED( hresult ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "Invalid CLSID" );
        return nullptr;
    }

    std::unique_ptr<ActiveX> nativeObject;
    IUnknown* unk = nullptr;
    hresult = GetActiveObject( clsid, nullptr, &unk );
    if ( SUCCEEDED( hresult ) && unk )
    {
        nativeObject.reset( new ActiveX( unk ) );
        if ( !nativeObject->pUnknown_ )
        {
            pfc::string8_fast cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
            JS_ReportErrorUTF8( cx, "Failed to create ActiveX object via IUnknown: %s", cStr.c_str() );
            return nullptr;
        }
    }

    if ( !nativeObject )
    {
        nativeObject.reset( new ActiveX( clsid ) );
        if ( !nativeObject->pUnknown_ )
        {
            pfc::string8_fast cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
            JS_ReportErrorUTF8( cx, "Failed to create ActiveX object via CLSID: %s", cStr.c_str() );
            return nullptr;
        }
    }

    return Create( cx, nativeObject.release() );
}

JSObject* ActiveX::Create( JSContext* cx, ActiveX* pPremadeNative )
{
    if ( !pPremadeNative )
    {
        JS_ReportErrorUTF8( cx, "Internal error: pPremadeNative is null" );
        return nullptr;
    }

    std::unique_ptr<ActiveX> autoNative( pPremadeNative );

    JS::RootedObject jsGlobal(cx, JS::CurrentGlobalOrNull( cx ));
    if ( !jsGlobal )
    {
        JS_ReportErrorUTF8( cx, "Internal error: pGlobal is null" );
        return nullptr;
    }

    JS::RootedObject jsProto(cx, mozjs::GetPrototype<ActiveX>(cx, jsGlobal, mozjs::JsPrototypeId::ActiveX ) );
    if ( !jsProto )
    {// report in GetPrototype
        return nullptr;
    }

    JS::RootedObject jsObj( cx,
                            JS_NewObjectWithGivenProto( cx, &jsClass, jsProto ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, pPremadeNative );
    autoNative.release();
    if ( !pPremadeNative->SetupMembers( cx, jsObj ) )
    {
        return nullptr;
    }

    return jsObj;
}

const JSClass ActiveX::JsClass = jsClass;

bool ActiveX::GetDispId( std::wstring_view name, DISPID& dispid )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    if ( name.empty() || name[0] == L'0' )
    {
        dispid = 0; 
        return true;
    }

    wchar_t* cname = const_cast<wchar_t*>( name.data() );
    HRESULT hresult = pDispatch_->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_USER_DEFAULT, &dispid );
    if ( !SUCCEEDED( hresult ) )
    {
        hresult = pDispatch_->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_SYSTEM_DEFAULT, &dispid );

    }
    return SUCCEEDED( hresult );
}

bool ActiveX::Invoke( JSContext* cx, unsigned argc, JS::Value* vp, DISPID dispid )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANT VarResult;
    std::unique_ptr<VARIANTARG[]> args;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args.reset(new VARIANTARG[argc]);
        dispparams.rgvarg = args.get();
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !JsToVariant( args[argc - i - 1], cx, callArgs[i] ) )
            {
                args[argc - i - 1].vt = VT_ERROR;
                args[argc - i - 1].scode = 0;
            }
        }
    }

    VariantInit( &VarResult );

    // don't use DispInvoke, because we don't know the TypeInfo
    hresult = pDispatch_->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_METHOD,
        &dispparams, &VarResult, &exception, &argerr );

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( cx, callArgs[i] ); //in case any empty ActiveX objects were filled in by Invoke()
        VariantClear( &args[i] ); //decrement AddRefs() done in SetupValue
    }

    if ( !SUCCEEDED( hresult ) )
    {
        VariantClear( &VarResult );
        ReportActiveXError( hresult, exception, argerr, cx );
        return false;
    }

    if ( !VariantToJs( VarResult, cx, callArgs.rval() ) )
    {
        callArgs.rval().setUndefined();
    }

    VariantClear( &VarResult );
    return true;
}

bool ActiveX::Get( JSContext* cx, unsigned argc, JS::Value* vp, DISPID dispid, bool exceptions /*= true */ )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANT VarResult;
    std::unique_ptr<VARIANTARG[]> args;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args.reset(new VARIANTARG[argc]);
        dispparams.rgvarg = args.get();
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !JsToVariant( args[argc - i - 1], cx, callArgs[i] ) )
            {
                args[argc - i - 1].vt = VT_ERROR;
                args[argc - i - 1].scode = 0;
            }
        }
    }

    VariantInit( &VarResult );

    hresult = pDispatch_->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_PROPERTYGET,
        &dispparams, &VarResult, &exception, &argerr );

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( cx, callArgs[i] );
        VariantClear( &args[i] );
    }

    if ( !SUCCEEDED( hresult ) )
    {
        if ( exceptions )
        {
            ReportActiveXError( hresult, exception, argerr, cx );
        }
        return false;
    }

    if ( !VariantToJs( VarResult, cx, callArgs.rval() ) )
    {
        callArgs.rval().setUndefined();
    }
    VariantClear( &VarResult );

    return true;
}

bool ActiveX::Set( JSContext* cx, unsigned argc, JS::Value* vp, DISPID dispid, bool ref )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    std::unique_ptr<VARIANTARG[]> args( new VARIANTARG[argc] );
    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { args.get(),&dispput,argc,1 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    //the index values, in reverse order
    for ( size_t i = 0; i < argc; i++ )
    {
        if ( !JsToVariant( args[argc - i - 1], cx, callArgs[i] ) )
        {
            args[argc - i - 1].vt = VT_ERROR;
            args[argc - i - 1].scode = 0;
        }
    }

    DWORD flag = DISPATCH_PROPERTYPUT;
    if ( ref && ( args[0].vt & VT_DISPATCH || args[0].vt & VT_UNKNOWN ) )
    {//must be passed by name
        flag = DISPATCH_PROPERTYPUTREF;
    }

    hresult = pDispatch_->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        (WORD)flag,
        &dispparams, nullptr, &exception, &argerr ); //no result

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( cx, callArgs[i] );
        VariantClear( &args[i] );
    }

    if ( !SUCCEEDED( hresult ) )
    {
        ReportActiveXError( hresult, exception, argerr, cx );
        return false;
    }

    return true;
}

ActiveX::PropInfo* ActiveX::Find( std::wstring_view name )
{
    auto elem = properties_.find( name.data() );
    if ( elem == properties_.end() )
    {
        return nullptr;
    }

    return ( elem->second ).get();
}

bool ActiveX::SetupMembers( JSContext* cx, JS::HandleObject obj )
{
    HRESULT hresult;
    if ( pUnknown_ && !pDispatch_ )
    {
        hresult = pUnknown_->QueryInterface( IID_IDispatch, (void * *)&pDispatch_ );
    }

    if ( !pDispatch_ )
    {
        return false;
    }

    JS::RootedObject doc( cx, JS_NewPlainObject( cx ) );
    JS_DefineProperty( cx, obj, "members", doc, JSPROP_ENUMERATE );

    if ( !pTypeInfo_ )
    {
        unsigned ctinfo;
        hresult = pDispatch_->GetTypeInfoCount( &ctinfo );

        if ( SUCCEEDED( hresult ) && ctinfo )
        {
            pDispatch_->GetTypeInfo( 0, 0, &pTypeInfo_ );
        }
    }

    if ( !pTypeInfo_ )
    {
        return false;
    }

    VARDESC * vardesc;
    for ( size_t i = 0; pTypeInfo_->GetVarDesc( i, &vardesc ) == S_OK && i < 255; ++i )
    {
        BSTR name = nullptr;
        BSTR desc = nullptr;
        if ( pTypeInfo_->GetDocumentation( vardesc->memid, &name, &desc, nullptr, nullptr ) == S_OK )
        {
            PropInfo * p = Find( (wchar_t*)name );
            if ( !p )
            {
                auto& [it, bRet] = properties_.emplace( name, std::make_unique<PropInfo>( (wchar_t*)name ) );
                p = it->second.get();

                JS_DefineUCProperty( cx, obj, (char16_t*)name, SysStringLen( name ),
                                     ActiveX_JSGet, ActiveX_JSSet, JSPROP_ENUMERATE );
            }
            p->Get = p->Put = true;

            JS::RootedValue d( cx );
            if ( desc && *desc )
            {
                std::wstring wStr( desc, SysStringLen( desc ) );
                mozjs::convert::to_js::ToValue( cx, wStr, &d );
            }
            else
            {
                d.setNull();
            }

            JS_DefineUCProperty( cx, doc, (char16_t*)name, SysStringLen( name ), d, JSPROP_ENUMERATE );

            SysFreeString( name );
            SysFreeString( desc );
        }
        pTypeInfo_->ReleaseVarDesc( vardesc );
    }

    FUNCDESC * funcdesc;
    for ( size_t i = 0; pTypeInfo_->GetFuncDesc( i, &funcdesc ) == S_OK; ++i )
    {
        BSTR name = nullptr;
        BSTR desc = nullptr;

        if ( pTypeInfo_->GetDocumentation( funcdesc->memid, &name, &desc, nullptr, nullptr ) == S_OK )
        {
            if ( funcdesc->invkind == INVOKE_FUNC )
            {
                JS_DefineUCFunction( cx, obj, (char16_t*)name, SysStringLen( name ),
                                     *ActiveX_Run, funcdesc->cParams, JSPROP_ENUMERATE );
            }
            else
            {
                PropInfo * p = Find( (wchar_t*)name );
                if ( !p )
                {
                    auto&[it, bRet] = properties_.emplace( name, std::make_unique<PropInfo>( (wchar_t*)name ) );
                    p = it->second.get();

                    JS_DefineUCProperty( cx, obj, (char16_t*)name,
                                         SysStringLen( name ), ActiveX_JSGet, ActiveX_JSSet, JSPROP_ENUMERATE );
                }

                if ( funcdesc->invkind & INVOKE_PROPERTYGET )
                {
                    p->Get = true;
                }
                if ( funcdesc->invkind & INVOKE_PROPERTYPUT )
                {
                    p->Put = true;
                }
                if ( funcdesc->invkind & INVOKE_PROPERTYPUTREF )
                {
                    p->PutRef = true;
                }
            }

            JS::RootedValue d( cx );
            if ( desc && *desc )
            {
                std::wstring wStr( desc, SysStringLen( desc ) );
                mozjs::convert::to_js::ToValue( cx, wStr, &d );
            }
            else
            {
                d.setNull();
            }

            JS_DefineUCProperty( cx, doc, (char16_t*)name, SysStringLen( name ), d, JSPROP_ENUMERATE );

            SysFreeString( name );
            SysFreeString( desc );
        }
        pTypeInfo_->ReleaseFuncDesc( funcdesc );
    }

    return true;
}


/*
CoInitialize(nullptr);
ActiveX_InitClass(cx,obj);
CoFreeUnusedLibraries();
CoUninitialize();
*/

