#include <stdafx.h>

#include "package.h"

#include <config/default_script.h>
#include <config/smp_package/package_manager.h>
#include <utils/guid_helpers.h>
#include <utils/path_helpers.h>

#include <component_paths.h>

#include <qwr/file_helpers.h>

namespace fs = std::filesystem;

namespace smp::config
{

fs::path SmpPackage::GetScriptsDir() const
{
    return packageDir / "scripts";
}

fs::path SmpPackage::GetAssetsDir() const
{
    return packageDir / "assets";
}

fs::path SmpPackage::GetStorageDir() const
{
    return path::SmpPackages_Storage() / id;
}

std::vector<fs::path> SmpPackage::GetScriptFiles() const
{
    try
    {
        std::vector<fs::path> files;

        if ( const auto scriptsDir = GetScriptsDir();
             fs::exists( scriptsDir ) )
        {
            files = smp::utils::GetFilesRecursive( scriptsDir );
            ranges::actions::sort( files, []( const auto& a, const auto& b ) { return ( a < b ); } );
        }

        ranges::actions::remove_if( files, [&]( const auto& path ) { return ( path.extension() != ".js" || path == entryFile ); } );
        files.insert( files.begin(), entryFile );
        return files;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

std::vector<fs::path> SmpPackage::GetAllFiles() const
{
    try
    {
        auto files = GetScriptFiles();

        if ( const auto assetsDir = GetAssetsDir();
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

void SmpPackage::ValidatePackagePath() const
{
    qwr::QwrException::ExpectTrue( packageDir.lexically_normal().u8string().ends_with( id ), "Corrupted `package.json`: id and path are mismatched" );
}

void SmpPackage::ToFile( const std::filesystem::path& packageJson ) const
{
    using json = nlohmann::json;

    try
    {
        if ( const auto packageJsonOpt = SmpPackageManager{}.GetPackage( name );
             packageJsonOpt )
        {
            const auto& oldPackageJson = *packageJsonOpt;

            json j = json::parse( qwr::file::ReadFile( oldPackageJson, false ) );
            qwr::QwrException::ExpectTrue( j.is_object(), "Corrupted `package.json`: not a JSON object" );

            j["name"] = name;
            j["author"] = author;
            j["version"] = version;
            j["description"] = description;
            j["enableDragDrop"] = enableDragDrop;
            j["shouldGrabFocus"] = shouldGrabFocus;

            qwr::file::WriteFile( oldPackageJson, j.dump( 2 ) );
        }
        else
        {
            const auto packageDir = SmpPackageManager::GetPathForNewPackage( id );

            auto j = json::object();

            j.push_back( { "id", id } );
            j.push_back( { "name", name } );
            j.push_back( { "author", author } );
            j.push_back( { "version", version } );
            j.push_back( { "description", description } );
            j.push_back( { "enableDragDrop", enableDragDrop } );
            j.push_back( { "shouldGrabFocus", shouldGrabFocus } );

            if ( !fs::exists( packageDir ) )
            {
                fs::create_directories( packageDir );
            }
            qwr::file::WriteFile( packageDir / "package.json", j.dump( 2 ) );
            qwr::file::WriteFile( packageDir / "main.js", config::GetDefaultScript() );
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

SmpPackage SmpPackage::FromFile( const std::filesystem::path& packageJson )
{
    using json = nlohmann::json;

    try
    {
        qwr::QwrException::ExpectTrue( fs::exists( packageJson ), "Package not found: `{}`", packageJson.parent_path().filename().u8string() );

        const json j = json::parse( qwr::file::ReadFile( packageJson, false ) );
        qwr::QwrException::ExpectTrue( j.is_object(), "Corrupted `package.json`: not a JSON object" );

        SmpPackage package;

        package.id = j.at( "id" );
        package.name = j.at( "name" );
        package.author = j.at( "author" );
        package.version = j.at( "version" );
        package.description = j.value( "description", "" );
        package.enableDragDrop = j.value( "enableDragDrop", false );
        package.shouldGrabFocus = j.value( "shouldGrabFocus", true );

        package.entryFile = packageJson.parent_path() / "main.js";
        package.packageDir = packageJson.parent_path();

        return package;
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

SmpPackage SmpPackage::GenerateNewPackage( const qwr::u8string& name )
{
    SmpPackageManager packageManager;
    packageManager.Refresh();
    const auto& packages = packageManager.GetPackages();

    qwr::u8string id;
    do
    {
        const auto guidStr = utils::GuidToStr( utils::GenerateGuid() );
        id = qwr::unicode::ToU8( guidStr );
    } while ( packages.count( id ) );

    const auto packageDir = packageManager.GetPathForNewPackage( id );

    SmpPackage package;
    package.id = id;
    package.name = name;
    package.entryFile = packageDir / "main.js";
    package.packageDir = packageDir;
    return package;
}

} // namespace smp::config
