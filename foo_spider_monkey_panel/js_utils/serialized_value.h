#pragma once

#include <optional>

namespace mozjs
{

enum class JsValueType
    : uint32_t
{// Take care changing this: used in config
    pt_boolean = 0,
    pt_int32 = 1,
    pt_double = 2,
    pt_string = 3,
};

struct SerializedJsValue
{
    JsValueType type;
    union
    {
        int32_t intVal;
        double doubleVal;
        bool boolVal;
    };
    std::string strVal; ///< JS uses wide strings internally, thus less conversions
};

std::optional<SerializedJsValue> SerializeJsValue( JSContext* cx, JS::HandleValue jsValue );
bool DeserializeJsValue( JSContext* cx, const SerializedJsValue& serializedValue, JS::MutableHandleValue jsValue );


}
