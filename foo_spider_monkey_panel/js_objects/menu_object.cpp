#include <stdafx.h>
#include "menu_object.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/winapi_error_helper.h>


namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsMenuObject::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "MenuObject",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsMenuObject, AppendMenuItem )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMenuObject, AppendMenuSeparator )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMenuObject, AppendTo )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMenuObject, CheckMenuItem )
MJS_DEFINE_JS_TO_NATIVE_FN( JsMenuObject, CheckMenuRadioItem )
MJS_DEFINE_JS_TO_NATIVE_FN_WITH_OPT( JsMenuObject, TrackPopupMenu, TrackPopupMenuWithOpt, 1 )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "AppendMenuItem", AppendMenuItem, 3, DefaultPropsFlags() ),
    JS_FN( "AppendMenuSeparator", AppendMenuSeparator, 0, DefaultPropsFlags() ),
    JS_FN( "AppendTo", AppendTo, 3, DefaultPropsFlags() ),
    JS_FN( "CheckMenuItem", CheckMenuItem, 2, DefaultPropsFlags() ),
    JS_FN( "CheckMenuRadioItem", CheckMenuRadioItem, 3, DefaultPropsFlags() ),
    JS_FN( "TrackPopupMenu", TrackPopupMenu, 2, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};


}

namespace mozjs
{

const JSClass JsMenuObject::JsClass = jsClass;
const JSFunctionSpec* JsMenuObject::JsFunctions = jsFunctions;
const JSPropertySpec* JsMenuObject::JsProperties = jsProperties;
const JsPrototypeId JsMenuObject::PrototypeId = JsPrototypeId::MenuObject;

JsMenuObject::JsMenuObject( JSContext* cx, HWND hParentWnd, HMENU hMenu )
    : pJsCtx_( cx )
    , hParentWnd_( hParentWnd )
    , hMenu_( hMenu )
{
}


JsMenuObject::~JsMenuObject()
{
    if ( !isDetached_ && hMenu_ && IsMenu( hMenu_ ) )
    {
        DestroyMenu( hMenu_ );
    }
}

std::unique_ptr<JsMenuObject>
JsMenuObject::CreateNative( JSContext* cx, HWND hParentWnd )
{
    HMENU hMenu = ::CreatePopupMenu();
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( cx, !!hMenu, nullptr, CreatePopupMenu );

    return std::unique_ptr<JsMenuObject>( new JsMenuObject( cx, hParentWnd, hMenu ) );
}

size_t JsMenuObject::GetInternalSize( HWND /* hParentWnd */ )
{
    return 0;
}

HMENU JsMenuObject::HMenu() const
{
    return hMenu_;
}

std::optional<std::nullptr_t> 
JsMenuObject::AppendMenuItem( uint32_t flags, uint32_t item_id, const std::wstring& text )
{
    if ( flags & MF_POPUP )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Invalid flags: MF_POPUP when adding menu item" );
        return std::nullopt;
    }

    BOOL bRet = ::AppendMenu( hMenu_, flags, item_id, text.c_str() );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, bRet, std::nullopt, AppendMenu );

    return nullptr;
}

std::optional<std::nullptr_t>
JsMenuObject::AppendMenuSeparator()
{
    BOOL bRet = ::AppendMenu( hMenu_, MF_SEPARATOR, 0, 0 );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, bRet, std::nullopt, AppendMenu );

    return nullptr;
}

std::optional<std::nullptr_t> 
JsMenuObject::AppendTo( JsMenuObject* parent, uint32_t flags, const std::wstring& text )
{
    if ( !parent )
    {
        JS_ReportErrorUTF8( pJsCtx_, "parent argument is null" );
        return std::nullopt;
    }

    BOOL bRet = ::AppendMenu( parent->HMenu(), flags | MF_STRING | MF_POPUP, (UINT_PTR)hMenu_, text.c_str() );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, bRet, std::nullopt, AppendMenu );

    isDetached_ = true;
    return nullptr;
}

std::optional<std::nullptr_t>
JsMenuObject::CheckMenuItem( uint32_t item_id, bool check )
{
    DWORD dRet = ::CheckMenuItem( hMenu_, item_id, check != VARIANT_FALSE ? MF_CHECKED : MF_UNCHECKED );
    if ( static_cast<DWORD>(-1) == dRet )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Menu item with specified id does not exist" );
        return std::nullopt;
    }

    return nullptr;
}

std::optional<std::nullptr_t>
JsMenuObject::CheckMenuRadioItem( uint32_t first, uint32_t last, uint32_t selected )
{
    if ( selected < first || selected > last )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    BOOL bRet = ::CheckMenuRadioItem( hMenu_, first, last, selected, MF_BYCOMMAND );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, bRet, std::nullopt, CheckMenuRadioItem );

    return nullptr;
}

std::optional<std::uint32_t> 
JsMenuObject::TrackPopupMenu( int32_t x, int32_t y, uint32_t flags )
{
    POINT pt = { x, y };

    // Only include specified flags
    flags |= TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON;
    flags &= ~TPM_RECURSE;

    BOOL bRet = ClientToScreen( hParentWnd_, &pt );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, bRet, std::nullopt, ClientToScreen );

    // Don't bother with error checking, since TrackPopupMenu returns numerous errors when clicked outside of menu
    return ::TrackPopupMenu( hMenu_, flags, pt.x, pt.y, 0, hParentWnd_, 0 );
}

std::optional<std::uint32_t> 
JsMenuObject::TrackPopupMenuWithOpt( size_t optArgCount, int32_t x, int32_t y, uint32_t flags )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return TrackPopupMenu( x, y );
    }

    return TrackPopupMenu( x, y, flags );
}

}
