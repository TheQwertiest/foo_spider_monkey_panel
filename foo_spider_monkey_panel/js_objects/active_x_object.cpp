/// Code based on from wrap_com.cpp from JSDB by Shanti Rao (see http://jsdb.org)

#include <stdafx.h>
#include "active_x_object.h"

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>
#include <convert/com.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_prototype_helpers.h>
#include <utils/com_error_helpers.h>
#include <utils/scope_helpers.h>
#include <utils/winapi_error_helpers.h>
#include <utils/string_helpers.h>
#include <com_objects/com_interface.h>
#include <com_objects/com_tools.h>

#include <smp_exception.h>
#include <com_message_scope.h>

#include <vector>
#include <string>

// TODO: cleanup the code

using namespace smp;

namespace
{

using namespace mozjs;

void RefreshValue( JSContext* cx, JS::HandleValue valToCheck )
{
    auto pNative = GetInnerInstancePrivate<ActiveXObject>( cx, valToCheck );
    if ( !pNative )
    {
        return;
    }

    if ( pNative->pUnknown_ && !pNative->pDispatch_ )
    {
        HRESULT hresult = pNative->pUnknown_->QueryInterface( IID_IDispatch, (void**)&pNative->pDispatch_ );
        if ( !SUCCEEDED( hresult ) )
        {
            pNative->pDispatch_ = nullptr;
        }
    }
}

// Wrapper to intercept indexed gets/sets.
class ActiveXObjectProxyHandler : public js::ForwardingProxyHandler
{
public:
    static const ActiveXObjectProxyHandler singleton;

    constexpr ActiveXObjectProxyHandler()
        : js::ForwardingProxyHandler( GetSmpProxyFamily() )
    {
    }

    // bool has( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, bool* bp ) const override;
    bool get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
              JS::HandleId id, JS::MutableHandleValue vp ) const override;
    bool set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
              JS::HandleValue receiver, JS::ObjectOpResult& result ) const override;
};

const ActiveXObjectProxyHandler ActiveXObjectProxyHandler::singleton;

/*
bool 
ActiveXObjectProxyHandler::has( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, bool* bp ) const
{
    if ( !JSID_IS_STRING( id ) )
    {
        return js::ForwardingProxyHandler::has( cx, proxy, id, bp );
    }

    JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
    auto pNativeTarget = static_cast<ActiveXObject*>(JS_GetPrivate( target ));
    assert( pNativeTarget );

    JS::RootedString jsString( cx, JSID_TO_STRING( id ) );
    assert( jsString );

    auto retVal = convert::to_native::ToValue( cx, jsString );
    if ( !retVal )
    {
        if ( !JS_IsExceptionPending( cx ) )
        {
            JS_ReportErrorUTF8( cx, "Failed to parse property name" );
        }
        return false;
    }
    
    *bp = pNativeTarget->Has( retVal.value() );
    return true;
}
*/

bool ActiveXObjectProxyHandler::get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
                                     JS::HandleId id, JS::MutableHandleValue vp ) const
{
    try
    {
        if ( JSID_IS_STRING( id ) || JSID_IS_INT( id ) )
        {
            JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
            auto pNativeTarget = static_cast<ActiveXObject*>( JS_GetPrivate( target ) );
            assert( pNativeTarget );

            std::wstring propName;
            if ( JSID_IS_STRING( id ) )
            {
                JS::RootedString jsString( cx, JSID_TO_STRING( id ) );
                assert( jsString );

                propName = convert::to_native::ToValue( cx, jsString );
            }
            else if ( JSID_IS_INT( id ) )
            {
                propName = std::to_wstring( JSID_TO_INT( id ) );
            }

            if ( pNativeTarget->IsGet( propName ) || JSID_IS_INT( id ) )
            {
                pNativeTarget->Get( propName, vp );
                return true;
            }
        }
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
        return false;
    }

    return js::ForwardingProxyHandler::get( cx, proxy, receiver, id, vp );
}

