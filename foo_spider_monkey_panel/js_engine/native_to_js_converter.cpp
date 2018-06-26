#include <stdafx.h>
#include "native_to_js_converter.h"


namespace mozjs
{

bool NativeToJsValue( JSContext * cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setObjectOrNull( inValue );
    return true;
}

template <>
bool NativeToJsValue<bool>( JSContext *, const bool& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setBoolean( inValue );
    return true;
}

template <>
bool NativeToJsValue<int32_t>( JSContext *, const int32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setInt32( inValue );
    return true;
}

template <>
bool NativeToJsValue<uint32_t>( JSContext *, const uint32_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool NativeToJsValue<double>( JSContext *, const double& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool NativeToJsValue<float>( JSContext *, const float& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setNumber( inValue );
    return true;
}

template <>
bool NativeToJsValue<std::string_view>( JSContext * cx, const std::string_view& inValue, JS::MutableHandleValue wrappedValue )
{
    JS::RootedString jsString (JS_NewStringCopyZ( cx, inValue.data() ));
    if ( !jsString )
    {
        return false;
    }

    wrappedValue.setString( jsString );
    return true;
}

template <>
bool NativeToJsValue<std::wstring_view>( JSContext * cx, const std::wstring_view& inValue, JS::MutableHandleValue wrappedValue )
{
    // <codecvt> is deprecated in C++17...
    std::string tmpString (pfc::stringcvt::string_utf8_from_wide( inValue.data() ));
    return NativeToJsValue<std::string_view>( cx, tmpString, wrappedValue );
}

template <>
bool NativeToJsValue<std::wstring>( JSContext * cx, const std::wstring& inValue, JS::MutableHandleValue wrappedValue )
{
    return NativeToJsValue<std::wstring_view>( cx, inValue, wrappedValue );
}


template <>
bool NativeToJsValue<std::nullptr_t>( JSContext *, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue )
{
    wrappedValue.setUndefined();
    return true;
}

}
