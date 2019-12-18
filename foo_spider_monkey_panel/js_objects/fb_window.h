#pragma once

#include <js_objects/object_base.h>

#include <memory>
#include <optional>
#include <string>

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
    ~JsFbWindow() override = default;

    static std::unique_ptr<JsFbWindow> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    void SetPseudoCaption( uint32_t x, uint32_t y, uint32_t w, uint32_t h );

public:
    JSObject* get_Aero();
    bool get_BlockMaximize();
    float get_FoobarCpuUsage();
    uint8_t get_FrameStyle();
    bool get_FullScreen();
    uint8_t get_MainWindowState();
    bool get_Sizing();
    float get_SystemCpuUsage();
    void put_BlockMaximize( bool block );
    void put_FrameStyle( uint8_t style );
    void put_FullScreen( bool is );
    void put_MainWindowState( uint8_t state );

    void put_Sizing( bool enable );

private:
    JsFbWindow( JSContext* cx, HWND hFbWnd );

private:
    JSContext* pJsCtx_ = nullptr;
    HWND hFbWnd_ = nullptr;

    struct CpuUsageStats
    {
        FILETIME userTime{};
        FILETIME kernelTime{};
        DWORD time = 0;
    } cpuUsageStats;
};

} // namespace mozjs
