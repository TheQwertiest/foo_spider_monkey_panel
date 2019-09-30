#pragma once

namespace smp::utils
{

class HookHandler
{
private:
    using HookCallback = std::function<void( int, WPARAM, LPARAM )>;

public:
    ~HookHandler();
    static HookHandler& GetInstance();

    /// @throw smp::SmpException
    template <typename T>
    uint32_t RegisterHook( T&& callback )
    {
        if ( callbacks_.empty() )
        {
            MaybeRegisterGlobalHook();
        }

        uint32_t id = curId_++;
        while ( callbacks_.count( id ) || !id )
        {
            id = curId_++;
        }

        callbacks_.emplace( id, std::make_shared<HookCallback>( std::move( callback ) ) );
        return id;
    }

    void UnregisterHook( uint32_t hookId );

private:
    HookHandler() = default;

    /// @throw smp::SmpException
    void MaybeRegisterGlobalHook();
    static LRESULT CALLBACK GetMsgProc( int code, WPARAM wParam, LPARAM lParam );

private:
    uint32_t curId_ = 1;

    // TODO: add handlers for different hooks if needed
    HHOOK hHook_ = nullptr;
    static std::unordered_map<uint32_t, std::shared_ptr<HookCallback>> callbacks_;
};

} // namespace smp::utils