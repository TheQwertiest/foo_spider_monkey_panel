#include <stdafx.h>

#include "parsed_panel_config.h"

#include <config/package_utils.h>

#include <component_paths.h>

#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>
#include <qwr/type_traits.h>

#include <filesystem>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

namespace
{

using namespace smp;

void Parse_InMemory( const config::PanelSettings_InMemory& settings, config::ParsedPanelSettings& parsedSettings )
{
    parsedSettings.enableDragDrop = settings.enableDragDrop;
    parsedSettings.shouldGrabFocus = settings.shouldGrabFocus;
    parsedSettings.script = settings.script;
}

void Parse_File( const config::PanelSettings_File& settings, config::ParsedPanelSettings& parsedSettings )
{
    try
    {
        parsedSettings.scriptPath = fs::u8path( settings.path );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void Parse_Sample( const config::PanelSettings_Sample& settings, config::ParsedPanelSettings& parsedSettings )
{
    namespace fs = std::filesystem;
    try
    {
        parsedSettings.scriptPath = ( path::ScriptSamples() / settings.sampleName );
        parsedSettings.isSample = true;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void Parse_Package( const config::PanelSettings_Package& settings, config::ParsedPanelSettings& parsedSettings )
{
    using json = nlohmann::json;

    qwr::QwrException::ExpectTrue( !settings.id.empty(), "Corrupted settings (package): `id` is empty" );

    try
    {
        const auto packageDirRet = config::FindPackage( settings.id );
        const auto valueOrEmpty = []( const qwr::u8string& str ) -> qwr::u8string {
            return ( str.empty() ? "<empty>" : str );
        };
        qwr::QwrException::ExpectTrue( packageDirRet.has_value(),
                                       "Can't find the required package: `{} ({} by {})`",
                                       settings.id,
                                       valueOrEmpty( settings.name ),
                                       valueOrEmpty( settings.author ) );

        config::FillPackageSettingsFromPath( *packageDirRet, parsedSettings );
        qwr::QwrException::ExpectTrue( settings.id == parsedSettings.packageId, "Corrupted package: `id` is mismatched with parent folder" );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( "Corrupted `package.json`: {}", e.what() );
    }
}

void Reparse_Package( config::ParsedPanelSettings& parsedSettings )
{
    using json = nlohmann::json;

    assert( parsedSettings.packageId );
    const auto packageId = *parsedSettings.packageId;

    try
    {
        const auto packageDirRet = config::FindPackage( packageId );
        const auto valueOrEmpty = []( const qwr::u8string& str ) -> qwr::u8string {
            return ( str.empty() ? "<empty>" : str );
        };
        qwr::QwrException::ExpectTrue( packageDirRet.has_value(),
                                       "Can't find the required package: `{} ({} by {})`",
                                       packageId,
                                       valueOrEmpty( parsedSettings.scriptName ),
                                       valueOrEmpty( parsedSettings.scriptAuthor ) );

        config::FillPackageSettingsFromPath( *packageDirRet, parsedSettings );
        qwr::QwrException::ExpectTrue( packageId == parsedSettings.packageId, "Corrupted package: `id` is mismatched with parent folder" );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( "Corrupted `package.json`: {}", e.what() );
    }
}

config::PanelSettings_Package GetPayload_Package( const config::ParsedPanelSettings& parsedSettings )
{
    config::PanelSettings_Package payload;

    assert( parsedSettings.packageId );
    payload.id = *parsedSettings.packageId;
    payload.author = parsedSettings.scriptAuthor;
    payload.version = parsedSettings.scriptVersion;
    payload.name = parsedSettings.scriptName;

    return payload;
}

config::PanelSettings_Sample GetPayload_Sample( const config::ParsedPanelSettings& parsedSettings )
{
    config::PanelSettings_Sample payload;

    assert( parsedSettings.scriptPath );
    payload.sampleName = fs::relative( *parsedSettings.scriptPath, path::ScriptSamples() ).u8string();

    return payload;
}

config::PanelSettings_File GetPayload_File( const config::ParsedPanelSettings& parsedSettings )
{
    config::PanelSettings_File payload;

    assert( parsedSettings.scriptPath );
    payload.path = parsedSettings.scriptPath->u8string();

    return payload;
}

config::PanelSettings_InMemory GetPayload_InMemory( const config::ParsedPanelSettings& parsedSettings )
{
    config::PanelSettings_InMemory payload;

    assert( parsedSettings.script );
    payload.script = *parsedSettings.script;
    payload.enableDragDrop = parsedSettings.enableDragDrop;
    payload.shouldGrabFocus = parsedSettings.shouldGrabFocus;

    return payload;
}

} // namespace

namespace smp::config
{

ParsedPanelSettings ParsedPanelSettings::GetDefault()
{
    return Parse( PanelSettings{} );
}

ParsedPanelSettings ParsedPanelSettings::Parse( const PanelSettings& settings )
{
    ParsedPanelSettings parsedSettings;
    parsedSettings.panelId = settings.id;
    parsedSettings.edgeStyle = settings.edgeStyle;
    parsedSettings.isPseudoTransparent = settings.isPseudoTransparent;

    std::visit( [&parsedSettings]( const auto& data ) {
        using T = std::decay_t<decltype( data )>;
        if constexpr ( std::is_same_v<T, smp::config::PanelSettings_InMemory> )
        {
            Parse_InMemory( data, parsedSettings );
        }
        else if constexpr ( std::is_same_v<T, smp::config::PanelSettings_File> )
        {
            Parse_File( data, parsedSettings );
        }
        else if constexpr ( std::is_same_v<T, smp::config::PanelSettings_Sample> )
        {
            Parse_Sample( data, parsedSettings );
        }
        else if constexpr ( std::is_same_v<T, smp::config::PanelSettings_Package> )
        {
            Parse_Package( data, parsedSettings );
        }
        else
        {
            static_assert( qwr::always_false_v<T>, "non-exhaustive visitor!" );
        }
    },
                settings.payload );

    return parsedSettings;
}

config::ParsedPanelSettings ParsedPanelSettings::Reparse( const ParsedPanelSettings& parsedSettings )
{
    auto reparsedSettings = parsedSettings;
    if ( parsedSettings.packageId )
    {
        Reparse_Package( reparsedSettings );
    }
    else
    { // these are set dynamically in script
        reparsedSettings.scriptName.clear();
        reparsedSettings.scriptVersion.clear();
        reparsedSettings.scriptAuthor.clear();
        reparsedSettings.enableDragDrop = false;
        reparsedSettings.shouldGrabFocus = false;
    }

    return reparsedSettings;
}

PanelSettings ParsedPanelSettings::GeneratePanelSettings() const
{
    config::PanelSettings settings;

    settings.id = panelId;
    settings.edgeStyle = edgeStyle;
    settings.isPseudoTransparent = isPseudoTransparent;
    settings.payload = [&]() -> decltype( settings.payload ) {
        switch ( GetSourceType() )
        {
        case ScriptSourceType::Package:
            return GetPayload_Package( *this );
        case ScriptSourceType::Sample:
            return GetPayload_Sample( *this );
        case ScriptSourceType::File:
            return GetPayload_File( *this );
        case ScriptSourceType::InMemory:
            return GetPayload_InMemory( *this );
        default:
            assert( false );
            return PanelSettings_InMemory{};
        }
    }();

    return settings;
}

ScriptSourceType ParsedPanelSettings::GetSourceType() const
{
    if ( packageId )
    {
        return ScriptSourceType::Package;
    }
    else if ( isSample )
    {
        return ScriptSourceType::Sample;
    }
    else if ( scriptPath )
    {
        return ScriptSourceType::File;
    }
    else
    {
        assert( script );
        return ScriptSourceType::InMemory;
    }
}

} // namespace smp::config
