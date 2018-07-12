#pragma once

class JSObject;
struct JSContext;
struct JSClass;

namespace js
{

class ProxyOptions;

}

namespace mozjs
{

enum class JsPrototypeId : uint32_t;

template <typename T>
class JsObjectBase
{
public:
    JsObjectBase() = default;
    JsObjectBase( const JsObjectBase& ) = delete;
    JsObjectBase& operator=( const JsObjectBase& ) = delete;
    virtual ~JsObjectBase() = default;

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

    // TODO: rename to CreateJs

    template <typename ... ArgTypes>
    static JSObject* Create( JSContext* cx, ArgTypes&&... args )
    {
        JS::RootedObject jsObject( cx );
        if constexpr ( T::HasProto )
        {
            JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
            assert( jsGlobal );

            JSObject* pRawJsProto = nullptr; ///< Raw JS pointer, be careful!
            if constexpr ( T::HasGlobalProto )
            {
                pRawJsProto = GetPrototype<T>( cx, jsGlobal, T::PrototypeId );
            }
            else
            {
                pRawJsProto = GetOrCreatePrototype<T>( cx, jsGlobal, T::PrototypeId );
            }
            JS::RootedObject jsProto( cx, pRawJsProto );
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
            jsObject.set( JS_NewObject( cx, &jsClass ) );
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

        std::unique_ptr<T> nativeObject = T::CreateNative( cx, std::forward<ArgTypes>( args )... );
        if ( !nativeObject )
        {// report in CreateNative
            return nullptr;
        }

        JS_SetPrivate( jsObject, nativeObject.release() );

        if constexpr ( T::HasProxy )
        {
            JS::RootedValue priv( cx, JS::ObjectValue( *jsObject ) );

            js::ProxyOptions options;
            return js::NewProxyObject( cx, &T::GetProxy(), priv, jsProto, options );;
        }
        else
        {
            return jsObject;
        }
    }
};

}
