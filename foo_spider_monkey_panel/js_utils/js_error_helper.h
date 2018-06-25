#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#include <jsfriendapi.h>
#pragma warning( pop )  

namespace mozjs
{

class AutoReportException
{
public:
    explicit AutoReportException( JSContext* cx );
    ~AutoReportException();

private: 
    static std::string GetStackTraceString( JSContext* cx, JS::HandleObject exn );

private:
    JSContext * cx;
};

std::string GetCurrentExceptionText( JSContext* cx );

}
