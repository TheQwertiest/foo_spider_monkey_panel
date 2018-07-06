#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

namespace mozjs
{

constexpr uint32_t DefaultClassFlags()
{
    return JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE;
}

constexpr uint16_t DefaultPropsFlags()
{
    return JSPROP_ENUMERATE | JSPROP_PERMANENT;
}

template<typename MozjsObjectType>
void JsFinalizeOp( [[maybe_unused]] JSFreeOp* fop, JSObject* obj )
{
    auto x = static_cast<MozjsObjectType*>(JS_GetPrivate( obj ));
    if ( x )
    {
        delete x;
        JS_SetPrivate( obj, nullptr );
    }
}

bool DummyGetter( JSContext* cx, unsigned argc, JS::Value* vp );

template<typename FuncType, typename ...ArgsType>
bool CreateAndInstallObject( JSContext* cx, JS::HandleObject parentObject, const pfc::string8_fast& propertyName, FuncType fn, ArgsType&&... args )
{
    JS::RootedObject objectToInstall( cx, fn( cx, args... ) );
    if ( !objectToInstall )
    {
        return false;
    }

    if ( !JS_DefineProperty( cx, parentObject, propertyName.c_str(), objectToInstall, DefaultPropsFlags() ) )
    {
        return false;
    }

    return true;
}

// TODO: remove this shit

JSObject* GetJsObjectFromValue( JSContext* cx, JS::HandleValue jsValue );

template <typename NativeType>
NativeType* GetNativeFromJsObject( JSContext* cx, JS::HandleObject jsObject )
{
    if ( !jsObject )
    {
        return nullptr;
    }

    const JSClass * jsClass = JS_GetClass( jsObject );
    if ( !jsClass
         || !( jsClass->flags & JSCLASS_HAS_PRIVATE ) )
    {
        return nullptr;
    }

    return static_cast<NativeType *>( JS_GetInstancePrivate( cx, jsObject, &NativeType::GetClass(), nullptr ) );
}

template <typename NativeType>
NativeType* GetNativeFromJsValue( JSContext* cx, JS::HandleValue jsValue )
{
    JS::RootedObject jsObject( cx, GetJsObjectFromValue( cx, jsValue ) );
    return GetNativeFromJsObject<NativeType>( cx, jsObject );
}

}
