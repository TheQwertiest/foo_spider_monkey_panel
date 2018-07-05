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
    JsFinalizeOp<JsMenuObject>,
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


JsMenuObject::JsMenuObject( JSContext* cx, HWND hParentWnd )
    : pJsCtx_( cx )
    , hParentWnd_( hParentWnd )
{
    hMenu_ = ::CreatePopupMenu();
    assert( hMenu_ );
    // TODO: move to create for error checking
}


JsMenuObject::~JsMenuObject()
{
    if ( !isDetached_ && hMenu_ && IsMenu( hMenu_ ) )
    {
        DestroyMenu( hMenu_ );
    }
}

JSObject* JsMenuObject::Create( JSContext* cx, HWND hParentWnd )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsMenuObject( cx, hParentWnd ) );

    return jsObj;
}

const JSClass& JsMenuObject::GetClass()
{
    return jsClass;
}

HMENU JsMenuObject::HMenu() const
{
    return hMenu_;
}

std::optional<std::nullptr_t> 
JsMenuObject::AppendMenuItem( uint32_t flags, uint32_t item_id, const std::wstring& text )
{
    assert( hMenu_ );

    if ( flags & MF_POPUP )
    {
        JS_ReportErrorASCII( pJsCtx_, "Invalid flags: MF_POPUP when adding menu item" );
        return std::nullopt;
    }

    BOOL bRet = ::AppendMenu( hMenu_, flags, item_id, text.c_str() );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, bRet, std::nullopt, AppendMenu );

    return nullptr;
}

std::optional<std::nullptr_t>
JsMenuObject::AppendMenuSeparator()
{
    assert( hMenu_ );

    BOOL bRet = ::AppendMenu( hMenu_, MF_SEPARATOR, 0, 0 );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, bRet, std::nullopt, AppendMenu );

    return nullptr;
}

std::optional<std::nullptr_t> 
JsMenuObject::AppendTo( JsMenuObject* parent, uint32_t flags, const std::wstring& text )
{
    assert( hMenu_ );

    if ( !parent )
    {
        JS_ReportErrorASCII( pJsCtx_, "parent argument is null" );
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
    assert( hMenu_ );

    DWORD dRet = ::CheckMenuItem( hMenu_, item_id, check != VARIANT_FALSE ? MF_CHECKED : MF_UNCHECKED );
    if ( static_cast<DWORD>(-1) == dRet )
    {
        JS_ReportErrorASCII( pJsCtx_, "Menu item with specified id does not exist" );
        return std::nullopt;
    }

    return nullptr;
}

std::optional<std::nullptr_t>
JsMenuObject::CheckMenuRadioItem( uint32_t first, uint32_t last, uint32_t selected )
{
    assert( hMenu_ );

    if ( selected < first || selected > last )
    {
        JS_ReportErrorASCII( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    BOOL bRet = ::CheckMenuRadioItem( hMenu_, first, last, selected, MF_BYCOMMAND );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, bRet, std::nullopt, CheckMenuRadioItem );

    return nullptr;
}

std::optional<std::uint32_t> 
JsMenuObject::TrackPopupMenu( int32_t x, int32_t y, uint32_t flags )
{
    assert( hMenu_ );

    POINT pt = { x, y };

    // Only include specified flags
    flags |= TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON;
    flags &= ~TPM_RECURSE;

    BOOL bRet = ClientToScreen( hParentWnd_, &pt );
    IF_WINAPI_FAILED_RETURN_WITH_REPORT( pJsCtx_, bRet, std::nullopt, ClientToScreen );

    SetLastError( ERROR_SUCCESS );
    DWORD itemIdx = ::TrackPopupMenu( hMenu_, flags, pt.x, pt.y, 0, hParentWnd_, 0 );
    if ( !itemIdx )
    {
        DWORD lastError = GetLastError();
        if ( ERROR_SUCCESS != lastError )
        {
            WINAPI_RETURN_WITH_REPORT( pJsCtx_, std::nullopt, TrackPopupMenu );
        }
    }

    return itemIdx;
}

std::optional<std::uint32_t> 
JsMenuObject::TrackPopupMenuWithOpt( size_t optArgCount, int32_t x, int32_t y, uint32_t flags )
{
    if ( optArgCount > 1 )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: invalid number of optional arguments specified: %d", optArgCount );
        return std::nullopt;
    }

    if ( optArgCount == 1 )
    {
        return TrackPopupMenu( x, y );
    }

    return TrackPopupMenu( x, y, flags );
}

}
