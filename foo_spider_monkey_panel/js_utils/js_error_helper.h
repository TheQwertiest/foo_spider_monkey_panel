#pragma once


namespace mozjs::error
{

template <typename F, typename... Args>
bool Execute_JsSafe( JSContext* cx, std::string_view functionName, F func, Args... args )
{
    bool bRet = true;
    try
    {
        bRet = func( cx, std::forward<Args>( args )... );
    }
    catch ( ... )
    {
        mozjs::error::ExceptionToJsError( cx );
        bRet = false;
    }
    if ( !bRet )
    {
        mozjs::error::ReportJsErrorWithFunctionName( cx, std::string( functionName.data(), functionName.size() ).c_str() );
    }
    return bRet;
}

class AutoJsReport
{
public:
    explicit AutoJsReport( JSContext* cx );
    ~AutoJsReport();

    void Disable();

private: 
    static pfc::string8_fast GetStackTraceString( JSContext* cx, JS::HandleObject exn );

private:
    JSContext * cx;
    bool isDisabled_ = false;
};

void ExceptionToJsError( JSContext* cx );
void ReportJsErrorWithFunctionName( JSContext* cx, const char* functionName );

}
