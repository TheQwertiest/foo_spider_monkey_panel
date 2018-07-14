#pragma once

#include <js_objects/internal/prototype_ids.h>

namespace mozjs
{

template<typename JsObjectType>
bool CreateAndSavePrototype( JSContext* cx, JS::HandleObject globalObject, JsPrototypeId protoId )
{
    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( globalObject );
    assert( JS::CurrentGlobalOrNull( cx ) == globalObject );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS_GetClass( globalObject ) ) );

    JS::RootedObject jsProto( cx, JsObjectType::CreateProto(cx) );
    if ( !jsProto )
    {// report in CreateProto
        return false;
    }

    JS::Value protoVal = JS::ObjectValue( *jsProto );
    JS_SetReservedSlot( globalObject, slotIdx, protoVal );

    return true;
}

template<typename JsObjectType>
bool CreateAndInstallPrototype( JSContext* cx, JS::HandleObject globalObject, JsPrototypeId protoId )
{
    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( globalObject );
    assert( JS::CurrentGlobalOrNull( cx ) == globalObject );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS_GetClass( globalObject ) ) );

    JS::RootedObject jsProto( cx, JsObjectType::InstallProto( cx, globalObject ) );
    if ( !jsProto )
    {// report in InstallProto
        return false;
    }

    JS::Value protoVal = JS::ObjectValue( *jsProto );
    JS_SetReservedSlot( globalObject, slotIdx, protoVal );

    return true;
}

template<typename JsObjectType>
JSObject* GetPrototype( JSContext* cx, JS::HandleObject globalObject, JsPrototypeId protoId )
{
    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( globalObject );
    assert( JS::CurrentGlobalOrNull( cx ) == globalObject );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS_GetClass( globalObject ) ) );

    JS::Value& valRef = JS_GetReservedSlot( globalObject, slotIdx );
    if ( !valRef.isObject() )
    {
        JS_ReportErrorUTF8(cx, "Internal error: Slot %u does not contain a prototype", slotIdx );
        return nullptr;
    }

    return &valRef.toObject();
}

template<typename JsObjectType>
JSObject* GetOrCreatePrototype( JSContext* cx, JS::HandleObject globalObject, JsPrototypeId protoId )
{
    uint32_t slotIdx = JSCLASS_GLOBAL_SLOT_COUNT + static_cast<uint32_t>( protoId );
    assert( globalObject );
    assert( JS::CurrentGlobalOrNull( cx ) == globalObject );
    assert( slotIdx < JSCLASS_RESERVED_SLOTS( JS_GetClass( globalObject ) ) );

    {
        JS::Value& valRef = JS_GetReservedSlot( globalObject, slotIdx );
        if ( valRef.isObject() )
        {
            return &valRef.toObject();
        }
    }

    if ( !CreateAndSavePrototype<JsObjectType>( cx, globalObject, protoId ) )
    {// report in CreateAndSavePrototype
        return nullptr;
    }

    JS::Value& valRef = JS_GetReservedSlot( globalObject, slotIdx );
    assert( valRef.isObject() );

    return &valRef.toObject();
}

}
