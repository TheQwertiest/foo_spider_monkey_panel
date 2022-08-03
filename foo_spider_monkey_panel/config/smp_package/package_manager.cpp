#include <stdafx.h>

#include "package_manager.h"

#include <config/module_package/module_specifier.h>

#include <component_paths.h>

#include <qwr/fb2k_paths.h>

namespace fs = std::filesystem;

namespace smp::config
{

const std::unordered_map<qwr::u8string, std::filesystem::path>&
SmpPackageManager::GetPackages( const std::optional<std::filesystem::path>& parentDir ) const
{
    if ( idToPackageJson_.empty() )
    {
        Refresh();
    }

    return idToPackageJson_;
}

std::optional<std::filesystem::path>
SmpPackageManager::GetPackage( const qwr::u8string& id ) const
{
    try
    {
        bool hasRefreshed = false;
        while ( true )
        {
            for ( const auto& path: { path::SmpPackages_Sample(),
                                      path::SmpPackages_Profile(),
                                      path::SmpPackages_Foobar2000() } )
            {
                const auto targetPath = path / id / "package.json";
                if ( fs::exists( targetPath ) && fs::is_regular_file( targetPath ) )
                {
                    return targetPath;
                }
            }

            if ( hasRefreshed )
            {
                break;
            }

            Refresh();
            hasRefreshed = true;
        }

        return std::nullopt;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void SmpPackageManager::Refresh() const
{
    try
    {
        std::vector<fs::path> packagesDirs{ path::SmpPackages_Profile(),
                                            path::SmpPackages_Sample() };
        if ( qwr::path::Profile() != qwr::path::Foobar2000() )
        { // these paths might be the same when fb2k is in portable mode
            packagesDirs.emplace_back( path::SmpPackages_Foobar2000() );
        }

        for ( const auto& packagesDir: packagesDirs )
        {
            if ( !fs::exists( packagesDir ) )
            {
                continue;
            }

            for ( const auto& dirIt: fs::directory_iterator( packagesDir ) )
            {
                const auto packageDir = dirIt.path();
                const auto packageJson = packageDir / L"package.json";
                if ( !fs::exists( packageJson ) || !fs::is_regular_file( packageJson ) )
                {
                    continue;
                }

                idToPackageJson_.try_emplace( packageDir.filename().u8string(), packageJson );
            }
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

std::filesystem::path SmpPackageManager::GetPathForNewPackage( const qwr::u8string& id )
{
    assert( !id.empty() );
    return path::SmpPackages_Profile() / id;
}

std::unordered_map<qwr::u8string, std::filesystem::path> SmpPackageManager::idToPackageJson_;

} // namespace smp::config
