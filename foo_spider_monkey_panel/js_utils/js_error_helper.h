#pragma once

namespace mozjs::error
{

template <typename F, typename... Args>
bool Execute_JsSafe( JSContext* cx, std::string_view functionName, F&& func, Args... args )
{
    try
    {
        func( cx, std::forward<Args>( args )... );
    }
    catch ( ... )
    {
        error::ExceptionToJsError( cx );
        const pfc::string8_fast additionalText = pfc::string8_fast( functionName.data(), functionName.size() ) + " failed";
        error::PrependTextToJsError( cx, additionalText );
        return false;
    }

    if ( JS_IsExceptionPending( cx ) )
    {
        const pfc::string8_fast additionalText = pfc::string8_fast( functionName.data(), functionName.size() ) + " failed";
        error::PrependTextToJsError( cx, additionalText );
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

pfc::string8_fast JsErrorToText( JSContext* cx );
void ExceptionToJsError( JSContext* cx );
void SuppressException( JSContext* cx );
void PrependTextToJsError( JSContext* cx, const pfc::string8_fast& text );

} // namespace mozjs::error
