#pragma once

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js\Proxy.h>
#include <js\Wrapper.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

struct JSFreeOp;
struct JSContext;
class JSObject;

namespace mozjs
{

inline constexpr uint32_t kDefaultClassFlags = JSCLASS_HAS_PRIVATE | JSCLASS_BACKGROUND_FINALIZE;
inline constexpr uint8_t kDefaultPropsFlags = JSPROP_ENUMERATE | JSPROP_PERMANENT;

/// @details Used to define write-only property with JS_PSGS
bool DummyGetter( JSContext* cx, unsigned argc, JS::Value* vp );

const void* GetSmpProxyFamily();

template <typename JsObjectType, typename... ArgsType>
void CreateAndInstallObject( JSContext* cx, JS::HandleObject parentObject, const qwr::u8string& propertyName, ArgsType&&... args )
{
    JS::RootedObject objectToInstall( cx, JsObjectType::CreateJs( cx, args... ) );
    assert( objectToInstall );

    if ( !JS_DefineProperty( cx, parentObject, propertyName.c_str(), objectToInstall, kDefaultPropsFlags ) )
    {
        throw smp::JsException();
    }
}

/// @brief Same as JS_GetInstancePrivate, but unwraps the object if it's a proxy.
template <typename T>
T* GetInnerInstancePrivate( JSContext* cx, JS::HandleObject jsObject )
{
    JS::RootedObject jsUnwrappedObject( cx, jsObject );
    if ( js::IsWrapper( jsObject ) )
    {
        jsUnwrappedObject = js::UncheckedUnwrap( jsObject );
    }

    if constexpr ( T::HasProxy )
    {
        if ( js::IsProxy( jsUnwrappedObject ) && js::GetProxyHandler( jsUnwrappedObject )->family() == GetSmpProxyFamily() )
        {
            jsUnwrappedObject = js::GetProxyTargetObject( jsUnwrappedObject );
        }
    }

    return static_cast<T*>( JS_GetInstancePrivate( cx, jsUnwrappedObject, &T::JsClass, nullptr ) );
}

/// @brief Same as GetInnerInstancePrivate, but also check for JS::Value
template <typename T>
T* GetInnerInstancePrivate( JSContext* cx, JS::HandleValue jsValue )
{
    if ( !jsValue.isObject() )
    {
        return nullptr;
    }

    JS::RootedObject jsObject( cx, &jsValue.toObject() );
    return GetInnerInstancePrivate<T>( cx, jsObject );
}

} // namespace mozjs
