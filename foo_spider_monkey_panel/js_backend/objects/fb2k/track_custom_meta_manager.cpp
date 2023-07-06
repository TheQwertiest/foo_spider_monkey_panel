#include <stdafx.h>

#include "track_custom_meta_manager.h"

#include <fb2k/custom_meta_fields.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/fb2k/track.h>
#include <js_backend/objects/fb2k/track_list.h>

#include <qwr/algorithm.h>

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
    TrackCustomMetaManager::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "TrackCustomMetaMnager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( getField, TrackCustomMetaManager::GetField );
MJS_DEFINE_JS_FN_FROM_NATIVE( refreshMeta, TrackCustomMetaManager::RefreshMeta );
MJS_DEFINE_JS_FN_FROM_NATIVE( setField, TrackCustomMetaManager::SetField );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getField", getField, 2, kDefaultPropsFlags ),
        JS_FN( "refreshMeta", refreshMeta, 1, kDefaultPropsFlags ),
        JS_FN( "setField", setField, 3, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::TrackCustomMetaManager );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<TrackCustomMetaManager>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<TrackCustomMetaManager>::JsFunctions = jsFunctions.data();

TrackCustomMetaManager::TrackCustomMetaManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

TrackCustomMetaManager::~TrackCustomMetaManager()
{
}

std::unique_ptr<TrackCustomMetaManager>
TrackCustomMetaManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<TrackCustomMetaManager>( new TrackCustomMetaManager( cx ) );
}

size_t TrackCustomMetaManager::GetInternalSize() const
{
    return 0;
}

JS::Value TrackCustomMetaManager::GetField( smp::not_null<Track*> track, const qwr::u8string& name ) const
{
    const auto fieldInfoOpt = custom_meta::GetFieldInfo( name );
    qwr::QwrException::ExpectTrue( fieldInfoOpt.has_value(), "Unknown custom meta field name" );

    const auto fields = custom_meta::GetData( track->GetHandle() );
    const auto pValue = qwr::FindAsPointer( fields, name );
    if ( !pValue )
    {
        return JS::NullValue();
    }

    JS::RootedValue jsValue( pJsCtx_ );
    std::visit( [&]( const auto& arg ) { return convert::to_js::ToValue( pJsCtx_, arg, &jsValue ); }, *pValue );
    return jsValue;
}

void TrackCustomMetaManager::RefreshMeta( JS::HandleValue tracks ) const
{
    if ( auto pNative = Track::ExtractNative( pJsCtx_, tracks ) )
    {
        custom_meta::RefreshData( pNative->GetHandle() );
    }
    else
    {
        const auto handles = TrackList::ValueToHandleList( pJsCtx_, tracks );
        custom_meta::RefreshData( handles );
    }
}

void TrackCustomMetaManager::SetField( smp::not_null<Track*> track, const qwr::u8string& name, JS::HandleValue value )
{
    const auto fieldInfoOpt = custom_meta::GetFieldInfo( name );
    qwr::QwrException::ExpectTrue( fieldInfoOpt.has_value(), "Unknown custom meta field name" );

    auto fields = custom_meta::GetData( track->GetHandle() );

    if ( value.isNullOrUndefined() )
    {
        fields.erase( name );
    }
    else
    {
        switch ( fieldInfoOpt->valueType )
        {
        case custom_meta::FieldValueType::kUInt32:
        {
            auto tmp = convert::to_native::ToValue<uint32_t>( pJsCtx_, value );
            fields.insert_or_assign( name, tmp );
            break;
        }
        case custom_meta::FieldValueType::kString:
        {
            auto tmp = convert::to_native::ToValue<qwr::u8string>( pJsCtx_, value );
            fields.insert_or_assign( name, tmp );
            break;
        }
        default:
            throw qwr::QwrException( "Internal error: unexpected field value type" );
        }
    }

    custom_meta::SetData( track->GetHandle(), fields );
}

} // namespace mozjs
