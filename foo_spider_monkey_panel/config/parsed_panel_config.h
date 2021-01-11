#pragma once

#include <config/panel_config.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace smp::config
{

struct ParsedPanelSettings
{
    using MenuAction = std::pair<std::string, std::string>;
    using MenuActions = std::vector<MenuAction>;

    std::u8string panelId;
    std::optional<std::u8string> script;
    std::optional<std::filesystem::path> scriptPath;
    std::optional<std::u8string> packageId;
    std::u8string scriptName;
    std::u8string scriptVersion;
    std::u8string scriptAuthor;
    std::u8string scriptDescription;

    // TODO: handle this
    bool isSample = false;

    EdgeStyle edgeStyle = EdgeStyle::Default;
    bool isPseudoTransparent = false;
    bool shouldGrabFocus = true;
    bool enableDragDrop = false;
    MenuActions menuActions;

    /// @throw qwr::QwrException
    [[nodiscard]] static ParsedPanelSettings Parse( const PanelSettings& settings );
    /// @throw qwr::QwrException
    [[nodiscard]] static ParsedPanelSettings Reparse( const ParsedPanelSettings& parsedSettings );

    static ParsedPanelSettings GetDefault();
    PanelSettings GeneratePanelSettings() const;
};

} // namespace smp::config