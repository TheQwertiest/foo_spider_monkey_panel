#pragma once

#include <optional>
#include <utils/string_helpers.h>

struct JSFreeOp;
struct JSContext;
class JSObject;

namespace mozjs
{

template <typename T>
std::optional<T> GetOptionalProperty( JSContext* cx, JS::HandleObject jsObject, const std::string& propName )
{
    bool hasProp;
    if ( !JS_HasProperty( cx, jsObject, propName.c_str(), &hasProp ) )
    {
        throw smp::JsException();
    }

    if ( !hasProp )
    {
        return std::nullopt;
    }

    JS::RootedValue jsValue( cx );
    if ( !JS_GetProperty( cx, jsObject, propName.c_str(), &jsValue ) )
    {
        throw smp::JsException();
    }

    auto retVal = convert::to_native::ToValue<T>( cx, jsValue );
    if ( !retVal )
    {
        throw smp::SmpException( smp::string::Formatter() << "`" << propName.c_str() << "` property is of wrong type" );
    }

    return retVal.value();
};

} // namespace mozjs
