#include <stdafx.h>

#include "panel_config_binary.h"

#include <qwr/string_helpers.h>
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

PanelSettings LoadSettings( stream_reader& reader, abort_callback& abort )
{
    try
    {
        PanelSettings panelSettings;
        PanelSettings_InMemory payload;

        reader.skip_object( sizeof( false ), abort ); // skip "delay load"
        panelSettings.id = [&] {
            GUID guid;
            reader.read_object_t( guid, abort );

            std::wstring guidStr;
            guidStr.resize( 64 );
            StringFromGUID2( guid, guidStr.data(), guidStr.size() );
            guidStr.resize( wcslen( guidStr.c_str() ) );

            return qwr::unicode::ToU8( guidStr );
        }();
        reader.read_object( &panelSettings.edgeStyle, sizeof( panelSettings.edgeStyle ), abort );
        panelSettings.properties = LoadProperties( reader, abort );
        reader.skip_object( sizeof( false ), abort ); // skip "disable before"
        reader.read_object_t( payload.shouldGrabFocus, abort );
        reader.skip_object( sizeof( WINDOWPLACEMENT ), abort ); // skip WINDOWPLACEMENT
        payload.script = qwr::pfc_x::ReadString( reader, abort );
        reader.read_object_t( panelSettings.isPseudoTransparent, abort );

        panelSettings.payload = payload;

        return panelSettings;
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

void SaveSettings( stream_writer& writer, abort_callback& abort, const PanelSettings& settings )
{
    try
    {
        assert( std::holds_alternative<PanelSettings_InMemory>( settings.payload ) );
        const auto& payload = std::get<PanelSettings_InMemory>( settings.payload );

        writer.write_object_t( false, abort ); // skip "delay load"
        {
            GUID guid;
            HRESULT hr = IIDFromString( qwr::unicode::ToWide( settings.id ).c_str(), &guid );
            qwr::error::CheckHR( hr, "IIDFromString" );

            writer.write_object_t( guid, abort );
        }
        writer.write_object_t( static_cast<uint8_t>( settings.edgeStyle ), abort );
        SaveProperties( writer, abort, settings.properties );
        writer.write_object_t( false, abort ); // skip "disable before"
        writer.write_object_t( payload.shouldGrabFocus, abort );
        {
            WINDOWPLACEMENT dummy{};
            writer.write_object( &dummy, sizeof( dummy ), abort ); // skip WINDOWPLACEMENT
        }
        qwr::pfc_x::WriteString( writer, payload.script, abort );
        writer.write_object_t( settings.isPseudoTransparent, abort );
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
            mozjs::SerializedJsValue serializedValue;

            const std::u8string u8PropName = qwr::pfc_x::ReadString( reader, abort );

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

void SaveProperties( stream_writer& writer, abort_callback& abort, const PanelProperties& properties )
{
    try
    {
        writer.write_lendian_t( static_cast<uint32_t>( properties.values.size() ), abort );

        for ( const auto& [name, pValue]: properties.values )
        {
            qwr::pfc_x::WriteString( writer, qwr::unicode::ToU8( name ), abort );

            const auto& serializedValue = *pValue;

            const JsValueType valueType = std::visit( []( auto&& arg ) {
                using T = std::decay_t<decltype( arg )>;
                if constexpr ( std::is_same_v<T, bool> )
                {
                    return JsValueType::pt_boolean;
                }
                else if constexpr ( std::is_same_v<T, int32_t> )
                {
                    return JsValueType::pt_int32;
                }
                else if constexpr ( std::is_same_v<T, double> )
                {
                    return JsValueType::pt_double;
                }
                else if constexpr ( std::is_same_v<T, std::u8string> )
                {
                    return JsValueType::pt_string;
                }
                else
                {
                    static_assert( false, "non-exhaustive visitor!" );
                }
            },
                                                      serializedValue );

            writer.write_lendian_t( static_cast<uint32_t>( valueType ), abort );

            std::visit( [&writer, &abort]( auto&& arg ) {
                using T = std::decay_t<decltype( arg )>;
                if constexpr ( std::is_same_v<T, std::u8string> )
                {
                    const auto& value = arg;
                    writer.write_string( value.c_str(), value.length(), abort );
                }
                else
                {
                    writer.write_lendian_t( arg, abort );
                }
            },
                        serializedValue );
        }
    }
    catch ( const pfc::exception& )
    {
    }
}

} // namespace smp::config::binary
