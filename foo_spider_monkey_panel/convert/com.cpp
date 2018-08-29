/// Code extracted from wrap_com.cpp from JSDB by Shanti Rao (see http://jsdb.org)

#include <stdafx.h>
#include "com.h"

#include <script_interface.h>
#include <com_tools.h>

#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_objects/active_x_object.h>

#include <convert/js_to_native.h>


// TODO: add VT_ARRAY <> JSArray and VT_DISPATCH <> JSFunction support

namespace
{

using namespace mozjs;

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

        pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>(JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ));
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

    STDMETHODIMP ExecuteValue( VARIANT* pVariant )
    {
        if ( !pVariant )
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

        if ( !convert::com::JsToVariant( pJsCtx_, retVal, *pVariant ) )
        {
            pVariant->vt = VT_ERROR;
            pVariant->scode = 0;
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

namespace mozjs::convert::com
{

/// VariantToJs assumes that the caller will call VariantClear, so call AddRef on new objects
bool VariantToJs( JSContext* cx, VARIANTARG& var, JS::MutableHandleValue rval )
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

            std::unique_ptr<ActiveXObject> x( new ActiveXObject( cx, FETCH( punkVal ), true ) );
            if ( !x->pUnknown_ && !x->pDispatch_ )
            {
                return false;
            }

            JS::RootedObject jsObject( cx, ActiveXObject::CreateJsFromNative( cx, std::move( x ) ) );
            if ( !jsObject )
            {
                return false;
            }

            rval.setObjectOrNull( jsObject );
            break;
        }
        case VT_DISPATCH:
        {
            if ( !FETCH( pdispVal ) )
            {
                rval.setNull(); 
                break;
            }

            std::unique_ptr<ActiveXObject> x( new ActiveXObject( cx, FETCH( pdispVal ), true ) );
            if ( !x->pUnknown_ && !x->pDispatch_ )
            {
                return false;
            }

            JS::RootedObject jsObject( cx, ActiveXObject::CreateJsFromNative( cx, std::move( x ) ) );
            if ( !jsObject )
            {
                return false;
            }

            rval.setObjectOrNull( jsObject );
            break;
        }
        case VT_VARIANT: //traverse the indirection list?
            if ( ref )
            {
                VARIANTARG* v = var.pvarVal;
                if ( v )
                {
                    return VariantToJs( cx, *v, rval );
                }
            }
            break;

        default:
            if ( type <= VT_CLSID )
            {
                JS::RootedObject jsObject( cx, ActiveXObject::CreateJsFromNative( cx, std::make_unique<ActiveXObject>( cx, var ) ) );
                if ( !jsObject )
                {
                    return false;
                }

                rval.setObjectOrNull( jsObject );
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

bool JsToVariant( JSContext* cx, JS::HandleValue rval, VARIANTARG& arg )
{
    VariantInit( &arg );

    if ( rval.isObject() )
    {
        JS::RootedObject j0( cx, rval.toObjectOrNull() );
        if ( j0 && JS_InstanceOf( cx, j0, &ActiveXObject::JsClass, 0 ) )
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

            arg.vt = VT_DISPATCH;
            arg.pdispVal = new com_object_impl_t<WrappedJs>( cx, func );
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
        auto str = convert::to_native::ToValue<std::wstring>( cx, rval );
        if ( !str )
        {
            arg.vt = VT_EMPTY; // Avoid cleaning up garbage
            return false;
        }

        _bstr_t bStr = str.value().c_str();

        arg.vt = VT_BSTR;
        arg.bstrVal = bStr.Detach();
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

}
