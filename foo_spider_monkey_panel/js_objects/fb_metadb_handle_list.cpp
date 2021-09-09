#include <stdafx.h>

#include "fb_metadb_handle_list.h"

#include <fb2k/stats.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list_iterator.h>
#include <js_objects/fb_title_format.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/art_helpers.h>
#include <utils/text_helpers.h>

#include <qwr/abort_callback.h>
#include <qwr/string_helpers.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Conversions.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

#include <utils/relative_filepath_trie.h>

using namespace smp;

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
    JsFbMetadbHandleList::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbMetadbHandleList",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( Add, JsFbMetadbHandleList::Add );
MJS_DEFINE_JS_FN_FROM_NATIVE( AddRange, JsFbMetadbHandleList::AddRange );
MJS_DEFINE_JS_FN_FROM_NATIVE( AttachImage, JsFbMetadbHandleList::AttachImage );
MJS_DEFINE_JS_FN_FROM_NATIVE( BSearch, JsFbMetadbHandleList::BSearch );
MJS_DEFINE_JS_FN_FROM_NATIVE( CalcTotalDuration, JsFbMetadbHandleList::CalcTotalDuration );
MJS_DEFINE_JS_FN_FROM_NATIVE( CalcTotalSize, JsFbMetadbHandleList::CalcTotalSize );
MJS_DEFINE_JS_FN_FROM_NATIVE( Clone, JsFbMetadbHandleList::Clone );
MJS_DEFINE_JS_FN_FROM_NATIVE( Convert, JsFbMetadbHandleList::Convert );
MJS_DEFINE_JS_FN_FROM_NATIVE( RemoveAttachedImage, JsFbMetadbHandleList::RemoveAttachedImage );
MJS_DEFINE_JS_FN_FROM_NATIVE( RemoveAttachedImages, JsFbMetadbHandleList::RemoveAttachedImages );
MJS_DEFINE_JS_FN_FROM_NATIVE( Find, JsFbMetadbHandleList::Find );
MJS_DEFINE_JS_FN_FROM_NATIVE( GetLibraryRelativePaths, JsFbMetadbHandleList::GetLibraryRelativePaths );
MJS_DEFINE_JS_FN_FROM_NATIVE( Insert, JsFbMetadbHandleList::Insert );
MJS_DEFINE_JS_FN_FROM_NATIVE( InsertRange, JsFbMetadbHandleList::InsertRange );
MJS_DEFINE_JS_FN_FROM_NATIVE( MakeDifference, JsFbMetadbHandleList::MakeDifference );
MJS_DEFINE_JS_FN_FROM_NATIVE( MakeIntersection, JsFbMetadbHandleList::MakeIntersection );
MJS_DEFINE_JS_FN_FROM_NATIVE( MakeUnion, JsFbMetadbHandleList::MakeUnion );
MJS_DEFINE_JS_FN_FROM_NATIVE( OrderByFormat, JsFbMetadbHandleList::OrderByFormat );
MJS_DEFINE_JS_FN_FROM_NATIVE( OrderByPath, JsFbMetadbHandleList::OrderByPath );
MJS_DEFINE_JS_FN_FROM_NATIVE( OrderByRelativePath, JsFbMetadbHandleList::OrderByRelativePath );
MJS_DEFINE_JS_FN_FROM_NATIVE( RefreshStats, JsFbMetadbHandleList::RefreshStats );
MJS_DEFINE_JS_FN_FROM_NATIVE( Remove, JsFbMetadbHandleList::Remove );
MJS_DEFINE_JS_FN_FROM_NATIVE( RemoveAll, JsFbMetadbHandleList::RemoveAll );
MJS_DEFINE_JS_FN_FROM_NATIVE( RemoveById, JsFbMetadbHandleList::RemoveById );
MJS_DEFINE_JS_FN_FROM_NATIVE( RemoveRange, JsFbMetadbHandleList::RemoveRange );
MJS_DEFINE_JS_FN_FROM_NATIVE( Sort, JsFbMetadbHandleList::Sort );
MJS_DEFINE_JS_FN_FROM_NATIVE( UpdateFileInfoFromJSON, JsFbMetadbHandleList::UpdateFileInfoFromJSON );

