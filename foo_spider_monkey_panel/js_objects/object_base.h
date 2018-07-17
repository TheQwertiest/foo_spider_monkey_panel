#pragma once

#include <js_engine/js_compartment_inner.h>
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

    template <typename ... ArgTypes>
    static JSObject* CreateJs( JSContext* cx, ArgTypes&&... args )
    {
        JS::RootedObject jsProto( cx );
        if constexpr ( T::HasProto )
        {
            jsProto = GetProto( cx );
            if ( !jsProto )
            {// report in GetPrototype
                return nullptr;
            }
        }

        JS::RootedObject jsObject( cx, CreateJsObject_Base(cx, jsProto) );
        if ( !jsObject )
        {// report in CreateJsObjectInternal
            return nullptr;
        }

        const size_t nativeObjectSize = sizeof( T ) + T::GetInternalSize( args... ); ///< don't forward: don't want to lose those smart ptrs
        std::unique_ptr<T> nativeObject = T::CreateNative( cx, std::forward<ArgTypes>( args )... );
        if ( !nativeObject )
        {// report in CreateNative
            return nullptr;
        }
        nativeObject->nativeObjectSize_ = nativeObjectSize;

        return CreateJsObject_Final( cx, jsProto, jsObject, std::move(nativeObject) );
    }
    
    static JSObject* CreateJsFromNative( JSContext* cx, std::unique_ptr<T> nativeObject )
    {
        JS::RootedObject jsProto( cx );
        if constexpr (T::HasProto)
        {
            jsProto = GetProto( cx );
            if ( !jsProto )
            {// report in GetPrototype
                return nullptr;
            }
        }

        JS::RootedObject jsObject( cx, CreateJsObject_Base( cx, jsProto ) );
        if ( !jsObject )
        {// report in CreateJsObjectInternal
            return nullptr;
        }

        nativeObject->nativeObjectSize_ = sizeof( T );

        return CreateJsObject_Final( cx, jsProto, jsObject, std::move( nativeObject ) );
    }

    static void FinalizeJsObject( JSFreeOp* fop, JSObject* pSelf )
    {
        auto pNative = static_cast<T*>(JS_GetPrivate( pSelf ));
        if ( pNative )
        {
            auto pJsCompartment = static_cast<JsCompartmentInner*>( JS_GetCompartmentPrivate( js::GetObjectCompartment( pSelf ) ));
            if ( pJsCompartment )
            {
                pJsCompartment->OnHeapDeallocate( pNative->nativeObjectSize_ );
            }

            delete pNative;
            JS_SetPrivate( pSelf, nullptr );
        }
    }

private:
    static JSObject* GetProto( JSContext* cx )
    {
        JS::RootedObject jsProto( cx );
        if constexpr (T::HasGlobalProto)
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

        return jsProto;
    }

    static JSObject* CreateJsObject_Base( JSContext* cx, JS::HandleObject jsProto )
    {
        JS::RootedObject jsObject( cx );
        if constexpr (T::HasProto)
        {
            assert( jsProto );
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

        return jsObject;
    }

    static JSObject* CreateJsObject_Final( JSContext* cx, 
                                           JS::HandleObject jsProto, 
                                           JS::HandleObject jsBaseObject, 
                                           std::unique_ptr<T> premadeNative )
    {
        auto pJsCompartment = static_cast<JsCompartmentInner*>(JS_GetCompartmentPrivate( js::GetContextCompartment( cx ) ));
        assert( pJsCompartment );
        pJsCompartment->OnHeapAllocate( premadeNative->nativeObjectSize_ );

        JS_SetPrivate( jsBaseObject, premadeNative.release() );

        if constexpr (T::HasProxy)
        {
            JS::RootedValue priv( cx, JS::ObjectValue( *jsBaseObject ) );

            js::ProxyOptions options;
            return js::NewProxyObject( cx, &T::JsProxy, priv, jsProto, options );;
        }
        else
        {
            return jsBaseObject;
        }
    }

private:
    uint32_t nativeObjectSize_ = 0;

};

}
