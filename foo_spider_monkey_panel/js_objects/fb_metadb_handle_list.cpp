#include <stdafx.h>

#include "fb_metadb_handle_list.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_title_format.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/art_helper.h>
#include <utils/string_helpers.h>

#include <helpers.h>
#include <stats.h>

#pragma warning( push )
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <js/Conversions.h>
#pragma warning( pop )

// TODO: add constructor

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
    DefaultClassFlags(),
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

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Add", Add, 1, DefaultPropsFlags() ),
    JS_FN( "AddRange", AddRange, 1, DefaultPropsFlags() ),
    JS_FN( "AttachImage", AttachImage, 2, DefaultPropsFlags() ),
    JS_FN( "BSearch", BSearch, 1, DefaultPropsFlags() ),
    JS_FN( "CalcTotalDuration", CalcTotalDuration, 0, DefaultPropsFlags() ),
    JS_FN( "CalcTotalSize", CalcTotalSize, 0, DefaultPropsFlags() ),
    JS_FN( "Clone", Clone, 0, DefaultPropsFlags() ),
    JS_FN( "Convert", Convert, 0, DefaultPropsFlags() ),
    JS_FN( "Find", Find, 1, DefaultPropsFlags() ),
    JS_FN( "GetLibraryRelativePaths", GetLibraryRelativePaths, 0, DefaultPropsFlags() ),
    JS_FN( "Insert", Insert, 2, DefaultPropsFlags() ),
    JS_FN( "InsertRange", InsertRange, 2, DefaultPropsFlags() ),
    JS_FN( "MakeDifference", MakeDifference, 1, DefaultPropsFlags() ),
    JS_FN( "MakeIntersection", MakeIntersection, 1, DefaultPropsFlags() ),
    JS_FN( "MakeUnion", MakeUnion, 1, DefaultPropsFlags() ),
    JS_FN( "OrderByFormat", OrderByFormat, 2, DefaultPropsFlags() ),
    JS_FN( "OrderByPath", OrderByPath, 0, DefaultPropsFlags() ),
    JS_FN( "OrderByRelativePath", OrderByRelativePath, 0, DefaultPropsFlags() ),
    JS_FN( "RefreshStats", RefreshStats, 0, DefaultPropsFlags() ),
    JS_FN( "Remove", Remove, 1, DefaultPropsFlags() ),
    JS_FN( "RemoveAll", RemoveAll, 0, DefaultPropsFlags() ),
    JS_FN( "RemoveAttachedImage", RemoveAttachedImage, 1, DefaultPropsFlags() ),
    JS_FN( "RemoveById", RemoveById, 1, DefaultPropsFlags() ),
    JS_FN( "RemoveRange", RemoveRange, 2, DefaultPropsFlags() ),
    JS_FN( "Sort", Sort, 0, DefaultPropsFlags() ),
    JS_FN( "UpdateFileInfoFromJSON", UpdateFileInfoFromJSON, 1, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Count, JsFbMetadbHandleList::get_Count );

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Count", get_Count, DefaultPropsFlags() ),
    JS_PS_END
};

} // namespace

namespace
{

// Wrapper to intercept indexed gets/sets.
class FbMetadbHandleListProxyHandler : public js::ForwardingProxyHandler
{
public:
    static const FbMetadbHandleListProxyHandler singleton;
    // family must contain unique pointer, so the value does not really matter
    static const char family;

    constexpr FbMetadbHandleListProxyHandler()
        : js::ForwardingProxyHandler( &family )
    {
    }

    bool get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
              JS::HandleId id, JS::MutableHandleValue vp ) const override;
    bool set( JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v,
              JS::HandleValue receiver, JS::ObjectOpResult& result ) const override;
};

const FbMetadbHandleListProxyHandler FbMetadbHandleListProxyHandler::singleton;
const char FbMetadbHandleListProxyHandler::family = 'Q';