bool ActiveXObjectProxyHandler::set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
                                     JS::HandleValue receiver, JS::ObjectOpResult& result ) const
{
    try
    {
        if ( !JSID_IS_STRING( id ) )
        {
            return js::ForwardingProxyHandler::set( cx, proxy, id, v, receiver, result );
        }

        JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
        auto pNativeTarget = static_cast<ActiveXObject*>( JS_GetPrivate( target ) );
        assert( pNativeTarget );

        JS::RootedString jsString( cx, JSID_TO_STRING( id ) );
        assert( jsString );

        const std::wstring propName = convert::to_native::ToValue( cx, jsString );

        if ( pNativeTarget->IsSet( propName ) )
        {
            pNativeTarget->Set( propName, v );
            result.succeed();
            return true;
        }
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
        return false;
    }

    return js::ForwardingProxyHandler::set( cx, proxy, id, v, receiver, result );
}
/*
bool ActiveXObjectProxyHandler::ownPropertyKeys( JSContext* cx, JS::HandleObject proxy, JS::AutoIdVector& props ) const
{
    JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
    auto pNativeTarget = static_cast<ActiveXObject*>(JS_GetPrivate( target ));
    assert( pNativeTarget );

    const auto memberList = pNativeTarget->GetAllMembers();

    props.clear();
    props.reserve( memberList.size() );
    JS::RootedId jsId( cx );
    for ( const auto& member : memberList )
    {
        JS::TwoByteChars jsString( (const char16_t*)member.c_str(), member.length() );
        if ( !JS_CharsToId( cx, jsString, &jsId ) || !props.append( jsId ) )
        {// report in JS_CharsToId
            return false;
        }        
    }
    
    return true;
}
*/

void JsToVariantSafe( JSContext* cx, JS::HandleValue rval, VARIANTARG& arg )
{
    try
    {
        convert::com::JsToVariant( cx, rval, arg );
    }
    catch ( ... )
    {
        mozjs::error::SuppressException( cx ); ///< reset, since we can't report
        arg.vt = VT_ERROR;
        arg.scode = 0;
    }
}

} // namespace

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
    ActiveXObject::FinalizeJsObject,
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
    smp::SmpException::ExpectTrue( argc && args[0].isString(), "Argument 1 is not a string" );

    const auto activeXName = mozjs::convert::to_native::ToValue<std::wstring>( cx, args[0] );
    args.rval().setObjectOrNull( ActiveXObject::CreateJs( cx, activeXName ) );
    return true;
}

bool ActiveX_Run_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    auto pNative = GetInnerInstancePrivate<ActiveXObject>( cx, args.thisv() );
    smp::SmpException::ExpectTrue( pNative, "`this` is not an object of valid type" );

    JS::RootedString jsString( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    smp::SmpException::ExpectTrue( jsString, "Failed to get function name" );

    pNative->Invoke( convert::to_native::ToValue( cx, jsString ), args );
    return true;
}

bool ActiveX_Get_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    auto pNative = GetInnerInstancePrivate<ActiveXObject>( cx, args.thisv() );
    smp::SmpException::ExpectTrue( pNative, "`this` is not an object of valid type" );

    pNative->Get( args );
    return true;
}

bool ActiveX_Set_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    auto pNative = GetInnerInstancePrivate<ActiveXObject>( cx, args.thisv() );
    smp::SmpException::ExpectTrue( pNative, "`this` is not an object of valid type" );

    pNative->Set( args );
    return true;
}

