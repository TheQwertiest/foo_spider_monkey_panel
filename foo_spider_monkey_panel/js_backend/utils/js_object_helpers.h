#pragma once

#include <js_backend/objects/core/object_base.h>

namespace mozjs
{

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
