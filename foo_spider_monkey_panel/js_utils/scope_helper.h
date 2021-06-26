#pragma once

#include <js_utils/js_error_helper.h>

struct JSContext;

namespace mozjs
{

class JsAutoRealmWithErrorReport
{
public:
    [[nodiscard]] JsAutoRealmWithErrorReport( JSContext* cx, JS::HandleObject global )
        : ac_( cx, global )
        , are_( cx )
    {
    }

    JsAutoRealmWithErrorReport( const JsAutoRealmWithErrorReport& ) = delete;
    JsAutoRealmWithErrorReport& operator=( const JsAutoRealmWithErrorReport& ) = delete;

    void DisableReport()
    {
        are_.Disable();
    }

private:
    JSAutoRealm ac_;
    mozjs::error::AutoJsReport are_;
};

} // namespace mozjs
