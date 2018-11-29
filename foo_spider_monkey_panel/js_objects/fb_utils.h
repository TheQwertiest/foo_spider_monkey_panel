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
    ~JsFbUtils();

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
    // TODO: remove after adding array methods to JsFbMetadbHandleList
    JSObject* CreateHandleList();
    JSObject* CreateMainMenuManager();
    JSObject* CreateProfiler( const pfc::string8_fast& name = "" );
    JSObject* CreateProfilerWithOpt( size_t optArgCount, const pfc::string8_fast& name );
    uint32_t DoDragDrop( uint32_t hWindow, JsFbMetadbHandleList* handles, uint32_t okEffects, JS::HandleValue options = JS::UndefinedHandleValue );
    uint32_t DoDragDropWithOpt( size_t optArgCount, uint32_t hWindow, JsFbMetadbHandleList* handles, uint32_t okEffects, JS::HandleValue options );
    void Exit();
    JSObject* GetClipboardContents( uint32_t hWindow );
    pfc::string8_fast GetDSPPresets();
    JSObject* GetFocusItem( bool force = true );
    JSObject* GetFocusItemWithOpt( size_t optArgCount, bool force );
    JSObject* GetLibraryItems();
    pfc::string8_fast GetLibraryRelativePath( JsFbMetadbHandle* handle );
    JSObject* GetNowPlaying();
    pfc::string8_fast GetOutputDevices();
    JSObject* GetQueryItems( JsFbMetadbHandleList* handles, const pfc::string8_fast& query );
    JSObject* GetSelection();
    JSObject* GetSelections( uint32_t flags = 0 );
    JSObject* GetSelectionsWithOpt( size_t optArgCount, uint32_t flags );
    uint32_t GetSelectionType();
    bool IsLibraryEnabled();
    bool IsMainMenuCommandChecked( const pfc::string8_fast& command );
    bool IsMetadbInMediaLibrary( JsFbMetadbHandle* handle );
    void LoadPlaylist();
    void Next();
    void Pause();
    void Play();
    void PlayOrPause();
    void Prev();
    void Random();
    bool RunContextCommand( const pfc::string8_fast& command, uint32_t flags = 0 );
    bool RunContextCommandWithOpt( size_t optArgCount, const pfc::string8_fast& command, uint32_t flags );
    bool RunContextCommandWithMetadb( const pfc::string8_fast& command, JS::HandleValue handle, uint32_t flags = 0 );
    bool RunContextCommandWithMetadbWithOpt( size_t optArgCount, const pfc::string8_fast& command, JS::HandleValue handle, uint32_t flags );
    bool RunMainMenuCommand( const pfc::string8_fast& command );
    void SavePlaylist();
    void SetDSPPreset( uint32_t idx );
    void SetOutputDevice( const std::wstring& output, const std::wstring& device );
    void ShowConsole();
    void ShowLibrarySearchUI( const pfc::string8_fast& query );
    void ShowPopupMessage( const pfc::string8_fast& msg, const pfc::string8_fast& title = "Spider Monkey Panel" );
    void ShowPopupMessageWithOpt( size_t optArgCount, const pfc::string8_fast& msg, const pfc::string8_fast& title );
    void ShowPreferences();
    void Stop();
    JSObject* TitleFormat( const pfc::string8_fast& expression );
    void VolumeDown();
    void VolumeMute();
    void VolumeUp();

public:
    bool get_AlwaysOnTop();
    pfc::string8_fast get_ComponentPath();
    bool get_CursorFollowPlayback();
    pfc::string8_fast get_FoobarPath();
    bool get_IsPaused();
    bool get_IsPlaying();
    bool get_PlaybackFollowCursor();
    double get_PlaybackLength();
    double get_PlaybackTime();
    pfc::string8_fast get_ProfilePath();
    uint32_t get_ReplaygainMode();
    bool get_StopAfterCurrent();
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
