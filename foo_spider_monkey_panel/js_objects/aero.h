#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>
#include <memory>


namespace mozjs
{

class JsAero
    : public JsObjectBase<JsAero>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsAero();

    static std::unique_ptr<JsAero> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    std::optional<bool> get_Active();
    std::optional<uint8_t> get_Effect();
    std::optional<uint32_t> get_Top();
    std::optional<uint32_t> get_Right();
    std::optional<uint32_t> get_Left();
    std::optional<uint32_t> get_Bottom();
    std::optional<nullptr_t> put_Active( bool is );
    std::optional<nullptr_t> put_Effect( uint8_t effect);
    std::optional<nullptr_t> put_Top( uint32_t value);
    std::optional<nullptr_t> put_Right( uint32_t value );
    std::optional<nullptr_t> put_Left( uint32_t value );
    std::optional<nullptr_t> put_Bottom( uint32_t value );

private:
    JsAero( JSContext* cx );

private:
    JSContext * pJsCtx_ = nullptr;
};

}

