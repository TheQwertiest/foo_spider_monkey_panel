#pragma once

struct JSContext;

namespace mozjs
{

inline constexpr uint32_t kDefaultClassFlags = JSCLASS_HAS_RESERVED_SLOTS( 1 ) | JSCLASS_BACKGROUND_FINALIZE;
inline constexpr uint8_t kDefaultPropsFlags = JSPROP_ENUMERATE | JSPROP_PERMANENT;
inline constexpr size_t kReservedObjectSlot = 0;

/// @details Used to define write-only property with JS_PSGS
bool DummyGetter( JSContext* cx, unsigned argc, JS::Value* vp );

const void* GetSmpProxyFamily();

} // namespace mozjs
