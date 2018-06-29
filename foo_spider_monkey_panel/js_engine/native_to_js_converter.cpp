#include <stdafx.h>
#include "native_to_js_converter.h"


namespace mozjs::convert::to_js
{

bool ToValue( JSContext * cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setObjectOrNull( inValue );
    return true;
}

template <>
bool ToValue<bool>( JSContext *, const bool& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setBoolean( inValue );
    return true;
}

template <>
bool ToValue<int32_t>( JSContext *, const int32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( inValue );
    return true;
}

template <>
bool ToValue<uint32_t>( JSContext *, const uint32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool ToValue<double>( JSContext *, const double& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool ToValue<float>( JSContext *, const float& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool ToValue<std::string_view>( JSContext * cx, const std::string_view& inValue, JS::MutableHandleValue wrappedValue )
{
    JS::RootedString jsString (cx, JS_NewStringCopyZ( cx, inValue.data() ));
    if ( !jsString )
    {
        return false;
    }

    wrappedValue.setString( jsString );
    return true;
}

template <>
bool ToValue<std::wstring_view>( JSContext * cx, const std::wstring_view& inValue, JS::MutableHandleValue wrappedValue )
{
    // <codecvt> is deprecated in C++17...
    std::string tmpString (pfc::stringcvt::string_utf8_from_wide( inValue.data() ));
    return ToValue<std::string_view>( cx, tmpString, wrappedValue );
}

template <>
bool ToValue<std::string>( JSContext * cx, const std::string& inValue, JS::MutableHandleValue wrappedValue )
{
    return ToValue<std::string_view>( cx, inValue, wrappedValue );
}

template <>
bool ToValue<std::wstring>( JSContext * cx, const std::wstring& inValue, JS::MutableHandleValue wrappedValue )
{
    return ToValue<std::wstring_view>( cx, inValue, wrappedValue );
}

template <>
bool ToValue<std::nullptr_t>( JSContext *, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setUndefined();
    return true;
}

}
