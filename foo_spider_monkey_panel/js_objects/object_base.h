#pragma once

// TODO: replace with prototype_helper.h
#include <js_utils/js_object_helper.h>

class JSObject;
struct JSContext;
struct JSClass;

namespace js
{

class ProxyOptions;

}

namespace mozjs
{
static uint32_t gcCounter = 0;
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
        JS::RootedObject jsProto( cx );
        if constexpr ( T::HasProto )
        {
            JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
            assert( jsGlobal );

            if constexpr ( T::HasGlobalProto )
            {
                jsProto = GetPrototype<T>( cx, jsGlobal, T::PrototypeId );
            }
            else
            {
                jsProto = GetOrCreatePrototype<T>( cx, jsGlobal, T::PrototypeId );
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
        JS_updateMallocCounter( cx, nativeObjectSize );

        // TODO: implement a better GC mechanism
        if ( !(gcCounter % 100) )
        {
            uint32_t bytes = JS_GetGCParameter( cx, JSGC_BYTES );
            std::wstring tmp = std::to_wstring( bytes ) + L'\n';

            OutputDebugString( tmp.c_str() );
            //JS_MaybeGC( cx );        
        }
        if ( gcCounter > 1000 )
        {
            //JS_GC( cx );
            gcCounter = 0;
        }
        ++gcCounter;

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
};

}
