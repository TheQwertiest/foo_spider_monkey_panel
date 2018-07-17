/// Code extracted from wrap_com.cpp from JSDB by Shanti Rao (see http://jsdb.org)

#include <stdafx.h>
#include "active_x_object.h"

#include <js_engine/js_engine.h>
#include <js_engine/js_to_native_converter.h>
#include <js_engine/native_to_js_converter.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_prototype_helpers.h>

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

#ifndef DISPID_PROPERTYPUT
#   define DISPID_PROPERTYPUT (-3)
#endif

bool VariantToJs( VARIANTARG& var, JSContext* cx, JS::MutableHandleValue rval );
bool JsToVariant( VARIANTARG& arg, JSContext* cx, JS::HandleValue rval );


namespace
{

using namespace mozjs;

// Wrapper to intercept indexed gets/sets.
class ActiveXObjectProxyHandler : public js::ForwardingProxyHandler
{
public:
    static const ActiveXObjectProxyHandler singleton;
    // family must contain unique pointer, so the value does not really matter
    static const char family;

    constexpr ActiveXObjectProxyHandler() : js::ForwardingProxyHandler( &family )
    {
    }

    bool get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
              JS::HandleId id, JS::MutableHandleValue vp ) const override;
    bool set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
              JS::HandleValue receiver, JS::ObjectOpResult& result ) const override;
    bool call( JSContext* cx, JS::HandleObject proxy, const JS::CallArgs& args ) const override;
};

const ActiveXObjectProxyHandler ActiveXObjectProxyHandler::singleton;
const char ActiveXObjectProxyHandler::family = 'Q';

