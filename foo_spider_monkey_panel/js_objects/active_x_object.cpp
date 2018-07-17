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
#include <js_utils/com_error_helper.h>
#include <js_utils/scope_helper.h>
#include <convert/com.h>

#include <script_interface.h>
#include <com_tools.h>

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <vector>
#include <string>

// TODO: add VT_ARRAY <> JSArray and VT_DISPATCH <> JSFunction support

#ifndef DISPID_PROPERTYPUT
#   define DISPID_PROPERTYPUT (-3)
#endif

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

    virtual bool isCallable( JSObject* obj ) const override;
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
    auto pNativeTarget = static_cast<ActiveXObject*>(JS_GetPrivate( target ));
    assert( pNativeTarget );

    JS::RootedString jsString( cx, JSID_TO_STRING( id ) );
    assert( jsString );

    bool isValid;
    std::wstring propName = convert::to_native::ToValue( cx, jsString, isValid );
    if ( !isValid )
    {
        if ( !JS_IsExceptionPending( cx ) )
        {
            JS_ReportErrorUTF8( cx, "Failed to parse property name" );
        }
        return false;
    }

    return pNativeTarget->Get( propName, vp );
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
    auto pNativeTarget = static_cast<ActiveXObject*>(JS_GetPrivate( target ));
    assert( pNativeTarget );

    JS::RootedString jsString( cx, JSID_TO_STRING( id ) );
    assert( jsString );

    bool isValid;
    std::wstring propName = convert::to_native::ToValue( cx, jsString, isValid );
    if ( !isValid )
    {
        if ( !JS_IsExceptionPending( cx ) )
        {
            JS_ReportErrorUTF8( cx, "Failed to parse property name" );
        }
        return false;
    }

    if ( !pNativeTarget->Set( propName, v ) )
    {// report in set
        return false;
    }

    result.succeed();
    return true;
}

bool ActiveXObjectProxyHandler::call( JSContext* cx, JS::HandleObject proxy, const JS::CallArgs& args ) const
{
    JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
    auto pNativeTarget = static_cast<ActiveXObject*>(JS_GetPrivate( target ));
    assert( pNativeTarget );

    JS::RootedString jsString( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    if ( !jsString )
    {
        JS_ReportErrorUTF8( cx, "Failed to get function name" );
        return false;
    }

    bool isValid;
    std::wstring functionName = convert::to_native::ToValue( cx, jsString, isValid );
    if ( !isValid )
    {
        if ( !JS_IsExceptionPending( cx ) )
        {
            JS_ReportErrorUTF8( cx, "Failed to parse property name" );
        }
        return false;
    }

    return pNativeTarget->Invoke( functionName, args );
}

bool ActiveXObjectProxyHandler::isCallable( JSObject* obj ) const
{
    return true;
}

}

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
    const std::wstring name = mozjs::convert::to_native::ToValue<std::wstring>( cx, args[0], bRet );
    if ( !bRet )
    {
        JS_ReportErrorUTF8( cx, "Failed to parse name argument" );
        return false;
    }

    JS::RootedObject jsObject( cx, ActiveXObject::CreateJs( cx, name ) );
    if ( !jsObject )
    {// report in CreateJs
        return false;
    }

    args.rval().setObjectOrNull( jsObject );
    return true;
}

MJS_WRAP_JS_TO_NATIVE_FN( ActiveX_Constructor, ActiveX_Constructor_Impl )

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

const JSClass ActiveXObject::JsClass = jsClass;
const JSFunctionSpec* ActiveXObject::JsFunctions = jsFunctions;
const JSPropertySpec* ActiveXObject::JsProperties = jsProperties;
const JsPrototypeId ActiveXObject::PrototypeId = JsPrototypeId::ActiveX;
const js::BaseProxyHandler& ActiveXObject::JsProxy = ActiveXObjectProxyHandler::singleton;

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

    unsigned ctinfo;
    HRESULT hresult = pDispatch_->GetTypeInfoCount( &ctinfo );
    if ( SUCCEEDED( hresult ) && ctinfo )
    {
        pDispatch_->GetTypeInfo( 0, 0, &pTypeInfo_ );
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
        return;
    }

    unsigned ctinfo;
    hresult = pDispatch_->GetTypeInfoCount( &ctinfo );
    if ( SUCCEEDED( hresult ) && ctinfo )
    {
        pDispatch_->GetTypeInfo( 0, 0, &pTypeInfo_ );
    }
}

ActiveXObject::ActiveXObject( JSContext* cx, CLSID& clsid )
    : pJsCtx_( cx )
{
    pUnknown_ = nullptr;
    pDispatch_ = nullptr;
    pTypeInfo_ = nullptr;
    memset( &variant_, 0, sizeof( variant_ ) );

    HRESULT hresult = CoCreateInstance( clsid, nullptr, CLSCTX_INPROC_SERVER,
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
        return;
    }

    unsigned ctinfo;
    hresult = pDispatch_->GetTypeInfoCount( &ctinfo );
    if ( SUCCEEDED( hresult ) && ctinfo )
    {
        pDispatch_->GetTypeInfo( 0, 0, &pTypeInfo_ );
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

std::unique_ptr<ActiveXObject> ActiveXObject::CreateNative( JSContext* cx, const std::wstring& name )
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

    return nativeObject;
}

size_t ActiveXObject::GetInternalSize( const std::wstring& name )
{
    return 0;
}

std::optional<DISPID> ActiveXObject::GetDispId( const std::wstring& name )
{
    if ( !pDispatch_ )
    {
        return std::nullopt;
    }

    if ( name.empty() || name[0] == L'0' )
    {
        return 0;
    }

    auto it = members_.find( name );
    if ( it != members_.end() && it->second->hasDispId )
    {
        return it->second->dispId;
    }

    DISPID dispId;
    wchar_t* cname = const_cast<wchar_t*>(name.c_str());
    HRESULT hresult = pDispatch_->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_USER_DEFAULT, &dispId );
    if ( !SUCCEEDED( hresult ) )
    {
        hresult = pDispatch_->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_SYSTEM_DEFAULT, &dispId );
        if ( !SUCCEEDED( hresult ) )
        {
            pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() );
            JS_ReportErrorUTF8( pJsCtx_, "Failed to get DISPID for `%s`", tmpStr.c_str() );
            return std::nullopt;
        }
    }

    if ( it != members_.end() )
    {
        it->second->hasDispId = true;
        it->second->dispId = dispId;
    }
    else
    {
        auto newMember = std::make_unique<MemberInfo>();
        newMember->hasDispId = true;
        newMember->dispId = dispId;

        members_.insert_or_assign( name, std::move( newMember ) );
    }
   
    return dispId;
}

