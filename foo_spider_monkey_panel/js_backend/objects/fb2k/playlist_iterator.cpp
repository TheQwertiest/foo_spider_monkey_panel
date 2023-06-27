#include <stdafx.h>

#include "playlist_iterator.h"

#include <convert/js_to_native.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/playlist.h>
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
    Playlist_Iterator::FinalizeJsObject,
    nullptr,
    nullptr,
    Playlist_Iterator::Trace
};

JSClass jsClass = {
    "Playlist_Iterator",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( next, Playlist_Iterator::Next )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "next", next, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<Playlist_Iterator>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<Playlist_Iterator>::JsFunctions = jsFunctions.data();
const JsPrototypeId JsObjectTraits<Playlist_Iterator>::PrototypeId = JsPrototypeId::New_PlaylistIterator;

Playlist_Iterator::Playlist_Iterator( JSContext* cx, JS::HandleObject playlist )
    : pJsCtx_( cx )
    , jsParent_( playlist )
    , pPlaylist_( convert::to_native::ToValue<smp::not_null<Playlist*>>( cx, playlist ) )
{
}

Playlist_Iterator::~Playlist_Iterator()
{
}

std::unique_ptr<Playlist_Iterator>
Playlist_Iterator::CreateNative( JSContext* cx, JS::HandleObject Playlist )
{
    return std::unique_ptr<Playlist_Iterator>( new Playlist_Iterator( cx, Playlist ) );
}

size_t Playlist_Iterator::GetInternalSize() const
{
    return 0;
}

void Playlist_Iterator::Trace( JSTracer* trc, JSObject* obj )
{
    auto pNative = JsObjectBase<Playlist_Iterator>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    JS::TraceEdge( trc, &pNative->jsParent_, "Heap: Playlist_Iterator: parent" );
    JS::TraceEdge( trc, &pNative->jsNext_, "Heap: Playlist_Iterator: next element" );
}

JSObject* Playlist_Iterator::Next()
{
    const bool isAtEnd = ( curPosition_ >= pPlaylist_->GetItemCount() );
    const auto autoIncrement = qwr::final_action( [&] {
        if ( !isAtEnd )
        {
            ++curPosition_;
        }
    } );

    if ( !jsNext_ )
    {
        jsNext_ = JS_NewPlainObject( pJsCtx_ );
    }

    JS::RootedValue jsValue( pJsCtx_ );
    if ( !isAtEnd )
    {
        jsValue = pPlaylist_->GetItem( curPosition_ );
    }

    JS::RootedObject jsNext( pJsCtx_, jsNext_ );
    utils::SetProperty( pJsCtx_, jsNext, "value", static_cast<JS::HandleValue>( jsValue ) );
    utils::SetProperty( pJsCtx_, jsNext, "done", isAtEnd );

    return jsNext;
}

} // namespace mozjs
