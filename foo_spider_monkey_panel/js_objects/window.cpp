#include <stdafx.h>
#include "window.h"

#include <js_engine/js_value_converter.h>
#include <js_engine/js_native_invoker.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>


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
    JSCLASS_HAS_PRIVATE | JSCLASS_FOREGROUND_FINALIZE,
    &jsOps
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
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
         || !JS_DefineProperties(cx, jsObj, jsProperties ) )
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

std::optional<uint32_t> 
JsWindow::get_DlgCode()
{

}

std::optional<uint32_t> 
JsWindow::get_Height()
{

}

std::optional<uint32_t> 
JsWindow::get_ID()
{

}

std::optional<uint32_t> 
JsWindow::get_InstanceType()
{

}

std::optional<bool> 
JsWindow::get_IsTransparent()
{

}

std::optional<bool> 
JsWindow::get_IsVisible()
{

}

std::optional<uint32_t> 
JsWindow::get_MaxHeight()
{

}

std::optional<uint32_t> 
JsWindow::get_MaxWidth()
{

}

std::optional<uint32_t> 
JsWindow::get_MinHeight()
{

}

std::optional<uint32_t> 
JsWindow::get_MinWidth()
{

}

std::optional<std::string> 
JsWindow::get_Name()
{

}

std::optional<uint32_t> 
JsWindow::get_Width()
{

}

std::optional<std::nullptr_t> 
JsWindow::put_DlgCode( uint32_t code )
{

}

std::optional<std::nullptr_t> 
JsWindow::put_MaxHeight( uint32_t height )
{

}

std::optional<std::nullptr_t> 
JsWindow::put_MaxWidth( uint32_t width )
{

}

std::optional<std::nullptr_t> 
JsWindow::put_MinHeight( uint32_t height )
{

}

std::optional<std::nullptr_t> 
JsWindow::put_MinWidth( uint32_t width )
{

}

std::optional<std::nullptr_t> 
JsWindow::ClearInterval( uint32_t intervalID )
{

}

std::optional<std::nullptr_t> 
JsWindow::ClearTimeout( uint32_t timeoutID )
{

}

std::optional<std::uint32_t> 
JsWindow::GetColourCUI( uint32_t type, std::wstring guidstr )
{

}

std::optional<std::uint32_t> 
JsWindow::GetColourDUI( uint32_t type )
{

}

std::optional<JSObject*> 
JsWindow::GetFontCUI( uint32_t type, std::wstring guidstr )
{

}

std::optional<JSObject*> 
JsWindow::GetFontDUI( uint32_t type )
{

}

std::optional<std::nullptr_t> 
JsWindow::Reload()
{

}

std::optional<std::nullptr_t> 
JsWindow::Repaint( bool force )
{

}

std::optional<std::nullptr_t> 
JsWindow::RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{

}

std::optional<std::nullptr_t> 
JsWindow::SetCursor( uint32_t id )
{

}

std::optional<std::nullptr_t> 
JsWindow::ShowConfigure()
{

}

std::optional<std::nullptr_t> 
JsWindow::ShowProperties()
{

}