bool FbMetadbHandleListProxyHandler::get( JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver,
                                          JS::HandleId id, JS::MutableHandleValue vp ) const
{
    if ( JSID_IS_INT( id ) )
    {
        JS::RootedObject target( cx, js::GetProxyTargetObject( proxy ) );
        auto pNativeTarget = static_cast<JsFbMetadbHandleList*>( JS_GetPrivate( target ) );
        assert( pNativeTarget );

        uint32_t index = static_cast<uint32_t>( JSID_TO_INT( id ) );
        try
        {
            vp.setObjectOrNull( pNativeTarget->get_Item( index ) );
        }
        catch ( ... )
        {
            error::ExceptionToJsError( cx );
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

        uint32_t index = static_cast<uint32_t>( JSID_TO_INT( id ) );

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
            error::ExceptionToJsError( cx );
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
const JSFunctionSpec* JsFbMetadbHandleList::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbMetadbHandleList::JsProperties = jsProperties;
const JsPrototypeId JsFbMetadbHandleList::PrototypeId = JsPrototypeId::FbMetadbHandleList;
const js::BaseProxyHandler& JsFbMetadbHandleList::JsProxy = FbMetadbHandleListProxyHandler::singleton;

JsFbMetadbHandleList::JsFbMetadbHandleList( JSContext* cx, metadb_handle_list_cref handles )
    : pJsCtx_( cx )
    , metadbHandleList_( handles )
{
}

JsFbMetadbHandleList::~JsFbMetadbHandleList()
{
}

std::unique_ptr<JsFbMetadbHandleList>
JsFbMetadbHandleList::CreateNative( JSContext* cx, metadb_handle_list_cref handles )
{
    return std::unique_ptr<JsFbMetadbHandleList>( new JsFbMetadbHandleList( cx, handles ) );
}

size_t JsFbMetadbHandleList::GetInternalSize( metadb_handle_list_cref handles )
{
    return sizeof( metadb_handle ) * handles.get_size();
}

metadb_handle_list_cref JsFbMetadbHandleList::GetHandleList() const
{
    return metadbHandleList_;
}

void JsFbMetadbHandleList::Add( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        throw smp::SmpException( "handle argument is null" );
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        throw smp::SmpException( "Internal error: FbMetadbHandle does not contain a valid handle" );
    }

    metadbHandleList_.add_item( fbHandle );
}

void JsFbMetadbHandleList::AddRange( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        throw smp::SmpException( "handles argument is null" );
    }

    metadbHandleList_.add_items( handles->GetHandleList() );
}

void JsFbMetadbHandleList::AttachImage( const pfc::string8_fast& image_path, uint32_t art_id )
{
    t_size count = metadbHandleList_.get_count();
    if ( !count )
    { // Nothing to do here
        return;
    }

    GUID what = art::GetGuidForArtId( art_id );
    abort_callback_dummy abort;
    album_art_data_ptr data;

    try
    {
        file::ptr file;
        pfc::string8_fast canPath;
        filesystem::g_get_canonical_path( image_path, canPath );
        if ( !filesystem::g_is_remote_or_unrecognized( canPath ) )
        {
            filesystem::g_open( file, canPath, filesystem::open_mode_read, abort );
        }
        if ( file.is_valid() )
        {
            service_ptr_t<album_art_data_impl> tmp = new service_impl_t<album_art_data_impl>;
            tmp->from_stream( file.get_ptr(), t_size( file->get_size_ex( abort ) ), abort );
            data = tmp;
        }
    }
    catch ( ... )
    {
        return;
    }

    if ( data.is_valid() )
    {
        threaded_process_callback::ptr cb( new service_impl_t<art::embed_thread>( 0, data, metadbHandleList_, what ) );
        threaded_process::get()->run_modeless( cb,
                                               threaded_process::flag_show_progress | threaded_process::flag_show_delayed | threaded_process::flag_show_item,
                                               core_api::get_main_window(),
                                               "Embedding images..." );
    }
}

int32_t JsFbMetadbHandleList::BSearch( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        throw smp::SmpException( "handle argument is null" );
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        throw smp::SmpException( "Internal error: FbMetadbHandle does not contain a valid handle" );
    }

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
    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::CreateJs( pJsCtx_, metadbHandleList_ ) );
    if ( !jsObject )
    { // TODO: Remove
        throw smp::JsException();
    }

    return jsObject;
}

