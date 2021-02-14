#pragma once

namespace smp::ui
{

class ITab
{
public:
    virtual ~ITab() = default;

    virtual HWND CreateTab( HWND hParent ) = 0;
    [[nodiscard]] virtual CDialogImplBase& Dialog() = 0;
    [[nodiscard]] virtual const wchar_t* Name() const = 0;

    [[nodiscard]] virtual bool HasChanged() = 0;
    virtual void Apply() = 0;
    virtual void Revert() = 0;
    virtual void Refresh() = 0;
};

} // namespace smp::ui
