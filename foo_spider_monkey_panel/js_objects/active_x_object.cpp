/// Code based on from wrap_com.cpp from JSDB by Shanti Rao (see http://jsdb.org)

#include <stdafx.h>

#include "active_x_object.h"

#include <com_objects/com_interface.h>
#include <com_objects/com_tools.h>
#include <com_utils/com_error_helpers.h>
#include <convert/com.h>
#include <convert/js_to_native.h>
#include <convert/native_to_js.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/active_x_object_iterator.h>
#include <js_objects/global_object.h>
#include <js_objects/internal/global_heap_manager.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_prototype_helpers.h>

#include <qwr/final_action.h>
#include <qwr/string_helpers.h>
#include <qwr/winapi_error_helpers.h>

#include <string>
#include <vector>

// TODO: cleanup the code

using namespace smp;

namespace
{

using namespace mozjs;

void RefreshValue( JSContext* cx, JS::HandleValue valToCheck )
{
    auto pNative = GetInnerInstancePrivate<JsActiveXObject>( cx, valToCheck );
    if ( !pNative )
    {
        return;
    }

    if ( pNative->pStorage_->pUnknown && !pNative->pStorage_->pDispatch )
    {
        HRESULT hresult = pNative->pStorage_->pUnknown->QueryInterface( IID_IDispatch, reinterpret_cast<void**>( &pNative->pStorage_->pDispatch ) );
        if ( FAILED( hresult ) )
        {
            pNative->pStorage_->pDispatch = nullptr;
        }
    }
}

MJS_DEFINE_JS_FN_FROM_NATIVE( ActiveX_Iterator, JsActiveXObject::CreateNewIterator )

// Wrapper to intercept indexed gets/sets.
class ActiveXObjectProxyHandler : public js::ForwardingProxyHandler
{
public:
    static const ActiveXObjectProxyHandler singleton;

    ActiveXObjectProxyHandler()
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
    auto pNativeTarget = static_cast<JsActiveXObject*>(JS_GetPrivate( target ));
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
    