JSObject* JsFbMetadbHandleList::Convert()
{
    t_size count = metadbHandleList_.get_count();

    JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, count ) );
    if ( !jsArray )
    {
        throw smp::JsException();
    }

    JS::RootedValue jsValue( pJsCtx_ );
    JS::RootedObject jsObject( pJsCtx_ );
    for ( t_size i = 0; i < count; ++i )
    {
        jsObject = JsFbMetadbHandle::CreateJs( pJsCtx_, metadbHandleList_.get_item_ref( i ) );
        if ( !jsObject )
        { // TODO: Remove
            throw smp::JsException();
        }

        jsValue.set( JS::ObjectValue( *jsObject ) );
        if ( !JS_SetElement( pJsCtx_, jsArray, i, jsValue ) )
        {
            throw smp::JsException();
        }
    }

    return jsArray;
}

int32_t JsFbMetadbHandleList::Find( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        throw smp::SmpException( "handle argument is null" );
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        throw smp::SmpException( "Internal error: FbMetadbHandle does not contain a valid handle" );
    }

    return static_cast<int32_t>( metadbHandleList_.find_item( fbHandle ) );
}

JSObject* JsFbMetadbHandleList::GetLibraryRelativePaths()
{
    auto api = library_manager::get();
    t_size count = metadbHandleList_.get_count();

    pfc::string8_fastalloc path;
    path.prealloc( 512 );
    metadb_handle_ptr item;

    JS::RootedObject jsArray( pJsCtx_, JS_NewArrayObject( pJsCtx_, count ) );
    if ( !jsArray )
    {
        throw smp::JsException();
    }

    JS::RootedValue jsValue( pJsCtx_ );
    for ( t_size i = 0; i < count; ++i )
    {
        item = metadbHandleList_.get_item_ref( i );
        if ( !api->get_relative_path( item, path ) )
        {
            path = "";
        }

        pfc::string8_fast tmpString( path.c_str(), path.length() );
        if ( !convert::to_js::ToValue( pJsCtx_, tmpString, &jsValue ) )
        {
            throw smp::JsException();
        }

        if ( !JS_SetElement( pJsCtx_, jsArray, i, jsValue ) )
        { // Report in JS_SetElement
            throw smp::JsException();
        }
    }

    return jsArray;
}

void JsFbMetadbHandleList::Insert( uint32_t index, JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        throw smp::SmpException( "handle argument is null" );
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        throw smp::SmpException( "Internal error: FbMetadbHandle does not contain valid handle" );
    }

    metadbHandleList_.insert_item( fbHandle, index );
}

void JsFbMetadbHandleList::InsertRange( uint32_t index, JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        throw smp::SmpException( "handles argument is null" );
    }

    metadbHandleList_.insert_items( handles->GetHandleList(), index );
}

void JsFbMetadbHandleList::MakeDifference( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        throw smp::SmpException( "handles argument is null" );
    }

    metadb_handle_list_cref fbHandles = handles->GetHandleList();
    metadb_handle_list result;
    t_size walk1 = 0;
    t_size walk2 = 0;
    t_size last1 = metadbHandleList_.get_count();
    t_size last2 = fbHandles.get_count();

    while ( walk1 != last1 && walk2 != last2 )
    {
        if ( metadbHandleList_[walk1] < fbHandles[walk2] )
        {
            result.add_item( metadbHandleList_[walk1] );
            ++walk1;
        }
        else if ( fbHandles[walk2] < metadbHandleList_[walk1] )
        {
            ++walk2;
        }
        else
        {
            ++walk1;
            ++walk2;
        }
    }

    metadbHandleList_ = result;
}

