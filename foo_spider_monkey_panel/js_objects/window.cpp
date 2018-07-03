#include <stdafx.h>
#include "window.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/menu_object.h>
#include <js_objects/theme_manager.h>
#include <js_objects/fb_tooltip.h>
#include <js_objects/gdi_font.h>
#include <js_objects/fb_properties.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/scope_helper.h>

#include <js_panel_window.h>
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
    assert( jsFbProperties_ );
    pFbProperties_ = static_cast<JsFbProperties*>( JS_GetPrivate( jsFbProperties_ ) );
    assert( pFbProperties_ );
}

JsWindow::~JsWindow()
{
    RemoveHeapTracer();
    jsFbProperties_.reset();
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

    JS::RootedObject fbProperties( cx, JsFbProperties::Create( cx, parentPanel ) );
    if ( !fbProperties )
    {
        return nullptr;
    }

    auto pNative = new JsWindow( cx, parentPanel );
    pNative->jsFbProperties_.init( cx, fbProperties );

    JS_SetPrivate( jsObj, pNative );

    return jsObj;
}

const JSClass& JsWindow::GetClass()
{
    return jsClass;
}

void JsWindow::RemoveHeapTracer()
{
    if ( jsFbProperties_.initialized() && pFbProperties_ )
    {
        pFbProperties_->RemoveHeapTracer();
    }
}

