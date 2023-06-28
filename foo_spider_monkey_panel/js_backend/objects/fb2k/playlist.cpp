#include <stdafx.h>

#include "playlist.h"

#include <convert/js_to_native.h>
#include <fb2k/playlist_index_manager.h>
#include <fb2k/playlist_lock.h>
#include <fb2k/title_format_manager.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/playlist_iterator.h>
#include <js_backend/objects/fb2k/track.h>
#include <js_backend/objects/fb2k/track_list.h>
#include <js_backend/utils/js_error_helper.h>
#include <js_backend/utils/js_object_constants.h>
#include <js_backend/utils/js_property_helper.h>

using namespace smp;

namespace
{

class OnProcessLocationsNotify_InsertHandles
    : public process_locations_notify
{
public:
    OnProcessLocationsNotify_InsertHandles( uint32_t playlistIdx, uint32_t baseIdx, bool shouldSelect );

    void on_completion( metadb_handle_list_cref items ) override;
    void on_aborted() override;

private:
    const uint32_t playlistIdx_ = 0;
    const uint32_t baseIdx_ = 0;
    const bool shouldSelect_ = true;
};

// Wrapper to intercept indexed gets/sets.
class PlaylistProxyHandler : public js::ForwardingProxyHandler
{
public:
    PlaylistProxyHandler();

    bool get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
              JS::HandleId id, JS::MutableHandleValue vp ) const override;
    bool set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
              JS::HandleValue receiver, JS::ObjectOpResult& result ) const override;
};

const PlaylistProxyHandler kProxySingleton;

} // namespace

namespace
{

OnProcessLocationsNotify_InsertHandles::OnProcessLocationsNotify_InsertHandles( uint32_t playlistIdx, uint32_t baseIdx, bool shouldSelect )
    : playlistIdx_( playlistIdx )
    , baseIdx_( baseIdx )
    , shouldSelect_( shouldSelect )
{
}

void OnProcessLocationsNotify_InsertHandles::on_completion( metadb_handle_list_cref handles )
{
    auto api = playlist_manager::get();
    if ( playlistIdx_ >= api->get_playlist_count()
         || ( api->playlist_lock_get_filter_mask( playlistIdx_ ) & playlist_lock::filter_add ) )
    {
        return;
    }

    pfc::bit_array_val selection( shouldSelect_ );
    api->playlist_insert_items( playlistIdx_, baseIdx_, handles, selection );
    if ( shouldSelect_ )
    {
        api->set_active_playlist( playlistIdx_ );
        api->playlist_set_focus_item( playlistIdx_, baseIdx_ );
    }
}

void OnProcessLocationsNotify_InsertHandles::on_aborted()
{
}

PlaylistProxyHandler::PlaylistProxyHandler()
    : js::ForwardingProxyHandler( mozjs::GetSmpProxyFamily() )
{
}

bool PlaylistProxyHandler::get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
                                JS::HandleId id, JS::MutableHandleValue vp ) const
{
    if ( !id.isInt() )
    {
        return js::ForwardingProxyHandler::get( cx, proxy, receiver, id, vp );
    }

    try
    {
        auto pNativeTarget = mozjs::Playlist::ExtractNative( cx, proxy );
        assert( pNativeTarget );

        vp.set( pNativeTarget->GetItem( static_cast<int32_t>( id.toInt() ) ) );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
        return false;
    }

    return true;
}

