#pragma once

namespace smp::ui
{

class ITab
{
public:
    virtual ~ITab() = default;

    virtual HWND CreateTab( HWND hParent ) = 0;
    virtual CDialogImplBase& Dialog() = 0;
    virtual const wchar_t* Name() const = 0;

    virtual bool HasChanged() = 0;
    virtual void Apply() = 0;
    virtual void Revert() = 0;
};

} // namespace smp::ui