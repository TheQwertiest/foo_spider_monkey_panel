#include <stdafx.h>

#include "component_paths.h"

#include <qwr/fb2k_paths.h>

namespace smp::path
{

std::filesystem::path JsDocsIndex()
{
    return qwr::path::Component() / L"docs/html/index.html";
}

std::filesystem::path ScriptSamples()
{
    return qwr::path::Component() / "samples";
}

std::filesystem::path ModulePackages_Sample()
{
    return ScriptSamples() / "modules";
}

std::filesystem::path ModulePackages_Profile()
{
    return qwr::path::Profile() / SMP_UNDERSCORE_NAME / "modules";
}

std::filesystem::path ModulePackages_Foobar2000()
{
    return qwr::path::Foobar2000() / SMP_UNDERSCORE_NAME / "modules";
}

std::filesystem::path SmpPackages_Sample()
{
    return ScriptSamples() / "packages";
}

std::filesystem::path SmpPackages_Profile()
{
    return qwr::path::Profile() / SMP_UNDERSCORE_NAME / "packages";
}

std::filesystem::path SmpPackages_Foobar2000()
{
    return qwr::path::Foobar2000() / SMP_UNDERSCORE_NAME / "packages";
}

std::filesystem::path SmpPackages_Storage()
{
    return qwr::path::Profile() / SMP_UNDERSCORE_NAME / "package_data";
}

std::filesystem::path TempFolder()
{
    return qwr::path::Profile() / SMP_UNDERSCORE_NAME / "tmp";
}

std::filesystem::path TempFolder_SmpPackageUnpack()
{
    return TempFolder() / "unpacked_package";
}

std::filesystem::path TempFolder_SmpPackageBackups()
{
    return TempFolder() / "package_backups";
}

std::filesystem::path TempFolder_SmpPackagesToInstall()
{
    return TempFolder() / "packages_to_install";
}

std::filesystem::path TempFolder_SmpPackagesToRemove()
{
    return TempFolder() / "packages_to_remove";
}

std::filesystem::path TempFolder_SmpPackagesInUse()
{
    return TempFolder() / "packages_in_use";
}

} // namespace smp::path
