#include <stdafx.h>

#include "package.h"

#include <config/default_script.h>
#include <config/module_package/module_specifier.h>
#include <config/module_package/package_manager.h>

#include <component_paths.h>

#include <qwr/file_helpers.h>
#include <qwr/semantic_version.h>
#include <qwr/string_helpers.h>

namespace fs = std::filesystem;

namespace smp::config
{

std::filesystem::path ModulePackage::GetEntryFile() const
{
    return ( mainOpt ? *mainOpt : "index.js" );
}

void ModulePackage::ValidatePackagePath() const
{
    qwr::QwrException::ExpectTrue( packageDir.lexically_normal().u8string().ends_with( name ), "Corrupted `package.json`: name and path are mismatched" );
}

void ModulePackage::ToFile( const std::filesystem::path& packageJson )
{
    using json = nlohmann::json;

    try
    {
        try
        {
            VerifyModulePackageName( name );
        }
        catch ( const qwr::QwrException& /*e*/ )
        {
            throw qwr::QwrException( "Corrupted settings: `name`" );
        }

        if ( const auto packageJsonOpt = ModulePackageManager{}.GetPackage( name );
             packageJsonOpt )
        {
            const auto& oldPackageJson = *packageJsonOpt;

            json j = json::parse( qwr::file::ReadFile( oldPackageJson, false ) );
            qwr::QwrException::ExpectTrue( j.is_object(), "Corrupted `package.json`: not a JSON object" );

            j["name"] = name;
            j["author"] = author;
            j["version"] = version.ToString();
            j["description"] = description;

            auto jSmp = json::object();
            jSmp.push_back( { "displayedName", displayedName } );
            jSmp.push_back( { "enableDragDrop", enableDragDrop } );
            jSmp.push_back( { "shouldGrabFocus", shouldGrabFocus } );
            j["fooSpiderMonkeyPanel"] = jSmp;

            qwr::file::WriteFile( oldPackageJson, j.dump( 2 ) );
        }
        else
        {
            const auto packageDir = ModulePackageManager::GetPathForNewPackage( name );

            auto j = json::object();

            j.push_back( { "name", name } );
            j.push_back( { "author", author } );
            j.push_back( { "version", version.ToString() } );
            j.push_back( { "description", description } );

            auto jSmp = json::object();
            jSmp.push_back( { "displayedName", displayedName } );
            jSmp.push_back( { "enableDragDrop", enableDragDrop } );
            jSmp.push_back( { "shouldGrabFocus", shouldGrabFocus } );
            j.push_back( { "fooSpiderMonkeyPanel", jSmp } );

            if ( !fs::exists( packageDir ) )
            {
                fs::create_directories( packageDir );
            }
            qwr::file::WriteFile( packageDir / "package.json", j.dump( 2 ) );
            qwr::file::WriteFile( packageDir / "index.js", config::GetDefaultScript() );
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

ModulePackage ModulePackage::FromFile( const std::filesystem::path& packageJson )
{
    using json = nlohmann::json;

    try
    {
        qwr::QwrException::ExpectTrue( fs::exists( packageJson ), "Package not found: `{}`", packageJson.parent_path().filename().u8string() );

        const json j = json::parse( qwr::file::ReadFile( packageJson, false ) );
        qwr::QwrException::ExpectTrue( j.is_object(), "Corrupted `package.json`: not a JSON object" );

        ModulePackage package;

        // base
        package.name = [&] {
            const auto name = j.at( "name" );
            try
            {
                VerifyModulePackageName( name );
            }
            catch ( const qwr::QwrException& /*e*/ )
            {
                throw qwr::QwrException( "Package is not compatible with `foo_spider_monkey_panel`: `name`" );
            }
            return name;
        }();
        package.author = j.at( "author" );
        package.version = [&] {
            const auto version = j.at( "version" );
            const auto semVerOpt = qwr::SemVer::ParseString( version );
            qwr::QwrException::ExpectTrue( semVerOpt.has_value(), "Corrupted `package.json`: `version`" );
            return *semVerOpt;
        }();
        package.description = j.value( "description", "" );
        if ( j.contains( "main" ) )
        {
            package.mainOpt = j.at( "main" ).get<std::string>();
        }

        // extensions
        package.displayedName = package.name;
        if ( j.contains( "fooSpiderMonkeyPanel" ) )
        {
            const auto jSmp = j.at( "fooSpiderMonkeyPanel" );
            package.displayedName = jSmp.at( "displayedName" );
            package.enableDragDrop = jSmp.value( "enableDragDrop", false );
            package.shouldGrabFocus = jSmp.value( "shouldGrabFocus", true );
        }

        // convenience
        package.scope = qwr::string::Split( package.name, '/' )[0];
        package.packageDir = packageJson.parent_path();
        package.isSample = ( packageJson.parent_path().lexically_normal().wstring().find( path::ModulePackages_Sample().lexically_normal().wstring() ) == 0 );

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

} // namespace smp::config
