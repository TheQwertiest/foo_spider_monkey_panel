#pragma once

#include <filesystem>

namespace smp::config
{

struct NonPackageScript
{
    virtual ~NonPackageScript() = default;

    qwr::u8string name;
    qwr::u8string version;
    qwr::u8string author;
    qwr::u8string description;
    bool shouldGrabFocus = true;
    bool enableDragDrop = false;
};

struct InMemoryScript : public NonPackageScript
{
    qwr::u8string script;
    bool isModule = false;
};

struct ScriptFile : public NonPackageScript
{
    std::filesystem::path scriptPath;
    bool isModule = false;
    bool isSample = false;
};

} // namespace smp::config
