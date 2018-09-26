#pragma once


namespace mozjs
{

class AutoReportException
{
public:
    explicit AutoReportException( JSContext* cx );
    ~AutoReportException();

    void Disable();

private: 
    static pfc::string8_fast GetStackTraceString( JSContext* cx, JS::HandleObject exn );

private:
    JSContext * cx;
    bool isDisabled_ = false;
};

void RethrowExceptionWithFunctionName( JSContext* cx, const char* functionName );

}
