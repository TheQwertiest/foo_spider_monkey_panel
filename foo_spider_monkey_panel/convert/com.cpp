#include <stdafx.h>

#include "com.h"

#include <com_objects/com_interface.h>
#include <com_objects/com_tools.h>
#include <com_objects/dispatch_ptr.h>
#include <convert/js_to_native.h>
#include <js_objects/active_x_object.h>
#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <panel/user_message.h>

#include <qwr/error_popup.h>
#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Date.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

using namespace smp;

namespace
{

using namespace mozjs;
using namespace convert::com;

class WrappedJs
    : public com::IDispatchImpl3<IWrappedJs>
    , public mozjs::IHeapUser
{
protected:
    WrappedJs( JSContext* cx, JS::HandleFunction jsFunction )
        : pJsCtx_( cx )
    {
        assert( cx );

        JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
        assert( jsGlobal );

        pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>( JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ) );
        assert( pNativeGlobal_ );

        auto& heapMgr = pNativeGlobal_->GetHeapManager();

        heapMgr.RegisterUser( this );

        funcId_ = heapMgr.Store( jsFunction );
        globalId_ = heapMgr.Store( jsGlobal );

        isJsAvailable_ = true;
    }

    /// @details Might be called off main thread
    ~WrappedJs() override = default;

    /// @details Might be called off main thread
    void FinalRelease() override
    { // most of the JS object might be invalid at GC time,
        // so we need to be extra careful
        std::scoped_lock sl( cleanupLock_ );
        if ( !isJsAvailable_ )
        {
            return;
        }

        auto& heapMgr = pNativeGlobal_->GetHeapManager();

        heapMgr.Remove( globalId_ );
        heapMgr.Remove( funcId_ );
        heapMgr.UnregisterUser( this );
    }

    void PrepareForGlobalGc() override
    {
        std::scoped_lock sl( cleanupLock_ );
        // Global is being destroyed, can't access anything
        isJsAvailable_ = false;
    }

    /// @details Executed on the main thread
    STDMETHODIMP ExecuteValue( VARIANT arg1, VARIANT arg2, VARIANT arg3, VARIANT arg4, VARIANT arg5, VARIANT arg6, VARIANT arg7,
                               VARIANT* pResult ) override
    {
        if ( !pResult )
        {
            return E_POINTER;
        }

        if ( !isJsAvailable_ )
        { // shutting down, no need to log errors here
            pResult->vt = VT_ERROR;
            pResult->scode = E_FAIL;
            return E_FAIL;
        }

        // Might be executed outside of main JS workflow, so we need to set realm

        auto& heapMgr = pNativeGlobal_->GetHeapManager();

        JS::RootedObject jsGlobal( pJsCtx_, heapMgr.Get( globalId_ ).toObjectOrNull() );
        assert( jsGlobal );
        JSAutoRealm ac( pJsCtx_, jsGlobal );

        JS::RootedValue vFunc( pJsCtx_, heapMgr.Get( funcId_ ) );
        JS::RootedFunction rFunc( pJsCtx_, JS_ValueToFunction( pJsCtx_, vFunc ) );
        std::array<VARIANT*, 7> args = { &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7 };
        JS::AutoValueArray<args.size()> wrappedArgs( pJsCtx_ );

        try
        {
            for ( size_t i = 0; i < args.size(); ++i )
            {
                convert::com::VariantToJs( pJsCtx_, *args[i], wrappedArgs[i] );
            }

            JS::RootedValue retVal( pJsCtx_ );
            if ( !JS::Call( pJsCtx_, jsGlobal, rFunc, wrappedArgs, &retVal ) )
            {
                throw JsException();
            }

            convert::com::JsToVariant( pJsCtx_, retVal, *pResult );
        }
        catch ( ... )
        {
            const auto hWnd = pNativeGlobal_->GetPanelHwnd();
            if ( !hWnd )
            {
                mozjs::error::SuppressException( pJsCtx_ );
            }
            else
            {
                const auto errorMsg = mozjs::error::ExceptionToText( pJsCtx_ );
                SendMessage( hWnd, static_cast<UINT>( InternalSyncMessage::script_fail ), 0, reinterpret_cast<LPARAM>( &errorMsg ) );
            }

            pResult->vt = VT_ERROR;
            pResult->scode = E_FAIL;
            return E_FAIL;
        }

        return S_OK;
    }

private:
    JSContext* pJsCtx_ = nullptr;
    uint32_t funcId_;
    uint32_t globalId_;
    mozjs::JsGlobalObject* pNativeGlobal_ = nullptr;

