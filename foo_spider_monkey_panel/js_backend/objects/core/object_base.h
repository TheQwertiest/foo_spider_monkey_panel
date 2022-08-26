#pragma once

#include <js_backend/engine/js_realm_inner.h>
#include <js_backend/utils/js_object_helper.h>
#include <js_backend/utils/js_prototype_helpers.h>

#include <js/TypeDecls.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Wrapper.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

#include <memory>

namespace js
{
class ProxyOptions;
}

namespace mozjs
{

/*
    Every object must define the following traits:

    // Indicates that object's methods and properties are inherited from it's own JS prototype.
    // If true, object must also define `HasGlobalProto` and `PrototypeId`.
    static constexpr bool HasProto;
*/

/*
    Traits that object might need to define (see above):

    // Indicates that object has a global JS constructor.
    // If true, object must also define `JsConstructor`.
    static constexpr bool HasGlobalProto;
*/

/*
    Optional traits:

    // Indicates that object needs to perform actions on create JS object to finalize it's construction.
    // If true, object must also define `PostCreate`.
    static constexpr bool HasPostCreate;

    // Indicates that object is wrapped in proxy.
    // If true, object must also define `JsProxy`.
    static constexpr bool HasProxy;

    // TODO
    static constexpr bool HasBaseProto;
    static constexpr bool IsExtendable;
    using BaseJsType;
*/

/*
    Every object must define and initialize the following properties:

    // Object's JS class
    // Note: it MUST contain `FinalizeJsObject` from this class!
    const JSClass JsClass;
*/

/*
    Optional properties:

    // List of object's JS methods.
    const JSFunctionSpec* JsFunctions;

    // List of object's static JS methods.
    // Requires HasGlobalProto to be true
    const JSFunctionSpec* JsStaticFunctions;

    // List of object's JS properties
    const JSPropertySpec* JsProperties;
*/

/*
    Properties that object might need to define (see above):

    // Unique id for the object's JS prototype
    const JsPrototypeId PrototypeId;

    // TODO
    const JsPrototypeId BasePrototypeId;
    const JsPrototypeId ParentPrototypeId;

    // Pointer to the object's JS constructor
    const JSNative JsConstructor;

    // Reference to the object's JS proxy
    const js::BaseProxyHandler& JsProxy;
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
    size_t GetInternalSize();
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
    using Self = JsObjectBase<T>;

public:
    // TODO: add check for `trace` and `PrepareForGc` when inherited
    JsObjectBase() = default;
    JsObjectBase( const JsObjectBase& ) = delete;
    JsObjectBase& operator=( const JsObjectBase& ) = delete;
    virtual ~JsObjectBase() = default;

public:
    [[nodiscard]] static JSObject* CreateProto( JSContext* cx )
    {
        JS::RootedObject pParentJsProto( cx, Self::GetParentProto( cx ) );
        JS::RootedObject jsObject( cx,
                                   JS_NewObjectWithGivenProto( cx, nullptr, pParentJsProto ) );
        if ( !jsObject )
        {
            throw smp::JsException();
        }

        if constexpr ( Self::Trait_HasJsFunctions() )
        {
            if ( !JS_DefineFunctions( cx, jsObject, T::JsFunctions ) )
            {
                throw smp::JsException();
            }
        }

        if constexpr ( Self::Trait_HasJsProperties() )
        {
            if ( !JS_DefineProperties( cx, jsObject, T::JsProperties ) )
            {
                throw smp::JsException();
            }
        }

        return jsObject;
    }

    [[nodiscard]] static JSObject* InstallProto( JSContext* cx, JS::HandleObject parentObject )
    {
        const JSFunctionSpec* staticFns = [] {
            if constexpr ( Self::Trait_HasJsStaticFunctions() )
            {
                return T::JsStaticFunctions;
            }
            else
            {
                return nullptr;
            }
        }();

        JS::RootedObject pParentJsProto( cx, Self::GetParentProto( cx ) );

        auto pJsProto = JS_InitClass( cx,
                                      parentObject,
                                      pParentJsProto,
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
        JS::RootedObject jsProto( cx, Self::GetObjectProto( cx ) );
        JS::RootedObject jsObject( cx, CreateJsObject_Base( cx, jsProto ) );
        assert( jsObject );

        std::unique_ptr<T> pNativeObject = T::CreateNative( cx, std::forward<ArgTypes>( args )... );
        assert( pNativeObject );
        pNativeObject->Self::nativeObjectSize_ = sizeof( T ) + pNativeObject->GetInternalSize();

        return CreateJsObject_Final( cx, jsProto, jsObject, std::move( pNativeObject ) );
    }

    [[nodiscard]] static JSObject* CreateJsFromNative( JSContext* cx, std::unique_ptr<T> pNativeObject )
    {
        JS::RootedObject jsProto( cx, Self::GetObjectProto( cx ) );
        JS::RootedObject jsObject( cx, CreateJsObject_Base( cx, jsProto ) );
        assert( jsObject );

        pNativeObject->Self::nativeObjectSize_ = sizeof( T ) + pNativeObject->GetInternalSize();

        return CreateJsObject_Final( cx, jsProto, jsObject, std::move( pNativeObject ) );
    }

    static void FinalizeJsObject( JSFreeOp* /*fop*/, JSObject* pSelf )
    {
        auto pNative = Self::ExtractNativeUnchecked( pSelf );
        if ( pNative )
        {
            auto pJsRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( js::GetNonCCWObjectRealm( pSelf ) ) );
            if ( pJsRealm )
            {
                pJsRealm->OnHeapDeallocate( pNative->Self::nativeObjectSize_ );
            }

            delete pNative;
            JS::SetPrivate( pSelf, nullptr );
        }
    }

    [[nodiscard]] static T* ExtractNative( JSContext* cx, JS::HandleObject jsObject )
    {
        if ( auto pNative = ExtractNativeExact( cx, jsObject ); pNative )
        {
            return pNative;
        }

        return Self::ExtractNativeFuzzy( cx, jsObject );
    }

    [[nodiscard]] static T* ExtractNative( JSContext* cx, JS::HandleValue jsValue )
    {
        if ( !jsValue.isObject() )
        {
            return nullptr;
        }

        JS::RootedObject jsObject( cx, &jsValue.toObject() );
        return Self::ExtractNative( cx, jsObject );
    }

    /// @brief Direct cast from private, does not unwrap value nor checks if JS object actually holds the desired type.
    ///        Should not be used directly with values received from JS code (use ExtractNative() instead).
    [[nodiscard]] static T* ExtractNativeUnchecked( JSObject* jsObject )
    {
        return Self::ExtractNativeFromVoid( JS::GetPrivate( jsObject ) );
    }

