#include <stdafx.h>

#include "track_info_snapshot.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/utils/js_error_helper.h>
#include <js_backend/utils/js_object_constants.h>

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
    TrackInfoSnapshot::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "TrackInfoSnapshot",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( getMetaCount, TrackInfoSnapshot::GetMetaCount );
MJS_DEFINE_JS_FN_FROM_NATIVE( getMetaIndex, TrackInfoSnapshot::GetMetaIndex );
MJS_DEFINE_JS_FN_FROM_NATIVE( getMetaName, TrackInfoSnapshot::GetMetaName );
MJS_DEFINE_JS_FN_FROM_NATIVE( getMetaValue, TrackInfoSnapshot::GetMetaValue );
MJS_DEFINE_JS_FN_FROM_NATIVE( getMetaValueCount, TrackInfoSnapshot::GetMetaValueCount );
MJS_DEFINE_JS_FN_FROM_NATIVE( getTechnicalInfoCount, TrackInfoSnapshot::GetTechnicalInfoCount );
MJS_DEFINE_JS_FN_FROM_NATIVE( getTechnicalInfoIndex, TrackInfoSnapshot::GetTechnicalInfoIndex );
MJS_DEFINE_JS_FN_FROM_NATIVE( getTechnicalInfoName, TrackInfoSnapshot::GetTechnicalInfoName );
MJS_DEFINE_JS_FN_FROM_NATIVE( getTechnicalInfoValue, TrackInfoSnapshot::GetTechnicalInfoValue );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getMetaCount", getMetaCount, 0, kDefaultPropsFlags ),
        JS_FN( "getMetaIndex", getMetaIndex, 1, kDefaultPropsFlags ),
        JS_FN( "getMetaName", getMetaName, 1, kDefaultPropsFlags ),
        JS_FN( "getMetaValue", getMetaValue, 2, kDefaultPropsFlags ),
        JS_FN( "getMetaValueCount", getMetaValueCount, 1, kDefaultPropsFlags ),
        JS_FN( "getTechnicalInfoCount", getTechnicalInfoCount, 0, kDefaultPropsFlags ),
        JS_FN( "getTechnicalInfoIndex", getTechnicalInfoIndex, 1, kDefaultPropsFlags ),
        JS_FN( "getTechnicalInfoName", getTechnicalInfoName, 1, kDefaultPropsFlags ),
        JS_FN( "getTechnicalInfoValue", getTechnicalInfoValue, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_fileSize, TrackInfoSnapshot::get_FileSize );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_length, TrackInfoSnapshot::get_Length );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "fileSize", get_fileSize, kDefaultPropsFlags ),
        JS_PSG( "length", get_length, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::TrackInfoSnapshot );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<TrackInfoSnapshot>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<TrackInfoSnapshot>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<TrackInfoSnapshot>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<TrackInfoSnapshot>::PrototypeId = JsPrototypeId::New_TrackInfoSnapshot;

TrackInfoSnapshot::TrackInfoSnapshot( JSContext* cx, metadb_info_container::ptr infoContainer )
    : pJsCtx_( cx )
    , infoContainer_( infoContainer )
    , fileInfo_( infoContainer->info() )
    , fileStats_( infoContainer->stats() )
{
}

std::unique_ptr<mozjs::TrackInfoSnapshot>
TrackInfoSnapshot::CreateNative( JSContext* cx, metadb_info_container::ptr infoContainer )
{
    qwr::QwrException::ExpectTrue( infoContainer.is_valid(), "Internal error: metadb_info_container object is null" );

    return std::unique_ptr<TrackInfoSnapshot>( new TrackInfoSnapshot( cx, infoContainer ) );
}

size_t TrackInfoSnapshot::GetInternalSize() const
{
    return sizeof( file_info_impl ) + sizeof( t_filestats2 );
}

uint32_t TrackInfoSnapshot::GetMetaCount() const
{
    return fileInfo_.meta_get_count();
}

int32_t TrackInfoSnapshot::GetMetaIndex( const qwr::u8string& name ) const
{
    const t_size idx = fileInfo_.meta_find_ex( name.c_str(), name.length() );
    return ( ( idx == pfc_infinite ) ? -1 : static_cast<int32_t>( idx ) );
}

qwr::u8string TrackInfoSnapshot::GetMetaName( uint32_t index ) const
{
    qwr::QwrException::ExpectTrue( index < fileInfo_.meta_get_count(), "Index is out of bounds" );

    return fileInfo_.meta_enum_name( index );
}

qwr::u8string TrackInfoSnapshot::GetMetaValue( uint32_t metaIndex, uint32_t valueIndex ) const
{
    qwr::QwrException::ExpectTrue( metaIndex < fileInfo_.meta_get_count(), "metaIndex is out of bounds" );
    qwr::QwrException::ExpectTrue( valueIndex < fileInfo_.meta_enum_value_count( metaIndex ), "valueIndex is out of bounds" );

    return fileInfo_.meta_enum_value( metaIndex, valueIndex );
}

uint32_t TrackInfoSnapshot::GetMetaValueCount( uint32_t index ) const
{
    qwr::QwrException::ExpectTrue( index < fileInfo_.meta_get_count(), "Index is out of bounds" );

    return fileInfo_.meta_enum_value_count( index );
}

uint32_t TrackInfoSnapshot::GetTechnicalInfoCount() const
{
    return fileInfo_.info_get_count();
}

int32_t TrackInfoSnapshot::GetTechnicalInfoIndex( const qwr::u8string& name ) const
{
    return fileInfo_.info_find_ex( name.c_str(), name.length() );
}

qwr::u8string TrackInfoSnapshot::GetTechnicalInfoName( uint32_t index ) const
{
    qwr::QwrException::ExpectTrue( index < fileInfo_.info_get_count(), "Index is out of bounds" );

    return fileInfo_.info_enum_name( index );
}

qwr::u8string TrackInfoSnapshot::GetTechnicalInfoValue( uint32_t index ) const
{
    qwr::QwrException::ExpectTrue( index < fileInfo_.info_get_count(), "Index is out of bounds" );

    return fileInfo_.info_enum_value( index );
}

uint64_t TrackInfoSnapshot::get_FileSize() const
{
    return fileStats_.m_size;
}

double TrackInfoSnapshot::get_Length() const
{
    return fileInfo_.get_length();
}

} // namespace mozjs