    std::mutex cleanupLock_;
    bool isJsAvailable_ = false;
};

bool ComArrayToJsArray( JSContext* cx, const VARIANT& src, JS::MutableHandleValue& dest )
{
    // We only support one dimensional arrays for now
    qwr::QwrException::ExpectTrue( SafeArrayGetDim( src.parray ) == 1, "Multi-dimensional array are not supported failed" );

    // Get the upper bound;
    long ubound; // NOLINT (google-runtime-int)
    HRESULT hr = SafeArrayGetUBound( src.parray, 1, &ubound );
    qwr::error::CheckHR( hr, "SafeArrayGetUBound" );

    // Get the lower bound
    long lbound; // NOLINT (google-runtime-int)
    hr = SafeArrayGetLBound( src.parray, 1, &lbound );
    qwr::error::CheckHR( hr, "SafeArrayGetLBound" );

    // Create the JS Array
    JS::RootedObject jsArray( cx, JS_NewArrayObject( cx, ubound - lbound + 1 ) );
    JsException::ExpectTrue( jsArray );

    // Divine the type of our array
    VARTYPE vartype;
    if ( ( src.vt & VT_ARRAY ) != 0 )
    {
        vartype = src.vt & ~VT_ARRAY;
    }
    else // This was maybe a VT_SAFEARRAY
    {
        hr = SafeArrayGetVartype( src.parray, &vartype );
        qwr::error::CheckHR( hr, "SafeArrayGetVartype" );
    }

    JS::RootedValue jsVal( cx );
    for ( long i = lbound; i <= ubound; ++i ) // NOLINT (google-runtime-int)
    {
        HRESULT hr;
        _variant_t var;
        if ( vartype == VT_VARIANT )
        {
            hr = SafeArrayGetElement( src.parray, &i, &var );
        }
        else
        {
            var.vt = vartype;
            hr = SafeArrayGetElement( src.parray, &i, &var.byref );
        }
        qwr::error::CheckHR( hr, "SafeArrayGetElement" );

        VariantToJs( cx, var, &jsVal );

        if ( !JS_SetElement( cx, jsArray, i, jsVal ) )
        {
            throw JsException();
        }
    }

    dest.setObjectOrNull( jsArray );
    return true;
}

template <typename T>
void PutCastedElementInSafeArrayData( void* arr, size_t idx, T value )
{
    auto castedArr = static_cast<T*>( arr );
    castedArr[idx] = value;
}

void PutVariantInSafeArrayData( void* arr, size_t idx, VARIANTARG& var )
{
    switch ( var.vt )
    {
    case VT_I1:
        PutCastedElementInSafeArrayData<int8_t>( arr, idx, var.cVal );
        break;
    case VT_I2:
        PutCastedElementInSafeArrayData<int16_t>( arr, idx, var.iVal );
        break;
    case VT_INT:
    case VT_I4:
        PutCastedElementInSafeArrayData<int32_t>( arr, idx, var.lVal );
        break;
    case VT_R4:
        PutCastedElementInSafeArrayData<float>( arr, idx, var.fltVal );
        break;
    case VT_R8:
        PutCastedElementInSafeArrayData<double>( arr, idx, var.dblVal );
        break;
    case VT_BOOL:
        PutCastedElementInSafeArrayData<int16_t>( arr, idx, var.boolVal );
        break;
    case VT_UI1:
        PutCastedElementInSafeArrayData<uint8_t>( arr, idx, var.bVal );
        break;
    case VT_UI2:
        PutCastedElementInSafeArrayData<uint16_t>( arr, idx, var.uiVal );
        break;
    case VT_UINT:
    case VT_UI4:
        PutCastedElementInSafeArrayData<uint32_t>( arr, idx, var.ulVal );
        break;
    default:
        throw qwr::QwrException( "ActiveX: unsupported array type: {:#x}", var.vt );
    }
}

} // namespace

