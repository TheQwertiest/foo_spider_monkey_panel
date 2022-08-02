#pragma once

#include <js_backend/objects/core/object_traits.h>
#include <js_backend/utils/js_prototype_helpers.h>

class JSFreeOp;
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
    JS::RootedObject objectToInstall( cx, JsObjectBase<JsObjectType>::CreateJs( cx, args... ) );
    assert( objectToInstall );

    if ( !JS_DefineProperty( cx, parentObject, propertyName.c_str(), objectToInstall, kDefaultPropsFlags ) )
    {
        throw smp::JsException();
    }
}

} // namespace mozjs
