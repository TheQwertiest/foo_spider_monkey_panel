#pragma once

#include <js_utils/js_error_helper.h>

struct JSContext;

namespace mozjs
{

class JsScope
{
public:
    JsScope( JSContext* cx, JS::HandleObject global, bool enableAutoReport = true )
        : ac_( cx, global )
        , are_( cx )
    {
        if ( !enableAutoReport )
        {
            are_.Disable();
        }
    }

    JsScope( const JsScope& ) = delete;
    JsScope& operator=( const JsScope& ) = delete;

    void DisableReport()
    {
        are_.Disable();
    }

private:    
    JSAutoRealm ac_;
    mozjs::error::AutoJsReport are_;
};

} // namespace mozjs