bool PlaylistProxyHandler::set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
                                JS::HandleValue receiver, JS::ObjectOpResult& result ) const
{
    if ( !id.isInt() )
    {
        return js::ForwardingProxyHandler::set( cx, proxy, id, v, receiver, result );
    }

    try
    {
        auto pNativeTarget = mozjs::Playlist::ExtractNative( cx, proxy );
        assert( pNativeTarget );

        auto pNativeValue = mozjs::convert::to_native::ToValue<smp::not_null<mozjs::Track*>>( cx, v );
        pNativeTarget->PutItem( static_cast<int32_t>( id.toInt() ), pNativeValue );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
        return false;
    }

    result.succeed();
    return true;
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
    Playlist::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Playlist",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( addPaths, Playlist::AddPaths, Playlist::AddPathsWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( clearSelection, Playlist::ClearSelection );
MJS_DEFINE_JS_FN_FROM_NATIVE( createUndoPoint, Playlist::CreateUndoPoint );
MJS_DEFINE_JS_FN_FROM_NATIVE( ensureTrackVisible, Playlist::EnsureTrackVisible );
MJS_DEFINE_JS_FN_FROM_NATIVE( executeDefaultTrackAction, Playlist::ExecuteDefaultTrackAction );
MJS_DEFINE_JS_FN_FROM_NATIVE( focusTrackByIndex, Playlist::FocusTrackByIndex );
MJS_DEFINE_JS_FN_FROM_NATIVE( focusTrackByValue, Playlist::FocusTrackByValue );
MJS_DEFINE_JS_FN_FROM_NATIVE( getAutoPlaylistDisplayName, Playlist::GetAutoPlaylistDisplayName );
MJS_DEFINE_JS_FN_FROM_NATIVE( getFocusedTrackIndex, Playlist::GetFocusedTrackIndex );
MJS_DEFINE_JS_FN_FROM_NATIVE( getLockName, Playlist::GetLockName );
MJS_DEFINE_JS_FN_FROM_NATIVE( getLockedOperations, Playlist::GetLockedOperations );
MJS_DEFINE_JS_FN_FROM_NATIVE( getSelectedTrackIndices, Playlist::GetSelectedTrackIndices );
MJS_DEFINE_JS_FN_FROM_NATIVE( getSelectedTracks, Playlist::GetSelectedTracks );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( getTracks, Playlist::GetTracks, Playlist::GetTracksWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( insertTracks, Playlist::InsertTracks, Playlist::InsertTracksWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( isAutoPlaylistUiAvailable, Playlist::IsAutoPlaylistUiAvailable );
MJS_DEFINE_JS_FN_FROM_NATIVE( isLocked, Playlist::IsLocked );
MJS_DEFINE_JS_FN_FROM_NATIVE( isRedoAvailable, Playlist::IsRedoAvailable );
MJS_DEFINE_JS_FN_FROM_NATIVE( isTrackSelected, Playlist::IsTrackSelected );
MJS_DEFINE_JS_FN_FROM_NATIVE( isUndoAvailable, Playlist::IsUndoAvailable );
MJS_DEFINE_JS_FN_FROM_NATIVE( moveSelection, Playlist::MoveSelection );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( orderTracksByFormat, Playlist::OrderTracksByFormat, Playlist::OrderTracksByFormatWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( redo, Playlist::Redo );
MJS_DEFINE_JS_FN_FROM_NATIVE( removeAll, Playlist::RemoveAll );
MJS_DEFINE_JS_FN_FROM_NATIVE( removeSelectedTracks, Playlist::RemoveSelectedTracks );
MJS_DEFINE_JS_FN_FROM_NATIVE( removeUnselectedTracks, Playlist::RemoveUnselectedTracks );
MJS_DEFINE_JS_FN_FROM_NATIVE( selectTracks, Playlist::SelectTracks );
MJS_DEFINE_JS_FN_FROM_NATIVE( selectTracksWithOpt, Playlist::SelectTracksWithOpt );
MJS_DEFINE_JS_FN_FROM_NATIVE( setAsActive, Playlist::SetAsActive );
MJS_DEFINE_JS_FN_FROM_NATIVE( setAsCurrentlyPlaying, Playlist::SetAsCurrentlyPlaying );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( setLockedOperations, Playlist::SetLockedOperations, Playlist::SetLockedOperationsWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( showAutoPlaylistUi, Playlist::ShowAutoPlaylistUi );
MJS_DEFINE_JS_FN_FROM_NATIVE( undo, Playlist::Undo );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( createIterator, Playlist::CreateIterator );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "addPaths", addPaths, 1, kDefaultPropsFlags ),
        JS_FN( "clearSelection", clearSelection, 0, kDefaultPropsFlags ),
        JS_FN( "createUndoPoint", createUndoPoint, 0, kDefaultPropsFlags ),
        JS_FN( "ensureTrackVisible", ensureTrackVisible, 0, kDefaultPropsFlags ),
        JS_FN( "executeDefaultTrackAction", executeDefaultTrackAction, 1, kDefaultPropsFlags ),
        JS_FN( "focusTrackByIndex", focusTrackByIndex, 1, kDefaultPropsFlags ),
        JS_FN( "focusTrackByValue", focusTrackByValue, 1, kDefaultPropsFlags ),
        JS_FN( "getAutoPlaylistDisplayName", getAutoPlaylistDisplayName, 0, kDefaultPropsFlags ),
        JS_FN( "getFocusedTrackIndex", getFocusedTrackIndex, 0, kDefaultPropsFlags ),
        JS_FN( "getLockName", getLockName, 0, kDefaultPropsFlags ),
        JS_FN( "getLockedOperations", getLockedOperations, 0, kDefaultPropsFlags ),
        JS_FN( "getSelectedTrackIndices", getSelectedTrackIndices, 0, kDefaultPropsFlags ),
        JS_FN( "getSelectedTracks", getSelectedTracks, 0, kDefaultPropsFlags ),
        JS_FN( "getTracks", getTracks, 0, kDefaultPropsFlags ),
        JS_FN( "insertTracks", insertTracks, 1, kDefaultPropsFlags ),
        JS_FN( "isAutoPlaylistUiAvailable", isAutoPlaylistUiAvailable, 0, kDefaultPropsFlags ),
        JS_FN( "isLocked", isLocked, 0, kDefaultPropsFlags ),
        JS_FN( "isRedoAvailable", isRedoAvailable, 0, kDefaultPropsFlags ),
        JS_FN( "isTrackSelected", isTrackSelected, 0, kDefaultPropsFlags ),
        JS_FN( "isUndoAvailable", isUndoAvailable, 0, kDefaultPropsFlags ),
        JS_FN( "moveSelection", moveSelection, 1, kDefaultPropsFlags ),
        JS_FN( "orderTracksByFormat", orderTracksByFormat, 1, kDefaultPropsFlags ),
        JS_FN( "redo", redo, 0, kDefaultPropsFlags ),
        JS_FN( "removeAll", removeAll, 0, kDefaultPropsFlags ),
        JS_FN( "removeSelectedTracks", removeSelectedTracks, 0, kDefaultPropsFlags ),
        JS_FN( "removeUnselectedTracks", removeUnselectedTracks, 0, kDefaultPropsFlags ),
        JS_FN( "selectTracks", selectTracks, 0, kDefaultPropsFlags ),
        JS_FN( "selectTracksWithOpt", selectTracksWithOpt, 0, kDefaultPropsFlags ),
        JS_FN( "setAsActive", setAsActive, 0, kDefaultPropsFlags ),
        JS_FN( "setAsCurrentlyPlaying", setAsCurrentlyPlaying, 0, kDefaultPropsFlags ),
        JS_FN( "setLockedOperations", setLockedOperations, 1, kDefaultPropsFlags ),
        JS_FN( "showAutoPlaylistUi", showAutoPlaylistUi, 0, kDefaultPropsFlags ),
        JS_FN( "undo", undo, 0, kDefaultPropsFlags ),
        JS_SYM_FN( iterator, createIterator, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_index, Playlist::get_Index );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_isValid, Playlist::get_IsValid );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_isAutoPlaylist, Playlist::get_IsAutoPlaylist );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_length, Playlist::get_Length );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_name, Playlist::get_Name );
