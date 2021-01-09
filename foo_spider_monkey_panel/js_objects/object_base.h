#pragma once

#include <js_engine/js_realm_inner.h>
#include <js_utils/js_prototype_helpers.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/proxy.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

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
    // If true, object must also define `JsConstructor` and `HasStaticFunctions`.
    static constexpr bool HasGlobalProto;

    // Indicates that object contains static methods.
    // If true, object must also define `JsStaticFunctions`.
    static constexpr bool HasStaticFunctions; 
*/

/*
    Every object must define and initialize the following properties:
    
    // Object's JS class
    // Note: it MUST contain `FinalizeJsObject` from this class!
    const JSClass JsClass; 

    // List of object's JS methods.
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

    // List of object's static JS methods.
    const JSFunctionSpec* JsStaticFunctions; 
*/

/*
    Every object must define and initialize the following methods:
    
    // Creates object T.
    // Must always return a valid object.
    // Throw smp::JsException or qwr::QwrException on error.
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
    // Throw smp::JsException or qwr::QwrException on error.
    static void PostCreate( JSContext* cx, JS::HandleObject self );
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
    [[nodiscard]] static JSObject* CreateProto( JSContext* cx )
    {
        JS::RootedObject jsObject( cx,
                                   JS_NewPlainObject( cx ) );
        if ( !jsObject )
        {
            throw smp::JsException();
        }

        if ( !JS_DefineFunctions( cx, jsObject, T::JsFunctions )
             || !JS_DefineProperties( cx, jsObject, T::JsProperties ) )
        {
            throw smp::JsException();
        }

        return jsObject;
    }

    [[nodiscard]] static JSObject* InstallProto( JSContext* cx, JS::HandleObject parentObject )
    {
        const JSFunctionSpec* staticFns = [] {
            if constexpr ( T::HasStaticFunctions )
            {
                return T::JsStaticFunctions;
            }
            else
            {
                return nullptr;
            }
        }();

        auto pJsProto = JS_InitClass( cx,
                                      parentObject,
                                      nullptr,
                                      &T::JsClass,
                                      T::JsConstructor,
                                      0,
                                      T::JsProperties,
                                      T::JsFunctions,
                                      nullptr,
                                      staticFns );
        if ( !pJsProto )
        {
            throw smp::JsException();
        }
        return pJsProto;
    }

    template <typename... ArgTypes>
    [[nodiscard]] static JSObject* CreateJs( JSContext* cx, ArgTypes&&... args )
    {
        JS::RootedObject jsProto( cx );
        if constexpr ( T::HasProto )
        {
            jsProto = GetProto( cx );
            assert( jsProto );
        }

        JS::RootedObject jsObject( cx, CreateJsObject_Base( cx, jsProto ) );
        assert( jsObject );

        const size_t nativeObjectSize = sizeof( T ) + T::GetInternalSize( args... ); ///< don't forward: don't want to lose those smart ptrs
        std::unique_ptr<T> nativeObject = T::CreateNative( cx, std::forward<ArgTypes>( args )... );
        assert( nativeObject );

        nativeObject->nativeObjectSize_ = nativeObjectSize;

        return CreateJsObject_Final( cx, jsProto, jsObject, std::move( nativeObject ) );
    }

    [[nodiscard]] static JSObject* CreateJsFromNative( JSContext* cx, std::unique_ptr<T> nativeObject )
    {
        JS::RootedObject jsProto( cx );
        if constexpr ( T::HasProto )
        {
            jsProto = GetProto( cx );
            assert( jsProto );
        }

        JS::RootedObject jsObject( cx, CreateJsObject_Base( cx, jsProto ) );
        assert( jsObject );

        nativeObject->nativeObjectSize_ = sizeof( T );

        return CreateJsObject_Final( cx, jsProto, jsObject, std::move( nativeObject ) );
    }

    static void FinalizeJsObject( JSFreeOp* /*fop*/, JSObject* pSelf )
    {
        auto pNative = static_cast<T*>( JS_GetPrivate( pSelf ) );
        if ( pNative )
        {
            auto pJsRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( js::GetNonCCWObjectRealm( pSelf ) ) );
            if ( pJsRealm )
            {
                pJsRealm->OnHeapDeallocate( pNative->nativeObjectSize_ );
            }

            delete pNative;
            JS_SetPrivate( pSelf, nullptr );
        }
    }

private:
    template <typename U = T, std::enable_if_t<U::HasProto, int> = 0>
    [[nodiscard]] static JSObject* GetProto( JSContext* cx )
    {
        JS::RootedObject jsProto( cx );
        if constexpr ( T::HasGlobalProto )
        {
            jsProto = GetPrototype<T>( cx, T::PrototypeId );
        }
        else
        {
            jsProto = GetOrCreatePrototype<T>( cx, T::PrototypeId );
        }
        assert( jsProto );
        return jsProto;
    }

    [[nodiscard]] static JSObject* CreateJsObject_Base( JSContext* cx, [[maybe_unused]] JS::HandleObject jsProto )
    {
        JS::RootedObject jsObject( cx );
        if constexpr ( T::HasProto )
        {
            assert( jsProto );
            jsObject.set( JS_NewObjectWithGivenProto( cx, &T::JsClass, jsProto ) );
            if ( !jsObject )
            {
                throw smp::JsException();
            }
        }
        else
        {
            jsObject.set( JS_NewObject( cx, &T::JsClass ) );
            if ( !jsObject )
            {
                throw smp::JsException();
            }

            if ( !JS_DefineFunctions( cx, jsObject, T::JsFunctions )
                 || !JS_DefineProperties( cx, jsObject, T::JsProperties ) )
            {
                throw smp::JsException();
            }
        }

        return jsObject;
    }

    [[nodiscard]] static JSObject* CreateJsObject_Final( JSContext* cx,
                                                         [[maybe_unused]] JS::HandleObject jsProto,
                                                         JS::HandleObject jsBaseObject,
                                                         std::unique_ptr<T> premadeNative )
    {
        auto pJsRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( js::GetContextRealm( cx ) ) );
        assert( pJsRealm );
        pJsRealm->OnHeapAllocate( premadeNative->nativeObjectSize_ );

        JS_SetPrivate( jsBaseObject, premadeNative.release() );

        if constexpr ( T::HasPostCreate )
        {
            T::PostCreate( cx, jsBaseObject );
        }

        if constexpr ( T::HasProxy )
        {
            JS::RootedValue jsBaseValue( cx, JS::ObjectValue( *jsBaseObject ) );
            JS::RootedObject jsProxyObject( cx, js::NewProxyObject( cx, &T::JsProxy, jsBaseValue, jsProto ) );
            if ( !jsProxyObject )
            {
                throw smp::JsException();
            }
            return jsProxyObject;
        }
        else
        {
            return jsBaseObject;
        }
    }

private:
    uint32_t nativeObjectSize_ = 0;
};

} // namespace mozjs
