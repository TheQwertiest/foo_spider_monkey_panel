#include <stdafx.h>

#include "track_list_iterator.h"

#include <convert/js_to_native.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/track_list.h>
#include <js_backend/utils/js_property_helper.h>

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
    TrackList_Iterator::FinalizeJsObject,
    nullptr,
    nullptr,
    TrackList_Iterator::Trace
};

JSClass jsClass = {
    "TrackList_Iterator",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( next, TrackList_Iterator::Next )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "next", next, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<TrackList_Iterator>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<TrackList_Iterator>::JsFunctions = jsFunctions.data();
const JsPrototypeId JsObjectTraits<TrackList_Iterator>::PrototypeId = JsPrototypeId::New_TrackListIterator;

TrackList_Iterator::TrackList_Iterator( JSContext* cx, JS::HandleObject trackList )
    : pJsCtx_( cx )
    , jsParent_( trackList )
    , pTrackList_( convert::to_native::ToValue<smp::not_null<TrackList*>>( cx, trackList ) )
{
}

TrackList_Iterator::~TrackList_Iterator()
{
}

std::unique_ptr<TrackList_Iterator>
TrackList_Iterator::CreateNative( JSContext* cx, JS::HandleObject trackList )
{
    return std::unique_ptr<TrackList_Iterator>( new TrackList_Iterator( cx, trackList ) );
}

size_t TrackList_Iterator::GetInternalSize() const
{
    return 0;
}

void TrackList_Iterator::Trace( JSTracer* trc, JSObject* obj )
{
    auto pNative = JsObjectBase<TrackList_Iterator>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    JS::TraceEdge( trc, &pNative->jsNext_, "Heap: TrackList_Iterator: next element" );
}

JSObject* TrackList_Iterator::Next()
{
    const bool isAtEnd = ( curPosition_ >= pTrackList_->GetHandleList().size() );
    const auto autoIncrement = qwr::final_action( [&] {
        if ( !isAtEnd )
        {
            ++curPosition_;
        }
    } );

    if ( !jsNext_ )
    {
        JS::RootedValue jsValue( pJsCtx_ );
        if ( !isAtEnd )
        {
            jsValue = pTrackList_->GetItem( curPosition_ );
        }

        JS::RootedObject jsNext( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
        // TODO: replace with add property
        utils::SetProperty( pJsCtx_, jsNext, "value", static_cast<JS::HandleValue>( jsValue ) );
        utils::SetProperty( pJsCtx_, jsNext, "done", isAtEnd );

        jsNext_ = jsNext;

        return jsNext;
    }
    else
    {
        JS::RootedValue jsValue( pJsCtx_ );
        if ( !isAtEnd )
        {
            jsValue = pTrackList_->GetItem( curPosition_ );
        }

        JS::RootedObject jsNext( pJsCtx_, jsNext_.get() );
        utils::SetProperty( pJsCtx_, jsNext, "value", static_cast<JS::HandleValue>( jsValue ) );
        utils::SetProperty( pJsCtx_, jsNext, "done", isAtEnd );

        return jsNext;
    }
}

} // namespace mozjs
