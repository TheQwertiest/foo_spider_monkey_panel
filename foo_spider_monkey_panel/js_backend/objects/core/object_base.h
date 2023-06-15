#pragma once

#include <js_backend/engine/js_realm_inner.h>
#include <js_backend/objects/core/object_traits_handler.h>
#include <js_backend/utils/js_object_constants.h>
#include <js_backend/utils/js_prototype_helpers.h>

#include <js/TypeDecls.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Wrapper.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

#include <memory>

namespace mozjs
{

// TODO: extract JsObjectBase from inherited class to a standalone
// TODO: rename GetNativeSize to SizeOfExcludingThis

// Methods required by T of JsObjectBase<T>

/*
// Object *must* define a factory method.
//
// Creates object T.
// Must always return a valid object.
// Throw smp::JsException or qwr::QwrException on error.
static std::unique_ptr<T> CreateNative( JSContext* cx, Args... args );
*/

/*
// Object *must* define a method that returns it's internal size.
//
// Uses same args as CreateNative
//
// Returns the size of properties of T, that can't be calculated by sizeof(T).
// E.g. if T has property `std::unique_ptr<BigStruct> bigStruct_`, then
// `GetInternalSize` must return sizeof( bigStruct_ ).
size_t GetInternalSize(Args... args);
*/

// TODO: cleanup docs
/// @remark See JsObjectTraitsHandler::IsValid method for description of the required structure of T object.
///
/// Grant friend access to required classes via MOZJS_ENABLE_OBJECT_BASE_ACCESS macro (see below).
/// Verify object correctness via MJS_VERIFY_OBJECT macro (define it in .cpp file to avoid compilation time hit).
template <typename T>
class JsObjectBase
{
    using Self = JsObjectBase<T>;

public:
    using TraitsT = std::conditional_t<requires { requires JsObjectTraits<T>::kIsFake; }, T, JsObjectTraits<T>>;
    using TraitsHandlerT = JsObjectTraitsHandler<TraitsT, T>;

public:
    // TODO: add check for `trace` and `PrepareForGc` when inherited
    JsObjectBase() = default;
    JsObjectBase( const JsObjectBase& ) = delete;
    JsObjectBase& operator=( const JsObjectBase& ) = delete;
    virtual ~JsObjectBase() = default;

public:
    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] static JSObject* CreateProto( JSContext* cx )
    {
        JS::RootedObject pParentJsProto( cx, Self::GetParentProto( cx ) );
        JS::RootedObject jsObject( cx,
                                   JS_NewObjectWithGivenProto( cx, nullptr, pParentJsProto ) );
        if ( !jsObject )
        {
            throw smp::JsException();
        }

        DefinePropertiesAndFunctions( cx, jsObject, TraitsHandlerT::Trait_GetJsProperties(), TraitsHandlerT::Trait_GetJsFunctions() );

        return jsObject;
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] static JSObject* InstallProto( JSContext* cx, JS::HandleObject parentObject )
    {
        JS::RootedObject pParentJsProto( cx, Self::GetParentProto( cx ) );

        auto pJsProto = JS_InitClass( cx,
                                      parentObject,
                                      pParentJsProto,
                                      &TraitsT::JsClass,
                                      TraitsT::JsConstructor,
                                      0,
                                      TraitsHandlerT::Trait_GetJsProperties(),
                                      TraitsHandlerT::Trait_GetJsFunctions(),
                                      nullptr,
                                      TraitsHandlerT::Trait_GetJsStaticFunctions() );
        if ( !pJsProto )
        {
            throw smp::JsException();
        }
        return pJsProto;
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    template <typename... ArgTypes>
    [[nodiscard]] static JSObject* CreateJs( JSContext* cx, ArgTypes&&... args )
    {
        JS::RootedObject jsProto( cx, Self::GetObjectProto( cx ) );
        JS::RootedObject jsObject( cx, CreateJsObject_Base( cx, jsProto ) );
        assert( jsObject );

        // TODO: replace unique_ptr with not_null_unique
        std::unique_ptr<T> pNativeObject = T::CreateNative( cx, std::forward<ArgTypes>( args )... );
        assert( pNativeObject );
        pNativeObject->Self::nativeObjectSize_ = sizeof( T ) + pNativeObject->GetInternalSize();

        return CreateJsObject_Final( cx, jsProto, jsObject, std::move( pNativeObject ) );
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] static JSObject* CreateJsFromNative( JSContext* cx, std::unique_ptr<T> pNativeObject )
    {
        JS::RootedObject jsProto( cx, Self::GetObjectProto( cx ) );
        JS::RootedObject jsObject( cx, CreateJsObject_Base( cx, jsProto ) );
        assert( jsObject );

        pNativeObject->Self::nativeObjectSize_ = sizeof( T ) + pNativeObject->GetInternalSize();

        return CreateJsObject_Final( cx, jsProto, jsObject, std::move( pNativeObject ) );
    }

    static void FinalizeJsObject( JS::GCContext* /*gcCtx*/, JSObject* pSelf )
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
            JS::SetReservedSlot( pSelf, kReservedObjectSlot, JS::UndefinedValue() );
        }
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] static T* ExtractNative( JSContext* cx, JS::HandleObject jsObject )
    {
        if ( auto pNative = ExtractNativeExact( cx, jsObject ); pNative )
        {
            return pNative;
        }

        return Self::ExtractNativeFuzzy( cx, jsObject );
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
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
        return Self::ExtractNativeFromVoid( mozjs::utils::GetMaybePtrFromReservedSlot( jsObject, kReservedObjectSlot ) );
    }

