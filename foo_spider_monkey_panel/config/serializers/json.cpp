#include <stdafx.h>

#include "json.h"

#include <component_paths.h>

#include <qwr/fb2k_paths.h>
#include <qwr/string_helpers.h>
#include <qwr/visitor.h>
#include <qwr/winapi_error_helpers.h>

#include <filesystem>

using namespace smp;
namespace fs = std::filesystem;

namespace
{

enum class ScriptType : uint8_t
{
    SimpleInMemory = 1,
    SimpleSample = 2,
    SimpleFile = 3,
    SmpPackage = 4,
    ModulePackage = 5
};

enum class LocationType : uint8_t
{
    Full = 0,
    Component = 1,
    Profile = 2,
    Fb2k = 3
};

constexpr const char kPropJsonConfigVersion[] = "1";
constexpr const char kPropJsonConfigId[] = "properties";

constexpr const char kSettingsJsonConfigVersion[] = "1";
constexpr const char kSettingsJsonConfigId[] = "settings";

} // namespace

namespace
{

nlohmann::json SerializePropertiesToObject( const config::PanelProperties& properties )
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

            std::visit( [&jsonValues, &propertyName]( auto&& arg ) { jsonValues.push_back( { propertyName, arg } ); },
                        serializedValue );
        }

        jsonMain.push_back( { "values", jsonValues } );

        return jsonMain;
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

config::PanelProperties DeserializePropertiesFromObject( const nlohmann::json& jsonMain )
{
    using json = nlohmann::json;

    try
    {
        config::PanelProperties properties;

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
}

} // namespace

