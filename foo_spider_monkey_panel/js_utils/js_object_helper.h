#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_objects/prototype_ids.h>

namespace mozjs
{

constexpr uint32_t DefaultClassFlags()
{
    return JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE;
}

constexpr uint16_t DefaultPropsFlags()
{
    return JSPROP_ENUMERATE | JSPROP_PERMANENT;
}

template<typename MozjsObjectType>
void JsFinalizeOp( [[maybe_unused]] JSFreeOp* fop, JSObject* obj )
{
    auto x = static_cast<MozjsObjectType*>(JS_GetPrivate( obj ));
    if ( x )
    {
        delete x;
        JS_SetPrivate( obj, nullptr );
    }
}

/// @details Used to define write-only property with JS_PSGS
bool DummyGetter( JSContext* cx, unsigned argc, JS::Value* vp );

template<typename JsObjectType, typename ...ArgsType>
bool CreateAndInstallObject( JSContext* cx, JS::HandleObject parentObject, const pfc::string8_fast& propertyName, ArgsType&&... args )
{
    JS::RootedObject objectToInstall( cx, JsObjectType::Create( cx, args... ) );
    if ( !objectToInstall )
    {
        return false;
    }

    if ( !JS_DefineProperty( cx, parentObject, propertyName.c_str(), objectToInstall, DefaultPropsFlags() ) )
    {
        return false;
    }

    return true;
}

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
