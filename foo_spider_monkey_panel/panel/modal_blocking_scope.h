#pragma once

#include <atomic>

namespace smp::modal
{

/// @remark For cases when modal might be called from another modal
class ConditionalModalScope
{
public:
    /// @param isScriptInvoking false, if should not be considered JS blocking
    [[nodiscard]] ConditionalModalScope( HWND hParent, bool isWhitelistedModal = false );
    ~ConditionalModalScope();

private:
    modal_dialog_scope scope_;
    bool needsModalScope_;
    bool isWhitelistedModal_;
};

class MessageBlockingScope
{
public:
    [[nodiscard]] MessageBlockingScope();
    ~MessageBlockingScope();
};

class ModalBlockingScope
{
public:
    /// @param isScriptInvoking false, if should not be considered JS blocking
    [[nodiscard]] ModalBlockingScope( HWND hParent, bool isWhitelistedModal = false );
    ~ModalBlockingScope();

private:
    modal_dialog_scope scope_;
    bool isWhitelistedModal_;
};

class WhitelistedScope
{
public:
    [[nodiscard]] WhitelistedScope();
    ~WhitelistedScope();
};

[[nodiscard]] bool IsModalBlocked();
[[nodiscard]] bool IsInWhitelistedModal();

} // namespace smp::modal
