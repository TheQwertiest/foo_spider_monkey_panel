#include <stdafx.h>

#include "package_utils.h"

#include <component_paths.h>

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
        parsedSettings.isSample = ( packageDir.parent_path() == path::Packages_Sample() );

        const json jsonMain = json::parse( qwr::file::ReadFile( packageJsonFile, false ) );
        qwr::QwrException::ExpectTrue( jsonMain.is_object(), "Corrupted `package.json`: not a JSON object" );

        parsedSettings.packageId = jsonMain.at( "id" ).get<std::string>();
        parsedSettings.scriptName = jsonMain.at( "name" ).get<std::string>();
        parsedSettings.scriptAuthor = jsonMain.at( "author" ).get<std::string>();
        parsedSettings.scriptVersion = jsonMain.at( "version" ).get<std::string>();
        parsedSettings.scriptDescription = jsonMain.value( "description", std::string() );
        parsedSettings.enableDragDrop = jsonMain.value( "enableDragDrop", false );
        parsedSettings.shouldGrabFocus = jsonMain.value( "shouldGrabFocus", true );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
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

        const auto packageDirRet = config::FindPackage( *parsedSettings.packageId );
        const auto packagePath = [&] {
            if ( packageDirRet )
            {
                return *packageDirRet;
            }
            else
            {
                return GetPackagePath( parsedSettings );
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
        throw qwr::QwrException( e );
    }
    catch ( const json::exception& e )
    {
        throw qwr::QwrException( fmt::format( "Corrupted settings: {}", e.what() ) );
    }
}

} // namespace

namespace smp::config
{

ParsedPanelSettings GetNewPackageSettings( const std::u8string& name )
{
    ParsedPanelSettings settings;

    try
    {
        fs::path packagePath;
        std::u8string id;
        do
        {
            GUID guid;
            (void)CoCreateGuid( &guid ); //< should not fail
            std::wstring guidStr;
            guidStr.resize( 64 );
            StringFromGUID2( guid, guidStr.data(), guidStr.size() );
            guidStr.resize( wcslen( guidStr.c_str() ) );
            id = qwr::unicode::ToU8( guidStr );
            packagePath = path::Packages_Profile() / id;
        } while ( fs::exists( packagePath ) );

        settings.packageId = id;
        settings.scriptName = name;
        settings.scriptPath = ( packagePath / "main.js" ).u8string();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }

    return settings;
}

std::optional<std::filesystem::path> FindPackage( const std::u8string& packageId )
{

    try
    {
        for ( const auto& path: { path::Packages_Sample(),
                                  path::Packages_Profile(),
                                  path::Packages_Foobar2000() } )
        {
            const auto targetPath = path / packageId;
            if ( fs::exists( targetPath ) && fs::is_directory( targetPath ) )
            {
                return targetPath;
            }
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }

    return std::nullopt;
}

ParsedPanelSettings GetPackageSettingsFromPath( const std::filesystem::path& packagePath )
{
    ParsedPanelSettings settings{};
    Parse_PackageFromPath( packagePath, settings );
    return settings;
}

void FillPackageSettingsFromPath( const std::filesystem::path& packagePath, ParsedPanelSettings& settings )
{
    Parse_PackageFromPath( packagePath, settings );
}

void MaybeSavePackageData( const ParsedPanelSettings& settings )
{
    if ( settings.GetSourceType() == ScriptSourceType::Package )
    {
        Save_PackageData( settings );
    }
}

std::filesystem::path GetPackagePath( const ParsedPanelSettings& settings )
{
    assert( settings.scriptPath );
    return settings.scriptPath->parent_path();
}

std::filesystem::path GetPackageScriptsDir( const ParsedPanelSettings& settings )
{
    return GetPackagePath( settings ) / "scripts";
}

std::vector<std::filesystem::path> GetPackageScriptFiles( const ParsedPanelSettings& settings )
{
    try
    {
        std::vector<std::filesystem::path> files;

        const auto packagePath = GetPackagePath( settings );

        assert( settings.scriptPath );
        const auto mainScript = *settings.scriptPath;

        files.emplace_back( mainScript );

        if ( const auto scriptsDir = packagePath / "scripts";
             fs::exists( scriptsDir ) )
        {
            for ( const auto it: fs::recursive_directory_iterator( scriptsDir ) )
            {
                if ( it.is_directory() )
                {
                    continue;
                }

                if ( it.path().extension() == ".js" && it.path() != mainScript )
                {
                    files.emplace_back( it.path() );
                }
            }
        }

        return files;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

std::vector<std::filesystem::path> GetPackageFiles( const ParsedPanelSettings& settings )
{
    try
    {
        auto files = GetPackageScriptFiles( settings );

        const auto packagePath = GetPackagePath( settings );

        if ( const auto assetsDir = packagePath / "assets";
             fs::exists( assetsDir ) )
        {
            for ( const auto it: fs::recursive_directory_iterator( assetsDir ) )
            {
                if ( it.is_directory() )
                {
                    continue;
                }

                files.emplace_back( it.path() );
            }
        }

        return files;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace smp::config