private:
    [[nodiscard]] static T* ExtractNativeExact( JSContext* cx, JS::HandleObject jsObject )
    {
        JS::RootedObject jsUnwrappedObject( cx, jsObject );
        if ( js::IsWrapper( jsObject ) )
        {
            jsUnwrappedObject = js::UncheckedUnwrap( jsObject );
        }

        if constexpr ( Self::Trait_HasProxy() )
        {
            if ( js::IsProxy( jsUnwrappedObject ) && js::GetProxyHandler( jsUnwrappedObject )->family() == GetSmpProxyFamily() )
            {
                jsUnwrappedObject = js::GetProxyTargetObject( jsUnwrappedObject );
            }
        }

        auto pVoid = JS_GetInstancePrivate( cx, jsUnwrappedObject, &T::JsClass, nullptr );
        return Self::ExtractNativeFromVoid( pVoid );
    }

    [[nodiscard]] static T* ExtractNativeFuzzy( JSContext* cx, JS::HandleObject jsObject )
    {
        if constexpr ( Self::Trait_IsExtendable() )
        {
            JS::RootedValue jsValue( cx );
            jsValue.setObject( *jsObject );

            JS::RootedObject jsPrototype( cx, utils::GetPrototype( cx, T::PrototypeId ) );
            JS::RootedObject jsConstructor( cx, JS_GetConstructor( cx, jsPrototype ) );
            bool isInstance = false;
            if ( !jsConstructor
                 || !JS_HasInstance( cx, jsConstructor, jsValue, &isInstance ) )
            {
                throw smp::JsException();
            }

            if ( !isInstance )
            {
                return nullptr;
            }

            return Self::ExtractNativeUnchecked( jsObject );
        }
        else
        {
            return nullptr;
        }
    }

    [[nodiscard]] static T* ExtractNativeFromVoid( void* pVoid )
    {
        if constexpr ( Self::Trait_HasParentProto() )
        {
            // TODO: add a proper check instead of naive static_cast
            return static_cast<T*>( static_cast<T::BaseJsType*>( pVoid ) );
        }
        else
        {
            return static_cast<T*>( pVoid );
        }
    }

    [[nodiscard]] static JSObject* GetObjectProto( JSContext* cx )
    {
        if constexpr ( T::HasProto )
        {
            JSObject* jsProto = nullptr;
            if constexpr ( T::HasGlobalProto )
            {
                jsProto = utils::GetPrototype( cx, T::PrototypeId );
            }
            else
            {
                jsProto = utils::GetOrCreatePrototype<T>( cx, T::PrototypeId );
            }
            assert( jsProto );
            return jsProto;
        }
        else
        {
            return Self::GetParentProto( cx );
        }
    }

    [[nodiscard]] static JSObject* GetParentProto( JSContext* cx )
    {
        if constexpr ( Self::Trait_HasParentProto() )
        {
            auto jsProto = utils::GetPrototype( cx, T::ParentPrototypeId );
            assert( jsProto );
            return jsProto;
        }
        else
        {
            return nullptr;
        }
    }

    [[nodiscard]] static JSObject* CreateJsObject_Base( JSContext* cx, JS::HandleObject jsProto )
    {
        JS::RootedObject jsObject( cx, JS_NewObjectWithGivenProto( cx, &T::JsClass, jsProto ) );
        if constexpr ( !T::HasProto )
        {
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
        pJsRealm->OnHeapAllocate( premadeNative->Self::nativeObjectSize_ );

        if constexpr ( Self::Trait_HasParentProto() )
        {
            JS::SetPrivate( jsBaseObject, static_cast<T::BaseJsType*>( premadeNative.release() ) );
        }
        else
        {
            JS::SetPrivate( jsBaseObject, premadeNative.release() );
        }

        if constexpr ( Self::Trait_HasPostCreate() )
        {
            T::PostCreate( cx, jsBaseObject );
        }

        if constexpr ( Self::Trait_HasProxy() )
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

private: // traits
    static constexpr bool Trait_HasPostCreate()
    {
        if constexpr ( requires { T::HasPostCreate; } )
        {
            return T::HasPostCreate;
        }
        else
        {
            return false;
        }
    }

    static constexpr bool Trait_HasProxy()
    {
        if constexpr ( requires { T::HasProxy; } )
        {
            return T::HasProxy;
        }
        else
        {
            return false;
        }
    }

    static constexpr bool Trait_HasParentProto()
    {
        if constexpr ( requires { T::HasParentProto; } )
        {
            return T::HasParentProto;
        }
        else
        {
            return false;
        }
    }

    static constexpr bool Trait_IsExtendable()
    {
        if constexpr ( requires { T::IsExtendable; } )
        {
            return T::IsExtendable;
        }
        else
        {
            return false;
        }
    }

    static constexpr bool Trait_HasJsFunctions()
    {
        if constexpr ( requires( T t ) {
                           {
                               t.JsFunctions
                               }
                               -> std::same_as<const JSFunctionSpec*&>;
                       } )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    static constexpr bool Trait_HasJsStaticFunctions()
    {
        if constexpr ( requires( T t ) {
                           {
                               t.JsStaticFunctions
                               }
                               -> std::same_as<const JSFunctionSpec*&>;
                       } )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    static constexpr bool Trait_HasJsProperties()
    {
        if constexpr ( requires( T t ) {
                           {
                               t.JsProperties
                               }
                               -> std::same_as<const JSPropertySpec*&>;
                       } )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    uint32_t nativeObjectSize_ = 0;
};

} // namespace mozjs