    *bp = pNativeTarget->Has( *retVal );
    return true;
}
*/

bool ActiveXObjectProxyHandler::get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
                                     JS::HandleId id, JS::MutableHandleValue vp ) const
{
    try
    {
        const auto isString = JSID_IS_STRING( id );
        const auto isInt = JSID_IS_INT( id );
        const auto isEnumSymbol = [&] {
            if ( !JSID_IS_SYMBOL( id ) )
            {
                return false;
            }
            JS::RootedSymbol sym( cx, JSID_TO_SYMBOL( id ) );
            return ( JS::GetSymbolCode( sym ) == JS::SymbolCode::iterator );
        }();

        if ( isEnumSymbol )
        {
            JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
            auto pNativeTarget = static_cast<JsActiveXObject*>( JS_GetPrivate( target ) );
            assert( pNativeTarget );

            if ( pNativeTarget->HasIterator() )
            {
                JS::RootedFunction jsFunc( cx, JS_NewFunction( cx, ActiveX_Iterator, 0, kDefaultPropsFlags, "iterator_impl" ) );
                vp.setObject( *JS_GetFunctionObject( jsFunc ) );
                return true;
            }
        }
        else if ( isString || isInt )
        {
            JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
            auto pNativeTarget = static_cast<JsActiveXObject*>( JS_GetPrivate( target ) );
            assert( pNativeTarget );

            std::wstring propName;
            if ( isString )
            {
                JS::RootedString jsString( cx, JSID_TO_STRING( id ) );
                assert( jsString );

                propName = convert::to_native::ToValue<std::wstring>( cx, jsString );
            }
            else if ( isInt )
            {
                propName = std::to_wstring( JSID_TO_INT( id ) );
            }

            if ( pNativeTarget->IsGet( propName ) )
            {
                pNativeTarget->GetProperty( propName, vp );
                return true;
            }
            else if ( isInt )
            {
                const auto fetchedAsProperty = pNativeTarget->TryGetProperty( propName, vp );
                if ( !fetchedAsProperty )
                {
                    pNativeTarget->GetItem( JSID_TO_INT( id ), vp );
                }
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
        auto pNativeTarget = static_cast<JsActiveXObject*>( JS_GetPrivate( target ) );
        assert( pNativeTarget );

        JS::RootedString jsString( cx, JSID_TO_STRING( id ) );
        assert( jsString );

        const std::wstring propName = convert::to_native::ToValue<std::wstring>( cx, jsString );

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
    auto pNativeTarget = static_cast<JsActiveXObject*>(JS_GetPrivate( target ));
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
    JsActiveXObject::FinalizeJsObject,
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

bool ActiveX_Run_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    auto pNative = GetInnerInstancePrivate<JsActiveXObject>( cx, args.thisv() );
    qwr::QwrException::ExpectTrue( pNative, "`this` is not an object of valid type" );

    JS::RootedString jsString( cx, JS_GetFunctionId( JS_ValueToFunction( cx, args.calleev() ) ) );
    qwr::QwrException::ExpectTrue( jsString, "Failed to get function name" );

    pNative->Invoke( convert::to_native::ToValue<std::wstring>( cx, jsString ), args );
    return true;
}

bool ActiveX_Get_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    auto pNative = GetInnerInstancePrivate<JsActiveXObject>( cx, args.thisv() );
    qwr::QwrException::ExpectTrue( pNative, "`this` is not an object of valid type" );

    pNative->Get( args );
    return true;
}

bool ActiveX_Set_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    auto pNative = GetInnerInstancePrivate<JsActiveXObject>( cx, args.thisv() );
    qwr::QwrException::ExpectTrue( pNative, "`this` is not an object of valid type" );

    pNative->Set( args );
    return true;
}

MJS_DEFINE_JS_FN( ActiveX_Run, ActiveX_Run_Impl )
MJS_DEFINE_JS_FN( ActiveX_Get, ActiveX_Get_Impl )
MJS_DEFINE_JS_FN( ActiveX_Set, ActiveX_Set_Impl )
MJS_DEFINE_JS_FN_FROM_NATIVE( ActiveX_CreateArray, JsActiveXObject::CreateFromArray )
MJS_DEFINE_JS_FN_FROM_NATIVE( ToString, JsActiveXObject::ToString )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "toString", ToString, 0, kDefaultPropsFlags ),
        JS_FN( "ActiveX_CreateArray", ActiveX_CreateArray, 2, kDefaultPropsFlags ),
        JS_FN( "ActiveX_Get", ActiveX_Get, 1, kDefaultPropsFlags ),
        JS_FN( "ActiveX_Set", ActiveX_Set, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsStaticFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "ActiveX_CreateArray", ActiveX_CreateArray, 2, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( ActiveXObject_Constructor, JsActiveXObject::Constructor )

} // namespace

