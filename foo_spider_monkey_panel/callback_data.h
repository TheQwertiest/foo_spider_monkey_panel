#pragma once

#include <user_message.h>

namespace smp::panel
{

class CallBackDataBase
{
public:
    CallBackDataBase() = default;
    virtual ~CallBackDataBase() = default;
    CallBackDataBase( const CallBackDataBase& ) = delete;
    CallBackDataBase operator=( const CallBackDataBase& ) = delete;

    void* DataPtr()
    {
        return pData_;
    }

    void* DataPtr() const
    {
        return pData_;
    }

protected:
    void* pData_ = nullptr;
};

template <typename... Args>
class CallBackData
    : public CallBackDataBase
{
public:

    template <typename... ArgsFwd>
    CallBackData( ArgsFwd&&... args )
        : data_( std::forward<ArgsFwd>(args)... )
    {
        pData_ = &data_;
    }

    ~CallBackData() override = default;
    CallBackData( const CallBackData& ) = delete;
    CallBackData operator=( const CallBackData& ) = delete;

    auto& Data()
    {
        return data_;
    }

    auto& Data() const
    {
        return data_;
    }

private:
    std::tuple<Args...> data_;
};

} // namespace smp::panel
