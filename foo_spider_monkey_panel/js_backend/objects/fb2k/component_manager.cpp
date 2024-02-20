#include <stdafx.h>

#include "component_manager.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/utils/js_property_helper.h>

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
    ComponentManager::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ComponentManager",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( findComponentByFilename, ComponentManager::FindComponentByFilename );
MJS_DEFINE_JS_FN_FROM_NATIVE( findComponentByName, ComponentManager::FindComponentByName );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "findComponentByFilename", findComponentByFilename, 1, kDefaultPropsFlags ),
        JS_FN( "findComponentByName", findComponentByName, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::ComponentManager );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<ComponentManager>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<ComponentManager>::JsFunctions = jsFunctions.data();

ComponentManager::ComponentManager( JSContext* cx )
    : pJsCtx_( cx )
{
}

ComponentManager::~ComponentManager()
{
}

std::unique_ptr<ComponentManager>
ComponentManager::CreateNative( JSContext* cx )
{
    return std::unique_ptr<ComponentManager>( new ComponentManager( cx ) );
}

size_t ComponentManager::GetInternalSize() const
{
    return 0;
}

JSObject* ComponentManager::FindComponentByFilename( const qwr::u8string& filename ) const
{
    qwr::QwrException::ExpectTrue( !filename.empty(), "Invalid filename value" );

    pfc::string8_fast temp;
    for ( const auto pComponent: service_enum_t<componentversion>{} )
    {
        pComponent->get_file_name( temp );

        if ( temp.c_str() == filename )
        {
            return CreateJsComponent( pComponent );
        }
    }

    return nullptr;
}

JSObject* ComponentManager::FindComponentByName( const qwr::u8string& name ) const
{
    qwr::QwrException::ExpectTrue( !name.empty(), "Invalid name value" );

    pfc::string8_fast temp;
    for ( const auto pComponent: service_enum_t<componentversion>{} )
    {
        pComponent->get_component_name( temp );

        if ( temp.c_str() == name )
        {
            return CreateJsComponent( pComponent );
        }
    }

    return nullptr;
}

JSObject* ComponentManager::CreateJsComponent( componentversion::ptr pComponent ) const
{
    JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
    smp::JsException::ExpectTrue( jsObject );

    pfc::string8_fast temp;
    pComponent->get_component_name( temp );
    utils::SetProperty( pJsCtx_, jsObject, "name", temp );

    pComponent->get_file_name( temp );
    utils::SetProperty( pJsCtx_, jsObject, "filename", temp );

    pComponent->get_component_version( temp );
    utils::SetProperty( pJsCtx_, jsObject, "version", temp );

    return jsObject;
}

} // namespace mozjs
