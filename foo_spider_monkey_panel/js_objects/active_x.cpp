/// Code extracted from wrap_com.cpp from JSDB by Shanti Rao (see http://jsdb.org)

#include <stdafx.h>
#include "active_x.h"

#include <js_engine/js_engine.h>
#include <js_engine/js_to_native_converter.h>
#include <js_engine/native_to_js_converter.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/global_object.h>
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


JSObject*
ActiveX_CreateObject( JSContext *cx, ActiveX* t );

bool VariantToJs( VARIANTARG& var, JSContext* cx, JS::MutableHandleValue rval );
bool JsToVariant( VARIANTARG& arg, JSContext* cx, JS::HandleValue rval );

/////////////////////////////////////////////////

class WrappedJs
    : public IDispatchImpl3<IWrappedJs>
    , public mozjs::IHeapUser
{
protected:
    WrappedJs( JSContext * cx, JS::HandleFunction jsFunction )
        : pJsCtx_(cx)
    {
        assert( cx );

        JS::RootedObject funcObject (cx, JS_GetFunctionObject( jsFunction ));
        JS::RootedValue funcValue( cx, JS::ObjectValue( *funcObject ));

        JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
        assert( jsGlobal );
        JS::RootedValue globalValue( cx, JS::ObjectValue( *jsGlobal ) );        

        pNativeGlobal_ = mozjs::GetNativeFromJsObject<mozjs::JsGlobalObject>( cx, jsGlobal );
        assert( pNativeGlobal_ );

        pNativeGlobal_->RegisterHeapUser( this );
        funcId_ = pNativeGlobal_->StoreToHeap( funcValue );
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
        JS::RootedObject jsGlobal( pJsCtx_, pNativeGlobal_->GetFromHeap(globalId_).toObjectOrNull() );
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

JSClass ActiveX_Class = {
    "ActiveXObject",
    JSCLASS_HAS_PRIVATE | JSCLASS_BACKGROUND_FINALIZE,
    &jsOps
};


//shadow property 0 to 255
bool
ActiveX_JSGet_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{// JSContext *cx, JSObject *obj, jsid id, jsval *rval 
    /*
    if ( !JSVAL_IS_INT( id ) ) return false;
    int x = JSVAL_TO_INT( id );
    */

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    // TODO: dirty hack! Think of a way to replace it
    JS::RootedString s( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    if ( !s )
    {
        JS_ReportErrorUTF8( cx, "ActiveX_Exec error: No property name" );
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
    
    /*
    GETENV;
    GETOBJ2( ActiveX, ActiveX, t );*/
    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    /*
    Default

    switch ( x )
    {
    case 255: 
    {
        JS::RootedString str (JS_NewUCStringCopyZ( cx, (char16_t*)L"ActiveX" ));
        if ( !str )
        {
            return false;
        }
        args.rval().setString( str );
        return true;
    }
    }
    */

    ActiveX::PropInfo * p = t->Find(name);
    DISPID dispid = 0;
    if ( p && t->Id( p->name, dispid ) )
    {
        return t->Get( dispid, cx, argc, vp );
    }

    return false;
}

bool
ActiveX_JSSet_Impl( JSContext *cx, unsigned argc, JS::Value* vp )
{
    /*
    if ( !JSVAL_IS_INT( id ) ) return false;
    int x = JSVAL_TO_INT( id );
    */

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
    /*
    GETENV;
    GETOBJ2( ActiveX, ActiveX, t );
    */

    // TODO: dirty hack! Think of a way to replace it
    JS::RootedString s( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    if ( !s )
    {
        JS_ReportErrorUTF8( cx, "ActiveX_Exec error: No property name" );
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
    name = name.substr( fPos + 1);    

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    // DISPID dispid=0;
    // if (t->Id(id,dispid))

    ActiveX::PropInfo * p = t->Find( name );
    DISPID dispid = 0;
    if ( p && t->Id( p->name, dispid ) )
    {
        return t->Set( dispid, cx, argc, vp, p->PutRef );
    }

    return false;
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
    /*
    GETENV;
    GETARGS;
    GETOBJ( ActiveX, ActiveX, t );
    */

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorUTF8( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }
    
    JS::RootedString s (cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ));
    if ( !s )
    {
        JS_ReportErrorUTF8( cx, "ActiveX_Exec error: No function name" );
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
    if ( !t->Id( name, dispid ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "ActiveX error: This object does not have that function: %s", tmpStr.c_str() );
        return false;
    }
    if ( !t->Invoke( dispid, cx, argc, vp ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "ActiveX error: IDispatch->Invoke failed: %s", tmpStr.c_str() );
        return false;
        //      RETOBJ(0);
                //JavaScript handles the exception with SetPendingException
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

#define FETCH(x) (ref? * (var.p ## x) : var.x)

/// VariantToJs assumes that the caller will call VariantClear, so call AddRef on new objects
bool VariantToJs( VARIANTARG& var, JSContext* cx, JS::MutableHandleValue rval )
{
    /*
    ENTERNATIVE( cx );
    */

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
        case VT_I1: rval.setInt32( static_cast<int32_t>(FETCH( cVal )) ); break;
        case VT_I2: rval.setInt32( static_cast<int32_t>(FETCH( iVal )) ); break;
        case VT_INT:
        case VT_I4: rval.setInt32( FETCH( lVal ) ); break;
        case VT_R4: rval.setNumber( FETCH( fltVal ) ); break;
        case VT_R8: rval.setNumber( FETCH( dblVal ) ); break;

        case VT_BOOL: rval.setBoolean( FETCH( boolVal ) ? true : false ); break;

        case VT_UI1: rval.setNumber( static_cast<uint32_t>(FETCH( bVal )) ); break;
        case VT_UI2: rval.setNumber( static_cast<uint32_t>(FETCH( uiVal )) ); break;
        case VT_UINT:
        case VT_UI4: rval.setNumber( static_cast<uint32_t>(FETCH( ulVal )) ); break;

        case VT_BSTR:
        {
            JS::RootedString rstr( cx, JS_NewUCStringCopyN( cx, (char16_t*) FETCH( bstrVal ), SysStringLen( FETCH( bstrVal ) ) ) );
            rval.setString( rstr );
            break;
            //              SysFreeString(FETCH(bstrVal));
            //              var.vt = VT_EMPTY;
        };
        case VT_DATE:
        {
            DATE d = FETCH( date );
            SYSTEMTIME time;
            VariantTimeToSystemTime( d, &time );
            JS::RootedObject rDate( cx, JS_NewDateObject( cx, time.wYear, time.wMonth - 1, time.wDay,
                                                          time.wHour, time.wMinute, time.wSecond ) );
            rval.setObjectOrNull( rDate );

            break;
        }

        case VT_UNKNOWN:
        {
            if ( !FETCH( punkVal ) ) { rval.setNull(); break; }
            std::unique_ptr<ActiveX> x( new ActiveX( FETCH( pdispVal ), true ) );
            if ( !x->pUnknown_ && !x->pDispatch_ ) { return false; }
            rval.setObjectOrNull( ActiveX_CreateObject( cx, x.release() ) );
            break;
        }
        case VT_DISPATCH:
        {
            if ( !FETCH( pdispVal ) ) { rval.setNull(); break; }
            std::unique_ptr<ActiveX> x (new ActiveX( FETCH( pdispVal ), true ));
            if ( !x->pUnknown_ && !x->pDispatch_ ) { return false; }
            rval.setObjectOrNull( ActiveX_CreateObject( cx, x.release() ) );
            break;
        }
        case VT_VARIANT: //traverse the indirection list?
            if ( ref )
            {
                VARIANTARG* v = var.pvarVal;
                if ( v )
                    return VariantToJs( *v, cx, rval );
            }
            break;

        default:
            if ( type <= VT_CLSID )
            {
                std::unique_ptr<ActiveX> x( new ActiveX( var ) );
                rval.setObjectOrNull( ActiveX_CreateObject( cx, x.release() ) );

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
    if ( rval.isObject() )
    {
        HRESULT hresult;
        JS::RootedObject j0 (cx, rval.toObjectOrNull());
        if ( j0 && JS_InstanceOf( cx, j0, &ActiveX_Class, 0 ) )
        {
            ActiveX* x = static_cast<ActiveX*>( JS_GetPrivate( j0 ) );            
            if ( x->pUnknown_ && !x->pDispatch_ )
            {
                hresult = x->pUnknown_->QueryInterface( IID_IDispatch, (void **)&x->pDispatch_ );
                if ( SUCCEEDED( hresult ) )
                {
                    x->SetupMembers( cx, j0 );
                }
                else
                {
                    x->pDispatch_ = 0;
                }
            }
        }
    }
}

bool JsToVariant( VARIANTARG& arg, JSContext* cx, JS::HandleValue rval )
{
    VariantInit( &arg );
    // arg.vt = VT_EMPTY;
    
    if ( rval.isObject() )
    {        
        JS::RootedObject j0( cx, rval.toObjectOrNull() );
        if ( j0 && JS_InstanceOf( cx, j0, &ActiveX_Class, 0 ) )
        {
            ActiveX* x = static_cast<ActiveX*>(JS_GetPrivate( j0 ));
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

        if ( j0 && JS_ObjectIsFunction(cx, j0 ) )
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
        JS::RootedString rStr(cx, rval.toString() );
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

            s << (w1) << (wchar_t*)L": " << (w2) << (wchar_t*)L"\n";

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

JSObject*
ActiveX_CreateObject( JSContext *cx, ActiveX* t)
{
    // TODO: add creation via prototype

    /*
    GETENV;
    ENTERNATIVE( cx );
    */

    JS::RootedObject obj( cx, JS_NewObject( cx, &ActiveX_Class ));
    //JS_DefineFunctions( cx, obj, ActiveX_functions );
    //JS_DefineProperties( cx, obj, ActiveX_properties );
    /*obj = JS_NewObject(cx, &ActiveX_Class,Env->ActiveX, nullptr);
    JS_DefineFunctions(cx,obj,ActiveX_functions);
    JS_DefineProperties(cx,obj,ActiveX_properties);   */
    if ( t )
    {        
        JS_SetPrivate( obj, t );
        t->SetupMembers( cx, obj );
    }
    return obj;
}

bool ActiveX_Constructor( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    /*
    GETENV;
    if ( Env->SafeMode )
    {
        JS_ReportErrorUTF8( cx, "ActiveX error: blocked by security settings" );
        args.rval().setUndefined();
        return false;
    }
    */

    if ( argc )
    {
        if ( !args[0].isString() )
        {
            JS_ReportErrorUTF8( cx, "ActiveX_Exec error: argument 1 is not a string" );
            return false;
        }
    }        
    //ENTERNATIVE(cx);

    //argc > 0 if clsid is valid
    CLSID clsid;
    HRESULT hresult;   
    bool bRet = true;
    std::wstring name = argc ? mozjs::convert::to_native::ToValue<std::wstring>( cx, args[0], bRet ) : std::wstring();
    if ( !bRet )
    {
        JS_ReportErrorUTF8( cx, "ActiveX error: failed to parse name" );
        return false;
    }

    if ( argc )
    {
        if ( name[0] == L'{' )
            hresult = CLSIDFromString( name.c_str(), &clsid );
        else
            hresult = CLSIDFromProgID( name.c_str(), &clsid );

        if ( !SUCCEEDED( hresult ) )
        {
            pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
            JS_ReportErrorUTF8( cx, "ActiveX error: invalid CLSID" );
            return false;
        }
    }

    std::unique_ptr<ActiveX> t;

    if ( argc == 0 )
    {
        t.reset( new ActiveX() );
    }
    else
    {
        IUnknown* unk = nullptr;
        if ( argc == 1 )
        {
            hresult = GetActiveObject( clsid, nullptr, &unk );
        }

        if ( SUCCEEDED( hresult ) && unk )
        {
            t.reset( new ActiveX( unk ) );
            if ( !t->pUnknown_ )
            {
                pfc::string8_fast cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
                JS_ReportErrorUTF8( cx, "ActiveX error: Can't create ActiveX object: %s", cStr.c_str() );
                args.rval().setUndefined();
                return false;
            }
        }
    }

    if ( !t )
    {
        t.reset( new ActiveX( clsid ) );
        if ( !t->pUnknown_ )
        {
            pfc::string8_fast cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
            JS_ReportErrorUTF8( cx, "ActiveX error: Can't create ActiveX object: %s", cStr.c_str() );
            args.rval().setUndefined();
            return false;
        }
    }
    if ( t )
    {
        JS::RootedObject retObj( cx, ActiveX_CreateObject( cx, t.get() ) );
        if ( !retObj )
        {
            return false;
        }

        args.rval().setObjectOrNull( retObj );
    }

    t.release();
    return true;
}

JSObject* CreateActiveXProto( JSContext *cx, JS::HandleObject obj )
{
    /* INITCLASS( ActiveX ); */
    return JS_InitClass( cx, obj, nullptr, &ActiveX_Class,
                         ActiveX_Constructor, 0,
                         nullptr, nullptr, nullptr, nullptr );
    /*
    Env->oActiveX = JS_InitClass( cx, obj, nullptr, &ActiveX_Class,
                                  ActiveX_Constructor, 0,
                                   ActiveX_properties, ActiveX_functions, nullptr,ActiveX_fnstatic);
    */
}

ActiveX::ActiveX( VARIANTARG& var )
{
    pUnknown_ = nullptr;
    pTypeInfo_ = nullptr;
    pDispatch_ = nullptr;
    VariantInit( &variant_ );
    VariantCopyInd( &variant_, &var );
}

ActiveX::ActiveX()
{
    pUnknown_ = nullptr;
    pTypeInfo_ = nullptr;
    pDispatch_ = nullptr;
    memset( &variant_, 0, sizeof( variant_ ) );
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

    HRESULT hresult;

    hresult = pUnknown_->QueryInterface( IID_IDispatch, (void * *)&pDispatch_ );

    if ( !SUCCEEDED( hresult ) )
    {
        pDispatch_ = 0;
    }

    // else  QueryInterface calls AddRef() for you
      //  pDispatch_->AddRef();
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

bool ActiveX::Id( std::wstring_view name, DISPID& dispid )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    dispid = 0;
    HRESULT hresult;

    if ( name.empty() || name[0] == L'0' ) 
    { 
        dispid = 0; return true; 
    }

    wchar_t* cname = const_cast<wchar_t*>(name.data());
    hresult = pDispatch_->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_USER_DEFAULT, &dispid );

    if ( !SUCCEEDED( hresult ) )
    {
        hresult = pDispatch_->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_SYSTEM_DEFAULT, &dispid );

    }
    return SUCCEEDED( hresult );
}

bool ActiveX::Invoke( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANT VarResult;
    VARIANTARG * args;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args = new VARIANTARG[argc];
        dispparams.rgvarg = args;
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
    if ( argc ) delete[] args;

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

bool ActiveX::Get( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp, bool exceptions )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANT VarResult;
    VARIANTARG * args;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args = new VARIANTARG[argc];
        dispparams.rgvarg = args;
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !JsToVariant( args[i], cx, callArgs[i] ) )
            {
                args[i].vt = VT_ERROR;
                args[i].scode = 0;
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

    if ( argc ) delete[] args;

    if ( !SUCCEEDED( hresult ) )
    {
        if ( exceptions )
        {
            ReportActiveXError( hresult, exception, argerr, cx );
        }
        return false;
    }
    else if ( !VariantToJs( VarResult, cx, callArgs.rval() ) )
    {
        callArgs.rval().setUndefined();
    }
    VariantClear( &VarResult );

    return true;
}

bool ActiveX::Set( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp, bool byref )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANTARG * args = new VARIANTARG[argc];
    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { args,&dispput,argc,1 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    //the set value
    /*
    if ( !JsToVariant( args[0], cx, callArgs.rval() ) )
    {
        return false;
    }
    */

    //the index values, in reverse order
    if ( argc )
    {
        //   dispparams.rgvarg = args; //initialized in the declaration
        //   dispparams.cArgs = argc+1;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !JsToVariant( args[i], cx, callArgs[i] ) )
            {
                args[i].vt = VT_ERROR;
                args[i].scode = 0;
            }
        }
    }

    DWORD flag = DISPATCH_PROPERTYPUT;
    if ( byref && (args[0].vt & VT_DISPATCH || args[0].vt & VT_UNKNOWN) )
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

    if ( argc ) delete[] args;

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

    return (elem->second).get();
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

    /*
    ENTERNATIVE( cx );
    */

    JS::RootedObject doc(cx, JS_NewPlainObject( cx ) );
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

    size_t i;
    VARDESC * vardesc;
    for ( i = 0; pTypeInfo_->GetVarDesc( i, &vardesc ) == S_OK && i < 255; i++ )
    {
        BSTR name = nullptr;
        BSTR desc = nullptr;
        if ( pTypeInfo_->GetDocumentation( vardesc->memid, &name, &desc, nullptr, nullptr ) == S_OK )
        {
            PropInfo * p = Find( (wchar_t*)name );
            if ( !p )
            {
                p = new ActiveX::PropInfo( (wchar_t*)name );
            }
            p->Get = p->Put = true;
            properties_[name] = std::shared_ptr<PropInfo>( p );

            //JS_DefineUCPropertyWithTinyId
            JS_DefineUCProperty( cx, obj, (char16_t*)name, SysStringLen( name ),
                                 ActiveX_JSGet, ActiveX_JSSet, JSPROP_ENUMERATE );
            if ( doc )
            {
                JS::RootedValue d(cx);
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
            }
            SysFreeString( name );
            SysFreeString( desc );
        }
        pTypeInfo_->ReleaseVarDesc( vardesc );
    }

    FUNCDESC * funcdesc;
    for ( i = 0; pTypeInfo_->GetFuncDesc( i, &funcdesc ) == S_OK; i++ )
    {
        BSTR name = nullptr;
        BSTR desc = nullptr;

        if ( pTypeInfo_->GetDocumentation( funcdesc->memid, &name, &desc, nullptr, nullptr ) == S_OK )
        {
            //    char* fname = DeflateString((jschar*)name,SysStringLen(name));

            if ( funcdesc->invkind == INVOKE_FUNC )
            {
                //       JS_DefineFunction(cx,obj,fname,*ActiveX_Run,funcdesc->cParams,0);
                JS_DefineUCFunction( cx, obj, (char16_t*)name, SysStringLen( name ),
                                     *ActiveX_Run, funcdesc->cParams, 0 );
            }
            else
            {
                PropInfo * p = Find( (wchar_t*)name );

                if ( !p )
                {
                    p = new PropInfo( (wchar_t*)name );
                    properties_[name] = std::shared_ptr<PropInfo>(p);
                    JS_DefineUCProperty( cx, obj, (char16_t*)name,
                                                   SysStringLen( name ), ActiveX_JSGet, ActiveX_JSSet, JSPROP_ENUMERATE );
                }

                if ( funcdesc->invkind & INVOKE_PROPERTYGET )
                    p->Get = true;
                if ( funcdesc->invkind & INVOKE_PROPERTYPUT )
                    p->Put = true;
                if ( funcdesc->invkind & INVOKE_PROPERTYPUTREF )
                    p->PutRef = true;
            }
            //    if (fname) free(fname);

            if ( doc )
            {
                JS::RootedValue d( cx );
                if ( desc && *desc)
                {
                    std::wstring wStr( desc, SysStringLen( desc ) );
                    mozjs::convert::to_js::ToValue( cx, wStr, &d );
                }
                else
                {
                    d.setNull();
                }

                JS_DefineUCProperty( cx, doc, (char16_t*)name, SysStringLen( name ), d, JSPROP_ENUMERATE );
            }

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

