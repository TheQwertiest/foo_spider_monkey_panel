#include <stdafx.h>

#include "parsed_panel_config.h"

#include <nlohmann/json.hpp>
#include <qwr/fb2k_paths.h>
#include <qwr/file_helpers.h>

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
        throw qwr::QwrException( e.what() );
    }
}

void Parse_Sample( const config::PanelSettings_Sample& settings, config::ParsedPanelSettings& parsedSettings )
{
    namespace fs = std::filesystem;
    try
    {
        parsedSettings.scriptPath = ( qwr::path::Component() / "samples" / settings.sampleName ).u8string();
        parsedSettings.isSample = true;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

std::optional<std::filesystem::path> FindPackage( const std::u8string& packageId )
{
    for ( const auto& path: { qwr::path::Component() / "samples" / "packages",
                              qwr::path::Profile() / SMP_UNDERSCORE_NAME / "packages",
                              qwr::path::Foobar2000() / SMP_UNDERSCORE_NAME / "packages" } )
    {
        const auto targetPath = path / packageId;
        if ( fs::exists( targetPath ) && fs::is_directory( targetPath ) )
        {
            return targetPath;
        }
    }
    return std::nullopt;
}

void Parse_PackageFromPath( const std::filesystem::path& packageDir, config::ParsedPanelSettings& parsedSettings )
{
    using json = nlohmann::json;

    try
    {
        const auto valueOrEmpty = []( const std::u8string& str ) -> std::u8string {
            return ( str.empty() ? "<empty>" : str );
        };
        qwr::QwrException::ExpectTrue( fs::exists( packageDir ),
                                       "Can't find the required package: `{}'",
                                       packageDir.u8string() );

        const auto packageJsonFile = packageDir / "package.json";
        qwr::QwrException::ExpectTrue( fs::exists( packageJsonFile ), "Corrupted package: can't find `package.json`" );

        parsedSettings.scriptPath = ( packageDir / "main.js" ).u8string();
        parsedSettings.isSample = ( packageDir.parent_path() == qwr::path::Component() / "samples" / "packages" );

        const json jsonMain = json::parse( qwr::file::ReadFile( packageJsonFile.u8string(), false ) );
        qwr::QwrException::ExpectTrue( jsonMain.is_object(), "Corrupted `package.json`: not a JSON object" );

        parsedSettings.packageId = jsonMain.at( "id" ).get<std::string>();
        parsedSettings.scriptName = jsonMain.at( "name" ).get<std::string>();
        parsedSettings.scriptAuthor = jsonMain.at( "author" ).get<std::string>();
        parsedSettings.scriptVersion = jsonMain.at( "version" ).get<std::string>();
        parsedSettings.scriptDescription = jsonMain.value( "description", std::string() );
        parsedSettings.enableDragDrop = jsonMain.value( "enableDragDrop", false );
        parsedSettings.shouldGrabFocus = jsonMain.value( "shouldGrabFocus", true );

        if ( jsonMain.find( "menuActions" ) != jsonMain.end() )
        {
            const auto menuActionsJson = jsonMain.at( "menuActions" );
            qwr::QwrException::ExpectTrue( menuActionsJson.is_object(), "Corrupted `package.json`: `menuActions` is not a JSON object" );

            config::ParsedPanelSettings::MenuActions menuActions;
            for ( const auto& [key, value]: menuActionsJson.items() )
            {
                qwr::QwrException::ExpectTrue( !key.empty(), "Corrupted `package.json`: empty key in `menuActions`" );
                menuActions.emplace_back( key, value.empty() ? std::string() : value.get<std::string>() );
            }

            parsedSettings.menuActions = menuActions;
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e.what() );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( fmt::format( "Corrupted `package.json`: {}", e.what() ) );
    }
}

void Parse_Package( const config::PanelSettings_Package& settings, config::ParsedPanelSettings& parsedSettings )
{
    using json = nlohmann::json;

    qwr::QwrException::ExpectTrue( !settings.id.empty(), "Corrupted settings (package): `id` is empty" );

    try
    {
        const auto packageDirRet = FindPackage( settings.id );
        const auto valueOrEmpty = []( const std::u8string& str ) -> std::u8string {
            return ( str.empty() ? "<empty>" : str );
        };
        qwr::QwrException::ExpectTrue( packageDirRet.has_value(),
                                       "Can't find the required package: `{} ({} by {})`",
                                       settings.id,
                                       valueOrEmpty( settings.name ),
                                       valueOrEmpty( settings.author ) );

        Parse_PackageFromPath( *packageDirRet, parsedSettings );
        qwr::QwrException::ExpectTrue( settings.id == parsedSettings.packageId, "Corrupted package: `id` is mismatched with parent folder" );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e.what() );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( fmt::format( "Corrupted `package.json`: {}", e.what() ) );
    }
}

void Reparse_Package( config::ParsedPanelSettings& parsedSettings )
{
    using json = nlohmann::json;

    assert( parsedSettings.packageId );
    const auto packageId = *parsedSettings.packageId;

    try
    {
        const auto packageDirRet = FindPackage( packageId );
        const auto valueOrEmpty = []( const std::u8string& str ) -> std::u8string {
            return ( str.empty() ? "<empty>" : str );
        };
        qwr::QwrException::ExpectTrue( packageDirRet.has_value(),
                                       "Can't find the required package: `{} ({} by {})`",
                                       packageId,
                                       valueOrEmpty( parsedSettings.scriptName ),
                                       valueOrEmpty( parsedSettings.scriptAuthor ) );

        Parse_PackageFromPath( *packageDirRet, parsedSettings );
        qwr::QwrException::ExpectTrue( packageId == parsedSettings.packageId, "Corrupted package: `id` is mismatched with parent folder" );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e.what() );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( fmt::format( "Corrupted `package.json`: {}", e.what() ) );
    }
}