MJS_DEFINE_JS_FN_FROM_NATIVE( put_name, Playlist::put_Name );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "index", get_index, kDefaultPropsFlags ),
        JS_PSG( "isValid", get_isValid, kDefaultPropsFlags ),
        JS_PSG( "isAutoPlaylist", get_isAutoPlaylist, kDefaultPropsFlags ),
        JS_PSG( "length", get_length, kDefaultPropsFlags ),
        JS_PSGS( "name", get_name, put_name, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::Playlist );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<Playlist>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<Playlist>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<Playlist>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<Playlist>::PrototypeId = JsPrototypeId::New_Playlist;
const js::BaseProxyHandler& JsObjectTraits<Playlist>::JsProxy = kProxySingleton;

Playlist::Playlist( JSContext* cx, uint32_t index )
    : pJsCtx_( cx )
    , id_( smp::PlaylistIndexManager::Get().GetId( index ) )
{
}

Playlist::~Playlist()
{
}

std::unique_ptr<Playlist>
Playlist::CreateNative( JSContext* cx, uint32_t index )
{
    return std::unique_ptr<Playlist>( new Playlist( cx, index ) );
}

size_t Playlist::GetInternalSize() const
{
    return 0;
}

JS::Value Playlist::GetItem( int32_t trackIndex ) const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    const auto trackCount = static_cast<int32_t>( GetItemCount() );

    qwr::QwrException::ExpectTrue( trackIndex < trackCount, "Index is out of bounds" );

    const auto adjustedTrackIndex = ( trackIndex < 0 ? trackCount + trackIndex : trackIndex );
    if ( adjustedTrackIndex < 0 )
    {
        return JS::UndefinedValue();
    }

    return JS::ObjectValue( *Track::CreateJs( pJsCtx_, api->playlist_get_item_handle( playlistIndex, adjustedTrackIndex ) ) );
}