namespace smp::config::json
{

PanelConfig LoadConfig( stream_reader& reader, abort_callback& abort )
{
    using json = nlohmann::json;

    try
    {

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

        PanelSettings panelSettings;
        if ( jsonMain.find( "panelId" ) != jsonMain.end() )
        {
            jsonMain.at( "panelId" ).get_to( panelSettings.id );
        }
        panelSettings.isPseudoTransparent = jsonMain.value( "isPseudoTransparent", false );
        panelSettings.edgeStyle = static_cast<EdgeStyle>( jsonMain.value( "edgeStyle", static_cast<uint8_t>( EdgeStyle::Default ) ) );

        RawScriptSourceVariant sourceVariant;
        const auto scriptType = jsonMain.at( "scriptType" ).get<ScriptType>();
        const auto jsonPayload = jsonMain.at( "payload" );
        switch ( scriptType )
        {
        case ScriptType::SimpleInMemory:
        {
            RawInMemoryScript source;
            source.script = jsonPayload.at( "script" );
            source.isModule = jsonPayload.value( "isModuleScript", false );

            sourceVariant = std::move( source );
            break;
        }
        case ScriptType::SimpleFile:
        {
            const auto path = fs::u8path( jsonPayload.at( "path" ).get<std::string>() ).lexically_normal();
            const auto fullPath = [&] {
                switch ( jsonPayload.at( "locationType" ).get<LocationType>() )
                {
                case LocationType::Component:
                {
                    return ( qwr::path::Component() / path );
                }
                case LocationType::Fb2k:
                {
                    return ( qwr::path::Foobar2000() / path );
                }
                case LocationType::Profile:
                {
                    return ( qwr::path::Profile() / path );
                }
                case LocationType::Full:
                {
                    return path;
                }
                default:
                    throw qwr::QwrException( "Corrupted serialized settings: unknown file location type" );
                }
            }();

            RawScriptFile source;
            source.scriptPath = fullPath;
            source.isModule = jsonPayload.value( "isModuleScript", false );

            sourceVariant = std::move( source );
            break;
        }
        case ScriptType::SimpleSample:
        {
            RawSampleFile source;
            source.name = jsonPayload.at( "sampleName" );
            source.isModule = jsonPayload.value( "isModuleScript", false );

            sourceVariant = std::move( source );
            break;
        }
        case ScriptType::SmpPackage:
        {
            RawSmpPackage source;
            source.id = jsonPayload.at( "id" );
            source.name = jsonPayload.at( "name" );
            source.author = jsonPayload.at( "author" );

            sourceVariant = std::move( source );
            break;
        }
        case ScriptType::ModulePackage:
        {
            RawModulePackage source;
            source.name = jsonPayload.at( "name" );

            sourceVariant = std::move( source );
            break;
        }
        default:
        {
            throw qwr::QwrException( "Corrupted serialized settings: unknown script type" );
        }
        }

        auto properties = DeserializePropertiesFromObject( jsonMain.at( "properties" ) );

        PanelConfig panelConfig;
        panelConfig.panelSettings = std::move( panelSettings );
        panelConfig.properties = std::move( properties );
        panelConfig.scriptSource = std::move( sourceVariant );
        return panelConfig;
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
    catch ( const pfc::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

void SaveConfig( stream_writer& writer, abort_callback& abort, const PanelConfig& config )
{
    using json = nlohmann::json;

    try
    {
        auto jsonMain = json::object();
        jsonMain.push_back( { "id", kSettingsJsonConfigId } );
        jsonMain.push_back( { "version", kSettingsJsonConfigVersion } );
        jsonMain.push_back( { "panelId", config.panelSettings.id } );

        bool isModule = false;
        json jsonPayload = json::object();
        const auto scriptType = std::visit(
            qwr::Visitor{
                [&]( const RawInMemoryScript& arg ) {
                    jsonPayload.push_back( { "script", arg.script } );
                    jsonPayload.push_back( { "isModuleScript", arg.isModule } );
                    return ScriptType::SimpleInMemory;
                },
                [&]( const RawScriptFile& arg ) {
                    const auto [path, locationType] = [&path = arg.scriptPath] {
                        try
                        {
                            const auto fsPath = path.lexically_normal();
                            const auto isSubpath = []( const auto& path, const auto& base ) {
                                return ( path.wstring().find( base.lexically_normal().wstring() ) == 0 );
                            };

                            if ( isSubpath( fsPath, qwr::path::Component() ) )
                            {
                                return std::make_tuple( fs::relative( fsPath, qwr::path::Component() ).u8string(), LocationType::Component );
                            }
                            if ( isSubpath( fsPath, qwr::path::Profile() ) )
                            {
                                return std::make_tuple( fs::relative( fsPath, qwr::path::Profile() ).u8string(), LocationType::Profile );
                            }
                            if ( isSubpath( fsPath, qwr::path::Foobar2000() ) )
                            {
                                return std::make_tuple( fs::relative( fsPath, qwr::path::Foobar2000() ).u8string(), LocationType::Fb2k );
                            }

                            return std::make_tuple( fsPath.u8string(), LocationType::Full );
                        }
                        catch ( const fs::filesystem_error& e )
                        {
                            throw qwr::QwrException( e );
                        }
                    }();

                    jsonPayload.push_back( { "path", path } );
                    jsonPayload.push_back( { "locationType", locationType } );
                    jsonPayload.push_back( { "isModuleScript", arg.isModule } );
                    return ScriptType::SimpleFile;
                },
                [&]( const RawSampleFile& arg ) {
                    jsonPayload.push_back( { "sampleName", arg.name } );
                    jsonPayload.push_back( { "isModuleScript", arg.isModule } );
                    return ScriptType::SimpleSample;
                },
                [&]( const RawSmpPackage& arg ) {
                    jsonPayload.push_back( { "id", arg.id } );
                    jsonPayload.push_back( { "name", arg.name } );
                    jsonPayload.push_back( { "author", arg.author } );
                    return ScriptType::SmpPackage;
                },
                [&]( const RawModulePackage& arg ) {
                    jsonPayload.push_back( { "name", arg.name } );
                    return ScriptType::ModulePackage;
                } },
            config.scriptSource );

        jsonMain.push_back( { "scriptType", static_cast<uint8_t>( scriptType ) } );
        jsonMain.push_back( { "payload", jsonPayload } );

        jsonMain.push_back( { "properties", SerializePropertiesToObject( config.properties ) } );

        jsonMain.push_back( { "edgeStyle", static_cast<uint8_t>( config.panelSettings.edgeStyle ) } );
        jsonMain.push_back( { "isPseudoTransparent", config.panelSettings.isPseudoTransparent } );

        qwr::pfc_x::WriteString( writer, jsonMain.dump( 2 ), abort );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
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

PanelProperties DeserializeProperties( const qwr::u8string& str )
{
    using json = nlohmann::json;

    try
    {
        return DeserializePropertiesFromObject( json::parse( str ) );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

qwr::u8string SerializeProperties( const PanelProperties& properties )
{
    using json = nlohmann::json;

    try
    {
        return SerializePropertiesToObject( properties ).dump( 2 );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

} // namespace smp::config::json
