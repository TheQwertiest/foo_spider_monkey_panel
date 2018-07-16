#pragma once

#include <js_engine/js_engine_proxy.h>
#include <js_utils/js_prototype_helpers.h>

class JSObject;
struct JSContext;
struct JSClass;

namespace js
{

class ProxyOptions;

}

namespace mozjs
{

template <typename T>
class JsObjectBase
{
public:
    JsObjectBase() = default;
    JsObjectBase( const JsObjectBase& ) = delete;
    JsObjectBase& operator=( const JsObjectBase& ) = delete;
    virtual ~JsObjectBase()
    {
        UpdateJsEngineOnHeapDeallocate( nativeObjectSize_ );
    };

public:
    static JSObject* CreateProto( JSContext* cx )
    {
        JS::RootedObject jsObject( cx,
                                   JS_NewPlainObject( cx ) );
        if ( !jsObject )
        {
            return nullptr;
        }

        if ( !JS_DefineFunctions( cx, jsObject, T::JsFunctions )
             || !JS_DefineProperties( cx, jsObject, T::JsProperties ) )
        {
            return nullptr;
        }

        return jsObject;
    }

    template <typename ... ArgTypes>
    static JSObject* CreateJs( JSContext* cx, ArgTypes&&... args )
    {
        JS::RootedObject jsObject( cx );
        JS::RootedObject jsProto( cx );
        if constexpr ( T::HasProto )
        {
            if constexpr ( T::HasGlobalProto )
            {
                jsProto = GetPrototype<T>( cx, T::PrototypeId );
            }
            else
            {
                jsProto = GetOrCreatePrototype<T>( cx, T::PrototypeId );
            }
            if ( !jsProto )
            {// report in GetPrototype
                return nullptr;
            }

            jsObject.set( JS_NewObjectWithGivenProto( cx, &T::JsClass, jsProto ) );
            if ( !jsObject )
            {// report in JS_NewObjectWithGivenProto
                return nullptr;
            }
        }
        else
        {
            jsObject.set( JS_NewObject( cx, &T::JsClass ) );
            if ( !jsObject )
            {// report in JS_NewObject
                return nullptr;
            }

            if ( !JS_DefineFunctions( cx, jsObject, T::JsFunctions )
                 || !JS_DefineProperties( cx, jsObject, T::JsProperties ) )
            {// report in JS_Define
                return nullptr;
            }
        }

        const size_t nativeObjectSize = sizeof( T ) + T::GetInternalSize( args... ); ///< don't forward: don't want to lose those smart ptrs
        std::unique_ptr<T> nativeObject = T::CreateNative( cx, std::forward<ArgTypes>( args )... );
        if ( !nativeObject )
        {// report in CreateNative
            return nullptr;
        }

        UpdateJsEngineOnHeapAllocate( nativeObjectSize );
        nativeObject->nativeObjectSize_ = nativeObjectSize;

        JS_SetPrivate( jsObject, nativeObject.release() );

        if constexpr ( T::HasProxy )
        {
            JS::RootedValue priv( cx, JS::ObjectValue( *jsObject ) );

            js::ProxyOptions options;
            return js::NewProxyObject( cx, &T::JsProxy, priv, jsProto, options );;
        }
        else
        {
            return jsObject;
        }
    }

private:
    uint32_t nativeObjectSize_ = 0;

};

}
