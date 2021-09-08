#include <stdafx.h>

#include "package_utils.h"

#include <utils/guid_helpers.h>
#include <utils/path_helpers.h>
#include <utils/relative_filepath_trie.h>

#include <component_paths.h>

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
        qwr::QwrException::ExpectTrue( fs::exists( packageDir ),
                                       "Can't find the required package: `{}`",
                                       packageDir.u8string() );

        const auto packageJsonFile = packageDir / "package.json";
        qwr::QwrException::ExpectTrue( fs::exists( packageJsonFile ), "Corrupted package: can't find `package.json`" );

        parsedSettings.scriptPath = ( packageDir / config::GetRelativePathToMainFile() );
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
        throw qwr::QwrException( "Corrupted `package.json`: {}", e.what() );
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

        const auto mainScriptPath = packagePath / config::GetRelativePathToMainFile();
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
        throw qwr::QwrException( "Corrupted settings: {}", e.what() );
    }
}

} // namespace

namespace smp::config
{

const fs::path& GetRelativePathToMainFile()
{
    static const fs::path main{ "main.js" };
    return main;
}

ParsedPanelSettings GetNewPackageSettings( const qwr::u8string& name )
{
    ParsedPanelSettings settings;

    try
    {
        fs::path packagePath;
        qwr::u8string id;
        do
        {
            const auto guidStr = utils::GuidToStr( utils::GenerateGuid() );
            id = qwr::unicode::ToU8( guidStr );
            packagePath = path::Packages_Profile() / id;
        } while ( fs::exists( packagePath ) );

        settings.packageId = id;
        settings.scriptName = name;
        settings.scriptPath = ( packagePath / GetRelativePathToMainFile() );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }

    return settings;
}

std::optional<std::filesystem::path> FindPackage( const qwr::u8string& packageId )
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

std::filesystem::path GetPackageAssetsDir( const ParsedPanelSettings& settings )
{
    return GetPackagePath( settings ) / "assets";
}

std::filesystem::path GetPackageStorageDir( const ParsedPanelSettings& settings )
{
    assert( settings.packageId );
    return path::Packages_Storage() / *settings.packageId;
}

std::vector<std::filesystem::path> GetPackageScriptFiles( const ParsedPanelSettings& settings )
{
    try
    {
        std::vector<fs::path> files;

        const auto packagePath = GetPackagePath( settings );

        if ( const auto scriptsDir = packagePath / "scripts";
             fs::exists( scriptsDir ) )
        {
            files = smp::utils::GetFilesRecursive( scriptsDir );
            ranges::actions::sort( files, []( const auto& a, const auto& b ) { return ( a < b ); } );
        }

        assert( settings.scriptPath );
        const auto mainScript = *settings.scriptPath;

        ranges::actions::remove_if( files, [&mainScript]( const auto& path ) { return ( path.extension() != ".js" || path == mainScript ); } );
        files.insert( files.begin(), mainScript );
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
            auto assetFiles = smp::utils::GetFilesRecursive( assetsDir );
            ranges::actions::sort( files, []( const auto& a, const auto& b ) { return ( a < b ); } );

            ranges::actions::push_back( files, std::move( assetFiles ) );
        }

        return files;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace smp::config