void Save_PackageData( const config::ParsedPanelSettings& parsedSettings )
{
    namespace fs = std::filesystem;
    using json = nlohmann::json;

    assert( parsedSettings.scriptPath );
    qwr::QwrException::ExpectTrue( !parsedSettings.scriptPath->empty(), "Corrupted settings: `scriptPath` is empty" );

    try
    {
        auto jsonMain = json::object();

        assert( parsedSettings.packageId );
        qwr::QwrException::ExpectTrue( !parsedSettings.packageId->empty(), "Corrupted settings: `id` is empty" );

        jsonMain.push_back( { "id", *parsedSettings.packageId } );
        jsonMain.push_back( { "name", parsedSettings.scriptName } );
        jsonMain.push_back( { "author", parsedSettings.scriptAuthor } );
        jsonMain.push_back( { "version", parsedSettings.scriptVersion } );
        jsonMain.push_back( { "description", parsedSettings.scriptDescription } );
        jsonMain.push_back( { "enableDragDrop", parsedSettings.enableDragDrop } );
        jsonMain.push_back( { "shouldGrabFocus", parsedSettings.shouldGrabFocus } );

        if ( !parsedSettings.menuActions.empty() )
        {
            json menuActionsJson = json::object();
            for ( const auto& [id, desc]: parsedSettings.menuActions )
            {
                menuActionsJson.push_back( { id, desc } );
            }

            jsonMain.push_back( { "menuActions", menuActionsJson } );
        }

        const auto packageDirRet = FindPackage( *parsedSettings.packageId );
        const auto packagePath = [&] {
            if ( packageDirRet )
            {
                return *packageDirRet;
            }
            else
            {
                return qwr::path::Profile() / SMP_UNDERSCORE_NAME / "packages";
            }
        }();

        if ( !fs::exists( packagePath ) )
        {
            fs::create_directories( packagePath );
        }

        const auto packageJsonFile = packagePath / L"package.json";
        qwr::file::WriteFile( packageJsonFile, jsonMain.dump( 2 ) );

        const auto mainScriptPath = packagePath / L"main.js";
        if ( !fs::exists( mainScriptPath ) )
        {
            qwr::file::WriteFile( mainScriptPath, config::PanelSettings_InMemory::GetDefaultScript() );
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e.what() );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( fmt::format( "Corrupted settings: {}", e.what() ) );
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
    payload.sampleName = parsedSettings.scriptPath->filename().u8string();

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
            static_assert( false, "non-exhaustive visitor!" );
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
    }

    return reparsedSettings;
}

PanelSettings ParsedPanelSettings::GeneratePanelSettings() const
{
    config::PanelSettings settings;

    settings.id = panelId;
    settings.edgeStyle = edgeStyle;
    settings.isPseudoTransparent = isPseudoTransparent;

    if ( packageId )
    {
        Save_PackageData( *this );
        settings.payload = GetPayload_Package( *this );
    }
    else if ( isSample )
    {
        settings.payload = GetPayload_Sample( *this );
    }
    else if ( scriptPath )
    {
        settings.payload = GetPayload_File( *this );
    }
    else
    {
        assert( script );
        settings.payload = GetPayload_InMemory( *this );
    }

    return settings;
}

} // namespace smp::config
