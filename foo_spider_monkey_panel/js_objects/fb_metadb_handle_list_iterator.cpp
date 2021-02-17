#include <stdafx.h>

#include "fb_metadb_handle_list_iterator.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>

#include <qwr/final_action.h>

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
    JsFbMetadbHandleList_Iterator::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbMetadbHandleList_Iterator",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( next, JsFbMetadbHandleList_Iterator::Next )

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

const JSClass JsFbMetadbHandleList_Iterator::JsClass = jsClass;
const JSFunctionSpec* JsFbMetadbHandleList_Iterator::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbMetadbHandleList_Iterator::JsProperties = jsProperties.data();
const JsPrototypeId JsFbMetadbHandleList_Iterator::PrototypeId = JsPrototypeId::FbMetadbHandleList_Iterator;

JsFbMetadbHandleList_Iterator::JsFbMetadbHandleList_Iterator( JSContext* cx, JsFbMetadbHandleList& handleList )
    : pJsCtx_( cx )
    , handleList_( handleList )
    , heapHelper_( cx )
{
}

JsFbMetadbHandleList_Iterator::~JsFbMetadbHandleList_Iterator()
{
    heapHelper_.Finalize();
}

std::unique_ptr<JsFbMetadbHandleList_Iterator>
JsFbMetadbHandleList_Iterator::CreateNative( JSContext* cx, JsFbMetadbHandleList& handleList )
{
    return std::unique_ptr<JsFbMetadbHandleList_Iterator>( new JsFbMetadbHandleList_Iterator( cx, handleList ) );
}

size_t JsFbMetadbHandleList_Iterator::GetInternalSize( JsFbMetadbHandleList& /*handleList*/ )
{
    return 0;
}

JSObject* JsFbMetadbHandleList_Iterator::Next()
{
    const bool isAtEnd = ( curPosition_ >= handleList_.get_Count() );
    const auto autoIncrement = qwr::final_action( [&] {
        if ( !isAtEnd )
        {
            ++curPosition_;
        }
    } );

    if ( !jsNextId_ )
    {
        JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );

        JS::RootedObject jsValueObject( pJsCtx_ );
        if ( !isAtEnd )
        {
            jsValueObject = handleList_.get_Item( curPosition_ );
        }
        AddProperty( pJsCtx_, jsObject, "value", static_cast<JS::HandleObject>( jsValueObject ) );
        AddProperty( pJsCtx_, jsObject, "done", isAtEnd );

        jsNextId_ = heapHelper_.Store( jsObject );

        JS::RootedObject jsNext( pJsCtx_, &heapHelper_.Get( *jsNextId_ ).toObject() );
        return jsNext;
    }
    else
    {
        JS::RootedObject jsNext( pJsCtx_, &heapHelper_.Get( *jsNextId_ ).toObject() );

        JS::RootedObject jsValueObject( pJsCtx_ );
        if ( !isAtEnd )
        {
            jsValueObject = handleList_.get_Item( curPosition_ );
        }
        SetProperty( pJsCtx_, jsNext, "value", static_cast<JS::HandleObject>( jsValueObject ) );
        SetProperty( pJsCtx_, jsNext, "done", isAtEnd );

        return jsNext;
    }
}

} // namespace mozjs
