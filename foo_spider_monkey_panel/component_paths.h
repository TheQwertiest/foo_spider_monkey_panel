#pragma once

#include <filesystem>

namespace smp::path
{

std::filesystem::path JsDocsIndex();

std::filesystem::path ScriptSamples();

std::filesystem::path ModulePackages_Sample();
std::filesystem::path ModulePackages_Profile();

std::filesystem::path SmpPackages_Sample();
std::filesystem::path SmpPackages_Profile();
std::filesystem::path SmpPackages_Foobar2000();

std::filesystem::path SmpPackages_Storage();

std::filesystem::path TempFolder();
std::filesystem::path TempFolder_SmpPackageUnpack();
std::filesystem::path TempFolder_SmpPackageBackups();
std::filesystem::path TempFolder_SmpPackagesToInstall();
std::filesystem::path TempFolder_SmpPackagesToRemove();
std::filesystem::path TempFolder_SmpPackagesInUse();

} // namespace smp::path