void JsFbMetadbHandleList::MakeIntersection( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        throw smp::SmpException( "handles argument is null" );
    }

    metadb_handle_list_cref fbHandles = handles->GetHandleList();
    metadb_handle_list result;
    t_size walk1 = 0;
    t_size walk2 = 0;
    t_size last1 = metadbHandleList_.get_count();
    t_size last2 = fbHandles.get_count();

    while ( walk1 != last1 && walk2 != last2 )
    {
        if ( metadbHandleList_[walk1] < fbHandles[walk2] )
        {
            ++walk1;
        }
        else if ( fbHandles[walk2] < metadbHandleList_[walk1] )
        {
            ++walk2;
        }
        else
        {
            result.add_item( metadbHandleList_[walk1] );
            ++walk1;
            ++walk2;
        }
    }

    metadbHandleList_ = result;
}

void JsFbMetadbHandleList::MakeUnion( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        throw smp::SmpException( "handles argument is null" );
    }

    metadbHandleList_.add_items( handles->GetHandleList() );
    metadbHandleList_.sort_by_pointer_remove_duplicates();
}

void JsFbMetadbHandleList::OrderByFormat( JsFbTitleFormat* script, int8_t direction )
{
    if ( !script )
    {
        JS_ReportErrorUTF8( pJsCtx_, "script argument is null" );
    }

    titleformat_object::ptr titleFormat = script->GetTitleFormat();
    metadbHandleList_.sort_by_format( titleFormat, nullptr, direction );
}

void JsFbMetadbHandleList::OrderByPath()
{
    metadbHandleList_.sort_by_path();
}

void JsFbMetadbHandleList::OrderByRelativePath()
{
    // TODO: investigate this code

    // lifted from metadb_handle_list.cpp - adds subsong index for better sorting. github issue #16
    auto api = library_manager::get();
    t_size i, count = metadbHandleList_.get_count();

    pfc::array_t<helpers::custom_sort_data> data;
    data.set_size( count );

    pfc::string8_fastalloc temp;
    temp.prealloc( 512 );

    for ( i = 0; i < count; ++i )
    {
        metadb_handle_ptr item;
        metadbHandleList_.get_item_ex( item, i );
        if ( !api->get_relative_path( item, temp ) )
        {
            temp = "";
        }
        temp << item->get_subsong_index();
        data[i].index = i;
        data[i].text = helpers::make_sort_string( temp );
    }

    pfc::sort_t( data, helpers::custom_sort_compare<1>, count );
    order_helper order( count );

    for ( i = 0; i < count; ++i )
    {
        order[i] = data[i].index;
    }

    metadbHandleList_.reorder( order.get_ptr() );
}

void JsFbMetadbHandleList::RefreshStats()
{
    const t_size count = metadbHandleList_.get_count();
    pfc::list_t<metadb_index_hash> hashes;
    for ( t_size i = 0; i < count; ++i )
    {
        metadb_index_hash hash;
        if ( stats::hashHandle( metadbHandleList_[i], hash ) )
        {
            hashes.add_item( hash );
        }
    }

    stats::refresh( hashes );
}

void JsFbMetadbHandleList::Remove( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        throw smp::SmpException( "handle argument is null" );
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        throw smp::SmpException( "Internal error: FbMetadbHandle does not contain a valid handle" );
    }

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

    GUID what = art::GetGuidForArtId( art_id );

    threaded_process_callback::ptr cb = new service_impl_t<art::embed_thread>( 1, album_art_data_ptr(), metadbHandleList_, what );
    threaded_process::get()->run_modeless( cb,
                                           threaded_process::flag_show_progress | threaded_process::flag_show_delayed | threaded_process::flag_show_item,
                                           core_api::get_main_window(),
                                           "Removing images..." );
}

void JsFbMetadbHandleList::RemoveById( uint32_t index )
{
    if ( index >= metadbHandleList_.get_count() )
    {
        throw smp::SmpException( "Index is out of bounds" );
    }
    metadbHandleList_.remove_by_idx( index );
}

void JsFbMetadbHandleList::RemoveRange( uint32_t from, uint32_t count )
{
    metadbHandleList_.remove_from_idx( from, count );
}

