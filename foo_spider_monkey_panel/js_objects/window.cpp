#include <stdafx.h>
#include "window.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/menu_object.h>
#include <js_objects/theme_manager.h>
#include <js_objects/fb_tooltip.h>
#include <js_objects/gdi_font.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/scope_helper.h>

#include <helpers.h>

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
    JsFinalizeOp<JsWindow>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Window",
    DefaultClassFlags(),
    &jsOps
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

JsWindow::JsWindow( JSContext* cx, js_panel_window& parentPanel )
    : pJsCtx_( cx )
    , parentPanel_( parentPanel )
{

}

JsWindow::~JsWindow()
{
}

JSObject* JsWindow::Create( JSContext* cx, js_panel_window& parentPanel )
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

    JS_SetPrivate( jsObj, new JsWindow( cx, parentPanel ) );

    return jsObj;
}

const JSClass& JsWindow::GetClass()
{
    return jsClass;
}

std::optional<std::nullptr_t>
JsWindow::ClearInterval( uint32_t intervalId )
{
    m_host->ClearIntervalOrTimeout( intervalID );
    return S_OK;
}

std::optional<std::nullptr_t>
JsWindow::ClearTimeout( uint32_t timeoutId )
{
    m_host->ClearIntervalOrTimeout( timeoutID );
    return S_OK;
}