bool
ActiveXObjectProxyHandler::get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
                                JS::HandleId id, JS::MutableHandleValue vp ) const
{
    if ( !JSID_IS_STRING( id ) )
    {
        JS_ReportErrorUTF8( cx, "Property id is not a string" );
        return false;
    }

    JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
    auto pNativeTarget = static_cast<ActiveXObject*>( JS_GetPrivate( target ) );
    assert( pNativeTarget );

    JS::RootedString jsString( cx, JSID_TO_STRING( id ) );
    assert( jsString );

    bool isValid;
    std::wstring propName = convert::to_native::ToValue( cx, jsString, isValid );
    if ( isValid )
    {
        if ( !JS_IsExceptionPending( cx ) )
        {
            JS_ReportErrorUTF8( cx, "Failed to parse property name" );
        }
        return false;
    }

    DISPID dispid;
    if ( !pNativeTarget->GetDispId( propName.c_str(), dispid ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( propName.c_str() );
        JS_ReportErrorUTF8( cx, "Failed to get dispid for `%s`", tmpStr.c_str() );
    }

    return pNativeTarget->Get( cx, nullptr, vp, dispid );
}

bool
ActiveXObjectProxyHandler::set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
                                JS::HandleValue receiver, JS::ObjectOpResult& result ) const
{
    if ( !JSID_IS_STRING( id ) )
    {
        JS_ReportErrorUTF8( cx, "Property id is not a string" );
        return false;
    }

    JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
    auto pNativeTarget = static_cast<ActiveXObject*>( JS_GetPrivate( target ) );
    assert( pNativeTarget );

    JS::RootedString jsString( cx, JSID_TO_STRING( id ) );
    assert( jsString );

    bool isValid;
    std::wstring propName = convert::to_native::ToValue( cx, jsString, isValid );
    if ( isValid )
    {
        if ( !JS_IsExceptionPending( cx ) )
        {
            JS_ReportErrorUTF8( cx, "Failed to parse property name" );
        }
        return false;
    }

    DISPID dispid;
    if ( !pNativeTarget->GetDispId( propName.c_str(), dispid ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( propName.c_str() );
        JS_ReportErrorUTF8( cx, "Failed to get dispid for `%s`", tmpStr.c_str() );
    }

    // TODO: Add by ref handling

    if ( !pNativeTarget->Set( cx, v, dispid, false ) )
    {// report in set
        return false;
    }

    result.succeed();
    return true;
}

bool ActiveXObjectProxyHandler::call( JSContext* cx, JS::HandleObject proxy, const JS::CallArgs& args ) const
{
    JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
    auto pNativeTarget = static_cast<ActiveXObject*>( JS_GetPrivate( target ) );
    assert( pNativeTarget );

    JS::RootedString jsString( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    if ( !jsString )
    {
        JS_ReportErrorUTF8( cx, "Failed to get function name" );
        return false;
    }

    bool isValid;
    std::wstring functionName = convert::to_native::ToValue( cx, jsString, isValid );
    if ( isValid )
    {
        if ( !JS_IsExceptionPending( cx ) )
        {
            JS_ReportErrorUTF8( cx, "Failed to parse property name" );
        }
        return false;
    }

    DISPID dispid;
    if ( !pNativeTarget->GetDispId( functionName.c_str(), dispid ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( functionName.c_str() );
        JS_ReportErrorUTF8( cx, "Failed to get dispid for `%s`", tmpStr.c_str() );
    }

    return pNativeTarget->Invoke( cx, args, dispid );
}

}


namespace
{

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

        pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ) );
        assert( pNativeGlobal_ );

        auto& heapMgr = pNativeGlobal_->GetHeapManager();

        heapMgr.RegisterUser( this );

        JS::RootedValue funcValue( cx, JS::ObjectValue( *funcObject ) );
        funcId_ = heapMgr.Store( funcValue );
        JS::RootedValue globalValue( cx, JS::ObjectValue( *jsGlobal ) );
        globalId_ = heapMgr.Store( globalValue );

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

        auto& heapMgr = pNativeGlobal_->GetHeapManager();

        heapMgr.Remove( globalId_ );
        heapMgr.Remove( funcId_ );
        heapMgr.UnregisterUser( this );
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

        auto& heapMgr = pNativeGlobal_->GetHeapManager();

        JSAutoRequest ar( pJsCtx_ );
        JS::RootedObject jsGlobal( pJsCtx_, heapMgr.Get( globalId_ ).toObjectOrNull() );
        assert( jsGlobal );
        JSAutoCompartment ac( pJsCtx_, jsGlobal );

        JS::RootedValue vFunc( pJsCtx_, heapMgr.Get( funcId_ ) );
        JS::RootedFunction rFunc( pJsCtx_, JS_ValueToFunction( pJsCtx_, vFunc ) );

        JS::RootedValue retVal( pJsCtx_ );
        if ( !JS::Call( pJsCtx_, jsGlobal, rFunc, JS::HandleValueArray::empty(), &retVal ) )
        {// TODO: set fail somehow
            JS_ClearPendingException( pJsCtx_ ); ///< can't forward exceptions inside ActiveXObject objects (see reasons above)...
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

}

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
    mozjs::JsFinalizeOp<ActiveXObject>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ActiveXObject",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE, // COM objects must be finalized in foreground
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

    auto jsObject = ActiveXObject::Create( cx, name );
    if ( !jsObject )
    {// report in ctor
        return false;
    }

    args.rval().setObjectOrNull( jsObject );
    return true;
}

MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_Constructor, ActiveX_Constructor_Impl )

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

            std::unique_ptr<ActiveXObject> x( new ActiveXObject( FETCH( punkVal ), true ) );
            if ( !x->pUnknown_ && !x->pDispatch_ )
            {
                return false;
            }

            rval.setObjectOrNull( ActiveXObject::Create( cx, x.release() ) );
            break;
        }
        case VT_DISPATCH:
        {
            if ( !FETCH( pdispVal ) )
            {
                rval.setNull(); 
                break;
            }

            std::unique_ptr<ActiveXObject> x( new ActiveXObject( FETCH( pdispVal ), true ) );
            if ( !x->pUnknown_ && !x->pDispatch_ )
            {
                return false;
            }

            rval.setObjectOrNull( ActiveXObject::Create( cx, x.release() ) );
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
                std::unique_ptr<ActiveXObject> x( new ActiveXObject( var ) );
                rval.setObjectOrNull( ActiveXObject::Create( cx, x.release() ) );

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

bool JsToVariant( VARIANTARG& arg, JSContext* cx, JS::HandleValue rval )
{
    VariantInit( &arg );

    if ( rval.isObject() )
    {
        JS::RootedObject j0( cx, rval.toObjectOrNull() );
        if ( j0 && JS_InstanceOf( cx, j0, &jsClass, 0 ) )
        {
            ActiveXObject* x = static_cast<ActiveXObject*>( JS_GetPrivate( j0 ) );
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

void CheckReturn( JSContext* cx, JS::HandleValue valToCheck )
{
    if ( !valToCheck.isObject() )
    {
        return;
    }

    HRESULT hresult;
    JS::RootedObject j0( cx, &valToCheck.toObject() );

    ActiveXObject* x = static_cast<ActiveXObject*>( JS_GetInstancePrivate( cx, j0, &jsClass, nullptr ) );
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

void ReportActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr, JSContext* cx )
{
    switch ( hresult )
    {
    case DISP_E_BADPARAMCOUNT:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Wrong number of parameters" );
        break;
    }
    case DISP_E_BADVARTYPE:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Bad variable type %d", argerr );
        break;
    }
    case DISP_E_EXCEPTION:
    {
        if ( exception.bstrDescription )
        {
            pfc::string8_fast descriptionStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrDescription, SysStringLen( exception.bstrDescription ) )
            );
            pfc::string8_fast sourceStr( pfc::stringcvt::string_utf8_from_wide(
                (wchar_t*)exception.bstrSource, SysStringLen( exception.bstrSource ) )
            );

            JS_ReportErrorUTF8( cx, "ActiveXObject: (%s) %s", sourceStr.c_str(), descriptionStr.c_str() );
        }
        else
        {
            JS_ReportErrorUTF8( cx, "ActiveXObject: Error code %d", exception.scode );
        }
        SysFreeString( exception.bstrSource );
        SysFreeString( exception.bstrDescription );
        SysFreeString( exception.bstrHelpFile );
        break;
    }
    case DISP_E_MEMBERNOTFOUND:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Function not found" );
        break;
    }
    case DISP_E_OVERFLOW:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Can not convert variable %d", argerr );
        break;
    }
    case DISP_E_PARAMNOTFOUND:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Parameter %d not found", argerr );
        break;
    }
    case DISP_E_TYPEMISMATCH:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Parameter %d type mismatch", argerr );
        break;
    }
    case DISP_E_UNKNOWNINTERFACE:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Unknown interface" );
        break;
    }
    case DISP_E_UNKNOWNLCID:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Unknown LCID" );
        break;
    }
    case DISP_E_PARAMNOTOPTIONAL:
    {
        JS_ReportErrorUTF8( cx, "ActiveXObject: Parameter %d is required", argerr );
        break;
    }
    default:
    {
    }
    }
}

namespace mozjs
{

const JSClass ActiveXObject::JsClass = jsClass;

ActiveXObject::ActiveXObject( JSContext* cx, VARIANTARG& var )
    : pJsCtx_( cx )
{
    pUnknown_ = nullptr;
    pTypeInfo_ = nullptr;
    pDispatch_ = nullptr;
    VariantInit( &variant_ );
    VariantCopyInd( &variant_, &var );
}

ActiveXObject::ActiveXObject( JSContext* cx, IDispatch *pDispatch, bool addref )
    : pJsCtx_( cx )
{
    memset( &variant_, 0, sizeof( variant_ ) );
    pDispatch_ = pDispatch;

    if ( !pDispatch_ )
    {
        return;
    }
    if ( addref )
    {
        pDispatch_->AddRef();
    }
}

ActiveXObject::ActiveXObject( JSContext* cx, IUnknown* pUnknown, bool addref )
    : pJsCtx_( cx )
{
    memset( &variant_, 0, sizeof( variant_ ) );

    pUnknown_ = pUnknown;
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

ActiveXObject::ActiveXObject( JSContext* cx, CLSID& clsid )
    : pJsCtx_( cx )
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
    }

