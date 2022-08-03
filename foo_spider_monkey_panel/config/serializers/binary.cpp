#include <stdafx.h>

#include "binary.h"

#include <utils/guid_helpers.h>

#include <qwr/string_helpers.h>
#include <qwr/visitor.h>
#include <qwr/winapi_error_helpers.h>

namespace
{

enum class JsValueType : uint32_t
{ // Take care changing this: used in config
    pt_boolean = 0,
    pt_int32 = 1,
    pt_double = 2,
    pt_string = 3,
};

}

namespace smp::config::binary
{

PanelConfig LoadSettings( stream_reader& reader, abort_callback& abort )
{
    try
    {
        PanelSettings panelSettings;
        RawInMemoryScript source;
        source.isModule = false;

        reader.skip_object( sizeof( bool ), abort ); // skip "delay load"
        panelSettings.id = [&] {
            GUID guid;
            reader.read_object_t( guid, abort );

            const auto guidStr = utils::GuidToStr( guid );
            return qwr::unicode::ToU8( guidStr );
        }();
        reader.read_object( &panelSettings.edgeStyle, sizeof( panelSettings.edgeStyle ), abort );
        auto properties = LoadProperties( reader, abort );
        reader.skip_object( sizeof( bool ), abort );            // skip "disable before"
        reader.skip_object( sizeof( bool ), abort );            // skip "should grab focus"
        reader.skip_object( sizeof( WINDOWPLACEMENT ), abort ); // skip WINDOWPLACEMENT
        source.script = qwr::pfc_x::ReadString( reader, abort );
        reader.read_object_t( panelSettings.isPseudoTransparent, abort );

        PanelConfig panelConfig;
        panelConfig.panelSettings = std::move( panelSettings );
        panelConfig.properties = std::move( properties );
        panelConfig.scriptSource = { std::move( source ) };
        return panelConfig;
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

PanelProperties LoadProperties( stream_reader& reader, abort_callback& abort )
{
    try
    {
        PanelProperties properties;

        uint32_t count;
        reader.read_lendian_t( count, abort );

        for ( auto i: ranges::views::indices( count ) )
        {
            (void)i;

            mozjs::SerializedJsValue serializedValue;

            const qwr::u8string u8PropName = qwr::pfc_x::ReadString( reader, abort );

            uint32_t valueType;
            reader.read_lendian_t( valueType, abort );

            switch ( static_cast<JsValueType>( valueType ) )
            {
            case JsValueType::pt_boolean:
            {
                bool value;
                reader.read_lendian_t( value, abort );
                serializedValue = value;
                break;
            }
            case JsValueType::pt_int32:
            {
                int32_t value;
                reader.read_lendian_t( value, abort );
                serializedValue = value;
                break;
            }
            case JsValueType::pt_double:
            {
                double value;
                reader.read_lendian_t( value, abort );
                serializedValue = value;
                break;
            }
            case JsValueType::pt_string:
            {
                serializedValue = qwr::pfc_x::ReadString( reader, abort );
                break;
            }
            default:
            {
                assert( 0 );
                continue;
            }
            }

            properties.values.emplace( qwr::unicode::ToWide( u8PropName ), std::make_shared<mozjs::SerializedJsValue>( serializedValue ) );
        }

        return properties;
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

} // namespace smp::config::binary
