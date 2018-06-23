#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

namespace mozjs
{

class AutoReportException
{
public:
    explicit AutoReportException( JSContext* cx );
    ~AutoReportException();

private: 
    static void PrintError();
    static void PrintStack( JSContext* cx, JS::HandleValue exn );

private:
    JSContext * cx;
};

}
