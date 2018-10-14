#pragma once


namespace mozjs::error
{

template <typename F, typename... Args>
bool Execute_JsSafe( JSContext* cx, std::string_view functionName, F func, Args... args )
{
    try
    {
        func( cx, std::forward<Args>( args )... );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
        mozjs::error::ReportJsErrorWithFunctionName( cx, std::string( functionName.data(), functionName.size() ).c_str() );
        return false;
    }    

    if (JS_IsExceptionPending(cx))
    {
        mozjs::error::ReportJsErrorWithFunctionName( cx, std::string( functionName.data(), functionName.size() ).c_str() );
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
    JSContext * cx;
    bool isDisabled_ = false;
};

pfc::string8_fast GetTextFromCurrentJsError( JSContext* cx );
void ExceptionToJsError( JSContext* cx );
void ReportJsErrorWithFunctionName( JSContext* cx, const char* functionName );

}
