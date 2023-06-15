#pragma once

struct JSContext;

namespace mozjs
{

constexpr size_t DefaultClassFlagsWithSlots( size_t additionalSlotCount )
{
    // one slot is always reserved for native object
    return JSCLASS_HAS_RESERVED_SLOTS( 1 + additionalSlotCount ) | JSCLASS_BACKGROUND_FINALIZE;
}

inline constexpr size_t kReservedObjectSlot = 0;
inline constexpr uint32_t kDefaultClassFlags = DefaultClassFlagsWithSlots( 0 );
inline constexpr uint8_t kDefaultPropsFlags = JSPROP_ENUMERATE | JSPROP_PERMANENT;

/// @details Used to define write-only property with JS_PSGS
bool DummyGetter( JSContext* cx, unsigned argc, JS::Value* vp );

const void* GetSmpProxyFamily();

} // namespace mozjs