    hresult = pUnknown_->QueryInterface( IID_IDispatch, (void * *)&pDispatch_ );

    //maybe I don't know what to do with it, but it might get passed to
    //another COM function
    if ( !SUCCEEDED( hresult ) )
    {
        pDispatch_ = nullptr;
    }
}

ActiveXObject::~ActiveXObject()
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

JSObject* ActiveXObject::InitPrototype( JSContext *cx, JS::HandleObject parentObject )
{
    return JS_InitClass( cx, parentObject, nullptr, &jsClass,
                         ActiveX_Constructor, 0,
                         nullptr, nullptr, nullptr, nullptr );
}
JSObject* ActiveXObject::Create( JSContext* cx, const std::wstring& name )
{
    CLSID clsid;
    HRESULT hresult = ( name[0] == L'{' )
        ? CLSIDFromString( name.c_str(), &clsid )
        : CLSIDFromProgID( name.c_str(), &clsid );
    if ( !SUCCEEDED( hresult ) )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
        JS_ReportErrorUTF8( cx, "Invalid CLSID" );
        return nullptr;
    }

    std::unique_ptr<ActiveXObject> nativeObject;
    IUnknown* unk = nullptr;
    hresult = GetActiveObject( clsid, nullptr, &unk );
    if ( SUCCEEDED( hresult ) && unk )
    {
        nativeObject.reset( new ActiveXObject( cx,unk ) );
        if ( !nativeObject->pUnknown_ )
        {
            pfc::string8_fast cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
            JS_ReportErrorUTF8( cx, "Failed to create ActiveXObject object via IUnknown: %s", cStr.c_str() );
            return nullptr;
        }
    }

    if ( !nativeObject )
    {
        nativeObject.reset( new ActiveXObject( cx, clsid ) );
        if ( !nativeObject->pUnknown_ )
        {
            pfc::string8_fast cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
            JS_ReportErrorUTF8( cx, "Failed to create ActiveXObject object via CLSID: %s", cStr.c_str() );
            return nullptr;
        }
    }

    return Create( cx, nativeObject.release() );
}

JSObject* ActiveXObject::Create( JSContext* cx, ActiveXObject* pPremadeNative )
{
    if ( !pPremadeNative )
    {
        JS_ReportErrorUTF8( cx, "Internal error: pPremadeNative is null" );
        return nullptr;
    }

    std::unique_ptr<ActiveXObject> autoNative( pPremadeNative );

    JS::RootedObject jsProto( cx, mozjs::GetPrototype<ActiveXObject>( cx, mozjs::JsPrototypeId::ActiveX ) );
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

bool ActiveXObject::GetDispId( std::wstring_view name, DISPID& dispid )
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

bool ActiveXObject::Invoke( const JS::CallArgs& callArgs, DISPID dispid )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    uint32_t argc = callArgs.length();
    VARIANT VarResult;
    std::unique_ptr<VARIANTARG[]> args;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args.reset( new VARIANTARG[argc] );
        dispparams.rgvarg = args.get();
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !JsToVariant( args[argc - i - 1], pJsCtx_, callArgs[i] ) )
            {
                args[argc - i - 1].vt = VT_ERROR;
                args[argc - i - 1].scode = 0;
            }
        }
    }

    VariantInit( &VarResult );

    // don't use DispInvoke, because we don't know the TypeInfo
    HRESULT hresult = pDispatch_->Invoke( dispid,
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          DISPATCH_METHOD,
                                          &dispparams, &VarResult, &exception, &argerr );

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( pJsCtx_, callArgs[i] ); //in case any empty ActiveXObject objects were filled in by Invoke()
        VariantClear( &args[i] );
    }

    if ( !SUCCEEDED( hresult ) )
    {
        VariantClear( &VarResult );
        ReportActiveXError( hresult, exception, argerr, pJsCtx_ );
        return false;
    }

    if ( !VariantToJs( VarResult, pJsCtx_, callArgs.rval() ) )
    {
        callArgs.rval().setUndefined();
    }

    VariantClear( &VarResult );
    return true;
}

