#include <stdafx.h>

#include "delayed_package_utils.h"

#include <config/package_utils.h>
#include <resources/resource.h>
#include <utils/resource_helpers.h>

#include <component_paths.h>

#include <qwr/error_popup.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace
{

using namespace smp;

/// @throw std::filesystem::filesystem_error
void ForceRemoveDirContents( const fs::path& dir )
{
    if ( !fs::exists( dir ) )
    {
        return;
    }

    assert( fs::is_directory( dir ) );
    for ( const auto& it: fs::recursive_directory_iterator( dir ) )
    { // Try to clear read-only flags
        try
        {
            fs::permissions( it, fs::perms::owner_write, fs::perm_options::add );
        }
        catch ( const fs::filesystem_error& )
        {
        }
    }
    for ( const auto& it: fs::recursive_directory_iterator( dir ) )
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

/// @throw std::filesystem::filesystem_error
void ForceRemoveDir( const fs::path& dir )
{
    if ( !fs::exists( dir ) )
    {
        return;
    }

    assert( fs::is_directory( dir ) );
    try
    {
        fs::remove_all( dir );
    }
    catch ( const fs::filesystem_error& )
    {
        ForceRemoveDirContents( dir );
        fs::remove_all( dir );
    }
}

/// @throw std::filesystem::filesystem_error
/// @throw qwr::QwrException
void CheckPackageBackups()
{
    const auto dir = path::TempFolder_PackageBackups();
    if ( !fs::exists( dir ) || !fs::is_directory( dir ) )
    {
        fs::remove_all( dir );
        return;
    }

    std::vector<qwr::u8string> backups;
    for ( const auto& backup: fs::directory_iterator( dir ) )
    {
        if ( !fs::is_directory( backup ) )
        {
            continue;
        }

        backups.emplace_back( backup.path().u8string() );
    }

    if ( !backups.empty() )
    { // in case user still haven't restored his package
        throw qwr::QwrException(
            "The following backups still exist:\n"
            "{}\n\n"
            "If you have completed package recovery process, remove them and restart foobar2000 to continue delayed package processing.",
            fmt::join( backups, ",\n" ) );
    }

    fs::remove_all( dir );
}

/// @throw std::filesystem::filesystem_error
/// @throw qwr::QwrException
void UpdatePackages()
{
    const auto packagesToProcessDir = path::TempFolder_PackagesToInstall();
    if ( !fs::exists( packagesToProcessDir ) || !fs::is_directory( packagesToProcessDir ) )
    {
        fs::remove_all( packagesToProcessDir );
        return;
    }

    const auto packagesDir = path::Packages_Profile();
    const auto packageBackupsDir = path::TempFolder_PackageBackups();

    for ( const auto& newPackageDir: fs::directory_iterator( packagesToProcessDir ) )
    {
        if ( !fs::is_directory( packagesToProcessDir ) )
        {
            continue;
        }

        qwr::final_action autoTmp( [&] {
            try
            {
                ForceRemoveDir( newPackageDir );
            }
            catch ( const fs::filesystem_error& )
            {
            }
        } );

        const auto packageId = newPackageDir.path().filename().u8string();
        const auto packageToUpdateDir = packagesDir / packageId;
        const auto packageBackupDir = packageBackupsDir / packageId;

        // Save old version

        fs::create_directories( packageBackupDir.parent_path() );

        try
        {
            fs::rename( packageToUpdateDir, packageBackupDir );
        }
        catch ( const fs::filesystem_error& e )
        {
            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME,
                                       fmt::format(
                                           "Failed to update package `{}`:\n"
                                           "{}",
                                           packageId,
                                           qwr::unicode::ToU8_FromAcpToWide( e.what() ) ) );
            continue;
        }

        try
        {
            // Try to update
            fs::remove_all( packageToUpdateDir );
            fs::create_directories( packageToUpdateDir.parent_path() );
            fs::rename( newPackageDir, packageToUpdateDir );
            smp::config::ClearPackageDelayStatus( packageId );
            ForceRemoveDir( packageBackupDir );
        }
        catch ( const fs::filesystem_error& )
        {
            // Enter in recovery process
            ForceRemoveDirContents( packageToUpdateDir );
            fs::create_directories( packageToUpdateDir );

            const auto restorationScriptOpt = LoadStringResource( IDR_RECOVERY_PACKAGE_SCRIPT, "Script" );
            assert( restorationScriptOpt );
            const auto restorationJsonOpt = LoadStringResource( IDR_RECOVERY_PACKAGE_JSON, "Script" );
            assert( restorationJsonOpt );

            auto j = json::parse( *restorationJsonOpt );
            j["id"] = packageId;

            qwr::file::WriteFile( packageToUpdateDir / config::GetRelativePathToMainFile(), *restorationScriptOpt );
            qwr::file::WriteFile( packageToUpdateDir / "package.json", j.dump( 2 ) );

            qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME,
                                       fmt::format( "Critical error encountered when updating package `{}`!\n\n"
                                                    "The panel was replaced with recovery package.\n"
                                                    "Follow the instructions to restore your old package.",
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

void ProcessDelayedPackages()
{
    try
    {
        fs::remove_all( path::TempFolder_PackagesInUse() );
        fs::remove_all( path::TempFolder_PackageUnpack() );
        ::RemovePackages();
        ::CheckPackageBackups();
        ::UpdatePackages();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace smp::config