/*

STDMETHODIMP FbWindow::ClearInterval(UINT intervalID)
{
    m_host->ClearIntervalOrTimeout(intervalID);
    return S_OK;
}

STDMETHODIMP FbWindow::ClearTimeout(UINT timeoutID)
{
    m_host->ClearIntervalOrTimeout(timeoutID);
    return S_OK;
}

STDMETHODIMP FbWindow::CreatePopupMenu(IMenuObj** pp)
{
    if (!pp) return E_POINTER;

    *pp = new com_object_impl_t<MenuObj>(m_host->GetHWND());
    return S_OK;
}

STDMETHODIMP FbWindow::CreateThemeManager(BSTR classid, IThemeManager** pp)
{
    if (!pp) return E_POINTER;

    IThemeManager* ptheme = NULL;

    try
    {
        ptheme = new com_object_impl_t<ThemeManager>(m_host->GetHWND(), classid);
    }
    catch (...)
    {
        if (ptheme)
        {
            ptheme->Dispose();
            delete ptheme;
            ptheme = NULL;
        }
    }

    *pp = ptheme;
    return S_OK;
}

STDMETHODIMP FbWindow::CreateTooltip(BSTR name, float pxSize, int style, IFbTooltip** pp)
{
    if (!pp) return E_POINTER;

    const auto& tooltip_param = m_host->PanelTooltipParam();
    tooltip_param->font_name = name;
    tooltip_param->font_size = pxSize;
    tooltip_param->font_style = style;
    *pp = new com_object_impl_t<FbTooltip>(m_host->GetHWND(), tooltip_param);
    return S_OK;
}

STDMETHODIMP FbWindow::GetColourCUI(UINT type, BSTR guidstr, int* p)
{
    if (!p) return E_POINTER;
    if (m_host->GetInstanceType() != HostComm::KInstanceTypeCUI) return E_NOTIMPL;

    GUID guid;

    if (!*guidstr)
    {
        memcpy(&guid, &pfc::guid_null, sizeof(guid));
    }
    else
    {
        if (CLSIDFromString(guidstr, &guid) != NOERROR)
        {
            return E_INVALIDARG;
        }
    }

    *p = m_host->GetColourCUI(type, guid);
    return S_OK;
}

STDMETHODIMP FbWindow::GetColourDUI(UINT type, int* p)
{
    if (!p) return E_POINTER;
    if (m_host->GetInstanceType() != HostComm::KInstanceTypeDUI) return E_NOTIMPL;

    *p = m_host->GetColourDUI(type);
    return S_OK;
}

STDMETHODIMP FbWindow::GetFontCUI(UINT type, BSTR guidstr, IGdiFont** pp)
{
    if (!pp) return E_POINTER;
    if (m_host->GetInstanceType() != HostComm::KInstanceTypeCUI) return E_NOTIMPL;

    GUID guid;

    if (!*guidstr)
    {
        memcpy(&guid, &pfc::guid_null, sizeof(guid));
    }
    else
    {
        if (CLSIDFromString(guidstr, &guid) != NOERROR)
        {
            return E_INVALIDARG;
        }
    }

    HFONT hFont = m_host->GetFontCUI(type, guid);

    *pp = NULL;

    if (hFont)
    {
        Gdiplus::Font* font = new Gdiplus::Font(m_host->GetHDC(), hFont);
        if (helpers::ensure_gdiplus_object(font))
        {
            *pp = new com_object_impl_t<GdiFont>(font, hFont);
        }
        else
        {
            if (font) delete font;
            *pp = NULL;
        }
    }

    return S_OK;
}

STDMETHODIMP FbWindow::GetFontDUI(UINT type, IGdiFont** pp)
{
    if (!pp) return E_POINTER;
    if (m_host->GetInstanceType() != HostComm::KInstanceTypeDUI) return E_NOTIMPL;

    HFONT hFont = m_host->GetFontDUI(type);
    *pp = NULL;

    if (hFont)
    {
        Gdiplus::Font* font = new Gdiplus::Font(m_host->GetHDC(), hFont);
        if (helpers::ensure_gdiplus_object(font))
        {
            *pp = new com_object_impl_t<GdiFont>(font, hFont, false);
        }
        else
        {
            if (font) delete font;
            *pp = NULL;
        }
    }

    return S_OK;
}

STDMETHODIMP FbWindow::GetProperty(BSTR name, VARIANT defaultval, VARIANT* p)
{
    if (!p) return E_POINTER;

    HRESULT hr;
    _variant_t var;
    pfc::stringcvt::string_utf8_from_wide uname(name);

    if (m_host->get_config_prop().get_config_item(uname, var))
    {
        hr = VariantCopy(p, &var);
    }
    else
    {
        m_host->get_config_prop().set_config_item(uname, defaultval);
        hr = VariantCopy(p, &defaultval);
    }

    if (FAILED(hr))
        p = NULL;

    return S_OK;
}

STDMETHODIMP FbWindow::NotifyOthers(BSTR name, VARIANT info)
{
    if (info.vt & VT_BYREF) return E_INVALIDARG;

    HRESULT hr = S_OK;
    _variant_t var;

    hr = VariantCopy(&var, &info);

    if (FAILED(hr)) return hr;

    simple_callback_data_2<_bstr_t, _variant_t>* notify_data = new simple_callback_data_2<_bstr_t, _variant_t>(name, NULL);

    notify_data->m_item2.Attach(var.Detach());

    panel_manager::instance().send_msg_to_others_pointer(m_host->GetHWND(), CALLBACK_UWM_ON_NOTIFY_DATA, notify_data);

    return S_OK;
}

STDMETHODIMP FbWindow::Reload()
{
    PostMessage(m_host->GetHWND(), UWM_RELOAD, 0, 0);
    return S_OK;
}

STDMETHODIMP FbWindow::Repaint(VARIANT_BOOL force)
{
    m_host->Repaint(force != FALSE);
    return S_OK;
}

STDMETHODIMP FbWindow::RepaintRect(LONG x, LONG y, LONG w, LONG h, VARIANT_BOOL force)
{
    m_host->RepaintRect(x, y, w, h, force != FALSE);
    return S_OK;
}

STDMETHODIMP FbWindow::SetCursor(UINT id)
{
    ::SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(id)));
    return S_OK;
}

STDMETHODIMP FbWindow::SetInterval(IDispatch* func, int delay, UINT* outIntervalID)
{
    if (!outIntervalID) return E_POINTER;

    *outIntervalID = m_host->SetInterval(func, delay);
    return S_OK;
}

STDMETHODIMP FbWindow::SetProperty(BSTR name, VARIANT val)
{
    m_host->get_config_prop().set_config_item(pfc::stringcvt::string_utf8_from_wide(name), val);
    return S_OK;
}

STDMETHODIMP FbWindow::SetTimeout(IDispatch* func, int delay, UINT* outTimeoutID)
{
    if (!outTimeoutID) return E_POINTER;

    *outTimeoutID = m_host->SetTimeout(func, delay);
    return S_OK;
}

STDMETHODIMP FbWindow::ShowConfigure()
{
    PostMessage(m_host->GetHWND(), UWM_SHOW_CONFIGURE, 0, 0);
    return S_OK;
}

STDMETHODIMP FbWindow::ShowProperties()
{
    PostMessage(m_host->GetHWND(), UWM_SHOW_PROPERTIES, 0, 0);
    return S_OK;
}

STDMETHODIMP FbWindow::get_DlgCode(UINT* p)
{
    if (!p) return E_POINTER;

    *p = m_host->DlgCode();
    return S_OK;
}

STDMETHODIMP FbWindow::get_Height(INT* p)
{
    if (!p) return E_POINTER;

    *p = m_host->GetHeight();
    return S_OK;
}

STDMETHODIMP FbWindow::get_ID(UINT* p)
{
    if (!p) return E_POINTER;

    *p = (UINT)m_host->GetHWND();
    return S_OK;
}

STDMETHODIMP FbWindow::get_InstanceType(UINT* p)
{
    if (!p) return E_POINTER;

    *p = m_host->GetInstanceType();
    return S_OK;
}

STDMETHODIMP FbWindow::get_IsTransparent(VARIANT_BOOL* p)
{
    if (!p) return E_POINTER;

    *p = TO_VARIANT_BOOL(m_host->get_pseudo_transparent());
    return S_OK;
}

STDMETHODIMP FbWindow::get_IsVisible(VARIANT_BOOL* p)
{
    if (!p) return E_POINTER;

    *p = TO_VARIANT_BOOL(IsWindowVisible(m_host->GetHWND()));
    return S_OK;
}

STDMETHODIMP FbWindow::get_MaxHeight(UINT* p)
{
    if (!p) return E_POINTER;

    *p = m_host->MaxSize().y;
    return S_OK;
}

STDMETHODIMP FbWindow::get_MaxWidth(UINT* p)
{
    if (!p) return E_POINTER;

    *p = m_host->MaxSize().x;
    return S_OK;
}

STDMETHODIMP FbWindow::get_MinHeight(UINT* p)
{
    if (!p) return E_POINTER;

    *p = m_host->MinSize().y;
    return S_OK;
}

STDMETHODIMP FbWindow::get_MinWidth(UINT* p)
{
    if (!p) return E_POINTER;

    *p = m_host->MinSize().x;
    return S_OK;
}

STDMETHODIMP FbWindow::get_Name(BSTR* p)
{
    if (!p) return E_POINTER;

    pfc::string8_fast name = m_host->ScriptInfo().name;
    if (name.is_empty())
    {
        name = pfc::print_guid(m_host->GetGUID());
    }

    *p = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(name));
    return S_OK;
}

STDMETHODIMP FbWindow::get_Width(INT* p)
{
    if (!p) return E_POINTER;

    *p = m_host->GetWidth();
    return S_OK;
}

STDMETHODIMP FbWindow::put_DlgCode(UINT code)
{
    m_host->DlgCode() = code;
    return S_OK;
}

STDMETHODIMP FbWindow::put_MaxHeight(UINT height)
{
    m_host->MaxSize().y = height;
    PostMessage(m_host->GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_maximum_height);
    return S_OK;
}

STDMETHODIMP FbWindow::put_MaxWidth(UINT width)
{
    m_host->MaxSize().x = width;
    PostMessage(m_host->GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_maximum_width);
    return S_OK;
}

STDMETHODIMP FbWindow::put_MinHeight(UINT height)
{
    m_host->MinSize().y = height;
    PostMessage(m_host->GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_minimum_height);
    return S_OK;
}

STDMETHODIMP FbWindow::put_MinWidth(UINT width)
{
    m_host->MinSize().x = width;
    PostMessage(m_host->GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_minimum_width);
    return S_OK;
}

*/

}
