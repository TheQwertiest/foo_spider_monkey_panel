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

enum class Version : uint32_t
{
    SMP_VERSION_100 = 1, // must start with 1 so we don't break component upgrades
    CONFIG_VERSION_CURRENT = SMP_VERSION_100
};

} // namespace

namespace smp::config
{

bool PanelProperties::LoadBinary( stream_reader& reader, abort_callback& abort )
{
    return LoadProperties_Binary( values, reader, abort );
}

bool PanelProperties::LoadJson( stream_reader& reader, abort_callback& abort, bool loadRawString )
{
    using json = nlohmann::json;

    values.clear();

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

        auto& jsonValues = jsonMain.at( "values" );
        if ( !jsonValues.is_object() )
        {
            return false;
        }

        for ( auto& [key, value]: jsonValues.items() )
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

            values.emplace( smp::unicode::ToWide( key ), std::make_shared<mozjs::SerializedJsValue>( serializedValue ) );
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

bool PanelProperties::LoadLegacy( stream_reader& reader, abort_callback& abort )
{
    return LoadProperties_Com( values, reader, abort );
}

void PanelProperties::SaveBinary( stream_writer& writer, abort_callback& abort ) const
{
    SaveProperties_Binary( values, writer, abort );
}

void PanelProperties::SaveJson( stream_writer& writer, abort_callback& abort, bool saveAsRawString ) const
{
    using json = nlohmann::json;

    try
    {
        json jsonMain = json::object( { { "id", kPropJsonConfigId },
                                        { "version", kPropJsonConfigVersion } } );

        json jsonValues = json::object();
        for ( const auto& [nameW, pValue]: values )
        {
            const auto propertyName = smp::unicode::ToU8( nameW );
            const auto& serializedValue = *pValue;

            std::visit( [&jsonValues, &propertyName]( auto&& arg ) {
                jsonValues.push_back( { propertyName, arg } );
            },
                        serializedValue );
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
    ResetToDefault();
}

std::u8string PanelSettings::GetDefaultScript()
{
    puResource puRes = uLoadResource( core_api::get_my_instance(), uMAKEINTRESOURCE( IDR_SCRIPT ), "SCRIPT" );
    if ( puRes )
    {
        return std::u8string{ static_cast<const char*>( puRes->GetPointer() ), puRes->GetSize() };
    }
    else
    {
        return std::u8string{};
    }
}

void PanelSettings::Load( stream_reader& reader, t_size size, abort_callback& abort )
{
    ResetToDefault();

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
            reader.read_object_t( guid, abort );
            reader.read_object( &edgeStyle, sizeof( edgeStyle ), abort );
            properties.LoadBinary( reader, abort );
            reader.skip_object( sizeof( false ), abort ); // HACK: skip over "disable before"
            reader.read_object_t( shouldGrabFocus, abort );
            reader.read_object( &windowPlacement, sizeof( windowPlacement ), abort );
            script = smp::pfc_x::ReadString( reader, abort );
            reader.read_object_t( isPseudoTransparent, abort );
        }
        catch ( const pfc::exception& )
        {
            ResetToDefault();
            FB2K_console_formatter() << "Error: " SMP_NAME_WITH_VERSION " Configuration has been corrupted. All settings have been reset.";
        }
    }
}

void PanelSettings::ResetToDefault()
{
    script = GetDefaultScript();
    isPseudoTransparent = false;
    windowPlacement.length = 0;
    shouldGrabFocus = true;
    edgeStyle = EdgeStyle::NO_EDGE;
    // should not fail
    (void)CoCreateGuid( &guid );
}

void PanelSettings::Save( stream_writer& writer, abort_callback& abort ) const
{
    try
    {
        const auto currentVersion = static_cast<uint32_t>( Version::CONFIG_VERSION_CURRENT );
        writer.write_object_t( currentVersion, abort );
        writer.write_object_t( false, abort ); // HACK: write this in place of "delay load"
        writer.write_object_t( guid, abort );
        writer.write_object( &edgeStyle, sizeof( edgeStyle ), abort );
        properties.SaveBinary( writer, abort );
        writer.write_object_t( false, abort ); // HACK: write this in place of "disable before"
        writer.write_object_t( shouldGrabFocus, abort );
        writer.write_object( &windowPlacement, sizeof( windowPlacement ), abort );
        writer.write_string( script.c_str(), abort );
        writer.write_object_t( isPseudoTransparent, abort );
    }
    catch ( const pfc::exception& )
    {
    }
}

void PanelSettings::SaveDefault( stream_writer& writer, abort_callback& abort )
{
    PanelSettings().Save( writer, abort );
}

} // namespace smp::config
