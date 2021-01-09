#pragma once

#include <panel/user_message.h>

namespace smp::panel
{

class CallbackData
{
public:
    CallbackData() = default;
    virtual ~CallbackData() = default;
    CallbackData( const CallbackData& ) = delete;
    CallbackData operator=( const CallbackData& ) = delete;

    template <typename... Args>
    std::tuple<Args...>& GetData()
    {
        assert( pData_ );
        return *static_cast<std::tuple<Args...>*>( pData_ );
    }

protected:
    void* pData_ = nullptr;
};

template <typename... Args>
class CallbackDataImpl
    : public CallbackData
{
public:
    template <typename... ArgsFwd>
    CallbackDataImpl( ArgsFwd&&... args )
        : data_( std::forward<ArgsFwd>( args )... )
    {
        pData_ = &data_;
    }

    ~CallbackDataImpl() override = default;
    CallbackDataImpl( const CallbackDataImpl& ) = delete;
    CallbackDataImpl operator=( const CallbackDataImpl& ) = delete;

private:
    std::tuple<Args...> data_;
};

} // namespace smp::panel