void Playlist::PutItem( int32_t trackIndex, smp::not_null<Track*> track )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    const auto trackCount = static_cast<int32_t>( GetItemCount() );

    qwr::QwrException::ExpectTrue( trackIndex < trackCount, "Index is out of bounds" );

    const auto adjustedTrackIndex = ( trackIndex < 0 ? trackCount + trackIndex : trackIndex );
    if ( adjustedTrackIndex < 0 )
    {
        return;
    }

    api->playlist_replace_item( playlistIndex, adjustedTrackIndex, track->GetHandle() );
}

size_t Playlist::GetItemCount() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    return api->playlist_get_item_count( playlistIndex );
}

void Playlist::AddPaths( const std::vector<qwr::u8string>& paths, uint32_t start, JS::HandleValue options )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    qwr::QwrException::ExpectTrue( options.isUndefined() || options.isObject(), "Invalid options type" );

    pfc::string_list_impl locationList;
    for ( const auto& path: paths )
    {
        locationList += path.c_str();
    }

    int32_t flags = playlist_incoming_item_filter_v2::op_flag_delay_ui;
    auto selectInsertedItems = true;
    if ( options.isObject() )
    {
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );
        if ( auto valueOpt = utils::GetOptionalProperty<bool>( pJsCtx_, jsOptions, "applyFilter" );
             valueOpt && !( *valueOpt ) )
        {
            flags |= playlist_incoming_item_filter_v2::op_flag_no_filter;
        }
        if ( auto valueOpt = utils::GetOptionalProperty<bool>( pJsCtx_, jsOptions, "selectInserted" ) )
        {
            selectInsertedItems = *valueOpt;
        }
    }

    playlist_incoming_item_filter_v2::get()->process_locations_async(
        locationList,
        flags,
        nullptr,
        nullptr,
        nullptr,
        fb2k::service_new<::OnProcessLocationsNotify_InsertHandles>( playlistIndex, start, selectInsertedItems ) );
}

void Playlist::AddPathsWithOpt( size_t optArgCount, const std::vector<qwr::u8string>& paths, uint32_t start, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return AddPaths( paths, start, options );
    case 1:
        return AddPaths( paths, start );
    case 2:
        return AddPaths( paths );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void Playlist::ClearSelection()
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->playlist_clear_selection( playlistIndex );
}

void Playlist::CreateUndoPoint()
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->playlist_undo_backup( playlistIndex );
}

void Playlist::EnsureTrackVisible( uint32_t trackIndex )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    const auto trackCount = GetItemCount();

    qwr::QwrException::ExpectTrue( trackIndex < trackCount, "Index is out of bounds" );

    playlist_manager::get()->playlist_ensure_visible( playlistIndex, trackIndex );
}

void Playlist::ExecuteDefaultTrackAction( uint32_t trackIndex ) const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    const auto trackCount = GetItemCount();

    qwr::QwrException::ExpectTrue( trackIndex < trackCount, "Index is out of bounds" );

    playlist_manager::get()->playlist_execute_default_action( playlistIndex, trackIndex );
}

pfc::string8_fast Playlist::GetAutoPlaylistDisplayName() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    const auto autoPlApi = autoplaylist_manager::get();

    qwr::QwrException::ExpectTrue( autoPlApi->is_client_present( playlistIndex ), "Playlist is not an auto-playlist" );

    autoplaylist_client_v2::ptr autoPlClientV2;
    try
    {
        autoplaylist_client::ptr autoPlClient = autoPlApi->query_client( playlistIndex );
        if ( !autoPlClient->service_query_t( autoPlClientV2 ) )
        {
            return "";
        }
    }
    catch ( const exception_autoplaylist& /*e*/ )
    {
        return "";
    }

    pfc::string8_fast name;
    autoPlClientV2->get_display_name( name );
    return name;
}

