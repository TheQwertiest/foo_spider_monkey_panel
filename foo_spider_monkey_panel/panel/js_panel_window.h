#pragma once

#include <config/parsed_panel_config.h>
#include <events/event.h>
#include <events/ievent_js_forwarder.h>
#include <panel/panel_info.h>
#include <panel/user_message.h>
#include <ui/ui_conf.h>

#include <queue>

namespace smp::com
{
class IDropTargetImpl;
}

namespace mozjs
{
class JsContainer;
}

namespace smp::panel
{

class CallbackData;

enum class PanelType : uint8_t
{
    CUI = 0,
    DUI = 1
};

class js_panel_window
    : public ui_helpers::container_window
{
public:
    js_panel_window( PanelType instanceType );
    virtual ~js_panel_window() = default;

public:
    // ui_helpers::container_window
    [[nodiscard]] class_data& get_class_data() const override;

    void ReloadScript();
    void LoadSettings( stream_reader& reader, t_size size, abort_callback& abort, bool reloadPanel = true );
    void SetSettings( const smp::config::ParsedPanelSettings& settings );
    bool UpdateSettings( const smp::config::PanelSettings& settings, bool reloadPanel = true );
    bool SaveSettings( stream_writer& writer, abort_callback& abort ) const;

    bool IsPanelIdOverridenByScript() const;

    void Fail( const qwr::u8string& errorText );

    void Repaint( bool force = false );
    void RepaintRect( const CRect& rc, bool force = false );
    /// @details Calls Repaint inside
    void RepaintBackground( const CRect& updateRc );

public: // accessors
    [[nodiscard]] qwr::u8string GetPanelId();
    [[nodiscard]] qwr::u8string GetPanelDescription( bool includeVersionAndAuthor = true );
    [[nodiscard]] HDC GetHDC() const;
    [[nodiscard]] HWND GetHWND() const;
    [[nodiscard]] POINT& MaxSize();
    [[nodiscard]] POINT& MinSize();
    [[nodiscard]] int GetHeight() const;
    [[nodiscard]] int GetWidth() const;
    [[nodiscard]] const config::ParsedPanelSettings& GetSettings() const;
    [[nodiscard]] config::PanelProperties& GetPanelProperties();

    [[nodiscard]] t_size& DlgCode();
    [[nodiscard]] PanelType GetPanelType() const;
    virtual DWORD GetColour( unsigned type, const GUID& guid ) = 0;
    virtual HFONT GetFont( unsigned type, const GUID& guid ) = 0;

    void SetScriptInfo( const qwr::u8string& scriptName, const qwr::u8string& scriptAuthor, const qwr::u8string& scriptVersion );
    void SetPanelName( const qwr::u8string& panelName );
    /// @throw qwr::QwrException
    void SetDragAndDropStatus( bool isEnabled );
    void SetCaptureFocusStatus( bool isEnabled );

protected:
    virtual void notify_size_limit_changed( LPARAM lp ) = 0;

    // ui_helpers::container_window
    LRESULT on_message( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) override;

    void EditScript();
    void ShowConfigure( HWND parent, ui::CDialogConf::Tab tab = ui::CDialogConf::Tab::def );

    void GenerateContextMenu( HMENU hMenu, int x, int y, uint32_t id_base );
    void ExecuteContextMenu( uint32_t id, uint32_t id_base );

private:
    bool ReloadSettings();
    bool LoadScript( bool isFirstLoad );
    void UnloadScript( bool force = false );
    void CreateDrawContext();
    void DeleteDrawContext();

public:
    void ExecuteJsTask( EventId id, IEvent_JsTask& task );

private: // callback handling
    std::optional<LRESULT> process_sync_messages( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_async_messages( UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_main_messages( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_window_messages( UINT msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_internal_sync_messages( InternalSyncMessage msg, WPARAM wp, LPARAM lp );
    std::optional<LRESULT> process_internal_async_messages( InternalAsyncMessage msg, WPARAM wp, LPARAM lp );

    // Internal callbacks
    void OpenDefaultContextManu( int x, int y );
    void OnCreate( HWND hWnd );
    void OnDestroy();

    // JS callbacks
    void on_drag_drop( LPARAM lp );
    void on_drag_enter( LPARAM lp );
    void on_drag_leave();
    void on_drag_over( LPARAM lp );
    void on_notify_data( WPARAM wp, LPARAM lp );
    void on_paint( HDC dc, const CRect& updateRc );
    void on_paint_error( HDC memdc );
    void on_paint_user( HDC memdc, const CRect& updateRc );
    void on_size( uint32_t w, uint32_t h );

private:
    const PanelType panelType_;
    config::ParsedPanelSettings settings_ = config::ParsedPanelSettings::GetDefault();
    config::PanelProperties properties_;

    std::shared_ptr<mozjs::JsContainer> pJsContainer_;

    CWindow wnd_;
    HDC hDc_ = nullptr;

    uint32_t height_ = 0;     // Used externally as well
    uint32_t width_ = 0;      // Used externally as well
    CBitmap bmp_ = nullptr;   // used only internally
    CBitmap bmpBg_ = nullptr; // used only internally

    bool hasFailed_ = false;                   // // used only internally
    bool isBgRepaintNeeded_ = false;           // used only internally
    bool isPaintInProgress_ = false;           // used only internally
    bool isMouseTracked_ = false;              // used only internally
    ui_selection_holder::ptr selectionHolder_; // used only internally

    CComPtr<smp::com::IDropTargetImpl> dropTargetHandler_; // used only internally

    bool isPanelIdOverridenByScript_ = false; // used only internally

    size_t dlgCode_ = 0;                   // modified only from external
    POINT maxSize_ = { INT_MAX, INT_MAX }; // modified only from external
    POINT minSize_ = { 0, 0 };             // modified only from external
};

} // namespace smp::panel
