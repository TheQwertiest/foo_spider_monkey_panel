#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_utils/js_object_helper.h>

#include <optional>

namespace mozjs
{

template <typename T>
class JsObjectFactory : public T
{
    using TT = typename T::ObjectType;
public:
    JsObjectFactory()
        : ops_{ nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                JsFinalizeOp<TT>,
                nullptr,
                nullptr,
                nullptr,
                nullptr }
        , class_{ T::className,
                  JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE | T::classFlags,
                  &ops_ }
    {
    }

    virtual ~JsObjectFactory()
    {

    }

    template <typename ... ArgTypes>
    JSObject* Create( JSContext* cx, ArgTypes&& ... args )
    {
        if ( !T::ValidateCtorArguments( cx, args... ) )
        {
            return nullptr;
        }

        JS::RootedObject jsObj( cx,
                                JS_NewObject( cx, &class_ ) );
        if ( !jsObj )
        {
            return nullptr;
        }

        if ( !JS_DefineFunctions( cx, jsObj, T::functions )
             || !JS_DefineProperties( cx, jsObj, T::properties ) )
        {
            return nullptr;
        }

        JS_SetPrivate( jsObj, new TT( cx, args... ) );

        return jsObj;
    }

    const JSClass& GetClass()
    {
        return class_;
    }

private:
    JsObjectFactory( const JsObjectFactory& ) = delete;
    JsObjectFactory& operator=( const JsObjectFactory& ) = delete;

private:
    JSClassOps ops_;
    JSClass class_;
};

}
