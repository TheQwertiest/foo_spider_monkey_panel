#pragma once

#include <js_objects/internal/prototype_ids.h>

#include <qwr/string_helpers.h>

namespace mozjs
{

/// @brief Create a prototype for the specified object
///        and store it in the current global object.
///        Created prototype is not accessible from JS.
template <typename JsObjectType>
void CreateAndSavePrototype( JSContext* cx, JsPrototypeId protoId )
{
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS_GetClass( globalObject ) ) );

    JS::RootedObject jsProto( cx, JsObjectType::CreateProto( cx ) );
    assert( jsProto );

    JS::Value protoVal = JS::ObjectValue( *jsProto );
    JS_SetReservedSlot( globalObject, slotIdx, protoVal );
}

/// @brief Create a prototype for the specified object
///        and store it in the current global object.
///        Created prototype is accessible from JS.
template <typename JsObjectType>
void CreateAndInstallPrototype( JSContext* cx, JsPrototypeId protoId )
{
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS_GetClass( globalObject ) ) );

    JS::RootedObject jsProto( cx, JsObjectType::InstallProto( cx, globalObject ) );
    assert( jsProto );

    JS::Value protoVal = JS::ObjectValue( *jsProto );
    JS_SetReservedSlot( globalObject, slotIdx, protoVal );
}

/// @brief Get the prototype for the specified object from the current global object.
template <typename JsObjectType>
JSObject* GetPrototype( JSContext* cx, JsPrototypeId protoId )
{
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS_GetClass( globalObject ) ) );

    JS::Value protoVal = JS_GetReservedSlot( globalObject, slotIdx );
    qwr::QwrException::ExpectTrue( protoVal.isObject(),
                                   "Internal error: Slot {}({}) does not contain a prototype",
                                   static_cast<uint32_t>( protoId ),
                                   slotIdx );

    return &protoVal.toObject();
}

/// @brief Get the prototype for the specified object from the current global object.
///        And create the prototype, if it's missing.
template <typename JsObjectType>
JSObject* GetOrCreatePrototype( JSContext* cx, JsPrototypeId protoId )
{
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS_GetClass( globalObject ) ) );

    { // Try fetching prototype
        JS::Value protoVal = JS_GetReservedSlot( globalObject, slotIdx );
        if ( protoVal.isObject() )
        {
            return &protoVal.toObject();
        }
    }

    CreateAndSavePrototype<JsObjectType>( cx, protoId );

    JS::Value protoVal = JS_GetReservedSlot( globalObject, slotIdx );
    assert( protoVal.isObject() );

    return &protoVal.toObject();
}

} // namespace mozjs
