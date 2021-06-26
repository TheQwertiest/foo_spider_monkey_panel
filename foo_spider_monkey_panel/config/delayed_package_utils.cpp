#include <stdafx.h>

#include "delayed_package_utils.h"

#include <component_paths.h>

#include <qwr/error_popup.h>
#include <qwr/final_action.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace
{

using namespace smp;

/// @throw std::filesystem::filesystem_error
void ForceRemoveDir( const fs::path& dir )
{
    if ( !fs::exists( dir ) )
    {
        return;
    }

    assert( fs::is_directory( dir ) );
    const auto dirToRemove = dir;
    try
    {
        fs::remove_all( dirToRemove );
    }
    catch ( const fs::filesystem_error& )
    {
        for ( const auto& it: fs::recursive_directory_iterator( dirToRemove ) )
        { // Try to clear read-only flags
            try
            {
                fs::permissions( it, fs::perms::owner_write, fs::perm_options::add );
            }
            catch ( const fs::filesystem_error& )
            {
            }
        }
        fs::remove_all( dirToRemove );
    }
}

/// @throw std::filesystem::filesystem_error
void UpdatePackages()
{
    const auto packagesToProcessDir = path::TempFolder_PackagesToInstall();
    if ( !fs::exists( packagesToProcessDir ) || !fs::is_directory( packagesToProcessDir ) )
    {
        fs::remove_all( packagesToProcessDir );
        return;
    }

    const auto packagesDir = path::Packages_Profile();
    const auto savedPackageDir = path::TempFolder_PackageUnpack();

    for ( const auto& newPackageDir: fs::directory_iterator( packagesToProcessDir ) )
    {
        if ( !fs::is_directory( packagesToProcessDir ) )
        {
            continue;
        }

        const auto packageId = newPackageDir.path().filename().u8string();
        const auto packageToUpdateDir = packagesDir / packageId;

        // Save old version
        fs::remove_all( savedPackageDir );
        fs::create_directories( savedPackageDir );
        fs::copy( packageToUpdateDir, savedPackageDir, fs::copy_options::recursive );
        qwr::final_action autoTmp( [&] {
            try
            {
                ForceRemoveDir( savedPackageDir );
            }
            catch ( const fs::filesystem_error& )
            {
            }
        } );

        try
        {
            // Try to update
            fs::remove_all( packageToUpdateDir );
            fs::create_directories( packageToUpdateDir );
            fs::copy( newPackageDir, packageToUpdateDir, fs::copy_options::recursive );
            smp::config::ClearPackageDelayStatus( packageId );
        }
        catch ( const fs::filesystem_error& )
        {
            // Try to restore old version
            // Clean up first
            if ( fs::exists( packageToUpdateDir ) )
            {
                for ( const auto& it: fs::recursive_directory_iterator( packageToUpdateDir ) )
                {
                    try
                    {
                        if ( fs::is_directory( it ) )
                        {
                            continue;
                        }
                        else
                        {
                            fs::remove( it );
                        }
                    }
                    catch ( const fs::filesystem_error& )
                    {
                    }
                }
            }

            // Restore
            try
            {
                fs::create_directories( packageToUpdateDir );
            }
            catch ( const fs::filesystem_error& )
            {
            }
            for ( const auto& it: fs::recursive_directory_iterator( savedPackageDir ) )
            {
                try
                {
                    const auto dstPath = packageToUpdateDir / fs::relative( it.path(), savedPackageDir );
                    if ( fs::is_directory( it ) )
                    {
                        fs::create_directories( dstPath );
                    }
                    else
                    {
                        fs::copy( it, dstPath, fs::copy_options::overwrite_existing );
                    }
                }
                catch ( const fs::filesystem_error& )
                {
                }
            }

            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME,
                                       fmt::format( "Failed to update package `{}`.\n"
                                                    "Restoration of old version was attempted...",
                                                    packageId ) );
            throw;
        }
    }

    fs::remove_all( packagesToProcessDir );
}

/// @throw std::filesystem::filesystem_error
void RemovePackages()
{
    const auto packagesToProcessDir = path::TempFolder_PackagesToRemove();
    if ( !fs::exists( packagesToProcessDir ) || !fs::is_directory( packagesToProcessDir ) )
    {
        fs::remove_all( packagesToProcessDir );
        return;
    }

    const auto packagesDir = path::Packages_Profile();

    for ( const auto& packageContent: fs::directory_iterator( packagesToProcessDir ) )
    {
        const auto packageId = packageContent.path().filename().u8string();
        fs::remove_all( packagesDir / packageId );

        smp::config::ClearPackageDelayStatus( packageId );
    }

    fs::remove_all( packagesToProcessDir );
}

} // namespace

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
        ::ForceRemoveDir( path::TempFolder_PackageUnpack() );
        ::UpdatePackages();
        ::RemovePackages();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }

    wasCalled = true;
}

} // namespace smp::config
