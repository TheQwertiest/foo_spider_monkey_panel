#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbMetadbHandleList;

class JsFbUiSelectionHolder
    : public JsObjectBase<JsFbUiSelectionHolder>
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
    ~JsFbUiSelectionHolder() override = default;

    static std::unique_ptr<JsFbUiSelectionHolder> CreateNative( JSContext* cx, const ui_selection_holder::ptr& holder );
    static size_t GetInternalSize( const ui_selection_holder::ptr& holder );

public:
    void SetPlaylistSelectionTracking();
    void SetPlaylistTracking();
    void SetSelection( JsFbMetadbHandleList* handles, uint8_t type = 0);
    void SetSelectionWithOpt( size_t optArgCount, JsFbMetadbHandleList* handles, uint8_t type );

private:
    JsFbUiSelectionHolder( JSContext* cx, const ui_selection_holder::ptr& holder );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;
    ui_selection_holder::ptr holder_;
};

} // namespace mozjs
