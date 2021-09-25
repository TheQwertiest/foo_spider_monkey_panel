#pragma once

#include <config/parsed_panel_config.h>
#include <events/event.h>
#include <events/event_js_executor.h>
#include <panel/drag_action_params.h>
#include <panel/user_message.h>
#include <ui/ui_conf.h>

#include <queue>

namespace smp::com
{
class IDropTargetImpl;
}

namespace smp
{
class TimeoutManager;
}

namespace mozjs
{
class JsContainer;
class JsAsyncTask;
} // namespace mozjs

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
    virtual ~js_panel_window();

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
    // TODO: move to a better place
    [[nodiscard]] TimeoutManager& GetTimeoutManager();

    [[nodiscard]] t_size& DlgCode();
    [[nodiscard]] PanelType GetPanelType() const;
    virtual DWORD GetColour( unsigned type, const GUID& guid ) = 0;
    virtual HFONT GetFont( unsigned type, const GUID& guid ) = 0;

    void SetSettings_ScriptInfo( const qwr::u8string& scriptName, const qwr::u8string& scriptAuthor, const qwr::u8string& scriptVersion );
    void SetSettings_PanelName( const qwr::u8string& panelName );
    /// @throw qwr::QwrException
    void SetSettings_DragAndDropStatus( bool isEnabled );
    void SetSettings_CaptureFocusStatus( bool isEnabled );

    void ResetLastDragParams();
    [[nodiscard]] const std::optional<DragActionParams>& GetLastDragParams() const;
    [[nodiscard]] bool HasInternalDrag() const;

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

    void SetCaptureMouseState( bool shouldCapture );
    /// @throw qwr::QwrException
    void SetDragAndDropStatus( bool isEnabled );

public: // event handling
    void ExecuteEvent_JsTask( EventId id, Event_JsExecutor& task );
    bool ExecuteEvent_JsCode( mozjs::JsAsyncTask& task );
    void ExecuteEvent_Basic( EventId id );

private: // callback handling
    void OnProcessingEventStart();
    void OnProcessingEventFinish();
    std::optional<LRESULT> ProcessEvent();
    void ProcessEventManually( Runnable& runnable );

    std::optional<MSG> GetStalledMessage();
    std::optional<LRESULT> ProcessStalledMessage( const MSG& msg );
    std::optional<LRESULT> ProcessSyncMessage( const MSG& msg );
    std::optional<LRESULT> ProcessCreationMessage( const MSG& msg );
    std::optional<LRESULT> ProcessWindowMessage( const MSG& msg );
    std::optional<LRESULT> ProcessInternalSyncMessage( InternalSyncMessage msg, WPARAM wp, LPARAM lp );

    // Internal callbacks
    void OnContextMenu( int x, int y );
    void OnCreate( HWND hWnd );
    void OnDestroy();

    // JS callbacks
    void OnPaint( HDC dc, const CRect& updateRc );
    void OnPaintErrorScreen( HDC memdc );
    void OnPaintJs( HDC memdc, const CRect& updateRc );
    void OnSizeDefault( uint32_t w, uint32_t h );
    void OnSizeUser( uint32_t w, uint32_t h );

private:
    const PanelType panelType_;
    config::ParsedPanelSettings settings_ = config::ParsedPanelSettings::GetDefault();
    config::PanelProperties properties_;

    std::shared_ptr<mozjs::JsContainer> pJsContainer_;
    std::shared_ptr<PanelTarget> pTarget_;
    std::unique_ptr<TimeoutManager> pTimeoutManager_;

    CWindow wnd_;
    HDC hDc_ = nullptr;

    uint32_t height_ = 0;     // used externally as well
    uint32_t width_ = 0;      // used externally as well
    CBitmap bmp_ = nullptr;   // used only internally
    CBitmap bmpBg_ = nullptr; // used only internally

    bool hasFailed_ = false; // // used only internally

    uint32_t eventNestedCounter_ = 0;

    int32_t hRepaintTimer_ = NULL;   // used only internally
    bool isBgRepaintNeeded_ = false; // used only internally
    bool isPaintInProgress_ = false; // used only internally

    bool isMouseTracked_ = false;              // used only internally
    bool isMouseCaptured_ = false;             // used only internally
    bool hasInternalDrag_ = false;             // used only internally
    bool isDraggingInside_ = false;            // used only internally
    ui_selection_holder::ptr selectionHolder_; // used only internally

    CComPtr<smp::com::IDropTargetImpl> dropTargetHandler_; // used only internally
    std::optional<DragActionParams> lastDragParams_;       // used externally as well

    bool isPanelIdOverridenByScript_ = false; // used only internally

    size_t dlgCode_ = 0;                   // modified only from external
    POINT maxSize_ = { INT_MAX, INT_MAX }; // modified only from external
    POINT minSize_ = { 0, 0 };             // modified only from external
};

} // namespace smp::panel