namespace mozjs
{

const JSClass JsActiveXObject::JsClass = jsClass;
const JSFunctionSpec* JsActiveXObject::JsFunctions = jsFunctions.data();
const JSFunctionSpec* JsActiveXObject::JsStaticFunctions = jsStaticFunctions.data();
const JSPropertySpec* JsActiveXObject::JsProperties = jsProperties.data();
const JsPrototypeId JsActiveXObject::PrototypeId = JsPrototypeId::ActiveX;
const JSNative JsActiveXObject::JsConstructor = ::ActiveXObject_Constructor;
const js::BaseProxyHandler& JsActiveXObject::JsProxy = ActiveXObjectProxyHandler::singleton;

JsActiveXObject::JsActiveXObject( JSContext* cx, VARIANTARG& var )
    : pJsCtx_( cx )
{
    HRESULT hr = VariantCopyInd( &pStorage_->variant, &var );
    if ( FAILED( hr ) )
    {
        return;
    }

    hasVariant_ = true;
}

JsActiveXObject::JsActiveXObject( JSContext* cx, IDispatch* pDispatch, bool addref )
    : pJsCtx_( cx )
{
    pStorage_->pDispatch = pDispatch;
    if ( !pStorage_->pDispatch )
    {
        return;
    }

    if ( addref )
    {
        pStorage_->pDispatch->AddRef();
    }

    unsigned ctinfo;
    HRESULT hresult = pStorage_->pDispatch->GetTypeInfoCount( &ctinfo );
    if ( SUCCEEDED( hresult ) && ctinfo )
    {
        pStorage_->pDispatch->GetTypeInfo( 0, 0, &pStorage_->pTypeInfo );
    }
}

JsActiveXObject::JsActiveXObject( JSContext* cx, IUnknown* pUnknown, bool addref )
    : pJsCtx_( cx )
{
    pStorage_->pUnknown = pUnknown;
    if ( !pStorage_->pUnknown )
    {
        return;
    }

    if ( addref )
    {
        pStorage_->pUnknown->AddRef();
    }

    HRESULT hresult = pStorage_->pUnknown->QueryInterface( IID_IDispatch, reinterpret_cast<void**>( &pStorage_->pDispatch ) );
    if ( FAILED( hresult ) )
    {
        pStorage_->pDispatch = nullptr;
        return;
    }

    unsigned ctinfo;
    hresult = pStorage_->pDispatch->GetTypeInfoCount( &ctinfo );
    if ( SUCCEEDED( hresult ) && ctinfo )
    {
        pStorage_->pDispatch->GetTypeInfo( 0, 0, &pStorage_->pTypeInfo );
    }
}

JsActiveXObject::JsActiveXObject( JSContext* cx, CLSID& clsid )
    : pJsCtx_( cx )
{
    HRESULT hresult = CoCreateInstance( clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, reinterpret_cast<void**>( &pStorage_->pUnknown ) );
    if ( FAILED( hresult ) )
    {
        pStorage_->pUnknown = nullptr;
        return;
    }

    hresult = pStorage_->pUnknown->QueryInterface( IID_IDispatch, reinterpret_cast<void**>( &pStorage_->pDispatch ) );
    //maybe I don't know what to do with it, but it might get passed to
    //another COM function
    if ( FAILED( hresult ) )
    {
        pStorage_->pDispatch = nullptr;
        return;
    }

    unsigned ctinfo;
    hresult = pStorage_->pDispatch->GetTypeInfoCount( &ctinfo );
    if ( SUCCEEDED( hresult ) && ctinfo )
    {
        pStorage_->pDispatch->GetTypeInfo( 0, 0, &pStorage_->pTypeInfo );
    }
}

JsActiveXObject::~JsActiveXObject()
{
    MarkStoredObjectAsToBeDeleted( pStorage_ );
    pStorage_ = nullptr;
}

std::unique_ptr<JsActiveXObject> JsActiveXObject::CreateNative( JSContext* cx, const std::wstring& name )
{
    CLSID clsid;
    HRESULT hresult = ( name[0] == L'{' )
                          ? CLSIDFromString( name.c_str(), &clsid )
                          : CLSIDFromProgID( name.c_str(), &clsid );
    qwr::QwrException::ExpectTrue( SUCCEEDED( hresult ), L"Invalid CLSID: {}", name );

    std::unique_ptr<JsActiveXObject> nativeObject;
    IUnknown* unk = nullptr;
    hresult = GetActiveObject( clsid, nullptr, &unk );
    if ( SUCCEEDED( hresult ) && unk )
    {
        nativeObject = std::make_unique<JsActiveXObject>( cx, unk );
        qwr::QwrException::ExpectTrue( nativeObject->pStorage_->pUnknown, L"Failed to create ActiveXObject object via IUnknown: {}", name );
    }

    if ( !nativeObject )
    {
        nativeObject = std::make_unique<JsActiveXObject>( cx, clsid );
        qwr::QwrException::ExpectTrue( nativeObject->pStorage_->pUnknown, L"Failed to create ActiveXObject object via CLSID: {}", name );
    }

    return nativeObject;
}

size_t JsActiveXObject::GetInternalSize( const std::wstring& /*name*/ )
{
    return 0;
}

void JsActiveXObject::PostCreate( JSContext* cx, JS::HandleObject self )
{
    auto pNative = static_cast<JsActiveXObject*>( JS_GetInstancePrivate( cx, self, &JsActiveXObject::JsClass, nullptr ) );
    assert( pNative );
    return pNative->SetupMembers( self );
}

JSObject* JsActiveXObject::Constructor( JSContext* cx, const std::wstring& name )
{
    return JsActiveXObject::CreateJs( cx, name );
}

JSObject* JsActiveXObject::CreateFromArray( JSContext* cx, JS::HandleValue arr, uint32_t elementVariantType )
{
    JS::RootedObject jsObjectIn( cx, arr.toObjectOrNull() );
    qwr::QwrException::ExpectTrue( jsObjectIn, "Value is not a JS object" );

    bool is;
    if ( !JS_IsArrayObject( cx, jsObjectIn, &is ) )
    {
        throw smp::JsException();
    }
    qwr::QwrException::ExpectTrue( is, "Object is not an array" );

    _variant_t var;
    convert::com::JsArrayToVariantArray( cx, jsObjectIn, elementVariantType, var );

    std::unique_ptr<JsActiveXObject> x( new JsActiveXObject( cx, var ) );
    JS::RootedObject jsObject( cx, JsActiveXObject::CreateJsFromNative( cx, std::move( x ) ) );
    assert( jsObject );

    return jsObject;
}

std::wstring JsActiveXObject::ToString()
{
    JS::RootedValue jsValue( pJsCtx_ );
    auto dispRet = GetDispId( L"toString", false );
    TryGetProperty( ( dispRet ? L"toString" : L"" ), &jsValue );

    return convert::to_native::ToValue<std::wstring>( pJsCtx_, jsValue );
}

JSObject* JsActiveXObject::CreateNewIterator()
{
    return JsActiveXObject_Iterator::CreateJs( pJsCtx_, *this );
}

bool JsActiveXObject::HasIterator() const
{
    return JsActiveXObject_Iterator::IsIterable( *this );
}

std::optional<DISPID> JsActiveXObject::GetDispId( const std::wstring& name, bool reportError )
{
    if ( !pStorage_->pDispatch )
    {
        if ( reportError )
        {
            throw qwr::QwrException( "Internal error: pStorage_->pDispatch is null" );
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
    auto* cname = const_cast<wchar_t*>( name.c_str() );
    HRESULT hresult = pStorage_->pDispatch->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_USER_DEFAULT, &dispId );
    if ( FAILED( hresult ) )
    {
        hresult = pStorage_->pDispatch->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_SYSTEM_DEFAULT, &dispId );
        if ( FAILED( hresult ) )
        {
            if ( reportError )
            {
                throw qwr::QwrException( "Failed to get DISPID for `{}`", qwr::unicode::ToU8( name ) );
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

void JsActiveXObject::GetImpl( int dispId, std::span<_variant_t> args, JS::MutableHandleValue vp, std::optional<std::function<void()>> refreshFn )
{
    DISPPARAMS dispparams = { nullptr, nullptr, 0, 0 };
    if ( !args.empty() )
    {
        dispparams.rgvarg = args.data();
        dispparams.cArgs = args.size();
    }

    _variant_t varResult;
    EXCEPINFO exception{};
    UINT argerr = 0;

    // don't use DispInvoke, because we don't know the TypeInfo
    HRESULT hresult = pStorage_->pDispatch->Invoke( dispId,
                                                    IID_NULL,
                                                    LOCALE_USER_DEFAULT,
                                                    DISPATCH_PROPERTYGET,
                                                    &dispparams,
                                                    &varResult,
                                                    &exception,
                                                    &argerr );

    if ( refreshFn )
    {
        std::invoke( *refreshFn );
    }

    if ( FAILED( hresult ) )
    {
        smp::com::ReportActiveXError( hresult, exception, argerr );
    }

    convert::com::VariantToJs( pJsCtx_, varResult, vp );
}

bool JsActiveXObject::Has( const std::wstring& name )
{
    return members_.contains( name );
}

bool JsActiveXObject::IsGet( const std::wstring& name )
{
    return members_.contains( name ) && members_[name]->isGet;
}

bool JsActiveXObject::IsSet( const std::wstring& name )
{
    return members_.contains( name ) && ( members_[name]->isPut || members_[name]->isPutRef );
}

bool JsActiveXObject::IsInvoke( const std::wstring& name )
{
    return members_.contains( name ) && members_[name]->isInvoke;
}

std::vector<std::wstring> JsActiveXObject::GetAllMembers()
{
    std::vector<std::wstring> memberList;
    for ( const auto& member: members_ )
    {
        memberList.push_back( member.first );
    }
    return memberList;
}

void JsActiveXObject::GetItem( int32_t index, JS::MutableHandleValue vp )
{
    qwr::QwrException::ExpectTrue( pStorage_->pDispatch, "Internal error: pStorage_->pDispatch is null" );

    const auto dispRet = GetDispId( L"item" );
    qwr::QwrException::ExpectTrue( dispRet.has_value(), L"Object is not subscriptable" );

    std::array args{ _variant_t{ index } };
    GetImpl( *dispRet, args, vp );
}

bool JsActiveXObject::TryGetProperty( const std::wstring& propName, JS::MutableHandleValue vp )
{
    qwr::QwrException::ExpectTrue( pStorage_->pDispatch, "Internal error: pStorage_->pDispatch is null" );

    auto dispRet = GetDispId( propName, false );
    if ( !dispRet )
    { // not an error
        return false;
    }

    GetImpl( *dispRet, {}, vp );
    return true;
}

void JsActiveXObject::GetProperty( const std::wstring& propName, JS::MutableHandleValue vp )
{
    if ( !TryGetProperty( propName, vp ) )
    {
        vp.setUndefined();
    }
}

void JsActiveXObject::Get( JS::CallArgs& callArgs )
{
    qwr::QwrException::ExpectTrue( pStorage_->pDispatch, "Internal error: pStorage_->pDispatch is null" );
    qwr::QwrException::ExpectTrue( callArgs.length(), "Property name is missing" );

    const auto propName = convert::to_native::ToValue<std::wstring>( pJsCtx_, callArgs[0] );

    const auto dispRet = GetDispId( propName );
    qwr::QwrException::ExpectTrue( dispRet.has_value(), L"Invalid property name: {}", propName );

    const uint32_t argc = callArgs.length() - 1;
    std::vector<_variant_t> args( argc );
    for ( auto&& [i, arg]: ranges::views::enumerate( args ) )
    {
        JsToVariantSafe( pJsCtx_, callArgs[argc - i], arg );
    }

    const auto refreshValues = [&] {
        // in case any empty ActiveXObject objects were filled in by Invoke()
        for ( auto i: ranges::views::indices( callArgs.length() - 1 ) )
        {
            RefreshValue( pJsCtx_, callArgs[1 + i] );
        }
    };
    GetImpl( *dispRet, args, callArgs.rval(), refreshValues );
}

void JsActiveXObject::Set( const std::wstring& propName, JS::HandleValue v )
{
    qwr::QwrException::ExpectTrue( pStorage_->pDispatch, "Internal error: pStorage_->pDispatch is null" );

    const auto dispRet = GetDispId( propName );
    qwr::QwrException::ExpectTrue( dispRet.has_value(), L"Invalid property name: {}", propName );

    _variant_t arg;
    JsToVariantSafe( pJsCtx_, v, arg );

    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { &arg, &dispput, 1, 1 };

    WORD flag = DISPATCH_PROPERTYPUT;
    if ( ( arg.vt == VT_DISPATCH || arg.vt == VT_UNKNOWN )
         && members_.contains( propName ) && members_[propName]->isPutRef )
    { //must be passed by name
        flag = DISPATCH_PROPERTYPUTREF;
    }

    EXCEPINFO exception{};
    UINT argerr = 0;

    HRESULT hresult = pStorage_->pDispatch->Invoke( *dispRet,
                                                    IID_NULL,
                                                    LOCALE_USER_DEFAULT,
                                                    flag,
                                                    &dispparams,
                                                    nullptr,
                                                    &exception,
                                                    &argerr );

    RefreshValue( pJsCtx_, v );

    if ( FAILED( hresult ) )
    {
        smp::com::ReportActiveXError( hresult, exception, argerr );
    }
}

void JsActiveXObject::Set( const JS::CallArgs& callArgs )
{
    qwr::QwrException::ExpectTrue( pStorage_->pDispatch, "Internal error: pStorage_->pDispatch is null" );
    qwr::QwrException::ExpectTrue( callArgs.length(), "Property name is missing" );

    const auto propName = convert::to_native::ToValue<std::wstring>( pJsCtx_, callArgs[0] );

    const auto dispRet = GetDispId( propName );
    qwr::QwrException::ExpectTrue( dispRet.has_value(), L"Invalid property name: {}", propName );

    const uint32_t argc = callArgs.length() - 1;
    std::vector<_variant_t> args( argc );
    for ( auto&& [i, arg]: ranges::views::enumerate( args ) )
    {
        JsToVariantSafe( pJsCtx_, callArgs[argc - i], arg );
    }

    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { nullptr, &dispput, 0, 1 };
    if ( !args.empty() )
    {
        dispparams.rgvarg = args.data();
        dispparams.cArgs = args.size();
    }

    EXCEPINFO exception{};
    UINT argerr = 0;

    WORD flag = DISPATCH_PROPERTYPUT;
    if ( ( args[argc - 1].vt == VT_DISPATCH || args[argc - 1].vt == VT_UNKNOWN )
         && members_.contains( propName ) && members_[propName]->isPutRef )
    { //must be passed by name
        flag = DISPATCH_PROPERTYPUTREF;
    }

    // don't use DispInvoke, because we don't know the TypeInfo
    HRESULT hresult = pStorage_->pDispatch->Invoke( *dispRet,
                                                    IID_NULL,
                                                    LOCALE_USER_DEFAULT,
                                                    flag,
                                                    &dispparams,
                                                    nullptr,
                                                    &exception,
                                                    &argerr );

    for ( auto i: ranges::views::indices( callArgs.length() - 1 ) )
    {
        RefreshValue( pJsCtx_, callArgs[1 + i] ); //in case any empty ActiveXObject objects were filled in by Invoke()
    }

    if ( FAILED( hresult ) )
    {
        smp::com::ReportActiveXError( hresult, exception, argerr );
    }
}

void JsActiveXObject::Invoke( const std::wstring& funcName, const JS::CallArgs& callArgs )
{
    qwr::QwrException::ExpectTrue( pStorage_->pDispatch, "Internal error: pStorage_->pDispatch is null" );

    const auto dispRet = GetDispId( funcName );
    qwr::QwrException::ExpectTrue( dispRet.has_value(), L"Invalid function name: {}", funcName );

    const uint32_t argc = callArgs.length();
    std::vector<_variant_t> args( argc );
    for ( auto&& [i, arg]: ranges::views::enumerate( args ) )
    {
        JsToVariantSafe( pJsCtx_, callArgs[( argc - 1 ) - i], arg );
    }

    DISPPARAMS dispparams = { nullptr, nullptr, 0, 0 };
    if ( !args.empty() )
    {
        dispparams.rgvarg = args.data();
        dispparams.cArgs = args.size();
    }

    _variant_t varResult;
    EXCEPINFO exception{};
    UINT argerr = 0;

    // don't use DispInvoke, because we don't know the TypeInfo
    HRESULT hresult = pStorage_->pDispatch->Invoke( *dispRet,
                                                    IID_NULL,
                                                    LOCALE_USER_DEFAULT,
                                                    DISPATCH_METHOD,
                                                    &dispparams,
                                                    &varResult,
                                                    &exception,
                                                    &argerr );

    for ( auto i: ranges::views::indices( callArgs.length() ) )
    {
        RefreshValue( pJsCtx_, callArgs[i] ); //in case any empty ActiveXObject objects were filled in by Invoke()
    }

    if ( FAILED( hresult ) )
    {
        smp::com::ReportActiveXError( hresult, exception, argerr );
    }

    convert::com::VariantToJs( pJsCtx_, varResult, callArgs.rval() );
}

void JsActiveXObject::SetupMembers( JS::HandleObject jsObject )
{
    if ( areMembersSetup_ )
    {
        return;
    }

    qwr::QwrException::ExpectTrue( pStorage_->pUnknown || pStorage_->pDispatch || hasVariant_, "Internal error: pStorage_->pUnknown and pStorage_->pDispatch are null and pStorage_->variant was not set" );
    if ( hasVariant_ )
    { // opaque data
        areMembersSetup_ = true;
        return;
    }

    if ( !pStorage_->pDispatch )
    {
        HRESULT hr = pStorage_->pUnknown->QueryInterface( IID_IDispatch, reinterpret_cast<void**>( &pStorage_->pDispatch ) );
        qwr::error::CheckHR( hr, "QueryInterface" );
    }

    if ( !pStorage_->pTypeInfo )
    {
        unsigned ctinfo;
        HRESULT hr = pStorage_->pDispatch->GetTypeInfoCount( &ctinfo );
        qwr::error::CheckHR( hr, "GetTypeInfoCount" );

        hr = pStorage_->pDispatch->GetTypeInfo( 0, 0, &pStorage_->pTypeInfo );
        qwr::error::CheckHR( hr, "GetTypeInfo" );
    }

    ParseTypeInfoRecursive( pJsCtx_, pStorage_->pTypeInfo, members_ );
    SetupMembers_Impl( jsObject );

    areMembersSetup_ = true;
}

void JsActiveXObject::ParseTypeInfoRecursive( JSContext* cx, ITypeInfo* pTypeInfo, MemberMap& members )
{
    ParseTypeInfo( pTypeInfo, members );

    TYPEATTR* pAttr = nullptr;
    HRESULT hr = pTypeInfo->GetTypeAttr( &pAttr );
    qwr::error::CheckHR( hr, "GetTypeAttr" );

    qwr::final_action autoTypeAttr( [pTypeInfo, pAttr] {
        pTypeInfo->ReleaseTypeAttr( pAttr );
    } );

    if ( !( pAttr->wTypeFlags & TYPEFLAG_FRESTRICTED )
         && ( TKIND_DISPATCH == pAttr->typekind || TKIND_INTERFACE == pAttr->typekind )
         && pAttr->cImplTypes )
    {
        for ( auto i: ranges::views::indices( pAttr->cImplTypes ) )
        {
            HREFTYPE hRef = 0;
            hr = pTypeInfo->GetRefTypeOfImplType( i, &hRef );
            qwr::error::CheckHR( hr, "GetTypeAttr" );

            ITypeInfo* pTypeInfoCur = nullptr;
            hr = pTypeInfo->GetRefTypeInfo( hRef, &pTypeInfoCur );
            if ( SUCCEEDED( hr ) && pTypeInfoCur )
            {
                qwr::final_action autoTypeInfo( [pTypeInfoCur] {
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

void JsActiveXObject::ParseTypeInfo( ITypeInfo* pTypeInfo, MemberMap& members )
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

void JsActiveXObject::SetupMembers_Impl( JS::HandleObject jsObject )
{
    for ( const auto& [name, member]: members_ )
    {
        if ( member->isInvoke )
        {
            if ( !JS_DefineUCFunction( pJsCtx_, jsObject, reinterpret_cast<const char16_t*>( name.c_str() ), name.length(), ActiveX_Run, 0, JSPROP_ENUMERATE ) )
            {
                throw JsException();
            }
        }
    }
}

} // namespace mozjs
