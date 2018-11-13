#pragma once

namespace mozjs::convert::to_js
{

template <typename InType>
void ToValue( JSContext* cx, std::unique_ptr<InType> inValue, JS::MutableHandleValue wrappedValue )
{
    static_assert( 0, "Unsupported type" );
}

template <>
void ToValue( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> inValue, JS::MutableHandleValue wrappedValue );

template <typename InJsType>
void ToValue( JSContext* cx, JS::Handle<InJsType> inValue, JS::MutableHandleValue wrappedValue )
{
    static_assert( 0, "Unsupported type" );
}

template <>
void ToValue( JSContext* cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, JS::HandleValue inValue, JS::MutableHandleValue wrappedValue );

template <typename InType>
void ToValue( JSContext* cx, const InType& inValue, JS::MutableHandleValue wrappedValue )
{
    static_assert( 0, "Unsupported type" );
}

template <>
void ToValue( JSContext* cx, const bool& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const int8_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const uint8_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const int32_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const uint32_t& inValue, JS::MutableHandleValue wrappedValue );

/// @details Returns only approximate int64_t value, use with care!
template <>
void ToValue( JSContext* cx, const int64_t& inValue, JS::MutableHandleValue wrappedValue );

/// @details Returns only approximate uint64_t value, use with care!
template <>
void ToValue( JSContext* cx, const uint64_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const double& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const float& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const pfc::string8_fast& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const std::wstring_view& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const std::wstring& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const std::nullptr_t& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const metadb_handle_ptr& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const metadb_handle_list& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const t_playback_queue_item& inValue, JS::MutableHandleValue wrappedValue );

template <typename T, typename F>
void ToArrayValue( JSContext* cx, const T& inVector, F&& accessorFunc, JS::MutableHandleValue wrappedValue )
{
    JS::RootedObject jsArray( cx, JS_NewArrayObject( cx, inVector.size() ) );
    smp::JsException::ExpectTrue( jsArray );

    JS::RootedValue jsValue( cx );
    for ( size_t i = 0; i < inVector.size(); ++i )
    {
        ToValue( cx, accessorFunc( inVector, i ), &jsValue );
        if ( !JS_SetElement( cx, jsArray, i, jsValue ) )
        {
            throw smp::JsException();
        }
    }

    wrappedValue.set( JS::ObjectValue( *jsArray ) );
}

} // namespace mozjs::convert::to_js
