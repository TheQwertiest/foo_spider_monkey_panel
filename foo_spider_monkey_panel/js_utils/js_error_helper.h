#pragma once

#include <utils/stacktrace.h>

#include <adv_config.h>

namespace mozjs::error
{

namespace internal
{

template <typename F, typename... Args>
void Execute_JsWithSehStackTrace( JSContext* cx, F&& func, Args&&... args )
{
    __try
    {
        func( cx, std::forward<Args>( args )... );
    }
    __except ( smp::SehHandler_ConsoleStacktrace( GetExceptionInformation(), GetExceptionCode() ) )
    {
        throw;
    }
}

} // namespace internal

template <typename F, typename... Args>
bool Execute_JsSafe( JSContext* cx, std::string_view functionName, F&& func, Args&&... args )
{
    try
    {
#ifdef SMP_ENABLE_CXX_STACKTRACE
        if ( smp::config::advanced::stacktrace.get() )
        {
            internal::Execute_JsWithSehStackTrace( cx, std::forward<F>( func ), std::forward<Args>( args )... );
        }
        else
#endif //SMP_ENABLE_CXX_STACKTRACE
        {
            func( cx, std::forward<Args>( args )... );
        }
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
    }

    if ( JS_IsExceptionPending( cx ) )
    {
        std::u8string additionalText = std::u8string( functionName.data(), functionName.size() );
        additionalText += " failed";
        mozjs::error::PrependTextToJsError( cx, additionalText );
        return false;
    }

    return true;
}

class AutoJsReport
{
public:
    explicit AutoJsReport( JSContext* cx );
    ~AutoJsReport();

    void Disable();

private:
    JSContext* cx;
    bool isDisabled_ = false;
};

std::u8string JsErrorToText( JSContext* cx );
void ExceptionToJsError( JSContext* cx );
void SuppressException( JSContext* cx );
void PrependTextToJsError( JSContext* cx, const std::u8string& text );

} // namespace mozjs::error
