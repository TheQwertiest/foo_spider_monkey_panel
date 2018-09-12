#include <stdafx.h>
#include "enumerator.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/active_x_object.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/dispatch_ptr.h>
#include <convert/com.h>

// TODO: add font caching

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
    JsEnumerator::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Enumerator",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE, // COM objects must be finalized in foreground
    &jsOps
};


bool Enumerator_Constructor_Impl( JSContext* cx, unsigned argc, JS::Value* vp )
{
    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
    if ( !argc || !args[0].isObject() )
    {
        JS_ReportErrorUTF8( cx, "Argument is not an ActiveX object" );
        return false;
    }

    JS::RootedObject jsArgObject( cx, &args[0].toObject() );
    auto retVal = mozjs::convert::to_native::ToValue<ActiveXObject*>( cx, jsArgObject );
    if ( !retVal )
    {
        JS_ReportErrorUTF8( cx, "Argument is not an ActiveX object" );
        return false;
    }

    auto pActiveXObject = retVal.value();
    JS::RootedObject jsObject( cx, 
                               JsEnumerator::CreateJs( cx, (pActiveXObject->pUnknown_ ? pActiveXObject->pUnknown_ : pActiveXObject->pDispatch_) )
    );
    if ( !jsObject )
    {// report in CreateJs
        return false;
    }

    args.rval().setObjectOrNull( jsObject );
    return true;
}

MJS_WRAP_JS_TO_NATIVE_FN( Enumerator_Constructor, Enumerator_Constructor_Impl )

MJS_DEFINE_JS_TO_NATIVE_FN( JsEnumerator, atEnd )
MJS_DEFINE_JS_TO_NATIVE_FN( JsEnumerator, item )
MJS_DEFINE_JS_TO_NATIVE_FN( JsEnumerator, moveFirst )
MJS_DEFINE_JS_TO_NATIVE_FN( JsEnumerator, moveNext )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "atEnd", atEnd, 0, DefaultPropsFlags() ),
    JS_FN( "item", item, 0, DefaultPropsFlags() ),
    JS_FN( "moveFirst", moveFirst, 0, DefaultPropsFlags() ),
    JS_FN( "moveNext", moveNext, 0, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

const JSClass JsEnumerator::JsClass = jsClass;
const JSFunctionSpec* JsEnumerator::JsFunctions = jsFunctions;
const JSPropertySpec* JsEnumerator::JsProperties = jsProperties;
const JsPrototypeId JsEnumerator::PrototypeId = JsPrototypeId::Enumerator;

JsEnumerator::JsEnumerator( JSContext* cx, EnumVARIANTComPtr pEnum, bool hasElements )
    : pJsCtx_( cx )
    , pEnum_( pEnum )
    , hasElements_( hasElements )
    , isAtEnd_( !hasElements )
{
}

JsEnumerator::~JsEnumerator()
{ 
}

JSObject* JsEnumerator::InstallProto( JSContext *cx, JS::HandleObject parentObject )
{
    // TODO: move to ObjectBase
    return JS_InitClass( cx, parentObject, nullptr, &jsClass,
                         Enumerator_Constructor, 0,
                         nullptr, jsFunctions, nullptr, nullptr );
}

std::unique_ptr<JsEnumerator> 
JsEnumerator::CreateNative( JSContext* cx, IUnknown* pUnknown )
{
    assert( pUnknown );

    try
    {
        CDispatchPtr pCollection( pUnknown );
        uint32_t collectionSize = pCollection.Get( L"Count" );
        EnumVARIANTComPtr pEnum( pCollection.Get( L"_NewEnum" ) );

        auto pNative = std::unique_ptr<JsEnumerator>( new JsEnumerator( cx, pEnum, !!collectionSize ) );
        if ( !pNative->GetCurrentElement() )
        {// reports
            return nullptr;
        }

        return pNative;
    }
    catch ( const _com_error& e )
    {
        pfc::string8_fast errorMsg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.ErrorMessage() );
        pfc::string8_fast errorSource8 = pfc::stringcvt::string_utf8_from_wide( e.Source().length() ? (const wchar_t*)e.Source() : L"" );
        pfc::string8_fast errorDesc8 = pfc::stringcvt::string_utf8_from_wide( e.Description().length() ? (const wchar_t*)e.Description() : L"" );
        JS_ReportErrorUTF8( cx, "COM error: message %s; source: %s; description: %s", errorMsg8.c_str(), errorSource8.c_str(), errorDesc8.c_str() );
        return nullptr;
    }
}

size_t JsEnumerator::GetInternalSize( IUnknown* pUnknown )
{
    return 0;
}


std::optional<bool> 
JsEnumerator::atEnd()
{
    return !hasElements_ || isAtEnd_;
}

std::optional<JS::Value> 
JsEnumerator::item()
{
    if ( !hasElements_ || isAtEnd_ )
    {
        return JS::UndefinedValue();
    }

    JS::RootedValue jsValue(pJsCtx_);
    if ( !convert::com::VariantToJs( pJsCtx_, curElem_, &jsValue ) )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Failed to convert COM object" );
        return std::nullopt;
    }

    return jsValue;
}

std::optional<std::nullptr_t> 
JsEnumerator::moveFirst()
{
    HRESULT hr = pEnum_->Reset();
    IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, std::nullopt, Reset );

    if ( !GetCurrentElement() )
    {// reports
        return std::nullopt;
    }
    
    return nullptr;
}

std::optional<std::nullptr_t> 
JsEnumerator::moveNext()
{
    if ( !GetCurrentElement() )
    {// reports
        return std::nullopt;
    }

    return nullptr;
}

bool JsEnumerator::GetCurrentElement()
{
    if ( !hasElements_ || isAtEnd_ )
    {
        return true;
    }

    ULONG fetchedElements = 0;
    HRESULT hr = pEnum_->Next( 1, curElem_.GetAddress(), &fetchedElements );
    if ( S_FALSE == hr )
    {// meaning that we reached the end
        fetchedElements = 0;
    }
    else
    {
        IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, false, Next );
    }
    
    if ( !fetchedElements )
    {
        isAtEnd_ = true;
        curElem_.Clear();
    }

    return true;
}

}
