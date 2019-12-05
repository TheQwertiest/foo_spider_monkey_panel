#pragma once

#include <js_objects/object_base.h>
#include <drop_action_params.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsDropSourceAction
    : public JsObjectBase<JsDropSourceAction>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsDropSourceAction();

    static std::unique_ptr<JsDropSourceAction> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    smp::panel::DropActionParams& GetDropActionParams();

public:
    uint32_t get_Effect();
    void put_Base( uint32_t base );
    void put_Effect( uint32_t effect );
    void put_Playlist( int32_t id );
    void put_Text( const std::wstring& text );
    void put_ToSelect( bool to_select );

private:
    JsDropSourceAction( JSContext* cx );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;

    smp::panel::DropActionParams actionParams_;
};

} // namespace mozjs
