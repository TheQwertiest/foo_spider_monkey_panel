#pragma once

namespace mozjs::convert::to_js
{

template <typename T>
void ToValue( JSContext* cx, const std::reference_wrapper<T>& inValue, JS::MutableHandleValue wrappedValue )
{
    ToValue( cx, inValue.get(), wrappedValue );
}

template <typename T>
void ToValue( JSContext* cx, JS::Handle<T> inValue, JS::MutableHandleValue wrappedValue )
{
    static_assert( 0, "Unsupported type" );
}

template <>
void ToValue( JSContext* cx, JS::HandleObject inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, JS::HandleValue inValue, JS::MutableHandleValue wrappedValue );

template <typename T>
void ToValue( JSContext* cx, const T& inValue, JS::MutableHandleValue wrappedValue )
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
void ToValue( JSContext* cx, const qwr::u8string& inValue, JS::MutableHandleValue wrappedValue );

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
void ToValue( JSContext* cx, const metadb_handle_list_cref& inValue, JS::MutableHandleValue wrappedValue );

template <>
void ToValue( JSContext* cx, const t_playback_queue_item& inValue, JS::MutableHandleValue wrappedValue );

template <typename T>
void ToValue( JSContext* cx, std::unique_ptr<T> inValue, JS::MutableHandleValue wrappedValue )
{
    static_assert( 0, "Unsupported type" );
}

template <>
void ToValue( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> inValue, JS::MutableHandleValue wrappedValue );

template <typename T>
void ToValue( JSContext* cx, std::shared_ptr<T> inValue, JS::MutableHandleValue wrappedValue )
{
    if ( !inValue )
    {
        wrappedValue.setNull();
        return;
    }

    ToValue( cx, *inValue, wrappedValue );
}

template <typename T>
void ToValue( JSContext* cx, const std::optional<T>& inValue, JS::MutableHandleValue wrappedValue )
{
    if ( !inValue )
    {
        wrappedValue.setNull();
        return;
    }

    ToValue( cx, *inValue, wrappedValue );
}

template <typename T, typename F>
void ToArrayValue( JSContext* cx, const T& inContainer, F&& accessorFunc, JS::MutableHandleValue wrappedValue )
{
    JS::RootedObject jsArray( cx, JS_NewArrayObject( cx, inContainer.size() ) );
    smp::JsException::ExpectTrue( jsArray );

    JS::RootedValue jsValue( cx );
    for ( const auto i: ranges::views::indices( inContainer.size() ) )
    {
        ToValue( cx, accessorFunc( inContainer, i ), &jsValue );
        if ( !JS_SetElement( cx, jsArray, i, jsValue ) )
        {
            throw smp::JsException();
        }
    }

    wrappedValue.set( JS::ObjectValue( *jsArray ) );
}

template <typename T>
void ToArrayValue( JSContext* cx, const T& inContainer, JS::MutableHandleValue wrappedValue )
{
    JS::RootedObject jsArray( cx, JS_NewArrayObject( cx, inContainer.size() ) );
    smp::JsException::ExpectTrue( jsArray );

    JS::RootedValue jsValue( cx );
    for ( const auto& [i, elem]: ranges::views::enumerate( inContainer ) )
    {
        ToValue( cx, elem, &jsValue );
        if ( !JS_SetElement( cx, jsArray, i, jsValue ) )
        {
            throw smp::JsException();
        }
    }

    wrappedValue.set( JS::ObjectValue( *jsArray ) );
}

} // namespace mozjs::convert::to_js
