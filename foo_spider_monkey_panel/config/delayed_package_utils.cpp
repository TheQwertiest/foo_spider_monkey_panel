#include <stdafx.h>

#include "delayed_package_utils.h"

#include <component_paths.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace smp::config
{

bool IsPackageInUse( const qwr::u8string& packageId )
{
    try
    {
        return fs::exists( path::TempFolder_PackagesInUse() / packageId );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

PackageDelayStatus GetPackageDelayStatus( const qwr::u8string& packageId )
{
    try
    {
        if ( fs::exists( path::TempFolder_PackagesToRemove() / packageId ) )
        {
            return PackageDelayStatus::ToBeRemoved;
        }
        else if ( const auto packagePath = path::TempFolder_PackagesToInstall() / packageId;
                  fs::exists( packagePath ) && fs::is_directory( packagePath ) )
        {
            return PackageDelayStatus::ToBeUpdated;
        }
        else
        {
            return PackageDelayStatus::NotDelayed;
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void ClearPackageDelayStatus( const qwr::u8string& packageId )
{
    try
    {
        for ( const auto& path: { path::TempFolder_PackagesToRemove() / packageId, path::TempFolder_PackagesToInstall() / packageId } )
        {
            fs::remove_all( path );
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void MarkPackageAsToBeRemoved( const qwr::u8string& packageId )
{
    ClearPackageDelayStatus( packageId );
    try
    {
        const auto path = path::TempFolder_PackagesToRemove() / packageId;

        fs::create_directories( path.parent_path() );
        std::ofstream f( path );
        f.close();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void MarkPackageAsToBeInstalled( const qwr::u8string& packageId, const std::filesystem::path& packageContent )
{
    ClearPackageDelayStatus( packageId );
    try
    {
        const auto path = path::TempFolder_PackagesToInstall() / packageId;

        fs::create_directories( path );
        fs::copy( packageContent, path, fs::copy_options::recursive );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void MarkPackageAsInUse( const qwr::u8string& packageId )
{
    try
    {
        const auto path = path::TempFolder_PackagesInUse() / packageId;

        fs::create_directories( path.parent_path() );
        std::ofstream f( path );
        f.close();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void ProcessDelayedPackagesOnce()
{
    static bool wasCalled = false;
    if ( wasCalled )
    {
        return;
    }

    try
    {
        fs::remove_all( path::TempFolder_PackagesInUse() );
        []() {
            const auto packagesToProcessDir = path::TempFolder_PackagesToInstall();
            if ( !fs::exists( packagesToProcessDir ) || !fs::is_directory( packagesToProcessDir ) )
            {
                fs::remove_all( packagesToProcessDir );
                return;
            }

            const auto packageDirPath = path::Packages_Profile();

            for ( const auto& packageContent: fs::directory_iterator( packagesToProcessDir ) )
            {
                if ( !fs::is_directory( packagesToProcessDir ) )
                {
                    continue;
                }

                const auto packageId = packageContent.path().filename().wstring();
                const auto packagePath = packageDirPath / packageId;
                fs::remove_all( packagePath );
                fs::create_directories( packagePath );
                fs::copy( packageContent, packagePath, fs::copy_options::recursive );

                ClearPackageDelayStatus( qwr::unicode::ToU8( packageId ) );
            }

            fs::remove_all( packagesToProcessDir );
        }();
        []() {
            const auto packagesToProcessDir = path::TempFolder_PackagesToRemove();
            if ( !fs::exists( packagesToProcessDir ) || !fs::is_directory( packagesToProcessDir ) )
            {
                fs::remove_all( packagesToProcessDir );
                return;
            }

            const auto packagePath = path::Packages_Profile();

            for ( const auto& packageContent: fs::directory_iterator( packagesToProcessDir ) )
            {
                const auto packageId = packageContent.path().filename().wstring();
                fs::remove_all( packagePath / packageId );

                ClearPackageDelayStatus( qwr::unicode::ToU8( packageId ) );
            }

            fs::remove_all( packagesToProcessDir );
        }();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }

    wasCalled = true;
}

} // namespace smp::config
