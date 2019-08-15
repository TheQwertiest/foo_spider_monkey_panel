#pragma once

#include <optional>
#include <variant>

namespace mozjs
{

using SerializedJsValue = std::variant<bool, int32_t, double, std::u8string>;

SerializedJsValue SerializeJsValue( JSContext* cx, JS::HandleValue jsValue );
void DeserializeJsValue( JSContext* cx, const SerializedJsValue& serializedValue, JS::MutableHandleValue jsValue );


}
