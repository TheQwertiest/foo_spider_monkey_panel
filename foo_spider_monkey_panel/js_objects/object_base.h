#pragma once

#include <js_engine/js_compartment_inner.h>
#include <js_utils/js_prototype_helpers.h>
#include <memory>

class JSObject;
struct JSContext;
struct JSClass;

namespace js
{

class ProxyOptions;

}

namespace mozjs
{

/*
    Every object must define the following traits:
     
    // Indicates that object is created from JS prototype.
    // If true, object must also define `HasGlobalProto` and `PrototypeId`.
    static constexpr bool HasProto; 

    // Indicates that object is wrapped in proxy.
    // If true, object must also define `JsProxy`.
    static constexpr bool HasProxy;

    // Indicates that object needs to perform actions on create JS object to finalize it's construction.
    // If true, object must also define `PostCreate`.
    static constexpr bool HasPostCreate;
*/

/*
    Traits that object might need to define (see above):
    
    // Indicates that object has a global JS constructor.
    // If true, object must also define `JsConstructor`.
    static constexpr bool HasGlobalProto = true;
*/

/*
    Every object must define and initialize the following properties:
    
    // Object's JS class
    const JSClass JsClass; 

    // List of object's JS methods.
    // Note: it MUST contain `FinalizeJsObject` from this class!
    const JSFunctionSpec* JsFunctions; 

    // List of object's JS properties
    const JSPropertySpec* JsProperties; 
*/

/*
    Properties that object might need to define (see above):
    
    // Unique id for the object's JS prototype
    const JsPrototypeId PrototypeId; 

    // Pointer to the object's JS constructor
    const JSNative JsConstructor; 

    // Reference to the object's JS proxy
    const js::BaseProxyHandler& JsProxy; 
*/

/*
    Every object must define and initialize the following methods:
    
    // Creates object T
    static std::unique_ptr<T> CreateNative( JSContext* cx, Args... args );

    // Returns the size of properties of T, that can't be calculated by sizeof(T).
    // E.g. if T has property `std::unique_ptr<BigStruct> bigStruct_`, then 
    // `GetInternalSize` must return sizeof( bigStruct_ ).
    // Note: `args` is the same as in `CreateNative`.
    static size_t GetInternalSize( Args... args );
*/

/*
    Methods that object might need to define (see above):
    
    // Finalizes the JS object that contains T.
    // Called before JS object is wrapped in proxy (if `HasProxy` is true).
    static bool PostCreate( JSContext* cx, JS::HandleObject self );
*/


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

    static JSObject* InstallProto( JSContext* cx, JS::HandleObject parentObject )
    {
        return JS_InitClass( cx, parentObject, nullptr, &T::JsClass, T::JsConstructor, 0, T::JsProperties, T::JsFunctions, nullptr, nullptr );
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

    static void FinalizeJsObject( JSFreeOp* /*fop*/, JSObject* pSelf )
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
    template <typename = typename std::enable_if_t<T::HasProto>>
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

    static JSObject* CreateJsObject_Base( JSContext* cx, [[maybe_unused]] JS::HandleObject jsProto )
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
                                           [[maybe_unused]] JS::HandleObject jsProto, 
                                           JS::HandleObject jsBaseObject, 
                                           std::unique_ptr<T> premadeNative )
    {
        auto pJsCompartment = static_cast<JsCompartmentInner*>(JS_GetCompartmentPrivate( js::GetContextCompartment( cx ) ));
        assert( pJsCompartment );
        pJsCompartment->OnHeapAllocate( premadeNative->nativeObjectSize_ );

        JS_SetPrivate( jsBaseObject, premadeNative.release() );

        if constexpr ( T::HasPostCreate )
        {
            if ( !T::PostCreate( cx, jsBaseObject ) )
            {// report in PostFinal
                return nullptr;
            }
        }

        if constexpr (T::HasProxy)
        {
            JS::RootedValue priv( cx, JS::ObjectValue( *jsBaseObject ) );
            return js::NewProxyObject( cx, &T::JsProxy, priv, jsProto );
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
