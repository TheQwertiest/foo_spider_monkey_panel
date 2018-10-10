#pragma once

#include <optional>
#include <string>

struct JSFreeOp;
struct JSContext;
class JSObject;

namespace mozjs
{
// TODO: replace hasFailed with exception
template <typename T>
std::optional<T> GetOptionalProperty( JSContext* cx, JS::HandleObject jsObject, const std::string& propName, bool& hasFailed )
{
    bool hasProp;
    if ( !JS_HasProperty( cx, jsObject, propName.c_str(), &hasProp ) )
    { // reports
        hasFailed = true;
        return std::nullopt;
    }

    if ( !hasProp )
    {
        hasFailed = false;
        return std::nullopt;
    }

     JS::RootedValue jsValue( cx );
     if ( !JS_GetProperty( cx, jsObject, propName.c_str(), &jsValue ) )
     { // reports
          hasFailed = true;
          return std::nullopt;
     }

     auto retVal = convert::to_native::ToValue<T>( cx, jsValue );
     if ( !retVal )
     {
          JS_ReportErrorUTF8( cx, "`%s` property is of wrong type", propName.c_str() );
          hasFailed = true;
          return std::nullopt;
     }

     hasFailed = false;
     return retVal.value();
};

}
