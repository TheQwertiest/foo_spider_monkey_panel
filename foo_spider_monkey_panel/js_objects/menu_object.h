#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsMenuObject
{
public:
    ~JsMenuObject();

    static JSObject* Create( JSContext* cx, HWND hParentWnd );

    static const JSClass& GetClass();

    HMENU HMenu() const;

public:
    std::optional<std::nullptr_t> AppendMenuItem( uint32_t flags, uint32_t item_id, std::wstring text );
    std::optional<std::nullptr_t> AppendMenuSeparator();
    std::optional<std::nullptr_t> AppendTo( JsMenuObject* parent, uint32_t flags, std::wstring text );
    std::optional<std::nullptr_t> CheckMenuItem( uint32_t item_id, bool check );
    std::optional<std::nullptr_t> CheckMenuRadioItem( uint32_t first, uint32_t last, uint32_t selected );
    std::optional<std::uint32_t> TrackPopupMenu( int32_t x, int32_t y, uint32_t flags );

private:
    JsMenuObject( JSContext* cx, HWND hParentWnd );
    JsMenuObject( const JsMenuObject& ) = delete;
    JsMenuObject& operator=( const JsMenuObject& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    HMENU hMenu_;
    HWND hParentWnd_;
    bool isDetached_ = false;
};

}
