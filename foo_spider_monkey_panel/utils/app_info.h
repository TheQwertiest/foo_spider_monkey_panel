#pragma once

#include <filesystem>
#include <vector>

namespace smp
{

struct AppInfo
{
    std::filesystem::path appPath;
    std::wstring appName;
};

/// @throw qwr::QwrException
std::vector<AppInfo> GetAppsAssociatedWithExtension( const std::wstring& ext );

CIcon GetAppIcon( const std::filesystem::path& appPath );

} // namespace smp
