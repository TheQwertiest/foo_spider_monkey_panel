#pragma once

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

/// @details Used to define write-only property with JS_PSGS
bool DummyGetter( JSContext* cx, unsigned argc, JS::Value* vp );

template<typename JsObjectType, typename ...ArgsType>
bool CreateAndInstallObject( JSContext* cx, JS::HandleObject parentObject, const pfc::string8_fast& propertyName, ArgsType&&... args )
{
    JS::RootedObject objectToInstall( cx, JsObjectType::CreateJs( cx, args... ) );
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

}
