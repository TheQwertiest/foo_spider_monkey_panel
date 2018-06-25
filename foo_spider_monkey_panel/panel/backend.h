#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_objects/gdi_graphics.h>
#include <js_objects/js_persistent_object_wrapper.h>

#include "script_preprocessor.h"
#include "script_interface_impl.h"
#include "config.h"
#include "user_message.h"
#include "host_drop_target.h"
#include "host_timer_dispatcher.h"
#include "script_callback_invoker.h"

class PanelBackend : public js_panel_vars
{
public:
     PanelBackend();
     ~PanelBackend();     

public:
     enum
     {
          KInstanceTypeCUI = 0,
          KInstanceTypeDUI,
     };

     enum MjsInstanceStatus
     {
         Mjs_Ready,
         Mjs_NotInitialized,
         Mjs_Failed,
     };

     void JsEngineFail( std::string_view errorText );

     MjsInstanceStatus GetJsEngineStatus() const;

     GUID GetGUID();
     HDC GetHDC();
     HWND GetHWND();
     POINT& MaxSize();
     POINT& MinSize();
     int GetHeight();
     int GetWidth();
     panel_tooltip_param_ptr& PanelTooltipParam();
     t_script_info& ScriptInfo();
     unsigned SetInterval(IDispatch* func, uint32_t delay);
     unsigned SetTimeout(IDispatch* func, uint32_t delay);
     uint32_t& DlgCode();
     uint32_t GetInstanceType();
     /*
     virtual DWORD GetColourCUI( uint32_t type, const GUID& guid) = 0;
     virtual DWORD GetColourDUI( uint32_t type) = 0;
     virtual HFONT GetFontCUI( uint32_t type, const GUID& guid) = 0;
     virtual HFONT GetFontDUI( uint32_t type) = 0;
     */
     void ClearIntervalOrTimeout( uint32_t timerId);
     void Redraw();
     void RefreshBackground(LPRECT lprcUpdate = NULL);
     void Repaint(bool force = false);
     void RepaintRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force = false);

private:
    bool jsEngineFailed_;

    std::unique_ptr<mozjs::JsPersistentObjectWrapper<mozjs::JsGlobalObject>> jsGlobalObject_;
    std::unique_ptr<mozjs::JsPersistentObjectWrapper<mozjs::JsGdiGraphics>> jsGraphicsObject_;

    HBITMAP m_gr_bmp;
    HBITMAP m_gr_bmp_bk;
    HDC m_hdc;
    HWND m_hwnd;
    POINT m_max_size;
    POINT m_min_size;
    bool m_paint_pending;
    bool m_suppress_drawing;
    uint32_t m_height;
    uint32_t m_instance_type;
    uint32_t m_width;
    panel_tooltip_param_ptr m_panel_tooltip_param_ptr;
    t_script_info m_script_info;
    uint32_t m_accuracy;
    uint32_t m_dlg_code;
    ui_selection_holder::ptr m_selection_holder;   
};