void JsFbMetadbHandleList::Sort()
{
    metadbHandleList_.sort_by_pointer_remove_duplicates();
}

void JsFbMetadbHandleList::UpdateFileInfoFromJSON( const pfc::string8_fast& str )
{
    using json = nlohmann::json;

    // TODO: investigate and cleanup
    uint32_t count = metadbHandleList_.get_count();
    if ( !count )
    { // Not an error
        return;
    }

    json jsonObject;
    bool isArray;

    try
    {
        jsonObject = json::parse( str.c_str() );
    }
    catch ( const json::parse_error& e )
    {
        throw smp::SmpException( smp::string::Formatter() << "JSON parsing failed: " << e.what() );
    }

    if ( jsonObject.is_array() )
    {
        if ( jsonObject.size() != count )
        {
            throw smp::SmpException( "Invalid JSON info: mismatched with handle count" );
        }
        isArray = true;
    }
    else if ( jsonObject.is_object() )
    {
        if ( !jsonObject.size() )
        {
            throw smp::SmpException( "Invalid JSON info: empty object" );
        }
        isArray = false;
    }
    else
    {
        throw smp::SmpException( "Invalid JSON info: unsupported value type" );
    }

    pfc::list_t<file_info_impl> info;
    info.set_size( count );

    auto iterator_to_string8 = []( json::iterator j ) {
        std::string value = j.value().type() == json::value_t::string
                                ? j.value().get<std::string>()
                                : j.value().dump();
        return pfc::string8_fast( value.c_str(), value.length() );
    };

    for ( t_size i = 0; i < count; ++i )
    {
        json obj = isArray ? jsonObject[i] : jsonObject;
        if ( !obj.is_object() || !obj.size() )
        {
            throw smp::SmpException( "Invalid JSON info: unsupported value" );
        }

        metadb_handle_ptr item = metadbHandleList_.get_item( i );
        item->get_info( info[i] );

        for ( json::iterator it = obj.begin(); it != obj.end(); ++it )
        {
            pfc::string8_fast key = it.key().c_str();
            if ( key.is_empty() )
            {
                throw smp::SmpException( "Invalid JSON info: key is empty" );
            }

            info[i].meta_remove_field( key );

            if ( it.value().is_array() )
            {
                for ( json::iterator ita = it.value().begin(); ita != it.value().end(); ++ita )
                {
                    pfc::string8 value = iterator_to_string8( ita );
                    if ( !value.is_empty() )
                    {
                        info[i].meta_add( key, value );
                    }
                }
            }
            else
            {
                pfc::string8 value = iterator_to_string8( it );
                if ( !value.is_empty() )
                {
                    info[i].meta_set( key, value );
                }
            }
        }
    }

    metadb_io_v2::get()->update_info_async_simple(
        metadbHandleList_,
        pfc::ptr_list_const_array_t<const file_info, file_info_impl*>( info.get_ptr(), info.get_count() ),
        core_api::get_main_window(),
        metadb_io_v2::op_flag_delay_ui,
        nullptr );
}

uint32_t JsFbMetadbHandleList::get_Count()
{
    return metadbHandleList_.get_count();
}

JSObject* JsFbMetadbHandleList::get_Item( uint32_t index )
{
    if ( index >= metadbHandleList_.get_count() )
    {
        throw smp::SmpException( "Index is out of bounds" );
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::CreateJs( pJsCtx_, metadbHandleList_.get_item_ref( index ) ) );
    if ( !jsObject )
    { // TODO: remove
        throw smp::JsException();
    }

    return jsObject;
}

void JsFbMetadbHandleList::put_Item( uint32_t index, JsFbMetadbHandle* handle )
{
    if ( index >= metadbHandleList_.get_count() )
    {
        throw smp::SmpException( "Index is out of bounds" );
    }

    if ( !handle )
    {
        throw smp::SmpException( "handle argument is null" );
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        throw smp::SmpException( "Internal error: FbMetadbHandle does not contain a valid handle" );
    }

    metadbHandleList_.replace_item( index, fbHandle );
}

} // namespace mozjs
