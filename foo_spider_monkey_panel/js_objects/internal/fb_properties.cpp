#include <stdafx.h>
#include "fb_properties.h"

#include <js_engine/js_to_native_invoker.h>
#include <convert/native_to_js.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/serialized_value.h>
#include <utils/string_helpers.h>

#include <js_panel_window.h>

using namespace smp;

namespace mozjs
{

FbProperties::FbProperties( JSContext* cx, js_panel_window& parentPanel )
    : pJsCtx_( cx )
    , parentPanel_( parentPanel )
{
}

FbProperties::~FbProperties()
{
    RemoveHeapTracer();
}

std::unique_ptr<FbProperties> 
FbProperties::Create( JSContext* cx, js_panel_window& parentPanel )
{
    std::unique_ptr<FbProperties> fbProps( new FbProperties( cx, parentPanel ) );

    if ( !JS_AddExtraGCRootsTracer( cx, FbProperties::TraceHeapValue, fbProps.get() ) )
    {
        return nullptr;
    }

    return fbProps;
}

void FbProperties::RemoveHeapTracer()
{
    JS_RemoveExtraGCRootsTracer( pJsCtx_, FbProperties::TraceHeapValue, this );
    properties_.clear();
}

std::optional<JS::Heap<JS::Value>>
FbProperties::GetProperty( const std::wstring& propName, JS::HandleValue propDefaultValue )
{    
    std::wstring trimmedPropName( smp::string::Trim( propName ) );

    bool hasProperty = false;
    if ( properties_.count( trimmedPropName ) )
    {
        hasProperty = true;
    }
    else
    {
        auto prop = parentPanel_.get_config_prop().get_config_item( trimmedPropName );
        if ( prop )
        {
            JS::RootedValue jsProp( pJsCtx_ );
            if ( DeserializeJsValue( pJsCtx_, prop.value(), &jsProp ) )
            {
                hasProperty = true;
                properties_.emplace( trimmedPropName, std::make_unique<HeapElement>( jsProp ) );
            }
        }        
    }

    if ( !hasProperty )
    {
        if ( propDefaultValue.isNullOrUndefined() )
        {// Not a error: user does not want to set default value
            JS::Heap<JS::Value> tmpVal;
            tmpVal.setNull();
            return std::make_optional( tmpVal );
        }

        if ( !SetProperty( trimmedPropName, propDefaultValue ) )
        {
            return std::nullopt;
        }
    }

    return std::make_optional( properties_[trimmedPropName]->value );
}

bool FbProperties::SetProperty( const std::wstring& propName, JS::HandleValue propValue )
{
    std::wstring trimmedPropName( smp::string::Trim( propName ) );

    if ( propValue.isNullOrUndefined() )
    {
        parentPanel_.get_config_prop().remove_config_item( trimmedPropName );
        properties_.erase( trimmedPropName );
        return true;
    }

    auto serializedValue = SerializeJsValue( pJsCtx_, propValue );
    if ( !serializedValue )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Unsupported value type" );
        return false;
    }

    properties_.insert_or_assign(trimmedPropName, std::make_unique<HeapElement>( propValue ));
    parentPanel_.get_config_prop().set_config_item( trimmedPropName, serializedValue.value() );
    
    return true;
}

void FbProperties::TraceHeapValue( JSTracer *trc, void *data )
{
    assert( data );
    auto jsObject = static_cast<FbProperties*>( data );
    auto& properties = jsObject->properties_;
    
    for ( auto& [name,heapElem] : properties )
    {
        JS::TraceEdge( trc, &heapElem->value, "CustomHeap_Properties" );
    }
}

}
