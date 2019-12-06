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
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsMenuObject() override;

    static std::unique_ptr<JsMenuObject> CreateNative( JSContext* cx, HWND hParentWnd );
    static size_t GetInternalSize( HWND hParentWnd );

public:
    [[nodiscard]] HMENU HMenu() const;

public:
    void AppendMenuItem( uint32_t flags, uint32_t item_id, const std::wstring& text );
    void AppendMenuSeparator();
    void AppendTo( JsMenuObject* parent, uint32_t flags, const std::wstring& text );
    void CheckMenuItem( uint32_t item_id, bool check );
    void CheckMenuRadioItem( uint32_t first, uint32_t last, uint32_t selected );
    uint32_t TrackPopupMenu( int32_t x, int32_t y, uint32_t flags = 0 );
    uint32_t TrackPopupMenuWithOpt( size_t optArgCount, int32_t x, int32_t y, uint32_t flags );

private:
    JsMenuObject( JSContext* cx, HWND hParentWnd, HMENU hMenu );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;
    HWND hParentWnd_ = nullptr;
    HMENU hMenu_ = nullptr;
    bool isDetached_ = false;
};

} // namespace mozjs
