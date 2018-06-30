#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbMetadbHandleList;

class JsFbUiSelectionHolder
{
public:
    ~JsFbUiSelectionHolder();

    static JSObject* Create( JSContext* cx, const ui_selection_holder::ptr& holder );

    static const JSClass& GetClass();

public:
   std::optional<std::nullptr_t> SetPlaylistSelectionTracking();
   std::optional<std::nullptr_t> SetPlaylistTracking();
   std::optional<std::nullptr_t> SetSelection( JsFbMetadbHandleList* handles );

private:
    JsFbUiSelectionHolder( JSContext* cx, const ui_selection_holder::ptr& holder );
    JsFbUiSelectionHolder( const JsFbUiSelectionHolder& ) = delete;
    JsFbUiSelectionHolder& operator=( const JsFbUiSelectionHolder& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    ui_selection_holder::ptr holder_;
};

}
