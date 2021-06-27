#pragma once

#include <filesystem>

namespace smp::path
{

std::filesystem::path JsDocsIndex();

std::filesystem::path ScriptSamples();

std::filesystem::path Packages_Sample();
std::filesystem::path Packages_Profile();
std::filesystem::path Packages_Foobar2000();

std::filesystem::path Packages_Storage();

std::filesystem::path TempFolder();
std::filesystem::path TempFolder_PackageUnpack();
std::filesystem::path TempFolder_PackageBackups();
std::filesystem::path TempFolder_PackagesToInstall();
std::filesystem::path TempFolder_PackagesToRemove();
std::filesystem::path TempFolder_PackagesInUse();

} // namespace smp::path
