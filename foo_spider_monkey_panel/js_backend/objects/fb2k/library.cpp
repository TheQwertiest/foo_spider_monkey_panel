#include <stdafx.h>

#include "library.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/objects/fb2k/track.h>
#include <js_backend/objects/fb2k/track_list.h>
#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/js_promise_event.h>
#include <utils/relative_filepath_trie.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

using namespace smp;

namespace
{

class TracksSearchThreadTask
{
public:
    [[nodiscard]] TracksSearchThreadTask( search_index::ptr pIndex,
                                          search_filter::ptr pFilter,
                                          metadb_handle_list handles,
                                          JSContext* cx,
                                          JS::HandleObject jsTarget,
                                          HWND hPanelWnd );

    [[nodiscard]] TracksSearchThreadTask( search_index::ptr pIndex,
                                          search_filter::ptr pFilter,
                                          JSContext* cx,
                                          JS::HandleObject jsTarget,
                                          HWND hPanelWnd );
    ~TracksSearchThreadTask() = default;

    TracksSearchThreadTask( const TracksSearchThreadTask& ) = delete;
    TracksSearchThreadTask& operator=( const TracksSearchThreadTask& ) = delete;

    void Run();

private:
    JSContext* pJsCtx_ = nullptr;
    mozjs::HeapDataHolder_Object heapHolder_;

    HWND hPanelWnd_ = nullptr;

    search_index::ptr pIndex_;
    search_filter::ptr pFilter_;
    metadb_handle_list handles_;
    bool useHandles_ = false;
};

} // namespace

namespace
{

TracksSearchThreadTask::TracksSearchThreadTask( search_index::ptr pIndex,
                                                search_filter::ptr pFilter,
                                                metadb_handle_list handles,
                                                JSContext* cx,
                                                JS::HandleObject jsTarget,
                                                HWND hPanelWnd )
    : pJsCtx_( cx )
    , heapHolder_( cx, jsTarget )
    , hPanelWnd_( hPanelWnd )
    , pIndex_( pIndex )
    , pFilter_( pFilter )
    , handles_( handles )
    , useHandles_( true )
{
}

TracksSearchThreadTask::TracksSearchThreadTask( search_index::ptr pIndex,
                                                search_filter::ptr pFilter,
                                                JSContext* cx,
                                                JS::HandleObject jsTarget,
                                                HWND hPanelWnd )
    : pJsCtx_( cx )
    , heapHolder_( cx, jsTarget )
    , hPanelWnd_( hPanelWnd )
    , pIndex_( pIndex )
    , pFilter_( pFilter )
    , useHandles_( false )
{
}

void TracksSearchThreadTask::Run()
{
    try
    {
        auto pArray = pIndex_->search( pFilter_, ( useHandles_ ? &handles_ : nullptr ), 0, fb2k::mainAborter() );
        const auto promiseResolver = [cx = pJsCtx_, pArray]() -> JS::Value {
            JS::RootedObject jsResult( cx, mozjs::TrackList::CreateJs( cx, pArray->as_list_of<metadb_handle>() ) );
            JS::RootedValue jsResultValue( cx, JS::ObjectValue( *jsResult ) );
            return jsResultValue;
        };
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  promiseResolver ) );
    }
    catch ( const qwr::QwrException& /*e*/ )
    {
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  std::current_exception() ) );
    }
    catch ( const foobar2000_io::exception_aborted& /*e*/ )
    {
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  std::current_exception() ) );
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
    JsObjectBase<Library>::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Library",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( contains, Library::Contains );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( filterTracks, Library::FilterTracks, Library::FilterTracksWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( getTracks, Library::GetTracks, Library::GetTracksWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( getTracksRelativePath, Library::GetTracksRelativePath );
MJS_DEFINE_JS_FN_FROM_NATIVE( orderTracksByRelativePath, Library::OrderTracksByRelativePath );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_isEnabled, Library::get_IsEnabled );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "contains", contains, 1, kDefaultPropsFlags ),
        JS_FN( "filterTracks", filterTracks, 1, kDefaultPropsFlags ),
        JS_FN( "getTracks", getTracks, 0, kDefaultPropsFlags ),
        JS_FN( "getTracksRelativePath", getTracksRelativePath, 1, kDefaultPropsFlags ),
        JS_FN( "orderTracksByRelativePath", orderTracksByRelativePath, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "isEnabled", get_isEnabled, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::Library );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<Library>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<Library>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<Library>::JsProperties = jsProperties.data();

const std::unordered_set<smp::EventId> Library::kHandledEvents{
    EventId::kNew_FbLibraryItemsAdded,
    EventId::kNew_FbLibraryItemsModified,
    EventId::kNew_FbLibraryItemsRemoved,
};

Library::Library( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
{
}

Library::~Library()
{
}

std::unique_ptr<Library>
Library::CreateNative( JSContext* cx )
{
    return std::unique_ptr<Library>( new Library( cx ) );
}

size_t Library::GetInternalSize() const
{
    return 0;
}

const std::string& Library::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbLibraryItemsAdded, "tracksAdd" },
        { EventId::kNew_FbLibraryItemsModified, "tracksModify" },
        { EventId::kNew_FbLibraryItemsRemoved, "tracksRemove" },
    };

    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

