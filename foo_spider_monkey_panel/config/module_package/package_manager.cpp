#include <stdafx.h>

#include "package_manager.h"

#include <config/module_package/module_specifier.h>

#include <component_paths.h>

namespace fs = std::filesystem;

namespace smp::config
{

std::vector<std::filesystem::path> ModulePackageManager::GetPackages( const std::optional<std::filesystem::path>& parentDir ) const
{
    // TODO
    return {};
}

std::optional<std::filesystem::path> ModulePackageManager::GetPackage( const qwr::u8string& name ) const
{
    try
    {
        VerifyModulePackageName( name );

        for ( const auto& path: { path::SmpPackages_Sample(),
                                  path::SmpPackages_Profile(),
                                  path::SmpPackages_Foobar2000() } )
        {
            const auto targetPath = path / name / "package.json";
            if ( fs::exists( targetPath ) && fs::is_regular_file( targetPath ) )
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

void ModulePackageManager::Refresh()
{
    // TODO
}

std::filesystem::path ModulePackageManager::GetPathForNewPackage( const qwr::u8string& name )
{
    assert( !name.empty() );
    return path::ModulePackages_Profile() / name;
}

bool ModulePackageManager::IsSamplePackage( const std::filesystem::path& packageJson )
{
    return ( packageJson.parent_path() == path::ModulePackages_Sample() );
}

} // namespace smp::config