MJS_DEFINE_JS_FN( ActiveX_Constructor, ActiveX_Constructor_Impl )
MJS_DEFINE_JS_FN( ActiveX_Run, ActiveX_Run_Impl )
MJS_DEFINE_JS_FN( ActiveX_Get, ActiveX_Get_Impl )
MJS_DEFINE_JS_FN( ActiveX_Set, ActiveX_Set_Impl )
MJS_DEFINE_JS_FN_FROM_NATIVE( ToString, ActiveXObject::ToString )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "toString", ToString, 0, DefaultPropsFlags() ),
    JS_FN( "ActiveX_Get", ActiveX_Get, 1, DefaultPropsFlags() ),
    JS_FN( "ActiveX_Set", ActiveX_Set, 1, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass ActiveXObject::JsClass = jsClass;
const JSFunctionSpec* ActiveXObject::JsFunctions = jsFunctions;
const JSPropertySpec* ActiveXObject::JsProperties = jsProperties;
const JsPrototypeId ActiveXObject::PrototypeId = JsPrototypeId::ActiveX;
const JSNative ActiveXObject::JsConstructor = ActiveX_Constructor;
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

ActiveXObject::ActiveXObject( JSContext* cx, IDispatch* pDispatch, bool addref )
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

    HRESULT hresult = pUnknown_->QueryInterface( IID_IDispatch, (void**)&pDispatch_ );
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

    HRESULT hresult = CoCreateInstance( clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pUnknown_ );
    if ( !SUCCEEDED( hresult ) )
    {
        pUnknown_ = 0;
        return;
    }

    hresult = pUnknown_->QueryInterface( IID_IDispatch, (void**)&pDispatch_ );
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
    smp::ComMessageScope cms;

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

std::unique_ptr<ActiveXObject> ActiveXObject::CreateNative( JSContext* cx, const std::wstring& name )
{
    CLSID clsid;
    HRESULT hresult = ( name[0] == L'{' )
                          ? CLSIDFromString( name.c_str(), &clsid )
                          : CLSIDFromProgID( name.c_str(), &clsid );
    if ( !SUCCEEDED( hresult ) )
    {
        const pfc::string8_fast cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() ).get_ptr();
        throw SmpException( smp::string::Formatter() << "Invalid CLSID: " << cStr.c_str() );
    }

    std::unique_ptr<ActiveXObject> nativeObject;
    IUnknown* unk = nullptr;
    hresult = GetActiveObject( clsid, nullptr, &unk );
    if ( SUCCEEDED( hresult ) && unk )
    {
        nativeObject.reset( new ActiveXObject( cx, unk ) );
        if ( !nativeObject->pUnknown_ )
        {
            const pfc::string8_fast cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() ).get_ptr();
            throw SmpException( smp::string::Formatter() << "Failed to create ActiveXObject object via IUnknown:" << cStr.c_str() );
        }
    }

    if ( !nativeObject )
    {
        nativeObject.reset( new ActiveXObject( cx, clsid ) );
        if ( !nativeObject->pUnknown_ )
        {
            const pfc::string8_fast cStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() ).get_ptr();
            throw SmpException( smp::string::Formatter() << "Failed to create ActiveXObject object via CLSID:" << cStr.c_str() );
        }
    }

    return nativeObject;
}

size_t ActiveXObject::GetInternalSize( const std::wstring& /*name*/ )
{
    return 0;
}

void ActiveXObject::PostCreate( JSContext* cx, JS::HandleObject self )
{
    auto pNative = static_cast<ActiveXObject*>( JS_GetInstancePrivate( cx, self, &ActiveXObject::JsClass, nullptr ) );
    assert( pNative );
    return pNative->SetupMembers( self );
}