MJS_DEFINE_JS_FN_FROM_NATIVE( CreateIterator, JsFbMetadbHandleList::CreateIterator );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "Add", Add, 1, kDefaultPropsFlags ),
        JS_FN( "AddRange", AddRange, 1, kDefaultPropsFlags ),
        JS_FN( "AttachImage", AttachImage, 2, kDefaultPropsFlags ),
        JS_FN( "BSearch", BSearch, 1, kDefaultPropsFlags ),
        JS_FN( "CalcTotalDuration", CalcTotalDuration, 0, kDefaultPropsFlags ),
        JS_FN( "CalcTotalSize", CalcTotalSize, 0, kDefaultPropsFlags ),
        JS_FN( "Clone", Clone, 0, kDefaultPropsFlags ),
        JS_FN( "Convert", Convert, 0, kDefaultPropsFlags ),
        JS_FN( "Find", Find, 1, kDefaultPropsFlags ),
        JS_FN( "GetLibraryRelativePaths", GetLibraryRelativePaths, 0, kDefaultPropsFlags ),
        JS_FN( "Insert", Insert, 2, kDefaultPropsFlags ),
        JS_FN( "InsertRange", InsertRange, 2, kDefaultPropsFlags ),
        JS_FN( "MakeDifference", MakeDifference, 1, kDefaultPropsFlags ),
        JS_FN( "MakeIntersection", MakeIntersection, 1, kDefaultPropsFlags ),
        JS_FN( "MakeUnion", MakeUnion, 1, kDefaultPropsFlags ),
        JS_FN( "OrderByFormat", OrderByFormat, 2, kDefaultPropsFlags ),
        JS_FN( "OrderByPath", OrderByPath, 0, kDefaultPropsFlags ),
        JS_FN( "OrderByRelativePath", OrderByRelativePath, 0, kDefaultPropsFlags ),
        JS_FN( "RefreshStats", RefreshStats, 0, kDefaultPropsFlags ),
        JS_FN( "Remove", Remove, 1, kDefaultPropsFlags ),
        JS_FN( "RemoveAll", RemoveAll, 0, kDefaultPropsFlags ),
        JS_FN( "RemoveAttachedImage", RemoveAttachedImage, 1, kDefaultPropsFlags ),
        JS_FN( "RemoveAttachedImages", RemoveAttachedImages, 0, kDefaultPropsFlags ),
        JS_FN( "RemoveById", RemoveById, 1, kDefaultPropsFlags ),
        JS_FN( "RemoveRange", RemoveRange, 2, kDefaultPropsFlags ),
        JS_FN( "Sort", Sort, 0, kDefaultPropsFlags ),
        JS_FN( "UpdateFileInfoFromJSON", UpdateFileInfoFromJSON, 1, kDefaultPropsFlags ),
        JS_SYM_FN( iterator, CreateIterator, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Count, JsFbMetadbHandleList::get_Count );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "Count", get_Count, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( FbMetadbHandleList_Constructor, JsFbMetadbHandleList::Constructor, JsFbMetadbHandleList::ConstructorWithOpt, 1 )

} // namespace

namespace
{

// Wrapper to intercept indexed gets/sets.
class FbMetadbHandleListProxyHandler : public js::ForwardingProxyHandler
{
public:
    static const FbMetadbHandleListProxyHandler singleton;

    FbMetadbHandleListProxyHandler()
        : js::ForwardingProxyHandler( GetSmpProxyFamily() )
    {
    }

    bool get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
              JS::HandleId id, JS::MutableHandleValue vp ) const override;
    bool set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
              JS::HandleValue receiver, JS::ObjectOpResult& result ) const override;
};

const FbMetadbHandleListProxyHandler FbMetadbHandleListProxyHandler::singleton;

bool FbMetadbHandleListProxyHandler::get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
                                          JS::HandleId id, JS::MutableHandleValue vp ) const
{
    if ( JSID_IS_INT( id ) )
    {
        JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
        auto pNativeTarget = static_cast<JsFbMetadbHandleList*>( JS_GetPrivate( target ) );
        assert( pNativeTarget );

        const auto index = static_cast<uint32_t>( JSID_TO_INT( id ) );
        try
        {
            vp.setObjectOrNull( pNativeTarget->get_Item( index ) );
        }
        catch ( ... )
        {
            mozjs::error::ExceptionToJsError( cx );
            return false;
        }

        return true;
    }

    return js::ForwardingProxyHandler::get( cx, proxy, receiver, id, vp );
}

