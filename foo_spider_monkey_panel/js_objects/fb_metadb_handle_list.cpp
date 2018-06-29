#include <stdafx.h>

#include "fb_metadb_handle_list.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <helpers.h>
#include <stats.h>


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
    JsFinalizeOp<JsFbMetadbHandleList>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbMetadbHandleList",
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

}

namespace mozjs
{

JsFbMetadbHandleList::JsFbMetadbHandleList( JSContext* cx, metadb_handle_list_cref handles )
    : pJsCtx_( cx )
    , metadbHandleList_( handles )
{

}

JsFbMetadbHandleList::~JsFbMetadbHandleList()
{ 
}

JSObject* JsFbMetadbHandleList::Create( JSContext* cx, metadb_handle_list_cref handles )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties(cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsFbMetadbHandleList( cx, handles ) );

    return jsObj;
}

const JSClass& JsFbMetadbHandleList::GetClass()
{
    return jsClass;
}

metadb_handle_list_ref JsFbMetadbHandleList::GetList()
{
    return metadbHandleList_;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::Add( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: FbMetadbHandle does not contain valid handle" );
        return std::nullopt;
    }

    metadbHandleList_.add_item( fbHandle );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::AddRange( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }
    
    metadbHandleList_.add_items( handles->GetList() );
    return nullptr;
}

std::optional<int32_t> 
JsFbMetadbHandleList::BSearch( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: FbMetadbHandle does not contain valid handle" );
        return std::nullopt;
    }

    return static_cast<int32_t>(metadbHandleList_.bsearch_by_pointer( fbHandle ));
}

std::optional<double> 
JsFbMetadbHandleList::CalcTotalDuration()
{
    return metadbHandleList_.calc_total_duration();
}