std::optional<DISPID> ActiveXObject::GetDispId( const std::wstring& name, bool reportError )
{
    if ( !pDispatch_ )
    {
        if ( reportError )
        {
            throw SmpException( "Internal error: pDispatch_ is null" );
        }
        return std::nullopt;
    }

    if ( name.empty() )
    {
        return 0;
    }

    auto it = members_.find( name );
    if ( it != members_.end() && it->second->hasDispId )
    {
        return it->second->dispId;
    }

    DISPID dispId;
    wchar_t* cname = const_cast<wchar_t*>( name.c_str() );
    HRESULT hresult = pDispatch_->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_USER_DEFAULT, &dispId );
    if ( !SUCCEEDED( hresult ) )
    {
        hresult = pDispatch_->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_SYSTEM_DEFAULT, &dispId );
        if ( !SUCCEEDED( hresult ) )
        {
            if ( reportError )
            {
                pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( name.c_str() ).get_ptr();
                throw SmpException( smp::string::Formatter() << "Failed to get DISPID for `" << tmpStr.c_str() << "`" );
            }
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

bool ActiveXObject::Has( const std::wstring& name )
{
    return members_.count( name );
}

bool ActiveXObject::IsGet( const std::wstring& name )
{
    return members_.count( name ) && members_[name]->isGet;
}

bool ActiveXObject::IsSet( const std::wstring& name )
{
    return members_.count( name ) && ( members_[name]->isPut || members_[name]->isPutRef );
}

bool ActiveXObject::IsInvoke( const std::wstring& name )
{
    return members_.count( name ) && members_[name]->isInvoke;
}

std::vector<std::wstring> ActiveXObject::GetAllMembers()
{
    std::vector<std::wstring> memberList;
    for ( const auto& member : members_ )
    {
        memberList.push_back( member.first );
    }
    return memberList;
}

std::wstring ActiveXObject::ToString()
{
    JS::RootedValue jsValue( pJsCtx_ );
    auto dispRet = GetDispId( L"toString", false );
    Get( ( dispRet ? L"toString" : L"" ), &jsValue );

    return convert::to_native::ToValue<std::wstring>( pJsCtx_, jsValue );
}

void ActiveXObject::Get( const std::wstring& propName, JS::MutableHandleValue vp )
{
    SmpException::ExpectTrue( pDispatch_, "Internal error: pDispatch_ is null" );

    auto dispRet = GetDispId( propName, false );
    if ( !dispRet )
    { // not an error
        vp.setUndefined();
        return;
    }

    DISPPARAMS dispparams = { nullptr, nullptr, 0, 0 };

    VARIANT VarResult;
    VariantInit( &VarResult );

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    HRESULT hresult = pDispatch_->Invoke( dispRet.value(),
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          DISPATCH_PROPERTYGET,
                                          &dispparams,
                                          &VarResult,
                                          &exception,
                                          &argerr );
    utils::final_action autoVarClear( [&VarResult] {
        VariantClear( &VarResult );
    } );

    if ( !SUCCEEDED( hresult ) )
    {
        smp::error::ReportActiveXError( hresult, exception, argerr );
    }

    convert::com::VariantToJs( pJsCtx_, VarResult, vp );
}

void ActiveXObject::Get( JS::CallArgs& callArgs )
{
    SmpException::ExpectTrue( pDispatch_, "Internal error: pDispatch_ is null" );
    SmpException::ExpectTrue( callArgs.length(), "Property name is missing" );

    const std::wstring propName = convert::to_native::ToValue<std::wstring>( pJsCtx_, callArgs[0] );

    auto dispRet = GetDispId( propName );
    if ( !dispRet )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( propName.c_str() ).get_ptr();
        throw SmpException( smp::string::Formatter() << "Invalid property name: " << tmpStr.c_str() );
    }

    uint32_t argc = callArgs.length() - 1;
    VARIANT VarResult;
    std::unique_ptr<VARIANTARG[]> args;
    DISPPARAMS dispparams = { nullptr, nullptr, 0, 0 };

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args.reset( new VARIANTARG[argc] );
        dispparams.rgvarg = args.get();
        dispparams.cArgs = argc;

        for ( size_t i = 0; i < argc; i++ )
        {
            JsToVariantSafe( pJsCtx_, callArgs[1 + i], args[argc - i - 1] );
        }
    }

    VariantInit( &VarResult );

    // don't use DispInvoke, because we don't know the TypeInfo
    HRESULT hresult = pDispatch_->Invoke( dispRet.value(),
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          DISPATCH_PROPERTYGET,
                                          &dispparams,
                                          &VarResult,
                                          &exception,
                                          &argerr );
    utils::final_action autoVarClear( [&VarResult] {
        VariantClear( &VarResult );
    } );

    for ( size_t i = 0; i < argc; i++ )
    {
        RefreshValue( pJsCtx_, callArgs[i] ); //in case any empty ActiveXObject objects were filled in by Invoke()
        VariantClear( &args[i] );
    }

    if ( !SUCCEEDED( hresult ) )
    {
        smp::error::ReportActiveXError( hresult, exception, argerr );
    }

    convert::com::VariantToJs( pJsCtx_, VarResult, callArgs.rval() );
}

