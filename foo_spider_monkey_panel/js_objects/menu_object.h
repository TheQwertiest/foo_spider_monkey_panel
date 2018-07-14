#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsMenuObject
    : public JsObjectBase<JsMenuObject>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsMenuObject();

    static std::unique_ptr<JsMenuObject> CreateNative( JSContext* cx, HWND hParentWnd );
    static size_t GetInternalSize( HWND hParentWnd );

public:
    HMENU HMenu() const;

public:
    std::optional<std::nullptr_t> AppendMenuItem( uint32_t flags, uint32_t item_id, const std::wstring& text );
    std::optional<std::nullptr_t> AppendMenuSeparator();
    std::optional<std::nullptr_t> AppendTo( JsMenuObject* parent, uint32_t flags, const std::wstring& text );
    std::optional<std::nullptr_t> CheckMenuItem( uint32_t item_id, bool check );
    std::optional<std::nullptr_t> CheckMenuRadioItem( uint32_t first, uint32_t last, uint32_t selected );
    std::optional<std::uint32_t> TrackPopupMenu( int32_t x, int32_t y, uint32_t flags = 0);
    std::optional<std::uint32_t> TrackPopupMenuWithOpt( size_t optArgCount, int32_t x, int32_t y, uint32_t flags );

private:
    JsMenuObject( JSContext* cx, HWND hParentWnd );

private:
    JSContext * pJsCtx_ = nullptr;
    HMENU hMenu_;
    HWND hParentWnd_;
    bool isDetached_ = false;
};

}
