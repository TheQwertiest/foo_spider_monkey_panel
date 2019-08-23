#include <stdafx.h>
#include "config.h"

#include <utils/string_helpers.h>
#include <config_legacy.h>
#include <resource.h>

#include <nlohmann/json.hpp>

namespace
{

/// @brief Specify properties serialization format.
///        1 - JSON.
constexpr uint32_t kPropConfigVersion = 1;

constexpr const char kPropJsonConfigVersion[] = "1";
constexpr const char kPropJsonConfigId[] = "properties";

} // namespace

namespace smp::config
{

DWORD edge_style_from_config( EdgeStyle edge_style )
{
    switch ( edge_style )
    {
    case EdgeStyle::SUNKEN_EDGE:
        return WS_EX_CLIENTEDGE;
    case EdgeStyle::GREY_EDGE:
        return WS_EX_STATICEDGE;
    default:
        return 0;
    }
}

PanelProperties::config_map& PanelProperties::get_val()
{
    return m_map;
}

std::optional<mozjs::SerializedJsValue> PanelProperties::get_config_item( const std::wstring& propName )
{
    auto it = m_map.find( propName );
    if ( it == m_map.end() )
    {
        return std::nullopt;
    }

    return *( it->second );
}

void PanelProperties::set_config_item( const std::wstring& propName, const mozjs::SerializedJsValue& serializedValue )
{
    m_map.insert_or_assign( propName, std::make_shared<mozjs::SerializedJsValue>( serializedValue ) );
}

void PanelProperties::remove_config_item( const std::wstring& propName )
{
    m_map.erase( propName );
}

bool PanelProperties::g_load( config_map& data, stream_reader& reader, abort_callback& abort )
{
    return LoadProperties_Binary( data, reader, abort );
}

bool PanelProperties::g_load_json( config_map& data, stream_reader& reader, abort_callback& abort, bool loadRawString )
{
    using json = nlohmann::json;

    data.clear();

    try
    {
        std::u8string jsonStr;
        if ( loadRawString )
        {
            jsonStr = smp::pfc_x::ReadRawString( reader, abort );
        }
        else
        {
            uint32_t version;
            reader.read_lendian_t( version, abort );
            if ( version != kPropConfigVersion )
            {
                return false;
            }

			jsonStr = smp::pfc_x::ReadString( reader, abort );
        }

        json jsonMain = json::parse( jsonStr );
        if ( !jsonMain.is_object() )
        {
            return false;
        }

        if ( jsonMain.at( "version" ).get<std::string>() != kPropJsonConfigVersion
             || jsonMain.at( "id" ).get<std::string>() != kPropJsonConfigId )
        {
            return false;
        }

        auto& values = jsonMain.at( "values" );
        if ( !values.is_object() )
        {
            return false;
        }

        for ( auto& [key, value]: values.items() )
        {
            if ( key.empty() )
            {
                return false;
            }

            mozjs::SerializedJsValue serializedValue;
            if ( value.is_boolean() )
            {
                serializedValue = value.get<bool>();
            }
            else if ( value.is_number_integer() )
            {
                serializedValue = value.get<int32_t>();
            }
            else if ( value.is_number_float() )
            {
                serializedValue = value.get<double>();
            }
            else if ( value.is_string() )
            {
                serializedValue = value.get<std::string>();
            }
            else
            {
                assert( 0 );
                continue;
            }

            data.emplace( smp::unicode::ToWide( key ), std::make_shared<mozjs::SerializedJsValue>( serializedValue ) );
        }
    }
    catch ( const json::exception& )
    {
        return false;
    }
    catch ( const pfc::exception& )
    {
        return false;
    }

    return true;
}

bool PanelProperties::g_load_legacy( config_map& data, stream_reader& reader, abort_callback& abort )
{
    return LoadProperties_Com( data, reader, abort );
}

void PanelProperties::load( stream_reader& reader, abort_callback& abort )
{
    g_load( m_map, reader, abort );
}

void PanelProperties::save( stream_writer& writer, abort_callback& abort ) const
{
    g_save( m_map, writer, abort );
}

void PanelProperties::g_save( const config_map& data, stream_writer& writer, abort_callback& abort )
{
    SaveProperties_Binary( data, writer, abort );
}

void PanelProperties::g_save_json( const config_map& data, stream_writer& writer, abort_callback& abort, bool saveAsRawString )
{
    using json = nlohmann::json;

    try
    {
        json jsonMain = json::object( { { "id", kPropJsonConfigId },
                                        { "version", kPropJsonConfigVersion } } );

        json jsonValues = json::object();
        for ( const auto& [nameW, pValue]: data )
        {
            const auto propertyName = smp::unicode::ToU8( nameW );
            const auto& serializedValue = *pValue;

            std::visit( [&jsonValues, &propertyName]( auto&& arg ) {
                jsonValues.push_back( { propertyName, arg } );
            }, serializedValue );
        }

        jsonMain.push_back( { "values", jsonValues } );

        const auto jsonStr = jsonMain.dump();
        if ( saveAsRawString )
        {
            writer.write_string_raw( jsonStr.c_str(), abort );
        }
        else
        {
            writer.write_lendian_t( kPropConfigVersion, abort );
            writer.write_string( jsonStr.c_str(), jsonStr.length(), abort );
        }
    }
    catch ( const json::exception& )
    {
    }
    catch ( const pfc::exception& )
    {
    }
}

PanelSettings::PanelSettings()
{
    reset_config();
}

GUID& PanelSettings::get_config_guid()
{
    return m_config_guid;
}

WINDOWPLACEMENT& PanelSettings::get_windowplacement()
{
    return m_wndpl;
}

bool& PanelSettings::get_grab_focus()
{
    return m_grab_focus;
}

bool& PanelSettings::get_pseudo_transparent()
{
    return m_pseudo_transparent;
}

const bool& PanelSettings::get_pseudo_transparent() const
{
    return m_pseudo_transparent;
}

const EdgeStyle& PanelSettings::get_edge_style() const
{
    return m_edge_style;
}

std::u8string& PanelSettings::get_script_code()
{
    return m_script_code;
}

PanelProperties& PanelSettings::get_config_prop()
{
    return m_config_prop;
}

std::u8string PanelSettings::get_default_script_code()
{
    puResource puRes = uLoadResource( core_api::get_my_instance(), uMAKEINTRESOURCE( IDR_SCRIPT ), "SCRIPT" );
    if ( puRes )
    {
        return std::u8string{ reinterpret_cast<const char*>( puRes->GetPointer() ), puRes->GetSize() };
    }
    else
    {
        return std::u8string{};
    }
}

EdgeStyle& PanelSettings::get_edge_style()
{
    return m_edge_style;
}

void PanelSettings::load_config( stream_reader& reader, t_size size, abort_callback& abort )
{
    reset_config();

    // TODO: remove old config values and up the version

    if ( size > sizeof( Version ) )
    {
        try
        {
            uint32_t ver = 0;
            reader.read_object_t( ver, abort );
            if ( ver > static_cast<uint32_t>( Version::CONFIG_VERSION_CURRENT ) )
            {
                throw pfc::exception();
            }
            reader.skip_object( sizeof( false ), abort ); // HACK: skip over "delay load"
            reader.read_object_t( m_config_guid, abort );
            reader.read_object( &m_edge_style, sizeof( m_edge_style ), abort );
            m_config_prop.load( reader, abort );
            reader.skip_object( sizeof( false ), abort ); // HACK: skip over "disable before"
            reader.read_object_t( m_grab_focus, abort );
            reader.read_object( &m_wndpl, sizeof( m_wndpl ), abort );
            m_script_code = smp::pfc_x::ReadString( reader, abort );
            reader.read_object_t( m_pseudo_transparent, abort );
        }
        catch ( const pfc::exception& )
        {
            reset_config();
            FB2K_console_formatter() << "Error: " SMP_NAME_WITH_VERSION " Configuration has been corrupted. All settings have been reset.";
        }
    }
}

void PanelSettings::reset_config()
{
    m_script_code = get_default_script_code();
    m_pseudo_transparent = false;
    m_wndpl.length = 0;
    m_grab_focus = true;
    m_edge_style = EdgeStyle::NO_EDGE;
	// should not fail
    (void)CoCreateGuid( &m_config_guid );
}

void PanelSettings::save_config( stream_writer& writer, abort_callback& abort ) const
{
    try
    {
        auto currentVersion = static_cast<uint32_t>( Version::CONFIG_VERSION_CURRENT );
        writer.write_object_t( currentVersion, abort );
        writer.write_object_t( false, abort ); // HACK: write this in place of "delay load"
        writer.write_object_t( m_config_guid, abort );
        writer.write_object( &m_edge_style, sizeof( m_edge_style ), abort );
        m_config_prop.save( writer, abort );
        writer.write_object_t( false, abort ); // HACK: write this in place of "disable before"
        writer.write_object_t( m_grab_focus, abort );
        writer.write_object( &m_wndpl, sizeof( m_wndpl ), abort );
        writer.write_string( m_script_code.c_str(), abort );
        writer.write_object_t( m_pseudo_transparent, abort );
    }
    catch ( const pfc::exception& )
    {
    }
}

} // namespace smp::config