void ActiveXObject::Set( const std::wstring& propName, JS::HandleValue v )
{
    SmpException::ExpectTrue( pDispatch_, "Internal error: pDispatch_ is null" );

    auto dispRet = GetDispId( propName );
    if ( !dispRet )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( propName.c_str() ).get_ptr();
        throw SmpException( smp::string::Formatter() << "Invalid property name: " << tmpStr.c_str() );
    }

    VARIANTARG arg;
    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { &arg, &dispput, 1, 1 };

    JsToVariantSafe( pJsCtx_, v, arg );

    WORD flag = DISPATCH_PROPERTYPUT;
    if ( ( arg.vt == VT_DISPATCH || arg.vt == VT_UNKNOWN )
         && members_.count( propName ) && members_[propName]->isPutRef )
    { //must be passed by name
        flag = DISPATCH_PROPERTYPUTREF;
    }

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    HRESULT hresult = pDispatch_->Invoke( dispRet.value(),
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          flag,
                                          &dispparams,
                                          nullptr,
                                          &exception,
                                          &argerr );

    RefreshValue( pJsCtx_, v );
    VariantClear( &arg );

    if ( !SUCCEEDED( hresult ) )
    {
        smp::error::ReportActiveXError( hresult, exception, argerr );
    }
}

void ActiveXObject::Set( const JS::CallArgs& callArgs )
{
    SmpException::ExpectTrue( pDispatch_, "Internal error: pDispatch_ is null" );
    SmpException::ExpectTrue( callArgs.length(), "Property name is missing" );

    const std::wstring propName = convert::to_native::ToValue<std::wstring>( pJsCtx_, callArgs[0] );

    auto dispRet = GetDispId( propName );
    if ( !dispRet )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( propName.c_str() ).get_ptr();
        throw SmpException( smp::string::Formatter() << "Invalid property name: " << tmpStr.c_str() );
    }

    uint32_t argc = callArgs.length() - 1;
    std::unique_ptr<VARIANTARG[]> args;
    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { nullptr, &dispput, 0, 1 };

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args.reset( new VARIANTARG[argc] );
        dispparams.rgvarg = args.get();
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            JsToVariantSafe( pJsCtx_, callArgs[1 + i], args[argc - i - 1] );
        }
    }

    WORD flag = DISPATCH_PROPERTYPUT;
    if ( ( args[argc - 1].vt == VT_DISPATCH || args[argc - 1].vt == VT_UNKNOWN )
         && members_.count( propName ) && members_[propName]->isPutRef )
    { //must be passed by name
        flag = DISPATCH_PROPERTYPUTREF;
    }

    // don't use DispInvoke, because we don't know the TypeInfo
    HRESULT hresult = pDispatch_->Invoke( dispRet.value(),
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          flag,
                                          &dispparams,
                                          nullptr,
                                          &exception,
                                          &argerr );

    for ( size_t i = 0; i < argc; i++ )
    {
        RefreshValue( pJsCtx_, callArgs[i] ); //in case any empty ActiveXObject objects were filled in by Invoke()
        VariantClear( &args[i] );
    }

    if ( !SUCCEEDED( hresult ) )
    {
        smp::error::ReportActiveXError( hresult, exception, argerr );
    }
}

