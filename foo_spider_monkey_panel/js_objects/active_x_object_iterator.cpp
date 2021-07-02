#include <stdafx.h>

#include "active_x_object_iterator.h"

#include <com_objects/dispatch_ptr.h>
#include <convert/com.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/active_x_object.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>

#include <qwr/winapi_error_helpers.h>

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
    JsActiveXObject_Iterator::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ActiveXObject_Iterator",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE, // COM objects must be finalized in foreground
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( next, JsActiveXObject_Iterator::Next )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "next", next, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsActiveXObject_Iterator::JsClass = jsClass;
const JSFunctionSpec* JsActiveXObject_Iterator::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsActiveXObject_Iterator::JsProperties = jsProperties.data();
const JsPrototypeId JsActiveXObject_Iterator::PrototypeId = JsPrototypeId::ActiveX_Iterator;

JsActiveXObject_Iterator::JsActiveXObject_Iterator( JSContext* cx, EnumVARIANTComPtr pEnum )
    : pJsCtx_( cx )
    , pEnum_( pEnum )
    , heapHelper_( cx )
{
}

JsActiveXObject_Iterator::~JsActiveXObject_Iterator()
{
    heapHelper_.Finalize();
}

std::unique_ptr<JsActiveXObject_Iterator>
JsActiveXObject_Iterator::CreateNative( JSContext* cx, JsActiveXObject& activeXObject )
{
    const auto pUnknown = ( activeXObject.pStorage_->pUnknown ? activeXObject.pStorage_->pUnknown : activeXObject.pStorage_->pDispatch );
    qwr::QwrException::ExpectTrue( pUnknown, "Object is not iterable" );

    CDispatchPtr pCollection( pUnknown );
    auto pEnum = [&] {
        try
        {
            return EnumVARIANTComPtr( pCollection.Get( static_cast<DISPID>( DISPID_NEWENUM ) ) );
        }
        catch ( const _com_error& )
        {
            throw qwr::QwrException( "Object is not iterable" );
        }
    }();

    return std::unique_ptr<JsActiveXObject_Iterator>( new JsActiveXObject_Iterator( cx, pEnum ) );
}

size_t JsActiveXObject_Iterator::GetInternalSize( JsActiveXObject& /*activeXObject*/ )
{
    return 0;
}

JSObject* JsActiveXObject_Iterator::Next()
{
    LoadCurrentElement();

    if ( !jsNextId_ )
    {
        JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );

        JS::RootedValue jsValue( pJsCtx_ );
        convert::com::VariantToJs( pJsCtx_, curElem_, &jsValue );
        AddProperty( pJsCtx_, jsObject, "value", static_cast<JS::HandleValue>( jsValue ) );
        AddProperty( pJsCtx_, jsObject, "done", isAtEnd_ );

        jsNextId_ = heapHelper_.Store( jsObject );

        JS::RootedObject jsNext( pJsCtx_, &heapHelper_.Get( *jsNextId_ ).toObject() );
        return jsNext;
    }
    else
    {
        JS::RootedObject jsNext( pJsCtx_, &heapHelper_.Get( *jsNextId_ ).toObject() );

        JS::RootedValue jsValue( pJsCtx_ );
        convert::com::VariantToJs( pJsCtx_, curElem_, &jsValue );
        SetProperty( pJsCtx_, jsNext, "value", static_cast<JS::HandleValue>( jsValue ) );
        SetProperty( pJsCtx_, jsNext, "done", isAtEnd_ );

        return jsNext;
    }
}

bool JsActiveXObject_Iterator::IsIterable( const JsActiveXObject& activeXObject )
{
    const auto pUnknown = ( activeXObject.pStorage_->pUnknown ? activeXObject.pStorage_->pUnknown : activeXObject.pStorage_->pDispatch );
    if ( !pUnknown )
    {
        return false;
    }

    try
    {
        (void)EnumVARIANTComPtr( CDispatchPtr( pUnknown ).Get( static_cast<DISPID>( DISPID_NEWENUM ) ) );
        return true;
    }
    catch ( const _com_error& )
    {
        return false;
    }
}

void JsActiveXObject_Iterator::LoadCurrentElement()
{
    HRESULT hr = pEnum_->Next( 1, curElem_.GetAddress(), nullptr );
    if ( S_FALSE == hr )
    { // means that we've reached the end
        curElem_.Clear();
        isAtEnd_ = true;
    }
    else
    {
        qwr::error::CheckHR( hr, "Next" );
        isAtEnd_ = false;
    }
}

} // namespace mozjs
