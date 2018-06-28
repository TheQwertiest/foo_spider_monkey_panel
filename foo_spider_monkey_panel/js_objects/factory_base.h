#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <optional>

namespace mozjs
{

template <typename T>
class JsObjectFactoryBase
{
public:
    virtual ~JsObjectFactoryBase()
    {

    }

    template <typename ... ArgTypes>
    JSObject* Create( JSContext* cx, ArgTypes&& ... args )
    {
        if ( !T::ValidateCtorArguments( arg... ) )
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

        JS_SetPrivate( jsObj, new T( cx, args... ) );

        return jsObj;
    }

    const JSClass& GetClass()
    {
        return class_;
    }

private:
    JsObjectFactoryBase()
        : ops_( nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                JsFinalizeOp<T>,
                nullptr,
                nullptr,
                nullptr,
                nullptr )
        , class_( T::GetClassName(),
                  JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE | T::GetClassFlags(),
                  &ops_ )
        ,
    {
    }
    JsObjectFactoryBase( const JsObjectFactoryBase& ) = delete;

private:
    JSClassOps ops_;
    JSClass class_;
};

}
