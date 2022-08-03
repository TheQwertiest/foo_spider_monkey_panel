#pragma once

#include <config/script_source_types.h>

#include <filesystem>
#include <string>
#include <variant>

namespace smp::config
{

struct RawModulePackage
{
    qwr::u8string name;
};

struct RawSmpPackage
{
    qwr::u8string id;
    qwr::u8string name;
    qwr::u8string author;
};

struct RawSampleFile
{
    qwr::u8string name;
    bool isModule = false;
};

struct RawScriptFile
{
    std::filesystem::path scriptPath;
    bool isModule = false;
};

struct RawInMemoryScript
{
    qwr::u8string script;
    bool isModule = false;
};

using RawScriptSourceVariant = std::variant<RawModulePackage, RawSmpPackage, RawScriptFile, RawSampleFile, RawInMemoryScript>;

[[nodiscard]] ScriptSourceType GetSourceType( const RawScriptSourceVariant& source );

} // namespace smp::config