std::optional<std::nullptr_t>
JsWindow::ClearInterval( uint32_t intervalId )
{
    parentPanel_.ClearIntervalOrTimeout( intervalId );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::ClearTimeout( uint32_t timeoutId )
{
    parentPanel_.ClearIntervalOrTimeout( timeoutId );
    return nullptr;
}

std::optional<JSObject*>
JsWindow::CreatePopupMenu()
{
    JS::RootedObject jsObject( pJsCtx_, JsMenuObject::Create( pJsCtx_, parentPanel_.GetHWND() ) );
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
    JS::RootedObject jsObject( pJsCtx_, JsThemeManager::Create( pJsCtx_, parentPanel_.GetHWND(), classid ) );
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
    auto& tooltip_param = parentPanel_.GetPanelTooltipParam();
    tooltip_param.fontName = name;
    tooltip_param.fontSize = pxSize;
    tooltip_param.fontStyle = style;

    JS::RootedObject jsObject( pJsCtx_, JsFbTooltip::Create( pJsCtx_, parentPanel_.GetHWND(), tooltip_param ) );
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
    if ( parentPanel_.GetPanelType() != js_panel_window::PanelType::CUI )
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

    return parentPanel_.GetColourCUI( type, guid );
}

std::optional<uint32_t>
JsWindow::GetColourDUI( uint32_t type )
{
    if ( parentPanel_.GetPanelType() != js_panel_window::PanelType::DUI )
    {
        JS_ReportErrorASCII( pJsCtx_, "Can be called only in DUI" );
        return std::nullopt;
    }

    return parentPanel_.GetColourDUI( type );
}

std::optional<JSObject*>
JsWindow::GetFontCUI( uint32_t type, const std::wstring& guidstr )
{
    if ( parentPanel_.GetPanelType() != js_panel_window::PanelType::CUI )
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

    HFONT hFont = parentPanel_.GetFontCUI( type, guid );
    scope::unique_ptr<std::remove_pointer_t<HFONT>> autoFont( hFont, []( auto obj )
                                                              {
                                                                  DeleteObject( obj );
                                                              } );

    if ( hFont )
    {// Not an error: font not found
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( parentPanel_.GetHDC(), hFont ) );
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
    if ( parentPanel_.GetPanelType() != js_panel_window::PanelType::DUI )
    {
        JS_ReportErrorASCII( pJsCtx_, "Can be called only in DUI" );
        return std::nullopt;
    }

    HFONT hFont = parentPanel_.GetFontDUI( type ); // No need to delete, it is managed by DUI

    if ( hFont )
    {// Not an error: font not found
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( parentPanel_.GetHDC(), hFont ) );
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

std::optional<JS::Value*>
JsWindow::GetProperty( const std::string& name, JS::HandleValue defaultval )
{
    auto retVal = pFbProperties_->GetProperty( name, defaultval );
    if ( !retVal )
    {// report in GetProperty
        return std::nullopt;
    }

    return &(retVal.value());
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

    panel_manager::instance().send_msg_to_others_pointer( parentPanel_.GetHWND(), CALLBACK_UWM_ON_NOTIFY_DATA, notify_data );

    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::Reload()
{
    PostMessage( parentPanel_.GetHWND(), UWM_RELOAD, 0, 0 );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::Repaint( bool force )
{
    parentPanel_.Repaint( force );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{
    parentPanel_.RepaintRect( x, y, w, h, force );
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
    return parentPanel_.SetInterval( func, delay );
}

std::optional<std::nullptr_t>
JsWindow::SetProperty( const std::string& name, JS::HandleValue val )
{
    if ( !pFbProperties_->SetProperty( name, val ) )
    {// report in SetProperty
        return std::nullopt;
    }

    return nullptr;
}

std::optional<uint32_t>
JsWindow::SetTimeout( JS::HandleFunction func, uint32_t delay )
{
    return parentPanel_.SetTimeout( func, delay );
}

std::optional<std::nullptr_t>
JsWindow::ShowConfigure()
{
    PostMessage( parentPanel_.GetHWND(), UWM_SHOW_CONFIGURE, 0, 0 );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::ShowProperties()
{
    PostMessage( parentPanel_.GetHWND(), UWM_SHOW_PROPERTIES, 0, 0 );
    return nullptr;
}

std::optional<uint32_t>
JsWindow::get_DlgCode()
{
    return parentPanel_.DlgCode();
}

std::optional<uint32_t>
JsWindow::get_Height()
{
    return parentPanel_.GetHeight();
}

std::optional<uint64_t>
JsWindow::get_Id()
{
    return reinterpret_cast<uint64_t>( parentPanel_.GetHWND() );
}

std::optional<uint32_t>
JsWindow::get_InstanceType()
{
    return static_cast<uint32_t>( parentPanel_.GetPanelType() );
}

std::optional<bool>
JsWindow::get_IsTransparent()
{
    return parentPanel_.get_pseudo_transparent();
}

std::optional<bool>
JsWindow::get_IsVisible()
{
    return  IsWindowVisible( parentPanel_.GetHWND() );
}

std::optional<uint32_t>
JsWindow::get_MaxHeight()
{
    return parentPanel_.MaxSize().y;
}

std::optional<uint32_t>
JsWindow::get_MaxWidth()
{
    return parentPanel_.MaxSize().x;
}

std::optional<uint32_t>
JsWindow::get_MinHeight()
{
    return  parentPanel_.MinSize().y;
}

std::optional<uint32_t>
JsWindow::get_MinWidth()
{
    return parentPanel_.MinSize().x;
}

std::optional<std::string>
JsWindow::get_Name()
{
    pfc::string8_fast name = parentPanel_.ScriptInfo().name;
    if ( name.is_empty() )
    {
        name = pfc::print_guid( parentPanel_.GetGUID() );
    }

    return std::string( name.c_str(), name.length() );
}

std::optional<uint32_t>
JsWindow::get_Width()
{
    return parentPanel_.GetWidth();
}

std::optional<std::nullptr_t>
JsWindow::put_DlgCode( uint32_t code )
{
    parentPanel_.DlgCode() = code;
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::put_MaxHeight( uint32_t height )
{
    parentPanel_.MaxSize().y = height;
    PostMessage( parentPanel_.GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_maximum_height );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::put_MaxWidth( uint32_t width )
{
    parentPanel_.MaxSize().x = width;
    PostMessage( parentPanel_.GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_maximum_width );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::put_MinHeight( uint32_t height )
{
    parentPanel_.MinSize().y = height;
    PostMessage( parentPanel_.GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_minimum_height );
    return nullptr;
}

std::optional<std::nullptr_t>
JsWindow::put_MinWidth( uint32_t width )
{
    parentPanel_.MinSize().x = width;
    PostMessage( parentPanel_.GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_minimum_width );
    return nullptr;
}

}
