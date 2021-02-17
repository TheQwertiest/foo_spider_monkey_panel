#include <stdafx.h>

#include "fb_file_info.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

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
    JsFbFileInfo::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbFileInfo",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( InfoFind, JsFbFileInfo::InfoFind );
MJS_DEFINE_JS_FN_FROM_NATIVE( InfoName, JsFbFileInfo::InfoName );
MJS_DEFINE_JS_FN_FROM_NATIVE( InfoValue, JsFbFileInfo::InfoValue );
MJS_DEFINE_JS_FN_FROM_NATIVE( MetaFind, JsFbFileInfo::MetaFind );
MJS_DEFINE_JS_FN_FROM_NATIVE( MetaName, JsFbFileInfo::MetaName );
MJS_DEFINE_JS_FN_FROM_NATIVE( MetaValue, JsFbFileInfo::MetaValue );
MJS_DEFINE_JS_FN_FROM_NATIVE( MetaValueCount, JsFbFileInfo::MetaValueCount );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "InfoFind", InfoFind, 1, kDefaultPropsFlags ),
        JS_FN( "InfoName", InfoName, 1, kDefaultPropsFlags ),
        JS_FN( "InfoValue", InfoValue, 1, kDefaultPropsFlags ),
        JS_FN( "MetaFind", MetaFind, 1, kDefaultPropsFlags ),
        JS_FN( "MetaName", MetaName, 1, kDefaultPropsFlags ),
        JS_FN( "MetaValue", MetaValue, 2, kDefaultPropsFlags ),
        JS_FN( "MetaValueCount", MetaValueCount, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_InfoCount, JsFbFileInfo::get_InfoCount );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MetaCount, JsFbFileInfo::get_MetaCount );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "InfoCount", get_InfoCount, kDefaultPropsFlags ),
        JS_PSG( "MetaCount", get_MetaCount, kDefaultPropsFlags ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsFbFileInfo::JsClass = jsClass;
const JSFunctionSpec* JsFbFileInfo::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbFileInfo::JsProperties = jsProperties.data();
const JsPrototypeId JsFbFileInfo::PrototypeId = JsPrototypeId::FbFileInfo;

JsFbFileInfo::JsFbFileInfo( JSContext* cx, metadb_info_container::ptr containerInfo )
    : pJsCtx_( cx )
    , containerInfo_( containerInfo )
    , fileInfo_( containerInfo->info() )
{
}

std::unique_ptr<mozjs::JsFbFileInfo>
JsFbFileInfo::CreateNative( JSContext* cx, metadb_info_container::ptr containerInfo )
{
    qwr::QwrException::ExpectTrue( containerInfo.is_valid(), "Internal error: metadb_info_container object is null" );

    return std::unique_ptr<JsFbFileInfo>( new JsFbFileInfo( cx, containerInfo ) );
}

size_t JsFbFileInfo::GetInternalSize( const metadb_info_container::ptr& /*containerInfo*/ )
{
    return sizeof( file_info_impl );
}

int32_t JsFbFileInfo::InfoFind( const qwr::u8string& name )
{
    return fileInfo_.info_find_ex( name.c_str(), name.length() );
}

qwr::u8string JsFbFileInfo::InfoName( uint32_t index )
{
    qwr::QwrException::ExpectTrue( index < fileInfo_.info_get_count(), "Index is out of bounds" );

    return fileInfo_.info_enum_name( index );
}

qwr::u8string JsFbFileInfo::InfoValue( uint32_t index )
{
    qwr::QwrException::ExpectTrue( index < fileInfo_.info_get_count(), "Index is out of bounds" );

    return fileInfo_.info_enum_value( index );
}

int32_t JsFbFileInfo::MetaFind( const qwr::u8string& name )
{
    const t_size idx = fileInfo_.meta_find_ex( name.c_str(), name.length() );
    return ( ( idx == pfc_infinite ) ? -1 : static_cast<int32_t>( idx ) );
}

qwr::u8string JsFbFileInfo::MetaName( uint32_t index )
{
    qwr::QwrException::ExpectTrue( index < fileInfo_.meta_get_count(), "Index is out of bounds" );

    return fileInfo_.meta_enum_name( index );
}

qwr::u8string JsFbFileInfo::MetaValue( uint32_t infoIndex, uint32_t valueIndex )
{
    qwr::QwrException::ExpectTrue( infoIndex < fileInfo_.meta_get_count(), "Index is out of bounds" );
    qwr::QwrException::ExpectTrue( valueIndex < fileInfo_.meta_enum_value_count( infoIndex ), "Index is out of bounds" );

    return fileInfo_.meta_enum_value( infoIndex, valueIndex );
}

uint32_t JsFbFileInfo::MetaValueCount( uint32_t index )
{
    qwr::QwrException::ExpectTrue( index < fileInfo_.meta_get_count(), "Index is out of bounds" );

    return fileInfo_.meta_enum_value_count( index );
}

uint32_t JsFbFileInfo::get_InfoCount()
{
    return fileInfo_.info_get_count();
}

uint32_t JsFbFileInfo::get_MetaCount()
{
    return fileInfo_.meta_get_count();
}

} // namespace mozjs