JSObject* Playlist::GetSelectedTracks() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    metadb_handle_list handles;
    api->playlist_get_selected_items( playlistIndex, handles );
    return TrackList::CreateJs( pJsCtx_, handles );
}

std::vector<uint32_t> Playlist::GetSelectedTrackIndices() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    pfc::bit_array_bittable mask;
    api->playlist_get_selection_mask( playlistIndex, mask );

    std::vector<uint32_t> selectedIndices;
    mask.walk( api->playlist_get_item_count( playlistIndex ), [&]( size_t n ) {
        selectedIndices.push_back( n );
    } );

    return selectedIndices;
}

int32_t Playlist::GetFocusedTrackIndex() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    return api->playlist_get_focus_item( playlistIndex );
}

std::optional<pfc::string8_fast> Playlist::GetLockName() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    qwr::QwrException::ExpectTrue( api->playlist_lock_is_present( playlistIndex ), "Playlist does not have a lock" );

    pfc::string8_fast name;
    if ( !api->playlist_lock_query_name( playlistIndex, name ) )
    {
        return std::nullopt;
    }

    return name;
}

std::vector<qwr::u8string> Playlist::GetLockedOperations() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    static const std::unordered_map<int, qwr::u8string> lockFlagToOperation = {
        { playlist_lock::filter_add, "addTracks" },
        { playlist_lock::filter_remove, "removeTracks" },
        { playlist_lock::filter_reorder, "reorderTracks" },
        { playlist_lock::filter_replace, "replaceTracks" },
        { playlist_lock::filter_rename, "renamePlaylist" },
        { playlist_lock::filter_remove_playlist, "deletePlaylist" },
        { playlist_lock::filter_default_action, "executeDefaultAction" }
    };

    const auto lockMask = api->playlist_lock_get_filter_mask( playlistIndex );
    const auto actions =
        lockFlagToOperation
        | ranges::views::filter( [&]( const auto& elem ) { return !!( lockMask & elem.first ); } )
        | ranges::views::transform( [&]( const auto& elem ) { return elem.second; } )
        | ranges::to_vector;

    if ( actions.empty() )
    {
        return { "unknownLock" };
    }

    return actions;
}

JSObject* Playlist::GetTracks( JS::HandleValue trackIndices ) const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    metadb_handle_list handles;
    if ( trackIndices.isUndefined() )
    {
        api->playlist_get_all_items( playlistIndex, handles );
    }
    else
    {
        const auto handleIndicesVec = convert::to_native::ToValue<std::vector<uint32_t>>( pJsCtx_, trackIndices );
        const auto handleCount = api->playlist_get_item_count( playlistIndex );
        pfc::bit_array_bittable handleIndicesBitArray;
        for ( auto index: handleIndicesVec )
        {
            qwr::QwrException::ExpectTrue( index < handleCount, "Index is out of bounds" );
            handleIndicesBitArray.set( index, true );
        }

        api->playlist_get_items( playlistIndex, handles, handleIndicesBitArray );
    }
    return TrackList::CreateJs( pJsCtx_, handles );
}

JSObject* Playlist::GetTracksWithOpt( size_t optArgCount, JS::HandleValue trackIndices ) const
{
    switch ( optArgCount )
    {
    case 0:
        return GetTracks( trackIndices );
    case 1:
        return GetTracks();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

bool Playlist::IsAutoPlaylistUiAvailable() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    const auto autoPlApi = autoplaylist_manager::get();
    if ( !autoPlApi->is_client_present( playlistIndex ) )
    {
        return false;
    }

    autoplaylist_client_v2::ptr autoPlClientV2;
    try
    {
        autoplaylist_client::ptr autoPlClient = autoPlApi->query_client( playlistIndex );
        if ( !autoPlClient->service_query_t( autoPlClientV2 ) )
        {
            return false;
        }
    }
    catch ( const exception_autoplaylist& /*e*/ )
    {
        return false;
    }

    return autoPlClientV2->show_ui_available();
}

bool Playlist::IsLocked() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    return api->playlist_lock_is_present( playlistIndex );
}

bool Playlist::IsTrackSelected( uint32_t trackIndex ) const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    const auto trackCount = GetItemCount();

    qwr::QwrException::ExpectTrue( trackIndex < trackCount, "Index is out of bounds" );

    return api->playlist_is_item_selected( playlistIndex, trackIndex );
}