namespace mozjs::convert::com
{

/// VariantToJs assumes that the caller will call VariantClear on `var`, so call AddRef on new objects
void VariantToJs( JSContext* cx, VARIANTARG& var, JS::MutableHandleValue rval )
{
    const bool ref = !!( var.vt & VT_BYREF );
    const int type = ( ref ? var.vt &= ~VT_BYREF : var.vt );

#define FETCH( x ) ( ref ? *( var.p##x ) : var.x )

    switch ( type )
    {
    case VT_ERROR:
        rval.setUndefined();
        break;
    case VT_NULL:
        rval.setNull();
        break;
    case VT_EMPTY:
        rval.setUndefined();
        break;
    case VT_I1:
        rval.setInt32( static_cast<int32_t>( FETCH( cVal ) ) );
        break;
    case VT_I2:
        rval.setInt32( static_cast<int32_t>( FETCH( iVal ) ) );
        break;
    case VT_INT:
    case VT_I4:
        rval.setInt32( FETCH( lVal ) );
        break;
    case VT_R4:
        rval.setNumber( FETCH( fltVal ) );
        break;
    case VT_R8:
        rval.setNumber( FETCH( dblVal ) );
        break;
    case VT_BOOL:
        rval.setBoolean( FETCH( boolVal ) );
        break;
    case VT_UI1:
        rval.setNumber( static_cast<uint32_t>( FETCH( bVal ) ) );
        break;
    case VT_UI2:
        rval.setNumber( static_cast<uint32_t>( FETCH( uiVal ) ) );
        break;
    case VT_UINT:
    case VT_UI4:
        rval.setNumber( static_cast<uint32_t>( FETCH( ulVal ) ) );
        break;
    case VT_BSTR:
    {
        JS::RootedString jsString( cx, JS_NewUCStringCopyN( cx, reinterpret_cast<const char16_t*>( FETCH( bstrVal ) ), SysStringLen( FETCH( bstrVal ) ) ) );
        JsException::ExpectTrue( jsString );

        rval.setString( jsString );
        break;
    };
    case VT_DATE:
    {
        DATE d = FETCH( date );
        SYSTEMTIME time;
        VariantTimeToSystemTime( d, &time );

        JS::RootedObject jsObject( cx, JS::NewDateObject( cx, time.wYear, time.wMonth - 1, time.wDay, time.wHour, time.wMinute, time.wSecond ) );
        JsException::ExpectTrue( jsObject );

        rval.setObjectOrNull( jsObject );
        break;
    }
    case VT_UNKNOWN:
    {
        if ( !FETCH( punkVal ) )
        {
            rval.setNull();
            break;
        }

        std::unique_ptr<JsActiveXObject> x( new JsActiveXObject( cx, FETCH( punkVal ), true ) );
        JS::RootedObject jsObject( cx, JsActiveXObject::CreateJsFromNative( cx, std::move( x ) ) );
        assert( jsObject );

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

        std::unique_ptr<JsActiveXObject> x( new JsActiveXObject( cx, FETCH( pdispVal ), true ) );
        JS::RootedObject jsObject( cx, JsActiveXObject::CreateJsFromNative( cx, std::move( x ) ) );
        assert( jsObject );

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
        if ( ( type & VT_ARRAY ) && !( type & VT_UI1 ) )
        { // convert all arrays that are not binary data: SMP has no use for it, but it's needed in COM interface
            ComArrayToJsArray( cx, var, rval );
            break;
        }
        else
        {
            qwr::QwrException::ExpectTrue( type <= VT_CLSID || type == ( VT_ARRAY | VT_UI1 ), "ActiveX: unsupported object type: {:#x}", type );

            JS::RootedObject jsObject( cx, JsActiveXObject::CreateJsFromNative( cx, std::make_unique<JsActiveXObject>( cx, var ) ) );
            assert( jsObject );

            rval.setObjectOrNull( jsObject );
        }
    }

#undef FETCH
}

void JsToVariant( JSContext* cx, JS::HandleValue rval, VARIANTARG& arg )
{
    VariantInit( &arg );

    if ( rval.isObject() )
    {
        JS::RootedObject j0( cx, &rval.toObject() );

        auto pNative = GetInnerInstancePrivate<JsActiveXObject>( cx, j0 );
        if ( pNative )
        {
            JsActiveXObject* x = pNative;
            if ( x->pStorage_->variant.vt != VT_EMPTY )
            {
                //1.7.2.3
                HRESULT hr = VariantCopyInd( &arg, &x->pStorage_->variant );
                qwr::error::CheckHR( hr, "VariantCopyInd" );
                //VariantCopy(&arg,&x->variant_);
                //1.7.2.2 could address invalid memory if x is freed before arg
                // arg.vt = VT_VARIANT | VT_BYREF;
                // arg.pvarVal = &x->variant_;
            }
            else if ( x->pStorage_->pDispatch )
            {
                arg.vt = VT_DISPATCH;
                arg.pdispVal = x->pStorage_->pDispatch;
                x->pStorage_->pDispatch->AddRef();
            }
            else if ( x->pStorage_->pUnknown )
            {
                arg.vt = VT_UNKNOWN;
                arg.punkVal = x->pStorage_->pUnknown;
                x->pStorage_->pUnknown->AddRef();
            }
            else
            {
                arg.vt = VT_BYREF | VT_UNKNOWN;
                arg.ppunkVal = &x->pStorage_->pUnknown;
            }
        }
        else if ( JS_ObjectIsFunction( j0 ) )
        {
            JS::RootedFunction func( cx, JS_ValueToFunction( cx, rval ) );

            arg.vt = VT_DISPATCH;
            arg.pdispVal = new smp::com::ComPtrImpl<WrappedJs>( cx, func );
        }
        else
        {
            bool is;
            if ( !JS_IsArrayObject( cx, rval, &is ) )
            {
                throw JsException();
            }

            if ( is )
            { // other types of arrays are created manually (e.g. VT_ARRAY|VT_I1)
                JsArrayToVariantArray( cx, j0, VT_VARIANT, arg );
            }
            else
            {
                throw qwr::QwrException( "ActiveX: unsupported JS object type" );
            }
        }
    }
    else if ( rval.isBoolean() )
    {
        arg.vt = VT_BOOL;
        arg.boolVal = rval.toBoolean() ? -1 : 0;
    }
    else if ( rval.isInt32() )
    {
        arg.vt = VT_I4;
        arg.lVal = rval.toInt32();
    }
    else if ( rval.isDouble() )
    {
        arg.vt = VT_R8;
        arg.dblVal = rval.toDouble();
    }
    else if ( rval.isNull() )
    {
        arg.vt = VT_NULL;
        arg.scode = 0;
    }
    else if ( rval.isUndefined() )
    {
        arg.vt = VT_EMPTY;
        arg.scode = 0;
    }
    else if ( rval.isString() )
    {
        const auto str = convert::to_native::ToValue<std::wstring>( cx, rval );
        _bstr_t bStr = str.c_str();

        arg.vt = VT_BSTR;
        arg.bstrVal = bStr.Detach();
    }
    else
    {
        throw qwr::QwrException( "ActiveX: unsupported JS value type" );
    }
}

void JsArrayToVariantArray( JSContext* cx, JS::HandleObject obj, int elementVariantType, VARIANT& var )
{
#ifndef NDEBUG
    {
        bool is;
        if ( !JS_IsArrayObject( cx, obj, &is ) )
        {
            throw smp::JsException();
        }
        assert( is );
    }
#endif

    uint32_t len;
    if ( !JS_GetArrayLength( cx, obj, &len ) )
    {
        throw JsException();
    }

    // Create the safe array of variants and populate it
    SAFEARRAY* safeArray = SafeArrayCreateVector( elementVariantType, 0, len );
    qwr::QwrException::ExpectTrue( safeArray, "SafeArrayCreateVector failed" );

    qwr::final_action autoSa( [safeArray]() {
        SafeArrayDestroy( safeArray );
    } );

    if ( len )
    {
        if ( elementVariantType == VT_VARIANT )
        {
            VARIANT* varArray = nullptr;
            HRESULT hr = SafeArrayAccessData( safeArray, reinterpret_cast<void**>( &varArray ) );
            qwr::error::CheckHR( hr, "SafeArrayAccessData" );

            qwr::final_action autoSaData( [safeArray]() {
                SafeArrayUnaccessData( safeArray );
            } );

            for ( uint32_t i = 0; i < len; ++i )
            {
                JS::RootedValue val( cx );
                if ( !JS_GetElement( cx, obj, i, &val ) )
                {
                    throw JsException();
                }

                JsToVariant( cx, val, varArray[i] );
            }
        }
        else
        {
            void* dataArray = nullptr;
            HRESULT hr = SafeArrayAccessData( safeArray, reinterpret_cast<void**>( &dataArray ) );
            qwr::error::CheckHR( hr, "SafeArrayAccessData" );

            qwr::final_action autoSaData( [safeArray]() {
                SafeArrayUnaccessData( safeArray );
            } );

            for ( uint32_t i = 0; i < len; ++i )
            {
                JS::RootedValue val( cx );
                if ( !JS_GetElement( cx, obj, i, &val ) )
                {
                    throw JsException();
                }

                _variant_t tmp;
                JsToVariant( cx, val, tmp );
                tmp.ChangeType( elementVariantType );

                PutVariantInSafeArrayData( dataArray, i, tmp );
            }
        }
    }

    var.vt = VT_ARRAY | elementVariantType;
    var.parray = safeArray;

    autoSa.cancel(); // cancel array destruction
}

} // namespace mozjs::convert::com
