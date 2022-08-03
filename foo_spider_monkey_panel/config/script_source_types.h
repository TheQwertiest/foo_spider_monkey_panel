#pragma once

namespace smp::config
{

enum class ScriptSourceType : uint8_t
{
    SmpPackage,
    ModulePackage,
    Sample,
    File,
    InMemory
};

} // namespace smp::config