private:
    [[nodiscard]] static T* ExtractNativeExact( JSContext* cx, JS::HandleObject jsObject )
    {
        JS::RootedObject jsUnwrappedObject( cx, jsObject );
        if ( js::IsWrapper( jsObject ) )
        {
            jsUnwrappedObject = js::UncheckedUnwrap( jsObject );
        }

        if constexpr ( TraitsHandlerT::Trait_HasProxy() )
        {
            if ( js::IsProxy( jsUnwrappedObject ) && js::GetProxyHandler( jsUnwrappedObject )->family() == GetSmpProxyFamily() )
            {
                jsUnwrappedObject = js::GetProxyTargetObject( jsUnwrappedObject );
            }
        }

        auto pVoid = mozjs::utils::GetInstanceFromReservedSlot( cx, jsUnwrappedObject, &TraitsT::JsClass, nullptr );
        return Self::ExtractNativeFromVoid( pVoid );
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] static T* ExtractNativeFuzzy( JSContext* cx, JS::HandleObject jsObject )
    {
        if constexpr ( TraitsHandlerT::Trait_IsExtendable() )
        {
            JS::RootedValue jsValue( cx );
            jsValue.setObject( *jsObject );

            JS::RootedObject jsPrototype( cx, utils::GetPrototype( cx, TraitsT::PrototypeId ) );
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
        if constexpr ( TraitsHandlerT::Trait_HasParentProto() )
        {
            // TODO: add a proper check instead of naive static_cast
            // e.g. struct{int magic, void* native};
            return static_cast<T*>( static_cast<TraitsHandlerT::BaseJsType*>( pVoid ) );
        }
        else
        {
            return static_cast<T*>( pVoid );
        }
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] static JSObject* GetObjectProto( JSContext* cx )
    {
        if constexpr ( TraitsT::HasProto )
        {
            JSObject* jsProto = nullptr;
            if constexpr ( TraitsHandlerT::Trait_HasGlobalProto() )
            {
                jsProto = utils::GetPrototype( cx, TraitsT::PrototypeId );
            }
            else
            {
                // TODO: replace Self with T
                jsProto = utils::GetOrCreatePrototype<Self>( cx, TraitsT::PrototypeId );
            }
            assert( jsProto );
            return jsProto;
        }
        else
        {
            return Self::GetParentProto( cx );
        }
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] static JSObject* GetParentProto( JSContext* cx )
    {
        if constexpr ( TraitsHandlerT::Trait_HasParentProto() )
        {
            auto jsProto = utils::GetPrototype( cx, JsObjectTraits<TraitsT::ParentJsType>::PrototypeId );
            assert( jsProto );
            return jsProto;
        }
        else
        {
            return nullptr;
        }
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] static JSObject* CreateJsObject_Base( JSContext* cx, JS::HandleObject jsProto )
    {
        JS::RootedObject jsObject( cx, JS_NewObjectWithGivenProto( cx, &TraitsT::JsClass, jsProto ) );
        if constexpr ( !TraitsT::HasProto )
        {
            DefinePropertiesAndFunctions( cx, jsObject, TraitsHandlerT::Trait_GetJsProperties(), TraitsHandlerT::Trait_GetJsFunctions() );
        }

        return jsObject;
    }

    /// @throw qwr::QwrException
    /// @throw smp::JsException
    [[nodiscard]] static JSObject* CreateJsObject_Final( JSContext* cx,
                                                         [[maybe_unused]] JS::HandleObject jsProto,
                                                         JS::HandleObject jsBaseObject,
                                                         std::unique_ptr<T> premadeNative )
    {
        auto pJsRealm = static_cast<JsRealmInner*>( JS::GetRealmPrivate( js::GetContextRealm( cx ) ) );
        assert( pJsRealm );
        pJsRealm->OnHeapAllocate( premadeNative->Self::nativeObjectSize_ );

        if constexpr ( TraitsHandlerT::Trait_HasParentProto() )
        {
            JS::SetReservedSlot( jsBaseObject,
                                 kReservedObjectSlot,
                                 JS::PrivateValue( static_cast<TraitsHandlerT::BaseJsType*>( premadeNative.release() ) ) );
        }
        else
        {
            JS::SetReservedSlot( jsBaseObject,
                                 kReservedObjectSlot,
                                 JS::PrivateValue( premadeNative.release() ) );
        }

        if constexpr ( TraitsHandlerT::Trait_HasPostCreate() )
        {
            TraitsT::PostCreate( cx, jsBaseObject );
        }

        if constexpr ( TraitsHandlerT::Trait_HasProxy() )
        {
            JS::RootedValue jsBaseValue( cx, JS::ObjectValue( *jsBaseObject ) );
            JS::RootedObject jsProxyObject( cx, js::NewProxyObject( cx, &TraitsT::JsProxy, jsBaseValue, jsProto ) );
            smp::JsException::ExpectTrue( jsProxyObject );

            return jsProxyObject;
        }
        else
        {
            return jsBaseObject;
        }
    }

private: // non trait related helpers
    /// @throw qwr::QwrException
    /// @throw smp::JsException
    static void DefinePropertiesAndFunctions( JSContext* cx,
                                              JS::HandleObject jsObject,
                                              const JSPropertySpec* ps,
                                              const JSFunctionSpec* fs )
    {
        if ( fs && !JS_DefineFunctions( cx, jsObject, fs ) )
        {
            throw smp::JsException();
        }
        if ( ps && !JS_DefineProperties( cx, jsObject, ps ) )
        {
            throw smp::JsException();
        }
    }

private:
    uint32_t nativeObjectSize_ = 0;
};

} // namespace mozjs

#define MOZJS_ENABLE_OBJECT_BASE_ACCESS( T ) \
    friend class JsObjectBase<T>;            \
    friend class JsObjectTraitsHandler<JsObjectTraits<T>, T>

/// @brief Verifies that all common object traits and fields are set correctly
#define MJS_VERIFY_OBJECT( T ) \
    static_assert( mozjs::JsObjectBase<T>::TraitsHandlerT::IsValid() )