std::optional<JSObject*>
JsWindow::CreatePopupMenu()
{
    JS::RootedObject jsObject( pJsCtx_, JsMenuObject::Create( pJsCtx_, m_host->GetHWND() ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsWindow::CreateThemeManager( const std::wstring& classid )
{
    JS::RootedObject jsObject( pJsCtx_, JsThemeManager::Create( pJsCtx_, m_host->GetHWND(), classid ) );
    if ( !jsObject )
    {// TODO: may not be an internal error, if classid is wrong
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsWindow::CreateTooltip( const std::wstring& name, uint32_t pxSize, uint32_t style )
{
    const auto& tooltip_param = m_host->PanelTooltipParam();
    tooltip_param->font_name = name;
    tooltip_param->font_size = pxSize;
    tooltip_param->font_style = style;

    JS::RootedObject jsObject( pJsCtx_, JsFbTooltip::Create( pJsCtx_, m_host->GetHWND(), tooltip_param ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<uint32_t>
JsWindow::GetColourCUI( uint32_t type, const std::wstring& guidstr )
{
    if ( m_host->GetInstanceType() != HostComm::KInstanceTypeCUI )
    {
        JS_ReportErrorASCII( pJsCtx_, "Can be called only in CUI" );
        return std::nullopt;
    }

    GUID guid;
    if ( guidstr.empty() )
    {
        memcpy( &guid, &pfc::guid_null, sizeof( guid ) );
    }
    else
    {
        HRESULT hr = CLSIDFromString( guidstr.c_str(), &guid );
        IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, std::nullopt, CLSIDFromString );        
    }

    return m_host->GetColourCUI( type, guid );
}

std::optional<uint32_t>
JsWindow::GetColourDUI( uint32_t type )
{
    if ( m_host->GetInstanceType() != HostComm::KInstanceTypeDUI )
    {
        JS_ReportErrorASCII( pJsCtx_, "Can be called only in DUI" );
        return std::nullopt;
    }

    return m_host->GetColourDUI( type );
}

std::optional<JSObject*>
JsWindow::GetFontCUI( uint32_t type, const std::wstring& guidstr )
{
    if ( m_host->GetInstanceType() != HostComm::KInstanceTypeCUI )
    {
        JS_ReportErrorASCII( pJsCtx_, "Can be called only in CUI" );
        return std::nullopt;
    }

    GUID guid;
    if ( guidstr.empty() )
    {
        memcpy( &guid, &pfc::guid_null, sizeof( guid ) );
    }
    else
    {
        HRESULT hr = CLSIDFromString( guidstr.c_str(), &guid );
        IF_HR_FAILED_RETURN_WITH_REPORT( pJsCtx_, hr, std::nullopt, CLSIDFromString );
    }

    HFONT hFont = m_host->GetFontCUI( type, guid );
    scope::unique_ptr<std::remove_pointer_t<HFONT>> autoFont( hFont, []( auto obj )
    {
        DeleteObject( obj );
    } );

    if ( hFont )
    {// Not an error: font not found
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( m_host->GetHDC(), hFont ) );
    if ( !helpers::ensure_gdiplus_object( pGdiFont.get() ) )
    {// Not an error: font not found
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiFont::Create( pJsCtx_, pGdiFont.get(), hFont, true ) );
    if ( !jsObject )
    {
        DeleteObject( hFont );

        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    pGdiFont.release();
    autoFont.release();
    return jsObject;
}

std::optional<JSObject*>
JsWindow::GetFontDUI( uint32_t type )
{
    if ( m_host->GetInstanceType() != HostComm::KInstanceTypeDUI )
    {
        JS_ReportErrorASCII( pJsCtx_, "Can be called only in DUI" );
        return std::nullopt;
    }

    HFONT hFont = m_host->GetFontDUI( type ); // No need to delete, it is managed by DUI

    if ( hFont )
    {// Not an error: font not found
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( m_host->GetHDC(), hFont ) );
    if ( !helpers::ensure_gdiplus_object( pGdiFont.get() ) )
    {// Not an error: font not found
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiFont::Create( pJsCtx_, pGdiFont.get(), hFont, false ) );
    if ( !jsObject )
    {
        DeleteObject( hFont );

        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    pGdiFont.release();
    return jsObject;
}

std::optional<JSObject*>
JsWindow::GetProperty( const std::string& name, JS::HandleValue defaultval )
{// TODO: rewrite config_prop
    HRESULT hr;
    _variant_t var;

    if ( m_host->get_config_prop().get_config_item( name.c_str(), var ) )
    {
        hr = VariantCopy( p, &var );
    }
    else
    {
        m_host->get_config_prop().set_config_item( name.c_str(), defaultval );
        hr = VariantCopy( p, &defaultval );
    }

    if ( FAILED( hr ) )
    {
        p = nullptr;
    }

    return S_OK;
}

std::optional<std::nullptr_t>
JsWindow::NotifyOthers( const std::string& name, JS::HandleValue info )
{// TODO: casts, a lot of casts
    if ( info.vt & VT_BYREF ) return E_INVALIDARG;

    HRESULT hr = S_OK;
    _variant_t var;

    hr = VariantCopy( &var, &info );

    if ( FAILED( hr ) ) return hr;

    simple_callback_data_2<_bstr_t, _variant_t>* notify_data = new simple_callback_data_2<_bstr_t, _variant_t>( name, nullptr );

    notify_data->m_item2.Attach( var.Detach() );

    panel_manager::instance().send_msg_to_others_pointer( m_host->GetHWND(), CALLBACK_UWM_ON_NOTIFY_DATA, notify_data );

    return S_OK;
}

std::optional<std::nullptr_t>
JsWindow::Reload()
{
    PostMessage( m_host->GetHWND(), UWM_RELOAD, 0, 0 );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::Repaint( bool force )
{
    m_host->Repaint( force );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{
    m_host->RepaintRect( x, y, w, h, force );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::SetCursor( uint8_t id )
{
    ::SetCursor( LoadCursor( nullptr, MAKEINTRESOURCE( id ) ) );
    return nullptr;
}

std::optional<uint32_t>
JsWindow::SetInterval( JS::HandleFunction func, uint32_t delay )
{
    return m_host->SetInterval( func, delay );
}

std::optional<std::nullptr_t>
JsWindow::SetProperty( const std::string& name, JS::HandleValue val )
{// TODO: rewrite
    m_host->get_config_prop().set_config_item( pfc::stringcvt::string_utf8_from_wide( name ), val );
    return S_OK;
}

std::optional<uint32_t>
JsWindow::SetTimeout( JS::HandleFunction func, uint32_t delay )
{
    return m_host->SetTimeout( func, delay );
}

std::optional<std::nullptr_t>
JsWindow::ShowConfigure()
{
    PostMessage( m_host->GetHWND(), UWM_SHOW_CONFIGURE, 0, 0 );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::ShowProperties()
{
    PostMessage( m_host->GetHWND(), UWM_SHOW_PROPERTIES, 0, 0 );
    return nullptr;
}

std::optional<uint32_t>
JsWindow::get_DlgCode()
{
    return m_host->DlgCode();    
}

std::optional<uint32_t>
JsWindow::get_Height()
{
    return m_host->GetHeight();
}

std::optional<uint64_t>
JsWindow::get_Id()
{
    return reinterpret_cast<uint64_t>(m_host->GetHWND());
}

std::optional<uint32_t>
JsWindow::get_InstanceType()
{
    return m_host->GetInstanceType();
}

std::optional<bool>
JsWindow::get_IsTransparent()
{
    return m_host->get_pseudo_transparent();
}

std::optional<bool>
JsWindow::get_IsVisible()
{
    return  IsWindowVisible( m_host->GetHWND() );
}

std::optional<uint32_t>
JsWindow::get_MaxHeight()
{
    return m_host->MaxSize().y;
}

std::optional<uint32_t>
JsWindow::get_MaxWidth()
{
    return m_host->MaxSize().x;
}

std::optional<uint32_t>
JsWindow::get_MinHeight()
{
    return  m_host->MinSize().y;
}

std::optional<uint32_t>
JsWindow::get_MinWidth()
{
    return m_host->MinSize().x;
}

std::optional<std::string>
JsWindow::get_Name()
{
    pfc::string8_fast name = m_host->ScriptInfo().name;
    if ( name.is_empty() )
    {
        name = pfc::print_guid( m_host->GetGUID() );
    }

    return std::string( name.c_str(), name.length());
}

std::optional<uint32_t>
JsWindow::get_Width()
{
    return m_host->GetWidth();
}

std::optional<std::nullptr_t>
JsWindow::put_DlgCode( uint32_t code )
{
    m_host->DlgCode() = code;
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::put_MaxHeight( uint32_t height )
{
    m_host->MaxSize().y = height;
    PostMessage( m_host->GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_maximum_height );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::put_MaxWidth( uint32_t width )
{
    m_host->MaxSize().x = width;
    PostMessage( m_host->GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_maximum_width );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::put_MinHeight( uint32_t height )
{
    m_host->MinSize().y = height;
    PostMessage( m_host->GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_minimum_height );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::put_MinWidth( uint32_t width )
{
    m_host->MinSize().x = width;
    PostMessage( m_host->GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_minimum_width );
    return nullptr;
}

}