void ActiveXObject::Invoke( const std::wstring& funcName, const JS::CallArgs& callArgs )
{
    SmpException::ExpectTrue( pDispatch_, "Internal error: pDispatch_ is null" );

    auto dispRet = GetDispId( funcName );
    if ( !dispRet )
    {
        pfc::string8_fast tmpStr = pfc::stringcvt::string_utf8_from_wide( funcName.c_str() ).get_ptr();
        throw SmpException( smp::string::Formatter() << "Invalid function name: " << tmpStr.c_str() );
    }

    uint32_t argc = callArgs.length();
    VARIANT VarResult;
    std::unique_ptr<VARIANTARG[]> args;
    DISPPARAMS dispparams = { nullptr, nullptr, 0, 0 };

    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args.reset( new VARIANTARG[argc] );
        dispparams.rgvarg = args.get();
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            JsToVariantSafe( pJsCtx_, callArgs[i], args[argc - i - 1] );
        }
    }

    VariantInit( &VarResult );

    // don't use DispInvoke, because we don't know the TypeInfo
    HRESULT hresult = pDispatch_->Invoke( dispRet.value(),
                                          IID_NULL,
                                          LOCALE_USER_DEFAULT,
                                          DISPATCH_METHOD,
                                          &dispparams,
                                          &VarResult,
                                          &exception,
                                          &argerr );
    utils::final_action autoVarClear( [&VarResult] {
        VariantClear( &VarResult );
    } );

    for ( size_t i = 0; i < argc; i++ )
    {
        RefreshValue( pJsCtx_, callArgs[i] ); //in case any empty ActiveXObject objects were filled in by Invoke()
        VariantClear( &args[i] );
    }

    if ( !SUCCEEDED( hresult ) )
    {
        smp::error::ReportActiveXError( hresult, exception, argerr );
    }

    convert::com::VariantToJs( pJsCtx_, VarResult, callArgs.rval() );
}

void ActiveXObject::SetupMembers( JS::HandleObject jsObject )
{
    if ( areMembersSetup_ )
    {
        return;
    }

    SmpException::ExpectTrue( pUnknown_ || pDispatch_, "Internal error: pUnknown_ and pDispatch_ are null" );

    if ( !pDispatch_ )
    {
        HRESULT hr = pUnknown_->QueryInterface( IID_IDispatch, (void**)&pDispatch_ );
        smp::error::CheckHR( hr, "QueryInterface" );
    }

    if ( !pTypeInfo_ )
    {
        unsigned ctinfo;
        HRESULT hr = pDispatch_->GetTypeInfoCount( &ctinfo );
        smp::error::CheckHR( hr, "GetTypeInfoCount" );

        hr = pDispatch_->GetTypeInfo( 0, 0, &pTypeInfo_ );
        smp::error::CheckHR( hr, "GetTypeInfo" );
    }

    ParseTypeInfoRecursive( pJsCtx_, pTypeInfo_, members_ );
    SetupMembers_Impl( jsObject );

    areMembersSetup_ = true;
}

