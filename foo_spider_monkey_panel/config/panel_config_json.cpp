#include <stdafx.h>

#include "panel_config_json.h"

#include <nlohmann/json.hpp>
#include <qwr/string_helpers.h>
#include <qwr/winapi_error_helpers.h>

namespace
{

enum class ScriptType : uint8_t
{
    SimpleInMemory = 1,
    SimpleSample = 2,
    SimpleFile = 3,
    Package = 4
};

constexpr const char kPropJsonConfigVersion[] = "1";
constexpr const char kPropJsonConfigId[] = "properties";

constexpr const char kSettingsJsonConfigVersion[] = "1";
constexpr const char kSettingsJsonConfigId[] = "settings";

} // namespace

namespace smp::config::json
{

PanelSettings LoadSettings( stream_reader& reader, abort_callback& abort )
{
    using json = nlohmann::json;

    try
    {
        PanelSettings panelSettings;

        const auto jsonMain = json::parse( qwr::pfc_x::ReadString( reader, abort ) );
        if ( !jsonMain.is_object() )
        {
            throw qwr::QwrException( "Corrupted serialized settings: not a JSON object" );
        }

        if ( jsonMain.at( "version" ).get<std::string>() != kSettingsJsonConfigVersion
             || jsonMain.at( "id" ).get<std::string>() != kSettingsJsonConfigId )
        {
            throw qwr::QwrException( "Corrupted serialized settings: version/id mismatch" );
        }

        if ( jsonMain.find( "panelId" ) != jsonMain.end() )
        {
            jsonMain.at( "panelId" ).get_to( panelSettings.id );
        }

        panelSettings.isPseudoTransparent = jsonMain.value( "isPseudoTransparent", false );

        const auto scriptType = jsonMain.at( "scriptType" ).get<ScriptType>();
        const auto jsonPayload = jsonMain.at( "payload" );
        switch ( scriptType )
        {
        case ScriptType::SimpleInMemory:
        {
            panelSettings.payload = PanelSettings_InMemory{ jsonPayload.at( "script" ).get<std::string>() };
            break;
        }
        case ScriptType::SimpleFile:
        {
            panelSettings.payload = PanelSettings_File{ jsonPayload.at( "path" ).get<std::string>() };
            break;
        }
        case ScriptType::SimpleSample:
        {
            panelSettings.payload = PanelSettings_Sample{ jsonPayload.at( "sampleName" ).get<std::string>() };
            break;
        }
        case ScriptType::Package:
        {
            panelSettings.payload = PanelSettings_Package{ jsonPayload.at( "id" ).get<std::string>(),
                                                           jsonPayload.at( "name" ).get<std::string>(),
                                                           jsonPayload.at( "author" ).get<std::string>() };
            break;
        }
        default:
        {
            throw qwr::QwrException( "Corrupted serialized settings: unknown script type" );
        }
        }

        panelSettings.properties = PanelProperties::FromJson( jsonMain.at( "properties" ).get<std::string>() );
        panelSettings.edgeStyle = static_cast<EdgeStyle>( jsonMain.value( "edgeStyle", static_cast<uint8_t>( EdgeStyle::Default ) ) );
        panelSettings.isPseudoTransparent = jsonMain.value( "isPseudoTransparent", false );

        return panelSettings;
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

void SaveSettings( stream_writer& writer, abort_callback& abort, const PanelSettings& settings )
{
    using json = nlohmann::json;

    try
    {
        auto jsonMain = json::object();
        jsonMain.push_back( { "id", kSettingsJsonConfigId } );
        jsonMain.push_back( { "version", kSettingsJsonConfigVersion } );
        jsonMain.push_back( { "panelId", settings.id } );

        json jsonPayload = json::object();
        const auto scriptType = std::visit( [&jsonPayload]( const auto& data ) {
            using T = std::decay_t<decltype( data )>;
            if constexpr ( std::is_same_v<T, smp::config::PanelSettings_InMemory> )
            {
                jsonPayload.push_back( { "script", data.script } );
                return ScriptType::SimpleInMemory;
            }
            else if constexpr ( std::is_same_v<T, smp::config::PanelSettings_File> )
            {
                jsonPayload.push_back( { "path", data.path } );
                return ScriptType::SimpleFile;
            }
            else if constexpr ( std::is_same_v<T, smp::config::PanelSettings_Sample> )
            {
                jsonPayload.push_back( { "sampleName", data.sampleName } );
                return ScriptType::SimpleSample;
            }
            else if constexpr ( std::is_same_v<T, smp::config::PanelSettings_Package> )
            {
                jsonPayload.push_back( { "id", data.id } );
                jsonPayload.push_back( { "name", data.name } );
                jsonPayload.push_back( { "author", data.author } );
                jsonPayload.push_back( { "version", data.version } );
                return ScriptType::Package;
            }
            else
            {
                static_assert( false, "non-exhaustive visitor!" );
            }
        },
                                            settings.payload );

        jsonMain.push_back( { "scriptType", static_cast<uint8_t>( scriptType ) } );
        jsonMain.push_back( { "payload", jsonPayload } );
        jsonMain.push_back( { "properties", settings.properties.ToJson() } );
        jsonMain.push_back( { "edgeStyle", static_cast<uint8_t>( settings.edgeStyle ) } );
        jsonMain.push_back( { "isPseudoTransparent", settings.isPseudoTransparent } );

        qwr::pfc_x::WriteString( writer, jsonMain.dump( 2 ), abort );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( e.what() );
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
        return DeserializeProperties( qwr::pfc_x::ReadString( reader, abort ) );
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
        qwr::pfc_x::WriteString( writer, SerializeProperties( properties ), abort );
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

PanelProperties DeserializeProperties( const std::u8string& str )
{
    using json = nlohmann::json;

    PanelProperties properties;

    try
    {
        json jsonMain = json::parse( str );
        if ( !jsonMain.is_object() )
        {
            throw qwr::QwrException( "Corrupted serialized properties: not a JSON object" );
        }

        if ( jsonMain.at( "version" ).get<std::string>() != kPropJsonConfigVersion
             || jsonMain.at( "id" ).get<std::string>() != kPropJsonConfigId )
        {
            throw qwr::QwrException( "Corrupted serialized properties: version/id mismatch" );
        }

        auto& jsonValues = jsonMain.at( "values" );
        if ( !jsonValues.is_object() )
        {
            throw qwr::QwrException( "Corrupted serialized properties: values" );
        }

        for ( auto& [key, value]: jsonValues.items() )
        {
            if ( key.empty() )
            {
                throw qwr::QwrException( "Corrupted serialized properties: empty key" );
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

            properties.values.emplace( qwr::unicode::ToWide( key ), std::make_shared<mozjs::SerializedJsValue>( serializedValue ) );
        }

        return properties;
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

std::u8string SerializeProperties( const PanelProperties& properties )
{
    using json = nlohmann::json;

    try
    {
        json jsonMain = json::object( { { "id", kPropJsonConfigId },
                                        { "version", kPropJsonConfigVersion } } );

        json jsonValues = json::object();
        for ( const auto& [nameW, pValue]: properties.values )
        {
            const auto propertyName = qwr::unicode::ToU8( nameW );
            const auto& serializedValue = *pValue;

            std::visit( [&jsonValues, &propertyName]( auto&& arg ) {
                jsonValues.push_back( { propertyName, arg } );
            },
                        serializedValue );
        }

        jsonMain.push_back( { "values", jsonValues } );

        return jsonMain.dump( 2 );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

} // namespace smp::config::json