bool Playlist::IsRedoAvailable() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    return api->playlist_is_redo_available( playlistIndex );
}

bool Playlist::IsUndoAvailable() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    return api->playlist_is_undo_available( playlistIndex );
}

void Playlist::FocusTrackByIndex( uint32_t trackIndex )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    const auto trackCount = GetItemCount();

    qwr::QwrException::ExpectTrue( trackIndex < trackCount, "Index is out of bounds" );

    playlist_manager::get()->playlist_set_focus_item( playlistIndex, trackIndex );
}

void Playlist::FocusTrackByValue( smp::not_null<Track*> track )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    playlist_manager::get()->playlist_set_focus_by_handle( playlistIndex, track->GetHandle() );
}

void Playlist::InsertTracks( JS::HandleValue tracks, uint32_t start, JS::HandleValue options )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    const auto handles = TrackList::ValueToHandleList( pJsCtx_, tracks );

    auto applyFilter = true;
    auto selectInsertedItems = true;
    if ( options.isObject() )
    {
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );
        if ( auto valueOpt = utils::GetOptionalProperty<bool>( pJsCtx_, jsOptions, "applyFilter" ) )
        {
            applyFilter = *valueOpt;
        }
        if ( auto valueOpt = utils::GetOptionalProperty<bool>( pJsCtx_, jsOptions, "selectInserted" ) )
        {
            selectInsertedItems = *valueOpt;
        }
    }

    if ( applyFilter )
    {
        api->playlist_insert_items_filter( playlistIndex, start, handles, selectInsertedItems );
    }
    else
    {
        api->playlist_insert_items( playlistIndex, start, handles, pfc::bit_array_val( selectInsertedItems ) );
    }
}

void Playlist::InsertTracksWithOpt( size_t optArgCount, JS::HandleValue tracks, uint32_t start, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return InsertTracks( tracks, start, options );
    case 1:
        return InsertTracks( tracks, start );
    case 2:
        return InsertTracks( tracks );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void Playlist::MoveSelection( int32_t delta )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    playlist_manager::get()->playlist_move_selection( playlistIndex, delta );
}

void Playlist::OrderTracksByFormat( const qwr::u8string& query, int8_t direction, bool sortSelectedOnly )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    if ( direction > 0 )
    {
        api->playlist_sort_by_format( playlistIndex, query.c_str(), sortSelectedOnly );
    }
    else
    {
        metadb_handle_list handles;
        api->playlist_get_all_items( playlistIndex, handles );

        auto order = ranges::views::indices( handles.get_count() ) | ranges::to_vector;
        auto pTitleFormat = smp::TitleFormatManager::Get().Load( query );
        metadb_handle_list_helper::sort_by_format_get_order( handles, order.data(), pTitleFormat, nullptr, direction );

        api->playlist_reorder_items( playlistIndex, order.data(), order.size() );
    }
}

void Playlist::OrderTracksByFormatWithOpt( size_t optArgCount, const qwr::u8string& query, int8_t direction, bool sortSelectedOnly )
{
    switch ( optArgCount )
    {
    case 0:
        return OrderTracksByFormat( query, direction, sortSelectedOnly );
    case 1:
        return OrderTracksByFormat( query, direction );
    case 2:
        return OrderTracksByFormat( query );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void Playlist::Redo()
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->playlist_redo_restore( playlistIndex );
}

void Playlist::RemoveAll()
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->playlist_clear( playlistIndex );
}

void Playlist::RemoveSelectedTracks()
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->playlist_remove_selection( playlistIndex, false );
}

void Playlist::RemoveUnselectedTracks()
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->playlist_remove_selection( playlistIndex, true );
}

void Playlist::SelectTracks( const std::vector<uint32_t>& trackIndices, bool invertSelection )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    const auto trackCount = GetItemCount();

    const auto trackIndicesSet = trackIndices | ranges::to<std::unordered_set>();

    pfc::bit_array_bittable affected( trackCount );
    for ( auto i: ranges::views::indices( trackCount ) )
    {
        affected.set( i, trackIndicesSet.contains( i ) );
    }

    api->playlist_set_selection( playlistIndex, affected, pfc::bit_array_val( !invertSelection ) );
}

