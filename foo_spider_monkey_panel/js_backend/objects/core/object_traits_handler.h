#pragma once

#include <js_backend/objects/core/object_traits.h>

#include <js/TypeDecls.h>
SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Wrapper.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

#include <type_traits>

namespace mozjs
{

namespace internal
{

template <typename T>
concept TraitsWithParentJsType = requires {
    typename JsObjectTraits<T>::ParentJsType;
};

template <typename T>
struct BaseJsTypeGetter
{
    using type = T;
};

template <TraitsWithParentJsType T>
struct BaseJsTypeGetter<T>
{
    using type = BaseJsTypeGetter<typename JsObjectTraits<typename T>::ParentJsType>::type;
};

} // namespace internal

template <typename TraitsT, typename ObjectT>
class JsObjectTraitsHandler
// TODO: inherit from TraitsT after migrating
{
public:
    using BaseJsType = mozjs::internal::BaseJsTypeGetter<ObjectT>::type;

public:
    static constexpr bool IsValid()
    {
        static_assert(
            requires( TraitsT t ) {
                {
                    TraitsT::JsClass
                }
                -> std::same_as<const JSClass&>;
            }, "Object must always define it's JS class" );
        // TODO: fails, since objects might have multiple CreateNative methods
        /*
        static_assert(
            requires {
                &ObjectT::CreateNative;
            }, "Object must define a factory method for itself" );
            */
        static_assert(
            requires {
                &ObjectT::GetInternalSize;
            }, "Object must define a method that returns it's internal size" );

        if constexpr ( requires { TraitsT::JsFunctions; } )
        {
            static_assert( Trait_IsValidTypeJsFunctions(), "Invalid type of JsFunctions field" );
        }

        if constexpr ( requires { TraitsT::JsStaticFunctions; } )
        {
            static_assert( Trait_HasGlobalProto(), "Object must have a global prototype to be able to define static methods" );
            static_assert( Trait_IsValidTypeJsStaticFunctions(), "Invalid type of JsFunctions field" );
        }

        if constexpr ( requires { TraitsT::JsProperties; } )
        {
            static_assert( Trait_IsValidTypeJsProperties(), "Invalid type of JsProperties field" );
        }

        static_assert(
            requires { TraitsT::HasProto; }, "Object must always explicitly define if it has a prototype" );
        if constexpr ( TraitsT::HasProto )
        {
            static_assert(
                requires {
                    TraitsT::PrototypeId;
                },
                "Object must reserve and define a unique id for it's prototype when it has the prototype enabled" );
        }

        if constexpr ( Trait_HasGlobalProto() )
        {
            static_assert(
                requires( TraitsT t ) {
                    {
                        t.JsConstructor
                    }
                    -> std::same_as<const JSNative&>;
                }, "Object must define a JS constructor when it has a global prototype enabled" );
        }

        if constexpr ( Trait_HasParentProto() )
        {
            static_assert(
                requires { typename TraitsT::ParentJsType; }, "Object must define the type of it's parent object for derivation to work" );
            static_assert(
                requires { requires mozjs::JsObjectTraits<TraitsT::ParentJsType>::IsExtendable; }, "Parent object must be extendable for derivation to work" );
        }

        if constexpr ( Trait_HasProxy() )
        {
            static_assert(
                requires( TraitsT t ) {
                    {
                        TraitsT::JsProxy
                    }
                    -> std::same_as<const js::BaseProxyHandler&>;
                }, "Object must set a reference to it's proxy handler type when it has the proxy wrapper enabled" );
        }

        if constexpr ( Trait_HasPostCreate() )
        {
            static_assert(
                requires( JSContext* cx, JS::HandleObject self ) {
                    {
                        TraitsT::PostCreate( cx, self )
                    }
                    -> std::same_as<void>;
                }, "Object must define a `post create` method if it has the corresponding trait enabled" );
        }

        return true;
    }

    // TODO: maybe move these to Traits struct instead

public:
    static constexpr bool Trait_HasGlobalProto()
    {
        return requires { requires TraitsT::HasGlobalProto; };
    }

    static constexpr bool Trait_HasPostCreate()
    {
        return requires { requires TraitsT::HasPostCreate; };
    }

    static constexpr bool Trait_HasProxy()
    {
        return requires { requires TraitsT::HasProxy; };
    }

    static constexpr bool Trait_HasParentProto()
    {
        return requires { requires TraitsT::HasParentProto; };
    }

    static constexpr bool Trait_IsExtendable()
    {
        return requires { requires TraitsT::IsExtendable; };
    }

    static constexpr bool Trait_IsExtendableRecursive()
    {
        return requires { requires TraitsT::HasGlobalProto; };
    }

    static constexpr bool Trait_IsValidTypeJsFunctions()
    {
        return requires( TraitsT t ) {
            {
                t.JsFunctions
            }
            -> std::same_as<const JSFunctionSpec*&>;
        };
    }

    static constexpr const JSFunctionSpec* Trait_GetJsFunctions()
    {
        if constexpr ( Trait_IsValidTypeJsFunctions() )
        {
            return TraitsT::JsFunctions;
        }
        else
        {
            return nullptr;
        }
    }

    static constexpr bool Trait_IsValidTypeJsStaticFunctions()
    {
        return requires( TraitsT t ) {
            {
                t.JsStaticFunctions
            }
            -> std::same_as<const JSFunctionSpec*&>;
        };
    }

    static constexpr const JSFunctionSpec* Trait_GetJsStaticFunctions()
    {
        if constexpr ( Trait_IsValidTypeJsStaticFunctions() )
        {
            return TraitsT::JsStaticFunctions;
        }
        else
        {
            return nullptr;
        }
    }

    static constexpr bool Trait_IsValidTypeJsProperties()
    {
        return requires( TraitsT t ) {
            {
                t.JsProperties
            }
            -> std::same_as<const JSPropertySpec*&>;
        };
    }

    static constexpr const JSPropertySpec* Trait_GetJsProperties()
    {
        if constexpr ( Trait_IsValidTypeJsProperties() )
        {
            return TraitsT::JsProperties;
        }
        else
        {
            return nullptr;
        }
    }
};

} // namespace mozjs
