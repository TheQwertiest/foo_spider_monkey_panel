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

template <typename T>
class JsObjectBase
{
public:
    JsObjectBase()
    {

    }
    virtual ~JsObjectBase()
    {

    }

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
    static JSObject* Create( JSContext* cx, ArgTypes&&... args )
    {
        JS::RootedObject jsObject( cx );
        if constexpr ( T::HasProto )
        {
            JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
            assert( jsGlobal );

            auto pNativeGlobal = static_cast<JsGlobalObject*>( JS_GetPrivate( jsGlobal ) );
            assert( pNativeGlobal );

            JS::RootedObject jsProto( cx, pNativeGlobal->GetPrototype<T>( jsGlobal ) );
            assert( jsProto );

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
            {
                return nullptr;
            }
        }

        if ( !T::ValidateCreateArgs( cx, args... ) )
        {// report in ValidateCtorArgs
            return nullptr;
        }

        std::unique_ptr<T> nativeObject( new T( cx, args... ) );
        if ( !nativeObject->PostCreate( cx, args... ) )
        {// report in PostCreate
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

private:
    JsObjectBase(const JsObjectBase& ) = delete;
    JsObjectBase& operator=( const JsObjectBase& ) = delete;
};

}
