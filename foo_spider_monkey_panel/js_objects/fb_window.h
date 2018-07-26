#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>
#include <memory>


namespace mozjs
{

class JsFbWindow;

class JsFbWindow
    : public JsObjectBase<JsFbWindow>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsFbWindow();

    static std::unique_ptr<JsFbWindow> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    std::optional<nullptr_t> SetPseudoCaption( uint32_t x, uint32_t y, uint32_t w, uint32_t h );

public:
    std::optional<JSObject*> get_Aero();
    std::optional<bool> get_BlockMaximize();
    std::optional<float> get_FoobarCpuUsage();
    std::optional<uint8_t> get_FrameStyle();
    std::optional<bool> get_FullScreen();
    std::optional<uint8_t> get_MainWindowState();
    std::optional<bool> get_Sizing();
    std::optional<float> get_SystemCpuUsage();
    std::optional<nullptr_t> put_BlockMaximize( bool block );
    std::optional<nullptr_t> put_FrameStyle( uint8_t style );
    std::optional<nullptr_t> put_FullScreen( bool is );    
    std::optional<nullptr_t> put_MainWindowState( uint8_t state );

    std::optional<nullptr_t> put_Sizing( bool enable );
private:
    JsFbWindow( JSContext* cx, HWND hFbWnd );

private:
    JSContext * pJsCtx_ = nullptr;
    HWND hFbWnd_ = nullptr;

    struct CpuUsageStats
    {
        FILETIME userTime;
        FILETIME kernelTime;
        DWORD time = 0;
    } cpuUsageStats;
};

}