bool FbMetadbHandleListProxyHandler::set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
                                          JS::HandleValue receiver, JS::ObjectOpResult& result ) const
{
    if ( JSID_IS_INT( id ) )
    {
        JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
        auto pNativeTarget = static_cast<JsFbMetadbHandleList*>( JS_GetPrivate( target ) );
        assert( pNativeTarget );

        const auto index = static_cast<uint32_t>( JSID_TO_INT( id ) );

        if ( !v.isObjectOrNull() )
        {
            JS_ReportErrorUTF8( cx, "Value in assignment is of wrong type" );
            return false;
        }

        JS::RootedObject jsObject( cx, v.toObjectOrNull() );
        JsFbMetadbHandle* pNativeValue =
            jsObject
                ? static_cast<JsFbMetadbHandle*>( JS_GetInstancePrivate( cx, jsObject, &JsFbMetadbHandle::JsClass, nullptr ) )
                : nullptr;

        try
        {
            pNativeTarget->put_Item( index, pNativeValue );
        }
        catch ( ... )
        {
            mozjs::error::ExceptionToJsError( cx );
            return false;
        }

        result.succeed();
        return true;
    }

    return js::ForwardingProxyHandler::set( cx, proxy, id, v, receiver, result );
}

} // namespace

namespace mozjs
{

const JSClass JsFbMetadbHandleList::JsClass = jsClass;
const JSFunctionSpec* JsFbMetadbHandleList::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbMetadbHandleList::JsProperties = jsProperties.data();
const JsPrototypeId JsFbMetadbHandleList::PrototypeId = JsPrototypeId::FbMetadbHandleList;
const JSNative JsFbMetadbHandleList::JsConstructor = ::FbMetadbHandleList_Constructor;
const js::BaseProxyHandler& JsFbMetadbHandleList::JsProxy = FbMetadbHandleListProxyHandler::singleton;

JsFbMetadbHandleList::JsFbMetadbHandleList( JSContext* cx, const metadb_handle_list& handles )
    : pJsCtx_( cx )
    , metadbHandleList_( handles )
{
}

std::unique_ptr<JsFbMetadbHandleList>
JsFbMetadbHandleList::CreateNative( JSContext* cx, const metadb_handle_list& handles )
{
    return std::unique_ptr<JsFbMetadbHandleList>( new JsFbMetadbHandleList( cx, handles ) );
}

size_t JsFbMetadbHandleList::GetInternalSize( const metadb_handle_list& handles )
{
    return sizeof( metadb_handle ) * handles.get_size();
}

const metadb_handle_list& JsFbMetadbHandleList::GetHandleList() const
{
    return metadbHandleList_;
}

JSObject* JsFbMetadbHandleList::Constructor( JSContext* cx, JS::HandleValue jsValue )
{
    if ( jsValue.isNullOrUndefined() )
    {
        return JsFbMetadbHandleList::CreateJs( cx, metadb_handle_list() );
    }

    if ( auto pNativeHandle = GetInnerInstancePrivate<JsFbMetadbHandle>( cx, jsValue );
         pNativeHandle )
    {
        metadb_handle_list handleList;
        handleList.add_item( pNativeHandle->GetHandle() );
        return JsFbMetadbHandleList::CreateJs( cx, handleList );
    }

    if ( auto pNativeHandleList = GetInnerInstancePrivate<JsFbMetadbHandleList>( cx, jsValue );
         pNativeHandleList )
    {
        return JsFbMetadbHandleList::CreateJs( cx, pNativeHandleList->GetHandleList() );
    }

    {
        bool is;
        if ( !JS_IsArrayObject( cx, jsValue, &is ) )
        {
            throw JsException();
        }
        if ( is )
        {
            metadb_handle_list handleList;
            convert::to_native::ProcessArray<JsFbMetadbHandle*>( cx, jsValue, [&handleList]( auto pNativeHandle ) {
                qwr::QwrException::ExpectTrue( pNativeHandle, "Array contains invalid value" );
                handleList.add_item( pNativeHandle->GetHandle() );
            } );
            return JsFbMetadbHandleList::CreateJs( cx, handleList );
        }
    }

    throw qwr::QwrException( "Unsupported argument type" );
}

JSObject* JsFbMetadbHandleList::ConstructorWithOpt( JSContext* cx, size_t optArgCount, JS::HandleValue jsValue )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor( cx, jsValue );
    case 1:
        return Constructor( cx );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void JsFbMetadbHandleList::Add( JsFbMetadbHandle* handle )
{
    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    qwr::QwrException::ExpectTrue( fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle" );

    metadbHandleList_.add_item( fbHandle );
}

void JsFbMetadbHandleList::AddRange( JsFbMetadbHandleList* handles )
{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );

    metadbHandleList_.add_items( handles->GetHandleList() );
}

void JsFbMetadbHandleList::AttachImage( const qwr::u8string& image_path, uint32_t art_id )
{
    t_size count = metadbHandleList_.get_count();
    if ( !count )
    { // Nothing to do here
        return;
    }

    const GUID& what = art::GetGuidForArtId( art_id );
    auto& abort = qwr::GlobalAbortCallback::GetInstance();
    album_art_data_ptr data;

    try
    {
        pfc::string8_fast canPath;
        filesystem::g_get_canonical_path( image_path.c_str(), canPath );

        file::ptr file;

        if ( !filesystem::g_is_remote_or_unrecognized( canPath ) )
        {
            filesystem::g_open( file, canPath, filesystem::open_mode_read, abort );
        }
        if ( file.is_valid() )
        {
            auto tmp = fb2k::service_new<album_art_data_impl>();
            tmp->from_stream( file.get_ptr(), t_size( file->get_size_ex( abort ) ), abort );
            data = tmp;
        }
    }
    catch ( const pfc::exception& )
    {
        return;
    }

    if ( data.is_valid() )
    {
        auto cb = fb2k::service_new<art::EmbedThread>( art::EmbedThread::EmbedAction::embed, data, metadbHandleList_, what );
        (void)threaded_process::get()->run_modeless( cb,
                                                     threaded_process::flag_show_progress | threaded_process::flag_show_delayed | threaded_process::flag_show_item,
                                                     core_api::get_main_window(),
                                                     "Embedding images..." );
    }
}

int32_t JsFbMetadbHandleList::BSearch( JsFbMetadbHandle* handle )
{
    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    qwr::QwrException::ExpectTrue( fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle" );

    return static_cast<int32_t>( metadbHandleList_.bsearch_by_pointer( fbHandle ) );
}

double JsFbMetadbHandleList::CalcTotalDuration()
{
    return metadbHandleList_.calc_total_duration();
}

std::uint64_t JsFbMetadbHandleList::CalcTotalSize()
{
    return static_cast<uint64_t>( metadb_handle_list_helper::calc_total_size( metadbHandleList_, true ) );
}

JSObject* JsFbMetadbHandleList::Clone()
{
    return JsFbMetadbHandleList::CreateJs( pJsCtx_, metadbHandleList_ );
}

JS::Value JsFbMetadbHandleList::Convert()
{
    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue( pJsCtx_, qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ ), &jsValue );
    return jsValue;
}

int32_t JsFbMetadbHandleList::Find( JsFbMetadbHandle* handle )
{
    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    qwr::QwrException::ExpectTrue( fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle" );

    return static_cast<int32_t>( metadbHandleList_.find_item( fbHandle ) );
}

JS::Value JsFbMetadbHandleList::GetLibraryRelativePaths()
{
    auto api = library_manager::get();

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue(
        pJsCtx_,
        qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ ),
        [&api]( const auto& vec, auto index ) {
            pfc::string8_fast path;
            api->get_relative_path( vec[index], path );
            return path;
        },
        &jsValue );

    return jsValue;
}

void JsFbMetadbHandleList::Insert( uint32_t index, JsFbMetadbHandle* handle )
{
    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    qwr::QwrException::ExpectTrue( fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle" );

    metadbHandleList_.insert_item( fbHandle, index );
}

void JsFbMetadbHandleList::InsertRange( uint32_t index, JsFbMetadbHandleList* handles )
{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );

    metadbHandleList_.insert_items( handles->GetHandleList(), index );
}

void JsFbMetadbHandleList::MakeDifference( JsFbMetadbHandleList* handles )
{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );

    const auto a = qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ );
    const auto b = qwr::pfc_x::Make_Stl_CRef( handles->GetHandleList() );
    qwr::pfc_x::Stl<metadb_handle_list> result;

    std::set_difference( a.cbegin(), a.cend(), b.cbegin(), b.cend(), std::back_inserter( result ) );

    metadbHandleList_ = result.Pfc();
}

void JsFbMetadbHandleList::MakeIntersection( JsFbMetadbHandleList* handles )
{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );

    const auto a = qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ );
    const auto b = qwr::pfc_x::Make_Stl_CRef( handles->GetHandleList() );
    qwr::pfc_x::Stl<metadb_handle_list> result;

    std::set_intersection( a.cbegin(), a.cend(), b.cbegin(), b.cend(), std::back_inserter( result ) );

    metadbHandleList_ = result.Pfc();
}

void JsFbMetadbHandleList::MakeUnion( JsFbMetadbHandleList* handles )
{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );

    const auto a = qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ );
    const auto b = qwr::pfc_x::Make_Stl_CRef( handles->GetHandleList() );
    qwr::pfc_x::Stl<metadb_handle_list> result;

    std::set_union( a.cbegin(), a.cend(), b.cbegin(), b.cend(), std::back_inserter( result ) );

    metadbHandleList_ = result.Pfc();
}

