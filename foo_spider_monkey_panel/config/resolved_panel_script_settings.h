#pragma once

#include <config/basic_script_sources.h>
#include <config/module_package/package.h>
#include <config/panel_config.h>
#include <config/panel_config_formats.h>
#include <config/script_source_types.h>
#include <config/smp_package/package.h>

#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace smp::config
{

using ScriptSourceVariant = std::variant<ModulePackage, SmpPackage, ScriptFile, InMemoryScript>;

class ResolvedPanelScriptSettings
{
public:
    ResolvedPanelScriptSettings() = default;

    [[nodiscard]] static ResolvedPanelScriptSettings ResolveSource( const RawScriptSourceVariant& source );

    [[nodiscard]] ScriptSourceType GetSourceType() const;
    [[nodiscard]] bool IsModuleScript() const;
    [[nodiscard]] bool IsSampleScript() const;

    [[nodiscard]] const qwr::u8string& GetScriptName() const;
    [[nodiscard]] qwr::u8string GetScriptVersion() const;
    [[nodiscard]] const qwr::u8string& GetScriptAuthor() const;
    [[nodiscard]] const qwr::u8string& GetScriptDescription() const;

    [[nodiscard]] bool ShouldGrabFocus() const;
    [[nodiscard]] bool ShouldEnableDragDrop() const;

    /// @remark Valid only if `InMemory`
    const qwr::u8string& GetScript() const;
    /// @remark Valid only if not `InMemory`
    std::filesystem::path GetScriptPath() const;
    /// @remark Valid only if `SmpPackage`
    const SmpPackage& GetSmpPackage() const;
    /// @remark Valid only if `ModulePackage`
    const ModulePackage& GetModulePackage() const;
    /// @remark Valid only if not `ModulePackage` and not `SmpPackage`
    NonPackageScript& GetScriptRuntimeData();

private:
    ResolvedPanelScriptSettings( const ScriptSourceVariant& source );

private:
    ScriptSourceVariant source_;
};

} // namespace smp::config
