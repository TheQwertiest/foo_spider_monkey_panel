#pragma once

#include <config/panel_config.h>
#include <config/resolved_panel_script_settings.h>
#include <events/event.h>
#include <events/event_js_executor.h>
#include <panel/drag_action_params.h>
#include <panel/panel_adaptor_iface.h>
#include <panel/user_message.h>
#include <ui/ui_conf.h>
#include <utils/not_null.h>

#include <queue>

namespace smp::com
{
class IDropTargetImpl;
}

namespace smp
{
class TimeoutManager;
}

namespace smp::panel
{
class CallbackData;
class PanelWindowGraphics;
class MouseMessageHandler;
class KeyboardMessageHandler;
} // namespace smp::panel

namespace mozjs
{
class JsContainer;
class JsAsyncTask;
} // namespace mozjs

namespace smp::panel
{

class PanelWindow
    : public uie::container_window_v3
{
    friend class PanelAdaptorCui;
    friend class PanelAdaptorDui;

public:
    PanelWindow( IPanelAdaptor& impl );
    virtual ~PanelWindow();

public:
    // uie::container_window_v3
    uie::container_window_v3_config get_window_config();

    void UpdateConfig( const smp::config::PanelConfig& settings, bool reloadPanel = true );
    void LoadConfig( stream_reader& reader, t_size size, abort_callback& abort, bool reloadPanel = true );
    bool SaveConfig( stream_writer& writer, abort_callback& abort ) const;

    bool IsPanelIdOverridenByScript() const;

    void Fail( const qwr::u8string& errorText );

    void Repaint( bool force = false );
    void RepaintRect( const CRect& rc, bool force = false );
    /// @details Calls Repaint inside
    void RepaintBackground();

public: // accessors
    [[nodiscard]] const config::PanelConfig& GetPanelConfig() const;
    [[nodiscard]] const config::ResolvedPanelScriptSettings& GetScriptSettings() const;
    [[nodiscard]] config::PanelProperties& GetPanelProperties();
    [[nodiscard]] qwr::u8string GetPanelDescription( bool includeVersionAndAuthor = true );

    [[nodiscard]] PanelWindowGraphics* GetGraphics();

    [[nodiscard]] HWND GetHWND() const;
    [[nodiscard]] POINT& MaxSize();
    [[nodiscard]] POINT& MinSize();
    // TODO: move to a better place
    [[nodiscard]] TimeoutManager& GetTimeoutManager();

    [[nodiscard]] t_size& DlgCode();
    [[nodiscard]] PanelType GetPanelType() const;
    DWORD GetColour( unsigned type, const GUID& guid );
    HFONT GetFont( unsigned type, const GUID& guid );

    // accessors for runtime settings changes from JS
    void SetSettings_ScriptInfo( const qwr::u8string& scriptName, const qwr::u8string& scriptAuthor, const qwr::u8string& scriptVersion );
    void SetSettings_PanelName( const qwr::u8string& panelName );
    /// @throw qwr::QwrException
    void SetSettings_DragAndDropStatus( bool isEnabled );
    void SetSettings_CaptureFocusStatus( bool isEnabled );

    // accessors for TrackDropTarget
    void ResetLastDragParams();
    [[nodiscard]] const std::optional<DragActionParams>& GetLastDragParams() const;
    [[nodiscard]] bool HasInternalDrag() const;

public: // event handling
    void ExecuteEvent( EventBase& event );

    void ExecuteEvent_JsTask( EventId id, Event_JsExecutor& task );
    bool ExecuteEvent_JsCode( mozjs::JsAsyncTask& task );
    void ExecuteEvent_Basic( EventId id );

private: // internal event handling
    void OnSizeLimitChanged( LPARAM lp );

    LRESULT OnMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp );

    void EditScript();
    void ShowConfigure( HWND parent, ui::CDialogConf::Tab tab = ui::CDialogConf::Tab::def );

    void GenerateContextMenu( HMENU hMenu, int x, int y, uint32_t id_base );
    void ExecuteContextMenu( uint32_t id, uint32_t id_base );

private:
    /// @throw qwr::QwrException
    void SetDragAndDropStatus( bool isEnabled );

private:
    bool ReloadScriptSettings();
    void LoadScript( bool isFirstLoad );
    void UnloadScript( bool force = false );
    void ReloadScript();

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

    void OnContextMenu( int x, int y );
    void OnCreate( HWND hWnd );
    void OnDestroy();
    void OnPaint( EventBase& event );

private:
    const PanelType panelType_;
    IPanelAdaptor& impl_;

    smp::not_null<std::unique_ptr<MouseMessageHandler>> pMouseMessageHandler_;
    smp::not_null<std::unique_ptr<KeyboardMessageHandler>> pKeyboardMessageHandler_;

    config::PanelConfig config_;
    config::ResolvedPanelScriptSettings scriptSettings_;

    std::shared_ptr<mozjs::JsContainer> pJsContainer_;
    std::shared_ptr<PanelTarget> pTarget_;
    std::unique_ptr<TimeoutManager> pTimeoutManager_;

    CWindow wnd_;
    std::unique_ptr<PanelWindowGraphics> pGraphics_;

    bool hasFailed_ = false; // // used only internally

    uint32_t eventNestedCounter_ = 0;

    int32_t hRepaintTimer_ = NULL;   // used only internally
    bool isBgRepaintNeeded_ = false; // used only internally
    bool isPaintInProgress_ = false; // used only internally

    bool isMouseTracked_ = false;   // used only internally
    bool isMouseCaptured_ = false;  // used only internally
    bool hasInternalDrag_ = false;  // used only internally
    bool isDraggingInside_ = false; // used only internally
    // TODO: is this needed at all?
    ui_selection_holder::ptr selectionHolder_; // used only internally

    CComPtr<smp::com::IDropTargetImpl> dropTargetHandler_; // used only internally
    std::optional<DragActionParams> lastDragParams_;       // used externally as well

    bool isPanelIdOverridenByScript_ = false; // used only internally

    size_t dlgCode_ = 0; // modified only from external
    // TODO: move to graphics class
    POINT maxSize_ = { INT_MAX, INT_MAX }; // modified only from external
    POINT minSize_ = { 0, 0 };             // modified only from external
};

} // namespace smp::panel
