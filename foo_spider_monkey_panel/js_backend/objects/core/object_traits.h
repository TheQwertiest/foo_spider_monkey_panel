#pragma once

#include <qwr/type_traits.h>

namespace mozjs
{

using PostJsCreateFn = void ( * )( JSContext* cx, JS::HandleObject self );

template <typename T>
struct JsObjectTraits
{
    static constexpr bool kIsFake = true;
    // TODO: uncomment after migration
    // static_assert( qwr::always_false_v<T>, "Unsupported type" );

    /*
    // Indicates whether object's methods and properties are inherited from it's own JS prototype.
    // Should be always true for non singleton objects, so as to avoid performance impact during instantiation.
    // If true, object must also define `HasGlobalProto` and `PrototypeId`.
    static constexpr bool HasProto;
    */

    /*
    // Indicates that object has a global JS constructor.
    // If true, object must also define `JsConstructor`.
    static constexpr bool HasGlobalProto;
    */

    /*
    // Indicates that object is derived from another object's prototype.
    // If true, must also define ParentJsType and requires Traits<ParentJsType>::IsExtendable == true.
    static constexpr bool HasParentProto;
    */

    /*
    // Indicates that object needs to perform actions on create JS object to finalize it's construction.
    // If true, object must also define `PostCreate`.
    static constexpr bool HasPostCreate;
    */

    /*
    // Indicates that object is wrapped in proxy.
    // If true, object must also define `JsProxy`.
    static constexpr bool HasProxy;

    // TODO: consider making all objects with constructor extendable
    /*
    // Indicates that object can be derived from.
    static constexpr bool IsExtendable;
    */

    // TODO: fix description
    /*
    // Object's JS class. Must be always defined.
    // Note: it MUST contain `FinalizeJsObject` from ???this class???!
    const JSClass JsClass;
    */

    /*
    // List of object's JS methods.
    const JSFunctionSpec* JsFunctions;
    */

    /*
    // List of object's static JS methods.
    // Requires HasGlobalProto to be true
    const JSFunctionSpec* JsStaticFunctions;
    */

    /*
    // List of object's JS properties
    const JSPropertySpec* JsProperties;
    */

    /*
    // Unique id for the object's JS prototype
    const JsPrototypeId PrototypeId;
    */

    /*
    // Pointer to the object's JS constructor
    const JSNative JsConstructor;
    */

    // TODO: rename to PostJsCreate after migration
    /*
    // Pointer to method, that will be called on created JS object before it's returned.
    // Called only when `HasPostCreate` == true.
    // Called before JS object is wrapped in proxy (if `HasProxy` == true).
    //
    // Throw smp::JsException or qwr::QwrException on error.
    const PostJsCreateFn PostCreate;
    */

    /*
    // Reference to the object's JS proxy
    const js::BaseProxyHandler& JsProxy;
    */
};

} // namespace mozjs
