#include <stdafx.h>

#include "fb_properties.h"

#include <convert/native_to_js.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/serialized_value.h>
#include <panel/js_panel_window.h>

#include <qwr/string_helpers.h>

using namespace smp;

namespace mozjs
{

FbProperties::FbProperties( JSContext* cx, panel::js_panel_window& parentPanel )
    : pJsCtx_( cx )
    , parentPanel_( parentPanel )
{
}

std::unique_ptr<FbProperties>
FbProperties::Create( JSContext* cx, panel::js_panel_window& parentPanel )
{
    return std::unique_ptr<FbProperties>( new FbProperties( cx, parentPanel ) );
}

void FbProperties::Trace( JSTracer* trc )
{
    for ( auto& [name, heapElem]: properties_ )
    {
        JS::TraceEdge( trc, &heapElem->value, "CustomHeap_Properties" );
    }
}

void FbProperties::PrepareForGc()
{
    properties_.clear();
}

JS::Value FbProperties::GetProperty( const std::wstring& propName, JS::HandleValue propDefaultValue )
{
    const std::wstring trimmedPropName( qwr::string::Trim<wchar_t>( propName ) );

    bool hasProperty = false;
    if ( properties_.contains( trimmedPropName ) )
    {
        hasProperty = true;
    }
    else
    {
        auto& panelPropertyValues = parentPanel_.GetPanelProperties().values;

        auto it = panelPropertyValues.find( trimmedPropName );
        if ( it != panelPropertyValues.end() )
        {
            hasProperty = true;

            JS::RootedValue jsProp( pJsCtx_ );
            DeserializeJsValue( pJsCtx_, *it->second, &jsProp );
            properties_.emplace( trimmedPropName, std::make_unique<HeapElement>( jsProp ) );
        }
    }

    if ( !hasProperty )
    {
        if ( propDefaultValue.isNullOrUndefined() )
        { // Not a error: user does not want to set default value
            return JS::NullValue();
        }

        SetProperty( trimmedPropName, propDefaultValue );
    }

    return properties_[trimmedPropName]->value.get();
}

void FbProperties::SetProperty( const std::wstring& propName, JS::HandleValue propValue )
{
    const std::wstring trimmedPropName( qwr::string::Trim<wchar_t>( propName ) );

    auto& panelPropertyValues = parentPanel_.GetPanelProperties().values;

    if ( propValue.isNullOrUndefined() )
    {
        panelPropertyValues.erase( trimmedPropName );
        properties_.erase( trimmedPropName );
        return;
    }

    auto serializedValue = SerializeJsValue( pJsCtx_, propValue );

    properties_.insert_or_assign( trimmedPropName, std::make_unique<HeapElement>( propValue ) );
    panelPropertyValues.insert_or_assign( trimmedPropName, std::make_shared<SerializedJsValue>( serializedValue ) );
}

} // namespace mozjs