void JsFbMetadbHandleList::OrderByFormat( JsFbTitleFormat* script, int8_t direction )
{
    qwr::QwrException::ExpectTrue( script, "script argument is null" );

    metadbHandleList_.sort_by_format( script->GetTitleFormat(), nullptr, direction );
}

void JsFbMetadbHandleList::OrderByPath()
{
    metadbHandleList_.sort_by_path();
}

void JsFbMetadbHandleList::OrderByRelativePath()
{
    // Note: there is built-in metadb_handle_list::sort_by_relative_path(),
    // but this implementation is much faster because of trie usage.
    // Also see `get_subsong_index` below.

    const auto stlHandleList = qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ );

    auto api = library_manager::get();

    pfc::string8_fastalloc temp;
    temp.prealloc( 512 );

    RelativeFilepathTrie<size_t> trie;
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

    metadbHandleList_.reorder( trie.get_sorted_values().data() );
}

void JsFbMetadbHandleList::RefreshStats()
{
    pfc::list_t<metadb_index_hash> hashes;
    for ( const auto& handle: qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ ) )
    {
        metadb_index_hash hash;
        if ( stats::HashHandle( handle, hash ) )
        {
            hashes.add_item( hash );
        }
    }

    stats::RefreshStats( hashes );
}

void JsFbMetadbHandleList::Remove( JsFbMetadbHandle* handle )
{
    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    qwr::QwrException::ExpectTrue( fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle" );

    metadbHandleList_.remove_item( fbHandle );
}

void JsFbMetadbHandleList::RemoveAll()
{
    metadbHandleList_.remove_all();
}

void JsFbMetadbHandleList::RemoveAttachedImage( uint32_t art_id )
{
    t_size count = metadbHandleList_.get_count();
    if ( !count )
    { // Nothing to do here
        return;
    }

    const GUID& what = art::GetGuidForArtId( art_id );

    auto cb = fb2k::service_new<art::EmbedThread>( art::EmbedThread::EmbedAction::remove, album_art_data_ptr(), metadbHandleList_, what );
    (void)threaded_process::get()->run_modeless( cb,
                                                 threaded_process::flag_show_progress | threaded_process::flag_show_delayed | threaded_process::flag_show_item,
                                                 core_api::get_main_window(),
                                                 "Removing images..." );
}

void JsFbMetadbHandleList::RemoveAttachedImages()
{
    t_size count = metadbHandleList_.get_count();
    if ( !count )
    { // Nothing to do here
        return;
    }

    auto cb = fb2k::service_new<art::EmbedThread>( art::EmbedThread::EmbedAction::removeAll, album_art_data_ptr(), metadbHandleList_, pfc::guid_null );
    (void)threaded_process::get()->run_modeless( cb,
                                                 threaded_process::flag_show_progress | threaded_process::flag_show_delayed | threaded_process::flag_show_item,
                                                 core_api::get_main_window(),
                                                 "Removing images..." );
}

void JsFbMetadbHandleList::RemoveById( uint32_t index )
{
    qwr::QwrException::ExpectTrue( index < metadbHandleList_.get_count(), "Index is out of bounds" );
    (void)metadbHandleList_.remove_by_idx( index );
}

void JsFbMetadbHandleList::RemoveRange( uint32_t from, uint32_t count )
{
    metadbHandleList_.remove_from_idx( from, count );
}

void JsFbMetadbHandleList::Sort()
{
    metadbHandleList_.sort_by_pointer_remove_duplicates();
}

void JsFbMetadbHandleList::UpdateFileInfoFromJSON( const qwr::u8string& str )
{
    using json = nlohmann::json;

    const auto handleList = qwr::pfc_x::Make_Stl_CRef( metadbHandleList_ );
    if ( handleList.empty() )
    { // Not an error
        return;
    }

    const auto jsonObject = [&str] {
        try
        {
            return json::parse( str );
        }
        catch ( const json::parse_error& e )
        {
            throw qwr::QwrException( "JSON parsing failed: {}", e.what() );
        }
    }();

    qwr::QwrException::ExpectTrue( jsonObject.is_array() || jsonObject.is_object(), "Invalid JSON info: unsupported value type" );

    const bool isArray = jsonObject.is_array();
    if ( isArray && jsonObject.size() != handleList.size() )
    {
        throw qwr::QwrException( "Invalid JSON info: mismatched with handle count" );
    }
    if ( !isArray && jsonObject.empty() )
    {
        throw qwr::QwrException( "Invalid JSON info: empty object" );
    }

    const auto info =
        ranges::views::enumerate( handleList )
        | ranges::views::transform(
            [isArray, &jsonObject]( const auto& zippedElem ) {
                const auto& [i, handle] = zippedElem;

                // TODO: think of a better way of handling unavalaible info,
                //       currently it uses dummy value instead
                file_info_impl fileInfo = handle->get_info_ref()->info();
                ModifyFileInfoWithJson( isArray ? jsonObject[i] : jsonObject, fileInfo );
                return fileInfo;
            } )
        | ranges::to_vector;

    metadb_io_v2::get()->update_info_async_simple(
        handleList.Pfc(),
        pfc::ptr_list_const_array_t<const file_info, const file_info_impl*>( info.data(), info.size() ),
        core_api::get_main_window(),
        metadb_io_v2::op_flag_delay_ui,
        nullptr );
}

JSObject* JsFbMetadbHandleList::CreateIterator()
{
    return JsFbMetadbHandleList_Iterator::CreateJs( pJsCtx_, *this );
}

uint32_t JsFbMetadbHandleList::get_Count()
{
    return metadbHandleList_.get_count();
}

JSObject* JsFbMetadbHandleList::get_Item( uint32_t index )
{
    qwr::QwrException::ExpectTrue( index < metadbHandleList_.get_count(), "Index is out of bounds" );

    return JsFbMetadbHandle::CreateJs( pJsCtx_, metadbHandleList_[index] );
}

void JsFbMetadbHandleList::put_Item( uint32_t index, JsFbMetadbHandle* handle )
{
    qwr::QwrException::ExpectTrue( index < metadbHandleList_.get_count(), "Index is out of bounds" );
    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    qwr::QwrException::ExpectTrue( fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle" );

    metadbHandleList_.replace_item( index, fbHandle );
}

void JsFbMetadbHandleList::ModifyFileInfoWithJson( const nlohmann::json& jsonObject, file_info_impl& fileInfo )
{
    using json = nlohmann::json;

    auto jsonToString = []( const json& value ) {
        return ( value.type() == json::value_t::string
                     ? value.get<std::string>()
                     : value.dump( 2 ) );
    };

    const json& obj = jsonObject;
    qwr::QwrException::ExpectTrue( obj.is_object() && !obj.empty(), "Invalid JSON info: unsupported value" );

    for ( const auto& [key, value]: obj.items() )
    {
        qwr::QwrException::ExpectTrue( !key.empty(), "Invalid JSON info: key is empty" );

        fileInfo.meta_remove_field( key.c_str() );

        if ( value.is_array() )
        {
            for ( const auto& arrValue: value )
            {
                if ( const std::string strValue = jsonToString( arrValue );
                     !strValue.empty() )
                {
                    fileInfo.meta_add( key.c_str(), strValue.c_str() );
                }
            }
        }
        else
        {
            if ( const std::string strValue = jsonToString( value );
                 !strValue.empty() )
            {
                fileInfo.meta_set( key.c_str(), strValue.c_str() );
            }
        }
    }
}

} // namespace mozjs
