#pragma once

#include <config/panel_config.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace smp::config
{

enum class ScriptSourceType : uint8_t
{
    Package = 0,
    Sample,
    File,
    InMemory,
};

struct ParsedPanelSettings
{
    std::u8string panelId;
    std::optional<std::u8string> script;
    std::optional<std::filesystem::path> scriptPath;
    std::optional<std::u8string> packageId;
    std::u8string scriptName;
    std::u8string scriptVersion;
    std::u8string scriptAuthor;
    std::u8string scriptDescription;
    bool isSample = false;

    EdgeStyle edgeStyle = EdgeStyle::Default;
    bool isPseudoTransparent = false;
    bool shouldGrabFocus = true;
    bool enableDragDrop = false;

    static [[nodiscard]] ParsedPanelSettings GetDefault();

    /// @throw qwr::QwrException
    [[nodiscard]] static ParsedPanelSettings Parse( const PanelSettings& settings );
    /// @throw qwr::QwrException
    [[nodiscard]] static ParsedPanelSettings Reparse( const ParsedPanelSettings& parsedSettings );

    [[nodiscard]] PanelSettings GeneratePanelSettings() const;

    [[nodiscard]] ScriptSourceType GetSourceType() const;
};

} // namespace smp::config