#pragma once

#pragma warning( push )
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <js\Wrapper.h>
#include <js\Proxy.h>
#pragma warning( pop ) 

struct JSFreeOp;
struct JSContext;
class JSObject;

namespace mozjs
{

constexpr uint32_t DefaultClassFlags()
{
    return JSCLASS_HAS_PRIVATE | JSCLASS_BACKGROUND_FINALIZE;
}

constexpr uint16_t DefaultPropsFlags()
{
    return JSPROP_ENUMERATE | JSPROP_PERMANENT;
}

/// @details Used to define write-only property with JS_PSGS
bool DummyGetter( JSContext* cx, unsigned argc, JS::Value* vp );

const void* GetSmpProxyFamily();

template <typename JsObjectType, typename... ArgsType>
void CreateAndInstallObject( JSContext* cx, JS::HandleObject parentObject, const std::u8string& propertyName, ArgsType&&... args )
{
    JS::RootedObject objectToInstall( cx, JsObjectType::CreateJs( cx, args... ) );
    assert( objectToInstall );

    if ( !JS_DefineProperty( cx, parentObject, propertyName.c_str(), objectToInstall, DefaultPropsFlags() ) )
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
