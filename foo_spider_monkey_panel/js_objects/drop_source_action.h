#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsDropSourceAction
{
public:
    ~JsDropSourceAction();

    static JSObject* Create( JSContext* cx );

    static const JSClass& GetClass();

    void Reset();

    uint32_t & Base();
    int32_t& Playlist();
    bool& ToSelect();
    uint32_t& Effect();

public:
    std::optional<uint32_t> get_Effect();
    std::optional<std::nullptr_t> put_Base( uint32_t base );
    std::optional<std::nullptr_t> put_Effect( uint32_t effect );
    std::optional<std::nullptr_t> put_Playlist( int32_t id );
    std::optional<std::nullptr_t> put_ToSelect( bool to_select );

private:
    JsDropSourceAction( JSContext* cx );
    JsDropSourceAction( const JsDropSourceAction& ) = delete;
    JsDropSourceAction& operator=( const JsDropSourceAction& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;

    // -1 means active playlist
    int32_t playlistIdx_;
    uint32_t base_;
    bool toSelect_;
    uint32_t effect_;
};

}
