#pragma once

#include <js_objects/object_base.h>
#include <utils/gdi_helpers.h>

#include <memory>
#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbTooltip
    : public JsObjectBase<JsFbTooltip>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    // @remark No need to cleanup JS here, since it must be performed manually beforehand anyway
    ~JsFbTooltip() override = default;

    static std::unique_ptr<JsFbTooltip> CreateNative( JSContext* cx, HWND hParentWnd );
    static size_t GetInternalSize( HWND hParentWnd );

    void PrepareForGc();

public:
    void Activate();
    void Deactivate();
    uint32_t GetDelayTime( uint32_t type );
    void SetDelayTime( uint32_t type, int32_t time );
    void SetFont( const std::wstring& name, uint32_t pxSize = 12, uint32_t style = 0 );
    void SetFontWithOpt( size_t optArgCount, const std::wstring& name, uint32_t pxSize, uint32_t style );
    void SetMaxWidth( uint32_t width );
    void TrackPosition( int x, int y );

public:
    std::wstring get_Text();
    void put_Text( const std::wstring& text );
    void put_TrackActivate( bool activate );

private:
    JsFbTooltip( JSContext* cx, HWND hParentWnd );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;

    HWND hParentWnd_ = nullptr;

    smp::gdi::unique_gdi_ptr<HFONT> pFont_;
    std::wstring tipBuffer_;
    std::wstring fontName_;
    uint32_t fontSize_{};
    uint32_t fontStyle_{};

    CToolTipCtrl tooltipManual_;
    std::unique_ptr<CToolInfo> pToolinfoManual_;
};

} // namespace mozjs
