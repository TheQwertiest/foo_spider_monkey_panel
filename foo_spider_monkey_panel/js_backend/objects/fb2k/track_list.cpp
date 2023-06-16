#include <stdafx.h>

#include "track_list.h"

#include <convert/js_to_native.h>
#include <fb2k/title_format_manager.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/track.h>
#include <js_backend/objects/fb2k/track_list_iterator.h>
#include <js_backend/utils/js_object_constants.h>
#include <utils/relative_filepath_trie.h>

using namespace smp;

namespace
{

// Wrapper to intercept indexed gets/sets.
class TrackListProxyHandler : public js::ForwardingProxyHandler
{
public:
    TrackListProxyHandler();

    bool get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
              JS::HandleId id, JS::MutableHandleValue vp ) const override;
    bool set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
              JS::HandleValue receiver, JS::ObjectOpResult& result ) const override;
};

const TrackListProxyHandler kProxySingleton;

} // namespace

namespace
{

TrackListProxyHandler::TrackListProxyHandler()
    : js::ForwardingProxyHandler( mozjs::GetSmpProxyFamily() )
{
}

bool TrackListProxyHandler::get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
                                 JS::HandleId id, JS::MutableHandleValue vp ) const
{
    if ( !id.isInt() )
    {
        return js::ForwardingProxyHandler::get( cx, proxy, receiver, id, vp );
    }

    try
    {
        auto pNativeTarget = mozjs::TrackList::ExtractNative( cx, proxy );
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

bool TrackListProxyHandler::set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
                                 JS::HandleValue receiver, JS::ObjectOpResult& result ) const
{
    if ( !id.isInt() )
    {
        return js::ForwardingProxyHandler::set( cx, proxy, id, v, receiver, result );
    }

    try
    {
        auto pNativeTarget = mozjs::TrackList::ExtractNative( cx, proxy );
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
    TrackList::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "TrackList",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( concat, TrackList::Concat );
MJS_DEFINE_JS_FN_FROM_NATIVE( concatInPlace, TrackList::ConcatInPlace );
MJS_DEFINE_JS_FN_FROM_NATIVE( difference, TrackList::Difference );
MJS_DEFINE_JS_FN_FROM_NATIVE( getTotalDuration, TrackList::GetTotalDuration );
MJS_DEFINE_JS_FN_FROM_NATIVE( getTotalSize, TrackList::GetTotalSize );
MJS_DEFINE_JS_FN_FROM_NATIVE( indexOf, TrackList::IndexOf );
MJS_DEFINE_JS_FN_FROM_NATIVE( intersection, TrackList::Intersection );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( optimizeForValueSearch, TrackList::OptimizeForValueSearch, TrackList::OptimizeForValueSearchWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( sortByFormat, TrackList::SortByFormat, TrackList::SortByFormatWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( sortByPath, TrackList::SortByPath );
MJS_DEFINE_JS_FN_FROM_NATIVE( sortByRelativePath, TrackList::SortByRelativePath );
MJS_DEFINE_JS_FN_FROM_NATIVE( push, TrackList::Push );
MJS_DEFINE_JS_FN_FROM_NATIVE( removeAll, TrackList::RemoveAll );
MJS_DEFINE_JS_FN_FROM_NATIVE( removeByIndex, TrackList::RemoveByIndex );
MJS_DEFINE_JS_FN_FROM_NATIVE( removeByValue, TrackList::RemoveByValue );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( splice, TrackList::Splice, TrackList::SpliceWithOpt, 2 );
MJS_DEFINE_JS_FN_FROM_NATIVE( toArray, TrackList::ToArray );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_LOG( Union, "union", TrackList::Union );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_SELF( createIterator, TrackList::CreateIterator );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "concat", concat, 1, kDefaultPropsFlags ),
        JS_FN( "concatInPlace", concatInPlace, 1, kDefaultPropsFlags ),
        JS_FN( "difference", difference, 1, kDefaultPropsFlags ),
        JS_FN( "getTotalDuration", getTotalDuration, 0, kDefaultPropsFlags ),
        JS_FN( "getTotalSize", getTotalSize, 0, kDefaultPropsFlags ),
        JS_FN( "indexOf", indexOf, 1, kDefaultPropsFlags ),
        JS_FN( "intersection", intersection, 1, kDefaultPropsFlags ),
        JS_FN( "optimizeForValueSearch", optimizeForValueSearch, 0, kDefaultPropsFlags ),
        JS_FN( "sortByFormat", sortByFormat, 1, kDefaultPropsFlags ),
        JS_FN( "sortByPath", sortByPath, 0, kDefaultPropsFlags ),
        JS_FN( "sortByRelativePath", sortByRelativePath, 0, kDefaultPropsFlags ),
        JS_FN( "push", push, 1, kDefaultPropsFlags ),
        JS_FN( "removeAll", removeAll, 0, kDefaultPropsFlags ),
        JS_FN( "removeByIndex", removeByIndex, 1, kDefaultPropsFlags ),
        JS_FN( "removeByValue", removeByValue, 1, kDefaultPropsFlags ),
        JS_FN( "splice", splice, 1, kDefaultPropsFlags ),
        JS_FN( "toArray", toArray, 0, kDefaultPropsFlags ),
        JS_FN( "union", Union, 1, kDefaultPropsFlags ),
        JS_SYM_FN( iterator, createIterator, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( from, TrackList::From );

constexpr auto jsStaticFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "from", from, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_length, TrackList::get_Length );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "length", get_length, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( TrackList_Constructor, TrackList::Constructor, TrackList::ConstructorWithOpt, 1 )

MJS_VERIFY_OBJECT( mozjs::TrackList );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<TrackList>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<TrackList>::JsFunctions = jsFunctions.data();
const JSFunctionSpec* JsObjectTraits<TrackList>::JsStaticFunctions = jsStaticFunctions.data();
const JSPropertySpec* JsObjectTraits<TrackList>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<TrackList>::PrototypeId = JsPrototypeId::New_TrackList;
const JSNative JsObjectTraits<TrackList>::JsConstructor = ::TrackList_Constructor;
const js::BaseProxyHandler& JsObjectTraits<TrackList>::JsProxy = kProxySingleton;

TrackList::TrackList( JSContext* cx, const metadb_handle_list& tracks )
    : pJsCtx_( cx )
    , metadbHandleList_( tracks )
{
}

std::unique_ptr<TrackList>
TrackList::CreateNative( JSContext* cx, const metadb_handle_list& tracks )
{
    return std::unique_ptr<TrackList>( new TrackList( cx, tracks ) );
}

size_t TrackList::GetInternalSize() const
{
    return 0;
}

const metadb_handle_list& TrackList::GetHandleList() const
{
    return metadbHandleList_;
}

JS::Value TrackList::GetItem( uint32_t index ) const
{
    qwr::QwrException::ExpectTrue( index < static_cast<int32_t>( metadbHandleList_.size() ), "Index is out of bounds" );

    return JS::ObjectValue( *Track::CreateJs( pJsCtx_, metadbHandleList_[index] ) );
}

void TrackList::PutItem( uint32_t index, smp::not_null<Track*> track )
{
    qwr::QwrException::ExpectTrue( index < static_cast<int32_t>( metadbHandleList_.size() ), "Index is out of bounds" );

    metadbHandleList_.replace_item( index, track->GetHandle() );
    isSorted_ = false;
}

metadb_handle_list TrackList::ValueToHandleList( JSContext* cx, JS::HandleValue tracks )
{
    if ( auto pTrackList = TrackList::ExtractNative( cx, tracks ) )
    {
        return pTrackList->GetHandleList();
    }

    {
        bool is;
        JsException::ExpectTrue( JS::IsArrayObject( cx, tracks, &is ) );
        if ( is )
        {
            metadb_handle_list handleList;
            convert::to_native::ProcessArray<smp::not_null<Track*>>( cx, tracks, [&handleList]( auto pTrack ) {
                handleList.add_item( pTrack->GetHandle() );
            } );
            return handleList;
        }
    }

    throw qwr::QwrException( "Unsupported argument type" );
}

JSObject* TrackList::Constructor( JSContext* cx, JS::HandleValue value )
{
    if ( value.isUndefined() )
    {
        return TrackList::CreateJs( cx, metadb_handle_list{} );
    }

    if ( auto pTrack = Track::ExtractNative( cx, value ) )
    {
        metadb_handle_list handleList;
        handleList.add_item( pTrack->GetHandle() );
        return TrackList::CreateJs( cx, handleList );
    }
    else
    {
        return TrackList::CreateJs( cx, ValueToHandleList( cx, value ) );
    }
}

JSObject* TrackList::ConstructorWithOpt( JSContext* cx, size_t optArgCount, JS::HandleValue value )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor( cx, value );
    case 1:
        return Constructor( cx );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* TrackList::Concat( JS::HandleValue tracks ) const
{
    auto handles = metadbHandleList_;
    handles += ValueToHandleList( pJsCtx_, tracks );
    return TrackList::CreateJs( pJsCtx_, handles );
}

void TrackList::ConcatInPlace( JS::HandleValue tracks )
{
    metadbHandleList_ += ValueToHandleList( pJsCtx_, tracks );
    isSorted_ = false;
}

JSObject* TrackList::Difference( smp::not_null<TrackList*> tracks ) const
{
    const auto a = qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ );
    const auto b = qwr::pfc_x::Make_Stl_CRef( tracks->GetHandleList() );
    qwr::pfc_x::Stl<metadb_handle_list> result;

    std::set_difference( a.cbegin(), a.cend(), b.cbegin(), b.cend(), std::back_inserter( result ) );

    return CreateJs( pJsCtx_, result.Pfc() );
}

double TrackList::GetTotalDuration() const
{
    return metadbHandleList_.calc_total_duration();
}

uint64_t TrackList::GetTotalSize() const
{
    return static_cast<uint64_t>( metadb_handle_list_helper::calc_total_size( metadbHandleList_, true ) );
}

int32_t TrackList::IndexOf( smp::not_null<Track*> track ) const
{
    if ( isSorted_ )
    {
        return static_cast<int32_t>( metadbHandleList_.bsearch_by_pointer( track->GetHandle() ) );
    }
    else
    {
        return static_cast<int32_t>( metadbHandleList_.find_item( track->GetHandle() ) );
    }
}

JSObject* TrackList::Intersection( smp::not_null<TrackList*> tracks ) const
{
    const auto a = qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ );
    const auto b = qwr::pfc_x::Make_Stl_CRef( tracks->GetHandleList() );
    qwr::pfc_x::Stl<metadb_handle_list> result;

    std::set_intersection( a.cbegin(), a.cend(), b.cbegin(), b.cend(), std::back_inserter( result ) );

    return CreateJs( pJsCtx_, result.Pfc() );
}

void TrackList::OptimizeForValueSearch( bool removeDuplicates )
{
    if ( removeDuplicates )
    {
        metadbHandleList_.sort_by_pointer_remove_duplicates();
    }
    else
    {
        metadbHandleList_.sort_by_pointer();
    }
    isSorted_ = true;
}

void TrackList::OptimizeForValueSearchWithOpt( size_t optArgCount, bool removeDuplicates )
{
    switch ( optArgCount )
    {
    case 0:
        return OptimizeForValueSearch( removeDuplicates );
    case 1:
        return OptimizeForValueSearch();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void TrackList::Push( smp::not_null<Track*> track )
{
    metadbHandleList_.add_item( track->GetHandle() );
    isSorted_ = false;
}

void TrackList::RemoveAll()
{
    metadbHandleList_.remove_all();
}

void TrackList::RemoveByIndex( int32_t index )
{
    const auto adjustedIndex = ( index < 0 ? metadbHandleList_.size() + index : index );
    qwr::QwrException::ExpectTrue( adjustedIndex < metadbHandleList_.size(), "Index is out of bounds" );

    metadbHandleList_.remove_by_idx( index );
}

void TrackList::RemoveByValue( smp::not_null<Track*> track )
{
    const auto idx = [&] {
        if ( isSorted_ )
        {
            return metadbHandleList_.bsearch_by_pointer( track->GetHandle() );
        }
        else
        {
            return metadbHandleList_.find_item( track->GetHandle() );
        }
    }();

    metadbHandleList_.remove_by_idx( idx );
}

void TrackList::SortByFormat( const qwr::u8string& query, int8_t direction )
{
    auto pTitleFormat = smp::TitleFormatManager::Get().Load( query );
    metadbHandleList_.sort_by_format( pTitleFormat, nullptr, direction );
    isSorted_ = false;
}

void TrackList::SortByFormatWithOpt( size_t optArgCount, const qwr::u8string& query, int8_t direction )
{
    switch ( optArgCount )
    {
    case 0:
        return SortByFormat( query, direction );
    case 1:
        return SortByFormat( query );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void TrackList::SortByPath()
{
    metadbHandleList_.sort_by_path_quick();
    isSorted_ = false;
}

void TrackList::SortByRelativePath()
{
    // Note: there is built-in metadb_handle_list::Sort_by_relative_path(),
    // but this implementation is much faster because of trie usage.
    // Also see `get_subsong_index` below.
    auto pLibManager = library_manager::get();

    pfc::string8_fastalloc buffer;
    buffer.prealloc( 512 );

    RelativeFilepathTrie<size_t> trie;
    for ( const auto& [i, handle]: ranges::views::enumerate( metadbHandleList_ ) )
    {
        buffer = ""; ///< get_relative_path won't fill data on fail
        pLibManager->get_relative_path( handle, buffer );

        // One physical file can have multiple handles,
        // which all return the same path, but have different subsong idx
        // (e.g. cuesheets or files with multiple chapters)
        buffer << handle->get_subsong_index();

        trie.emplace( qwr::unicode::ToWide( buffer ), i );
    }

    metadbHandleList_.reorder( trie.get_sorted_values().data() );
    isSorted_ = false;
}

JSObject* TrackList::Splice( int32_t start, JS::HandleValue deleteCount, JS::HandleValue tracks )
{
    const auto newHandles = ValueToHandleList( pJsCtx_, tracks );
    const auto iHandleSize = static_cast<int32_t>( metadbHandleList_.size() );
    const auto adjustedStart = [&] {
        if ( start < 0 )
        {
            if ( start < iHandleSize )
            {
                return 0;
            }
            else
            {
                return iHandleSize + start;
            }
        }
        return start;
    }();

    const auto shouldDeleteAll = ( deleteCount == JS::InfinityValue() );
    const auto adjustedDeleteCount = [&] {
        const auto maxDeleteCount = std::max( adjustedStart, iHandleSize );
        if ( shouldDeleteAll )
        {
            return maxDeleteCount;
        }
        else
        {
            return std::min( convert::to_native::ToValue<int32_t>( pJsCtx_, deleteCount ), maxDeleteCount );
        }
    }();

    metadb_handle_list removedHandles;
    const auto isPastEnd = ( adjustedStart >= iHandleSize );
    if ( !isPastEnd && adjustedDeleteCount > 0 )
    {
        for ( auto i: ranges::views::indices( adjustedDeleteCount - adjustedStart ) )
        {
            removedHandles += metadbHandleList_[i];
        }
        metadbHandleList_.remove_from_idx( adjustedStart, adjustedDeleteCount );
    }

    if ( isPastEnd )
    {
        metadbHandleList_ += newHandles;
    }
    else
    {
        metadbHandleList_.insert_items( newHandles, adjustedStart );
    }

    isSorted_ = false;
    return TrackList::CreateJs( pJsCtx_, removedHandles );
}

JSObject* TrackList::SpliceWithOpt( size_t optArgCount, int32_t start, JS::HandleValue deleteCount, JS::HandleValue tracks )
{
    switch ( optArgCount )
    {
    case 0:
        return Splice( start, deleteCount, tracks );
    case 1:
        return Splice( start, deleteCount );
    case 2:
        return Splice( start );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JS::Value TrackList::ToArray() const
{
    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, metadbHandleList_, &jsValue );
    return jsValue;
}

JSObject* TrackList::Union( smp::not_null<TrackList*> tracks ) const
{
    const auto a = qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ );
    const auto b = qwr::pfc_x::Make_Stl_CRef( tracks->GetHandleList() );
    qwr::pfc_x::Stl<metadb_handle_list> result;

    std::set_union( a.cbegin(), a.cend(), b.cbegin(), b.cend(), std::back_inserter( result ) );

    return CreateJs( pJsCtx_, result.Pfc() );
}

JSObject* TrackList::CreateIterator( JS::HandleObject jsSelf ) const
{
    return TrackList_Iterator::CreateJs( pJsCtx_, jsSelf );
}

JSObject* TrackList::From( JSContext* cx, JS::HandleValue tracks )
{
    return TrackList::CreateJs( cx, ValueToHandleList( cx, tracks ) );
}

uint32_t TrackList::get_Length() const
{
    return metadbHandleList_.size();
}

} // namespace mozjs
