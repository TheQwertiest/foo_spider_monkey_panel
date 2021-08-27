#pragma once

#include <optional>
#include <unordered_map>

namespace smp
{

class DynamicMainMenuManager
{
public:
    struct CommandData
    {
        qwr::u8string name;
        std::optional<qwr::u8string> description;
    };

    struct PanelData
    {
        qwr::u8string name;
        std::unordered_map<uint32_t, CommandData> commands;
    };

public:
    static [[nodiscard]] DynamicMainMenuManager& Get();

    void RegisterPanel( HWND hWnd, const qwr::u8string& panelName );
    void UnregisterPanel( HWND hWnd );

    /// @throw qwr::QwrException
    void RegisterCommand( HWND hWnd, uint32_t id, const qwr::u8string& name, const std::optional<qwr::u8string>& description );
    /// @throw qwr::QwrException
    void UnregisterCommand( HWND hWnd, uint32_t id );

    const std::unordered_map<HWND, PanelData>& GetAllCommandData() const;

private:
    DynamicMainMenuManager() = default;

private:
    std::unordered_map<HWND, PanelData> panels_;
};

} // namespace smp
