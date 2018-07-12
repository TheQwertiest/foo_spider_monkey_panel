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

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsFbUiSelectionHolder();

    static std::unique_ptr<JsFbUiSelectionHolder> CreateNative( JSContext* cx, const ui_selection_holder::ptr& holder );

public:
   std::optional<std::nullptr_t> SetPlaylistSelectionTracking();
   std::optional<std::nullptr_t> SetPlaylistTracking();
   std::optional<std::nullptr_t> SetSelection( JsFbMetadbHandleList* handles );

private:
    JsFbUiSelectionHolder( JSContext* cx, const ui_selection_holder::ptr& holder );

private:
    JSContext * pJsCtx_ = nullptr;
    ui_selection_holder::ptr holder_;
};

}