EventStatus Library::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    JS::RootedObject jsEvent( pJsCtx_,
                              mozjs::JsEvent::CreateJs( pJsCtx_, eventType, JsEvent::EventProperties{ .cancelable = false } ) );
    JS::RootedValue jsEventValue( pJsCtx_, JS::ObjectValue( *jsEvent ) );
    DispatchEvent( self, jsEventValue );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

bool Library::Contains( smp::not_null<Track*> track ) const
{
    const auto api = library_manager_v6::get();
    return api->is_item_in_library( track->GetHandle() );
}

JSObject* Library::FilterTracks( JS::HandleValue tracks, const qwr::u8string& query ) const
{
    auto handles = TrackList::ValueToHandleList( pJsCtx_, tracks );

    JS::RootedObject jsPromise( pJsCtx_, JS::NewPromiseObject( pJsCtx_, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    const auto pSearchFilter = [&] {
        try
        {
            return search_filter_manager_v3::get()->create( query.c_str() );
        }
        catch ( const pfc::exception& e )
        { // create throws on invalid query
            throw qwr::QwrException( e.what() );
        }
    }();
    const auto pSearchIndex = search_index_manager::get()->get_library_index();
    const auto pSearchTask = std::make_shared<TracksSearchThreadTask>( pSearchIndex,
                                                                       pSearchFilter,
                                                                       handles,
                                                                       pJsCtx_,
                                                                       jsPromise,
                                                                       GetPanelHwndForCurrentGlobal( pJsCtx_ ) );
    fb2k::inCpuWorkerThread( [pSearchTask] { pSearchTask->Run(); } );

    return jsPromise;
}

JSObject* Library::FilterTracksWithOpt( size_t optArgCount, JS::HandleValue tracks, const qwr::u8string& query ) const
{
    switch ( optArgCount )
    {
    case 0:
        return FilterTracks( tracks, query );
    case 1:
        return FilterTracks( tracks );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* Library::GetTracks( const qwr::u8string& query ) const
{
    JS::RootedObject jsPromise( pJsCtx_, JS::NewPromiseObject( pJsCtx_, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    if ( query.empty() )
    {
        metadb_handle_list items;
        library_manager::get()->get_all_items( items );

        JS::RootedObject jsTrackList( pJsCtx_, TrackList::CreateJs( pJsCtx_, items ) );
        JS::RootedValue jsTrackListValue( pJsCtx_, JS::ObjectValue( *jsTrackList ) );
        JS::ResolvePromise( pJsCtx_, jsPromise, jsTrackListValue );
    }
    else
    {
        const auto pSearchFilter = [&] {
            try
            {
                return search_filter_manager_v3::get()->create( query.c_str() );
            }
            catch ( const std::exception& e )
            { // create throws on invalid query
                throw qwr::QwrException( e.what() );
            }
        }();
        const auto pSearchIndex = search_index_manager::get()->get_library_index();
        const auto pSearchTask = std::make_shared<TracksSearchThreadTask>( pSearchIndex,
                                                                           pSearchFilter,
                                                                           pJsCtx_,
                                                                           jsPromise,
                                                                           GetPanelHwndForCurrentGlobal( pJsCtx_ ) );
        fb2k::inCpuWorkerThread( [pSearchTask] { pSearchTask->Run(); } );
    }

    return jsPromise;
}

JSObject* Library::GetTracksWithOpt( size_t optArgCount, const qwr::u8string& query ) const
{
    switch ( optArgCount )
    {
    case 0:
        return GetTracks( query );
    case 1:
        return GetTracks();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

std::vector<pfc::string8_fast> Library::GetTracksRelativePath( JS::HandleValue tracks ) const
{
    const auto api = library_manager::get();

    const auto handles = TrackList::ValueToHandleList( pJsCtx_, tracks );
    return handles | ranges::views::transform( [&]( const auto& handle ) {
               pfc::string8_fast path;
               api->get_relative_path( handle, path );
               return path;
           } )
           | ranges::to<std::vector>();
}

void Library::OrderTracksByRelativePath( smp::not_null<TrackList*> tracks )
{
    // TODO: consider moving this off-thread

    // Note: there is built-in metadb_handle_list::sort_by_relative_path(),
    // but this implementation is much faster because of trie usage.
    // Also see `get_subsong_index` below.

    const auto api = library_manager::get();

    auto& handles = tracks->GetMutableHandleList();
    const auto stlHandleList = qwr::pfc_x::Make_Stl_CRef( handles );

    pfc::string8_fastalloc temp;
    temp.prealloc( 512 );

    smp::RelativeFilepathTrie<size_t> trie;
    for ( const auto& [i, handle]: ranges::views::enumerate( stlHandleList ) )
    {
        temp = ""; ///< get_relative_path won't fill data on fail
        api->get_relative_path( handle, temp );

        // One physical file can have multiple handles,
        // which all return the same path, but have different subsong idx
        // (e.g. cuesheets or files with multiple chapters)
        temp << handle->get_subsong_index();

        trie.emplace( qwr::unicode::ToWide( temp ), i );
    }

    handles.reorder( trie.get_sorted_values().data() );
}

bool Library::get_IsEnabled() const
{
    const auto api = library_manager::get();
    return api->is_library_enabled();
}

} // namespace mozjs