void Playlist::SelectTracksWithOpt( size_t optArgCount, const std::vector<uint32_t>& trackIndices, bool invertSelection )
{
    switch ( optArgCount )
    {
    case 0:
        return SelectTracks( trackIndices, invertSelection );
    case 1:
        return SelectTracks( trackIndices );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void Playlist::SetAsActive()
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->set_active_playlist( playlistIndex );
}

void Playlist::SetAsCurrentlyPlaying()
{
    auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->set_playing_playlist( playlistIndex );
}

void Playlist::SetLockedOperations( JS::HandleValue lockedOperations )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    qwr::QwrException::ExpectTrue( lockedOperations.isObject() || lockedOperations.isUndefined(), "`lockedOperations` argument is not an object nor null" );

    auto& playlistLockManager = PlaylistLockManager::Get();

    qwr::QwrException::ExpectTrue( !api->playlist_lock_is_present( playlistIndex ) || playlistLockManager.HasLock( playlistIndex ), "This lock is owned by a different component" );

    uint32_t newLockMask = 0;
    if ( lockedOperations.isObject() )
    {
        static const std::unordered_map<qwr::u8string, int> operationToLockFlag = {
            { "addTracks", playlist_lock::filter_add },
            { "removeTracks", playlist_lock::filter_remove },
            { "reorderTracks", playlist_lock::filter_reorder },
            { "replaceTracks", playlist_lock::filter_replace },
            { "renamePlaylist", playlist_lock::filter_rename },
            { "deletePlaylist", playlist_lock::filter_remove_playlist },
            { "executeDefaultAction", playlist_lock::filter_default_action }
        };

        const auto lockedOperationsVec = convert::to_native::ToValue<std::vector<qwr::u8string>>( pJsCtx_, lockedOperations );
        for ( const auto& op: lockedOperationsVec )
        {
            qwr::QwrException::ExpectTrue( operationToLockFlag.contains( op ), "Unknown operation name: {}", op );
            newLockMask |= operationToLockFlag.at( op );
        }
    }

    const auto currentLockMask = api->playlist_lock_get_filter_mask( playlistIndex );
    if ( newLockMask == currentLockMask )
    {
        return;
    }

    if ( playlistLockManager.HasLock( playlistIndex ) )
    {
        playlistLockManager.RemoveLock( playlistIndex );
    }

    if ( newLockMask )
    {
        playlistLockManager.InstallLock( playlistIndex, newLockMask );
    }
}

void Playlist::SetLockedOperationsWithOpt( size_t optArgCount, JS::HandleValue lockedOperations )
{
    switch ( optArgCount )
    {
    case 0:
        return SetLockedOperations( lockedOperations );
    case 1:
        return SetLockedOperations();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void Playlist::ShowAutoPlaylistUi() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();
    const auto autoPlApi = autoplaylist_manager::get();

    qwr::QwrException::ExpectTrue( autoPlApi->is_client_present( playlistIndex ), "Playlist is not an auto-playlist" );

    try
    {
        auto client = autoPlApi->query_client( playlistIndex );
        client->show_ui( playlistIndex );
    }
    catch ( const exception_autoplaylist& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

void Playlist::Undo()
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->playlist_undo_restore( playlistIndex );
}

JSObject* Playlist::CreateIterator( JS::HandleObject jsSelf ) const
{
    return Playlist_Iterator::CreateJs( pJsCtx_, jsSelf );
}

uint32_t Playlist::get_Index() const
{
    const auto idxOpt = smp::PlaylistIndexManager::Get().GetIndex( id_ );
    qwr::QwrException::ExpectTrue( idxOpt.has_value(), "Playlist was removed and can't be used" );

    return *idxOpt;
}

bool Playlist::get_IsValid() const
{
    return smp::PlaylistIndexManager::Get().GetIndex( id_ ).has_value();
}

bool Playlist::get_IsAutoPlaylist() const
{
    const auto playlistIndex = get_Index();
    return autoplaylist_manager::get()->is_client_present( playlistIndex );
}

uint32_t Playlist::get_Length() const
{
    return GetItemCount();
}

pfc::string8_fast Playlist::get_Name() const
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    pfc::string8_fast name;
    api->playlist_get_name( playlistIndex, name );
    return name;
}

void Playlist::put_Name( const qwr::u8string& name )
{
    const auto api = playlist_manager::get();
    const auto playlistIndex = get_Index();

    api->playlist_rename( playlistIndex, name.c_str(), name.length() );
}

} // namespace mozjs
