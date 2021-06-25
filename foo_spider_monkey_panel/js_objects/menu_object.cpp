#include <stdafx.h>

#include "menu_object.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <panel/modal_blocking_scope.h>

#include <qwr/winapi_error_helpers.h>

using namespace smp;

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
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( AppendMenuItem, JsMenuObject::AppendMenuItem )
MJS_DEFINE_JS_FN_FROM_NATIVE( AppendMenuSeparator, JsMenuObject::AppendMenuSeparator )
MJS_DEFINE_JS_FN_FROM_NATIVE( AppendTo, JsMenuObject::AppendTo )
MJS_DEFINE_JS_FN_FROM_NATIVE( CheckMenuItem, JsMenuObject::CheckMenuItem )
MJS_DEFINE_JS_FN_FROM_NATIVE( CheckMenuRadioItem, JsMenuObject::CheckMenuRadioItem )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( TrackPopupMenu, JsMenuObject::TrackPopupMenu, JsMenuObject::TrackPopupMenuWithOpt, 1 )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "AppendMenuItem", AppendMenuItem, 3, kDefaultPropsFlags ),
        JS_FN( "AppendMenuSeparator", AppendMenuSeparator, 0, kDefaultPropsFlags ),
        JS_FN( "AppendTo", AppendTo, 3, kDefaultPropsFlags ),
        JS_FN( "CheckMenuItem", CheckMenuItem, 2, kDefaultPropsFlags ),
        JS_FN( "CheckMenuRadioItem", CheckMenuRadioItem, 3, kDefaultPropsFlags ),
        JS_FN( "TrackPopupMenu", TrackPopupMenu, 2, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsMenuObject::JsClass = jsClass;
const JSFunctionSpec* JsMenuObject::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsMenuObject::JsProperties = jsProperties.data();
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
    qwr::error::CheckWinApi( !!hMenu, "CreatePopupMenu" );

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

void JsMenuObject::AppendMenuItem( uint32_t flags, uint32_t item_id, const std::wstring& text )
{
    if ( flags & MF_POPUP )
    {
        throw qwr::QwrException( "Invalid flags: MF_POPUP when adding menu item" );
    }

    BOOL bRet = ::AppendMenu( hMenu_, flags, item_id, text.c_str() );
    qwr::error::CheckWinApi( bRet, "AppendMenu" );
}

void JsMenuObject::AppendMenuSeparator()
{
    BOOL bRet = ::AppendMenu( hMenu_, MF_SEPARATOR, 0, nullptr );
    qwr::error::CheckWinApi( bRet, "AppendMenu" );
}

void JsMenuObject::AppendTo( JsMenuObject* parent, uint32_t flags, const std::wstring& text )
{
    qwr::QwrException::ExpectTrue( parent, "parent argument is null" );

    BOOL bRet = ::AppendMenu( parent->HMenu(), flags | MF_STRING | MF_POPUP, (UINT_PTR)hMenu_, text.c_str() );
    qwr::error::CheckWinApi( bRet, "AppendMenu" );

    isDetached_ = true;
}

void JsMenuObject::CheckMenuItem( uint32_t item_id, bool check )
{
    DWORD dRet = ::CheckMenuItem( hMenu_, item_id, check ? MF_CHECKED : MF_UNCHECKED );
    if ( static_cast<DWORD>( -1 ) == dRet )
    {
        throw qwr::QwrException( "Menu item with specified id does not exist" );
    }
}

void JsMenuObject::CheckMenuRadioItem( uint32_t first, uint32_t last, uint32_t selected )
{
    qwr::QwrException::ExpectTrue( selected >= first && selected <= last, "Index is out of bounds" );

    BOOL bRet = ::CheckMenuRadioItem( hMenu_, first, last, selected, MF_BYCOMMAND );
    qwr::error::CheckWinApi( bRet, "CheckMenuRadioItem" );
}

uint32_t JsMenuObject::TrackPopupMenu( int32_t x, int32_t y, uint32_t flags )
{
    POINT pt = { x, y };

    // Only include specified flags
    flags |= TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON;
    flags &= ~TPM_RECURSE;

    BOOL bRet = ClientToScreen( hParentWnd_, &pt );
    qwr::error::CheckWinApi( bRet, "ClientToScreen" );

    if ( modal::IsModalBlocked() )
    {
        return 0;
    }

    modal::MessageBlockingScope scope;

    // Don't bother with error checking, since TrackPopupMenu returns numerous errors when clicked outside of menu
    return ::TrackPopupMenu( hMenu_, flags, pt.x, pt.y, 0, hParentWnd_, nullptr );
}

uint32_t JsMenuObject::TrackPopupMenuWithOpt( size_t optArgCount, int32_t x, int32_t y, uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return TrackPopupMenu( x, y, flags );
    case 1:
        return TrackPopupMenu( x, y );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

} // namespace mozjs