bool ActiveXObject::Get( JS::HandleValue idxArg, JS::MutableHandleValue vp, DISPID dispid )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    VARIANTARG additionalArg;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };

    if ( idxArg.isNumber() )
    {
        dispparams.rgvarg = &additionalArg;
        dispparams.cArgs = 1;

        if ( !JsToVariant( additionalArg, pJsCtx_, idxArg ) )
        {
            additionalArg.vt = VT_ERROR;
            additionalArg.scode = 0;
        }
    }

    VARIANT VarResult;
    VariantInit( &VarResult );

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    HRESULT hresult = pDispatch_->Invoke( dispid,
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          DISPATCH_PROPERTYGET,
                                          &dispparams, &VarResult, &exception, &argerr );
    VariantClear( &additionalArg );

    if ( !SUCCEEDED( hresult ) )
    {
        ReportActiveXError( hresult, exception, argerr, pJsCtx_ );
        return false;
    }

    if ( !VariantToJs( VarResult, pJsCtx_, vp ) )
    {
        vp.setUndefined();
    }
    VariantClear( &VarResult );

    return true;
}

bool ActiveXObject::Set( JS::HandleValue v, DISPID dispid, bool ref )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    VARIANTARG arg;
    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { &arg,&dispput,1,1 };

    if ( !JsToVariant( arg, pJsCtx_, v ) )
    {
        arg.vt = VT_ERROR;
        arg.scode = 0;
    }

    DWORD flag = DISPATCH_PROPERTYPUT;
    if ( ref && ( arg.vt & VT_DISPATCH || arg.vt & VT_UNKNOWN ) )
    {//must be passed by name
        flag = DISPATCH_PROPERTYPUTREF;
    }

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    HRESULT hresult = pDispatch_->Invoke( dispid,
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          (WORD)flag,
                                          &dispparams, nullptr, &exception, &argerr );

    CheckReturn( pJsCtx_, v );
    VariantClear( &arg );

    if ( !SUCCEEDED( hresult ) )
    {
        ReportActiveXError( hresult, exception, argerr, pJsCtx_ );
        return false;
    }

    return true;
}

bool ActiveXObject::GetAllPutProperties()
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
    for ( size_t i = 0; pTypeInfo_->GetVarDesc( i, &vardesc ) == S_OK; ++i )
    {
        BSTR name = nullptr;
        //BSTR desc = nullptr;
        if ( pTypeInfo_->GetDocumentation( vardesc->memid, &name, nullptr /*&desc*/, nullptr, nullptr ) == S_OK )
        {
            properties_.emplace( name, std::make_unique<PropInfo>( (wchar_t*)name ) );

            SysFreeString( name );
            //SysFreeString( desc );
        }
        pTypeInfo_->ReleaseVarDesc( vardesc );
    }

    FUNCDESC * funcdesc;
    for ( size_t i = 0; pTypeInfo_->GetFuncDesc( i, &funcdesc ) == S_OK; ++i )
    {
        BSTR name = nullptr;
        //BSTR desc = nullptr;

        if ( pTypeInfo_->GetDocumentation( funcdesc->memid, &name, nullptr /*&desc*/, nullptr, nullptr ) == S_OK )
        {
            if ( INVOKE_PROPERTYPUT == funcdesc->invkind
                 || INVOKE_PROPERTYPUTREF == funcdesc->invkind )
            {
                auto[it, bRet] = properties_.emplace( name, std::make_unique<PropInfo>( (wchar_t*)name ) );
                if ( INVOKE_PROPERTYPUT == funcdesc->invkind )
                {
                    auto pProp = it->second.get();
                    pProp->isPutRef = true;
                }
            }

            SysFreeString( name );
            //SysFreeString( desc );
        }
        pTypeInfo_->ReleaseFuncDesc( funcdesc );
    }

    return true;
}

}
