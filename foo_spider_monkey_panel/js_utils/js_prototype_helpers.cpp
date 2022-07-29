#include <stdafx.h>

#include "js_prototype_helpers.h"

namespace mozjs::utils
{

JSObject* GetPrototype( JSContext* cx, JsPrototypeId protoId )
{
    JS::RootedObject globalObject( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( globalObject );

    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS::GetClass( globalObject ) ) );

    JS::Value protoVal = JS::GetReservedSlot( globalObject, slotIdx );
    qwr::QwrException::ExpectTrue( protoVal.isObject(),
                                   "Internal error: Slot {}({}) does not contain a prototype",
                                   static_cast<uint32_t>( protoId ),
                                   slotIdx );

    return &protoVal.toObject();
}

} // namespace mozjs::utils
