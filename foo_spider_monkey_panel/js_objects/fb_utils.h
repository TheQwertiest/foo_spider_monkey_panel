#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbMetadbHandle;
class JsFbMetadbHandleList;
class JsGdiBitmap;

class JsFbUtils
    : public JsObjectBase<JsFbUtils>
{
public:
    static constexpr bool HasProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsFbUtils() override = default;

    static std::unique_ptr<JsFbUtils> CreateNative( JSContext* cx );
    static size_t GetInternalSize();

public:
    JSObject* AcquireUiSelectionHolder();
    void AddDirectory();
    void AddFiles();
    bool CheckClipboardContents();
    void ClearPlaylist();
    bool CopyHandleListToClipboard( JsFbMetadbHandleList* handles );
    JSObject* CreateContextMenuManager();
    // TODO v2: remove
    JSObject* CreateHandleList();
    JSObject* CreateMainMenuManager();
    JSObject* CreateProfiler( const qwr::u8string& name = "" );
    JSObject* CreateProfilerWithOpt( size_t optArgCount, const qwr::u8string& name );
    uint32_t DoDragDrop( uint32_t hWnd, JsFbMetadbHandleList* handles, uint32_t okEffects, JS::HandleValue options = JS::UndefinedHandleValue );
    uint32_t DoDragDropWithOpt( size_t optArgCount, uint32_t hWnd, JsFbMetadbHandleList* handles, uint32_t okEffects, JS::HandleValue options );
    void Exit();
    JSObject* GetClipboardContents( uint32_t hWnd = 0 );
    JSObject* GetClipboardContentsWithOpt( size_t optArgCount, uint32_t hWnd );
    qwr::u8string GetDSPPresets();
    JSObject* GetFocusItem( bool force = true );
    JSObject* GetFocusItemWithOpt( size_t optArgCount, bool force );
    JSObject* GetLibraryItems();
    pfc::string8_fast GetLibraryRelativePath( JsFbMetadbHandle* handle );
    JSObject* GetNowPlaying();
    qwr::u8string GetOutputDevices();
    JSObject* GetQueryItems( JsFbMetadbHandleList* handles, const qwr::u8string& query );
    JSObject* GetSelection();
    JSObject* GetSelections( uint32_t flags = 0 );
    JSObject* GetSelectionsWithOpt( size_t optArgCount, uint32_t flags );
    uint32_t GetSelectionType();
    bool IsLibraryEnabled();
    bool IsMainMenuCommandChecked( const qwr::u8string& command );
    bool IsMetadbInMediaLibrary( JsFbMetadbHandle* handle );
    void LoadPlaylist();
    void Next();
    void Pause();
    void Play();
    void PlayOrPause();
    void Prev();
    void Random();
    void RegisterMainMenuCommand( uint32_t id, const qwr::u8string& name, const std::optional<qwr::u8string>& description = std::nullopt );
    void RegisterMainMenuCommandWithOpt( size_t optArgCount, uint32_t id, const qwr::u8string& name, const std::optional<qwr::u8string>& description );
    void Restart();
    bool RunContextCommand( const qwr::u8string& command, uint32_t flags = 0 );
    bool RunContextCommandWithOpt( size_t optArgCount, const qwr::u8string& command, uint32_t flags );
    bool RunContextCommandWithMetadb( const qwr::u8string& command, JS::HandleValue handle, uint32_t flags = 0 );
    bool RunContextCommandWithMetadbWithOpt( size_t optArgCount, const qwr::u8string& command, JS::HandleValue handle, uint32_t flags );
    bool RunMainMenuCommand( const qwr::u8string& command );
    void SavePlaylist();
    void SetDSPPreset( uint32_t idx );
    void SetOutputDevice( const std::wstring& output, const std::wstring& device );
    void ShowConsole();
    void ShowLibrarySearchUI( const qwr::u8string& query );
    void ShowPopupMessage( const qwr::u8string& msg, const qwr::u8string& title = "Spider Monkey Panel" );
    void ShowPopupMessageWithOpt( size_t optArgCount, const qwr::u8string& msg, const qwr::u8string& title );
    void ShowPreferences();
    void Stop();
    JSObject* TitleFormat( const qwr::u8string& expression );
    void UnregisterMainMenuCommand( uint32_t id );
    void VolumeDown();
    void VolumeMute();
    void VolumeUp();

public:
    bool get_AlwaysOnTop();
    qwr::u8string get_ComponentPath();
    bool get_CursorFollowPlayback();
    qwr::u8string get_FoobarPath();
    bool get_IsPaused();
    bool get_IsPlaying();
    bool get_PlaybackFollowCursor();
    double get_PlaybackLength();
    double get_PlaybackTime();
    qwr::u8string get_ProfilePath();
    uint32_t get_ReplaygainMode();
    bool get_StopAfterCurrent();
    qwr::u8string get_Version();
    float get_Volume();
    void put_AlwaysOnTop( bool p );
    void put_CursorFollowPlayback( bool p );
    void put_PlaybackFollowCursor( bool p );
    void put_PlaybackTime( double time );
    void put_ReplaygainMode( uint32_t p );
    void put_StopAfterCurrent( bool p );
    void put_Volume( float value );

private:
    JsFbUtils( JSContext* cx );

    struct DoDragDropOptions
    {
        bool useTheming = true;
        bool useAlbumArt = true;
        bool showText = true;
        Gdiplus::Bitmap* pCustomImage = nullptr;
    };
    DoDragDropOptions ParseDoDragDropOptions( JS::HandleValue options );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
