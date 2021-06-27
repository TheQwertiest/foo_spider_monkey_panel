#pragma once

namespace smp::config
{

enum class PackageDelayStatus
{
    ToBeRemoved,
    ToBeUpdated,
    NotDelayed
};

/// @throw qwr::QwrException
bool IsPackageInUse( const qwr::u8string& packageId );

/// @throw qwr::QwrException
PackageDelayStatus GetPackageDelayStatus( const qwr::u8string& packageId );

/// @throw qwr::QwrException
void ClearPackageDelayStatus( const qwr::u8string& packageId );

/// @throw qwr::QwrException
void MarkPackageAsToBeRemoved( const qwr::u8string& packageId );

/// @throw qwr::QwrException
void MarkPackageAsToBeInstalled( const qwr::u8string& packageId, const std::filesystem::path& packageContent );

/// @throw qwr::QwrException
void MarkPackageAsInUse( const qwr::u8string& packageId );

/// @throw qwr::QwrException
void ProcessDelayedPackages();

} // namespace smp::config
