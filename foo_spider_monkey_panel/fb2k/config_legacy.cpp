#include <stdafx.h>

#include "config_legacy.h"

#include <qwr/string_helpers.h>
#include <qwr/type_traits.h>

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

namespace smp::config
{

bool LoadProperties_Binary( PanelProperties::PropertyMap& data, stream_reader& reader, abort_callback& abort )
{
    data.clear();

    try
    {
        uint32_t count;
        reader.read_lendian_t( count, abort );

        for ( uint32_t i = 0; i < count; ++i )
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

            data.emplace( qwr::unicode::ToWide( u8PropName ), std::make_shared<mozjs::SerializedJsValue>( serializedValue ) );
        }
    }
    catch ( const pfc::exception& )
    {
        return false;
    }

    return true;
}

void SaveProperties_Binary( const PanelProperties::PropertyMap& data, stream_writer& writer, abort_callback& abort )
{
    try
    {
        writer.write_lendian_t( static_cast<uint32_t>( data.size() ), abort );

        for ( const auto& [name, pValue]: data )
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
                    static_assert( qwr::always_false_v<T>, "non-exhaustive visitor!" );
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

bool LoadProperties_Com( PanelProperties::PropertyMap& data, stream_reader& reader, abort_callback& abort )
{
    data.clear();

    try
    {
        t_size count;
        reader.read_lendian_t( count, abort );

        for ( t_size i = 0; i < count; ++i )
        {
            const std::u8string u8propName = qwr::string::Trim<char8_t>( qwr::pfc_x::ReadString( reader, abort ) );

            VARTYPE vt;
            reader.read_lendian_t( vt, abort );

            mozjs::SerializedJsValue serializedValue;

            switch ( vt )
            {
            case VT_UI1:
            case VT_I1:
            {
                int8_t val;
                reader.read( &val, sizeof( val ), abort );
                serializedValue = static_cast<int32_t>( val );

                break;
            }
            case VT_I2:
            case VT_UI2:
            {
                int16_t val;
                reader.read( &val, sizeof( val ), abort );
                serializedValue = static_cast<int32_t>( val );

                break;
            }

            case VT_BOOL:
            {
                int16_t val;
                reader.read( &val, sizeof( val ), abort );
                serializedValue = !!val;

                break;
            }
            case VT_I4:
            case VT_UI4:
            case VT_INT:
            case VT_UINT:
            {
                int32_t val;
                reader.read( &val, sizeof( val ), abort );
                serializedValue = val;

                break;
            }
            case VT_R4:
            {
                float val;
                reader.read( &val, sizeof( val ), abort );
                serializedValue = static_cast<double>( val );

                break;
            }
            case VT_I8:
            case VT_UI8:
            {
                int64_t val;
                reader.read( &val, sizeof( val ), abort );
                serializedValue = static_cast<int32_t>( val );

                break;
            }
            case VT_R8:
            case VT_CY:
            case VT_DATE:
            {
                double val;
                reader.read( &val, sizeof( val ), abort );
                serializedValue = val;

                break;
            }
            case VT_BSTR:
            {
                serializedValue = qwr::pfc_x::ReadString( reader, abort );
                break;
            }
            default:
            {
                continue;
            }
            }

            data.emplace( qwr::unicode::ToWide( u8propName ), std::make_shared<mozjs::SerializedJsValue>( serializedValue ) );
        }
    }
    catch ( const pfc::exception& )
    {
        return false;
    }

    return true;
}

} // namespace smp::config