bool ActiveXObject::Get( const std::wstring& propName, JS::MutableHandleValue vp )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    auto dispRet = GetDispId( propName );
    if ( !dispRet )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( propName.c_str() );
        JS_ReportErrorUTF8( pJsCtx_, "Invalid property name: %s", tmpStr.c_str() );
        return false;
    }

    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };

    VARIANT VarResult;
    VariantInit( &VarResult );

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    HRESULT hresult = pDispatch_->Invoke( dispRet.value(),
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          DISPATCH_PROPERTYGET,
                                          &dispparams, &VarResult, &exception, &argerr );
    scope::unique_ptr<VARIANT> autoVarClear( &VarResult, []( auto pVar )
    {
        VariantClear( pVar );
    } );

    if ( !SUCCEEDED( hresult ) )
    {
        ReportActiveXError( pJsCtx_, hresult, exception, argerr );
        return false;
    }

    if ( !convert::com::VariantToJs( pJsCtx_, VarResult, vp ) )
    {// report in VariantToJs
        return false;
    }

    return true;
}

bool ActiveXObject::Set( const std::wstring& propName, JS::HandleValue v )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    auto dispRet = GetDispId( propName );
    if ( !dispRet )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( propName.c_str() );
        JS_ReportErrorUTF8( pJsCtx_, "Invalid property name: %s", tmpStr.c_str() );
        return false;
    }

    if ( !UpdateAllPutProperties() )
    {// report in GetAllPutProperties
        return false;
    }

    VARIANTARG arg;
    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { &arg,&dispput,1,1 };

    if ( !convert::com::JsToVariant( pJsCtx_, v, arg ) )
    {
        arg.vt = VT_ERROR;
        arg.scode = 0;
    }

    DWORD flag = DISPATCH_PROPERTYPUT;
    if (( arg.vt == VT_DISPATCH || arg.vt == VT_UNKNOWN )
         && members_.count( propName ) && members_[propName]->isPutRef )
    {//must be passed by name
        flag = DISPATCH_PROPERTYPUTREF;
    }

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    HRESULT hresult = pDispatch_->Invoke( dispRet.value(),
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          (WORD)flag,
                                          &dispparams, nullptr, &exception, &argerr );

    convert::com::CheckReturn( pJsCtx_, v );
    VariantClear( &arg );

    if ( !SUCCEEDED( hresult ) )
    {
        ReportActiveXError( pJsCtx_, hresult, exception, argerr );
        return false;
    }

    return true;
}

bool ActiveXObject::Invoke( const std::wstring& funcName, const JS::CallArgs& callArgs )
{
    if ( !pDispatch_ )
    {
        return false;
    }

    auto dispRet = GetDispId( funcName );
    if ( !dispRet )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( funcName.c_str() );
        JS_ReportErrorUTF8( pJsCtx_, "Invalid function name: %s", tmpStr.c_str() );
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
            if ( !convert::com::JsToVariant( pJsCtx_, callArgs[i], args[argc - i - 1] ) )
            {
                args[argc - i - 1].vt = VT_ERROR;
                args[argc - i - 1].scode = 0;
            }
        }
    }

    VariantInit( &VarResult );

    // don't use DispInvoke, because we don't know the TypeInfo
    HRESULT hresult = pDispatch_->Invoke( dispRet.value(),
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          DISPATCH_METHOD,
                                          &dispparams, &VarResult, &exception, &argerr );
    scope::unique_ptr<VARIANT> autoVarClear( &VarResult, []( auto pVar )
    {
        VariantClear( pVar );
    } );

    for ( size_t i = 0; i < argc; i++ )
    {
        convert::com::CheckReturn( pJsCtx_, callArgs[i] ); //in case any empty ActiveXObject objects were filled in by Invoke()
        VariantClear( &args[i] );
    }

    if ( !SUCCEEDED( hresult ) )
    {
        ReportActiveXError( pJsCtx_, hresult, exception, argerr );
        return false;
    }

    if ( !convert::com::VariantToJs( pJsCtx_, VarResult, callArgs.rval() ) )
    {// report in VariantToJs
        return false;
    }

    return true;
}

bool ActiveXObject::UpdateAllPutProperties()
{
    if ( isTypeInfoParsed_ )
    {
        return true;
    }

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
            members_.emplace( name, std::make_unique<MemberInfo>() );

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
                auto[it, bRet] = members_.emplace( name, std::make_unique<MemberInfo>() );
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

    isTypeInfoParsed_ = true;
    return true;
}

}
