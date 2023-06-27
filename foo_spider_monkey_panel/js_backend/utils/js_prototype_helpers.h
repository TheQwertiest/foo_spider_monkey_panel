#pragma once

#include <js_backend/objects/core/object_traits.h>
#include <js_backend/objects/core/prototype_ids.h>

#include <qwr/string_helpers.h>

namespace mozjs::utils
{

// TODO: remove in ESR102
inline void* GetMaybePtrFromReservedSlot( JSObject* obj, size_t slot )
{
    JS::Value v = JS::GetReservedSlot( obj, slot );
    return v.isUndefined() ? nullptr : v.toPrivate();
}

// TODO: remove
inline void* GetInstanceFromReservedSlot( JSContext* cx,
                                          JS::Handle<JSObject*> obj,
                                          const JSClass* clasp,
                                          JS::CallArgs* args )
{
    if ( !JS_InstanceOf( cx, obj, clasp, args ) )
    {
        return nullptr;
    }
    return GetMaybePtrFromReservedSlot( obj, 0 /* first slot is always reserved for private */ );
}

/// @brief Create a prototype for the specified object
///        and store it in the current global object.
///        Created prototype is not accessible from JS.
template <typename JsObjectType>
void CreateAndSavePrototype( JSContext* cx, JsPrototypeId protoId )
{
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS::GetClass( globalObject ) ) );

    JS::RootedObject jsProto( cx, JsObjectType::CreateProto( cx ) );
    assert( jsProto );

    JS::Value protoVal = JS::ObjectValue( *jsProto );
    JS_SetReservedSlot( globalObject, slotIdx, protoVal );
}

// TODO: divine protoId from type
/// @brief Create a prototype for the specified object
///        and store it in the current global object.
///        Created prototype is accessible from JS.
template <typename JsObjectType>
void CreateAndInstallPrototype( JSContext* cx, JsPrototypeId protoId )
{
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS::GetClass( globalObject ) ) );

    JS::RootedObject jsProto( cx, JsObjectType::InstallProto( cx, globalObject ) );
    assert( jsProto );

    JS::Value protoVal = JS::ObjectValue( *jsProto );
    JS_SetReservedSlot( globalObject, slotIdx, protoVal );
}

// TODO: replace with version below
template <typename JsObjectType>
void CreateAndInstallPrototype( JSContext* cx, JS::HandleObject jsObject, JsPrototypeId protoId )
{
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS::GetClass( globalObject ) ) );

    JS::RootedObject jsProto( cx, JsObjectType::InstallProto( cx, jsObject ) );
    assert( jsProto );

    JS::Value protoVal = JS::ObjectValue( *jsProto );
    JS_SetReservedSlot( globalObject, slotIdx, protoVal );
}

/// @brief Create a prototype for the specified object.
///        Created prototype is accessible from JS.
///
/// @remark This should only be applied to singleton objects
template <typename JsObjectType>
void CreateAndInstallPrototype( JSContext* cx, JS::HandleObject jsObject )
{
    // TODO: why global slot? maybe move to corresponding objects instead?
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    const auto protoId = static_cast<uint32_t>( JsObjectTraits<typename JsObjectType::BaseT>::PrototypeId );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + protoId;
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS::GetClass( globalObject ) ) );

    JS::RootedObject jsProto( cx, JsObjectType::InstallProto( cx, jsObject ) );
    assert( jsProto );

    JS::Value protoVal = JS::ObjectValue( *jsProto );
    JS_SetReservedSlot( globalObject, slotIdx, protoVal );
}

/// @brief Get the prototype for the specified object from the current global object.
JSObject* GetPrototype( JSContext* cx, JsPrototypeId protoId );

/// @brief Get the prototype for the specified object from the current global object.
///        And create the prototype, if it's missing.
template <typename JsObjectType>
JSObject* GetOrCreatePrototype( JSContext* cx, JsPrototypeId protoId )
{
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS::GetClass( globalObject ) ) );

    { // Try fetching prototype
        JS::Value protoVal = JS::GetReservedSlot( globalObject, slotIdx );
        if ( protoVal.isObject() )
        {
            return &protoVal.toObject();
        }
    }

    CreateAndSavePrototype<JsObjectType>( cx, protoId );

    JS::Value protoVal = JS::GetReservedSlot( globalObject, slotIdx );
    assert( protoVal.isObject() );

    return &protoVal.toObject();
}

} // namespace mozjs::utils