void ActiveXObject::ParseTypeInfoRecursive( JSContext* cx, ITypeInfo* pTypeInfo, MemberMap& members )
{
    ParseTypeInfo( pTypeInfo, members );

    TYPEATTR* pAttr = nullptr;
    HRESULT hr = pTypeInfo->GetTypeAttr( &pAttr );
    smp::error::CheckHR( hr, "GetTypeAttr" );

    utils::final_action autoTypeAttr( [pTypeInfo, pAttr] {
        if ( pTypeInfo && pAttr )
        {
            pTypeInfo->ReleaseTypeAttr( pAttr );
        }
    } );

    if ( !( pAttr->wTypeFlags & TYPEFLAG_FRESTRICTED )
         && ( TKIND_DISPATCH == pAttr->typekind || TKIND_INTERFACE == pAttr->typekind )
         && pAttr->cImplTypes )
    {
        for ( size_t i = 0; i < pAttr->cImplTypes; ++i )
        {
            HREFTYPE hRef = 0;
            hr = pTypeInfo->GetRefTypeOfImplType( i, &hRef );
            smp::error::CheckHR( hr, "GetTypeAttr" );

            ITypeInfo* pTypeInfoCur = nullptr;
            hr = pTypeInfo->GetRefTypeInfo( hRef, &pTypeInfoCur );
            if ( SUCCEEDED( hr ) && pTypeInfoCur )
            {
                utils::final_action autoTypeInfo( [pTypeInfoCur] {
                    pTypeInfoCur->Release();
                } );

                ParseTypeInfoRecursive( cx, pTypeInfoCur, members );
            }

            /*
            if ( FAILED( hresult ) )
            {
                JS_ReportErrorUTF8( cx, "Failed to GetRefTypeInfo" );
                return false;
            }
            */
        }
    }
}

void ActiveXObject::ParseTypeInfo( ITypeInfo* pTypeInfo, MemberMap& members )
{
    VARDESC* vardesc;
    for ( size_t i = 0; pTypeInfo->GetVarDesc( i, &vardesc ) == S_OK; ++i )
    {
        _bstr_t name;
        //_bstr_t desc;
        if ( pTypeInfo->GetDocumentation( vardesc->memid, name.GetAddress(), nullptr /*&desc*/, nullptr, nullptr ) == S_OK )
        {
            if ( !( vardesc->wVarFlags & VARFLAG_FRESTRICTED )
                 && !( vardesc->wVarFlags & VARFLAG_FHIDDEN ) )
            {
                auto [it, bRet] = members.try_emplace( name.GetBSTR(), std::make_unique<MemberInfo>() );
                auto pProp = it->second.get();
                pProp->isGet = true;
                pProp->isPut = true;
            }
        }
        pTypeInfo->ReleaseVarDesc( vardesc );
    }

    FUNCDESC* funcdesc;
    for ( size_t i = 0; pTypeInfo->GetFuncDesc( i, &funcdesc ) == S_OK; ++i )
    {
        _bstr_t name;
        //_bstr_t desc;
        if ( pTypeInfo->GetDocumentation( funcdesc->memid, name.GetAddress(), nullptr /*&desc*/, nullptr, nullptr ) == S_OK )
        {
            if ( !( funcdesc->wFuncFlags & FUNCFLAG_FRESTRICTED )
                 && !( funcdesc->wFuncFlags & FUNCFLAG_FHIDDEN ) )
            {
                auto [it, bRet] = members.try_emplace( name.GetBSTR(), std::make_unique<MemberInfo>() );
                auto pProp = it->second.get();
                if ( INVOKE_PROPERTYPUT == funcdesc->invkind )
                {
                    pProp->isPut = true;
                }
                if ( INVOKE_PROPERTYPUTREF == funcdesc->invkind )
                {
                    pProp->isPutRef = true;
                }
                if ( INVOKE_PROPERTYGET == funcdesc->invkind )
                {
                    pProp->isGet = true;
                }
                if ( INVOKE_FUNC == funcdesc->invkind )
                {
                    pProp->isInvoke = true;
                }
            }
        }
        pTypeInfo->ReleaseFuncDesc( funcdesc );
    }
}

void ActiveXObject::SetupMembers_Impl( JS::HandleObject jsObject )
{
    for ( const auto& [name, member] : members_ )
    {
        if ( member->isInvoke )
        {
            if ( !JS_DefineUCFunction( pJsCtx_, jsObject, (const char16_t*)name.c_str(), name.length(), ActiveX_Run, 0, JSPROP_ENUMERATE ) )
            {
                throw JsException();
            }
        }
    }
}

} // namespace mozjs