std::optional<JSObject*> 
JsFbMetadbHandleList::Clone()
{
    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, metadbHandleList_ ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JsFbMetadbHandleList" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<int32_t> 
JsFbMetadbHandleList::Find( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: FbMetadbHandle does not contain valid handle" );
        return std::nullopt;
    }

    return static_cast<int32_t>( metadbHandleList_.find_item( fbHandle ) );
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::Insert( uint32_t index, JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: FbMetadbHandle does not contain valid handle" );
        return std::nullopt;
    }

    metadbHandleList_.insert_item( fbHandle, index );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::InsertRange( uint32_t index, JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    metadbHandleList_.insert_items( handles->GetList(), index );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::MakeDifference( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    metadb_handle_list_ref fbHandles = handles->GetList();
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
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::MakeIntersection( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    metadb_handle_list_ref fbHandles = handles->GetList();
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
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::MakeUnion( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    metadbHandleList_.add_items( handles->GetList() );
    metadbHandleList_.sort_by_pointer_remove_duplicates();
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::OrderByPath()
{
    metadbHandleList_.sort_by_path();
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::OrderByRelativePath()
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
        delete[] data[i].text;
    }

    metadbHandleList_.reorder( order.get_ptr() );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::RefreshStats()
{
    // TODO: investigate this code

    const t_size count = metadbHandleList_.get_count();
    pfc::avltree_t<metadb_index_hash> tmp;
    for ( t_size i = 0; i < count; ++i )
    {
        metadb_index_hash hash;
        if ( stats::g_client->hashHandle( metadbHandleList_[i], hash ) )
        {
            tmp += hash;
        }
    }
    pfc::list_t<metadb_index_hash> hashes;
    for ( auto iter = tmp.first(); iter.is_valid(); ++iter )
    {
        const metadb_index_hash hash = *iter;
        hashes += hash;
    }
    stats::theAPI()->dispatch_refresh( g_guid_jsp_metadb_index, hashes );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::Remove( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr fbHandle( handle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: FbMetadbHandle does not contain valid handle" );
        return std::nullopt;
    }

    metadbHandleList_.remove_item( fbHandle );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::RemoveAll()
{
    metadbHandleList_.remove_all();
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::RemoveById( uint32_t index )
{
    if ( index >= metadbHandleList_.get_count() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }
    metadbHandleList_.remove_by_idx( index );
    return nullptr;    
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::RemoveRange( uint32_t from, uint32_t count )
{
    metadbHandleList_.remove_from_idx( from, count );
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::Sort()
{
    metadbHandleList_.sort_by_pointer_remove_duplicates();
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbMetadbHandleList::UpdateFileInfoFromJSON( std::string str )
{
    // TODO: investigate
    uint32_t count = metadbHandleList_.get_count();
    if ( !count )
    {// Not an error
        return nullptr;
    }

    json o;
    bool isArray;

    try
    {
        o = json::parse( str.c_str() );
        if ( o.is_array() )
        {
            if ( o.size() != count )
            {
                JS_ReportErrorASCII( pJsCtx_, "Invalid JSON info: mismatched with handle count" );
                return std::nullopt;
            }
            isArray = true;
        }
        else if ( o.is_object() )
        {
            if ( o.size() == 0 )
            {
                //JS_ReportErrorASCII( pJsCtx_, "???" );
                return std::nullopt;
            }
            isArray = false;
        }
        else
        {
            //JS_ReportErrorASCII( pJsCtx_, "???" );
            return std::nullopt;
        }
    }
    catch ( ... )
    {// TODO: add error handling
        JS_ReportErrorASCII( pJsCtx_, "JSON parsing failed" );
        return std::nullopt;
    }

    pfc::list_t<file_info_impl> info;
    info.set_size( count );

    for ( t_size i = 0; i < count; i++ )
    {
        json obj = isArray ? o[i] : o;
        if ( !obj.is_object() || obj.size() == 0 )
        {
            //JS_ReportErrorASCII( pJsCtx_, "???" );
            return std::nullopt;
        }

        metadb_handle_ptr item = metadbHandleList_.get_item( i );
        item->get_info( info[i] );

        for ( json::iterator it = obj.begin(); it != obj.end(); ++it )
        {
            std::string key = it.key();
            pfc::string8 key8 = key.c_str();
            if ( key8.is_empty() )
            {
                //JS_ReportErrorASCII( pJsCtx_, "???" );
                return std::nullopt;
            }

            info[i].meta_remove_field( key8 );

            if ( it.value().is_array() )
            {
                for ( json::iterator ita = it.value().begin(); ita != it.value().end(); ++ita )
                {
                    pfc::string8 value = helpers::iterator_to_string8( ita );
                    if ( !value.is_empty() )
                    {
                        info[i].meta_add( key8, value );
                    }
                }
            }
            else
            {
                pfc::string8 value = helpers::iterator_to_string8( it );
                if ( !value.is_empty() )
                {
                    info[i].meta_set( key8, value );
                }
            }
        }
    }

    metadb_io_v2::get()->update_info_async_simple(
        metadbHandleList_,
        pfc::ptr_list_const_array_t<const file_info, file_info_impl *>( info.get_ptr(), info.get_count() ),
        core_api::get_main_window(),
        metadb_io_v2::op_flag_delay_ui,
        nullptr
    );

    return nullptr;
}

std::optional<uint32_t> 
JsFbMetadbHandleList::get_Count()
{
    return metadbHandleList_.get_count();
}

std::optional<JSObject*> 
JsFbMetadbHandleList::get_Item( uint32_t index )
{
    if( index >= metadbHandleList_.get_count() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::Create( pJsCtx_, metadbHandleList_.get_item_ref( index ) ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JsFbMetadbHandle" );
        return std::nullopt;
    }
    
    return jsObject;
}


std::optional<std::nullptr_t> 
JsFbMetadbHandleList::put_Item( uint32_t index, JS::HandleValue handle )
{
    if ( index >= metadbHandleList_.get_count() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    auto pNativeHandle = GetNativeFromJsValue<JsFbMetadbHandle>( pJsCtx_, handle );
    if ( !pNativeHandle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is not a FbMetadbHandle object" );
        return std::nullopt;
    }

    metadb_handle_ptr fbHandle( pNativeHandle->GetHandle() );
    if ( fbHandle.is_empty() )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: FbMetadbHandle does not contain valid handle" );
        return std::nullopt;
    }

    metadbHandleList_.replace_item( index, fbHandle );
    return nullptr;
}

}
