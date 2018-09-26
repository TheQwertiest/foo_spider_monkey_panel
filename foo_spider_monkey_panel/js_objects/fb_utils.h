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
    std::optional<JSObject*> AcquireUiSelectionHolder();
    std::optional<std::nullptr_t> AddDirectory();
    std::optional<std::nullptr_t> AddFiles();
    // TODO: document - removed parameter (window id argument was unused)
    std::optional<bool> CheckClipboardContents();
    std::optional<std::nullptr_t> ClearPlaylist();
    std::optional<bool> CopyHandleListToClipboard( JsFbMetadbHandleList* handles );
    std::optional<JSObject*> CreateContextMenuManager();
    // TODO: remove after adding array methods to JsFbMetadbHandleList
    std::optional<JSObject*> CreateHandleList();
    std::optional<JSObject*> CreateMainMenuManager();
    std::optional<JSObject*> CreateProfiler( const pfc::string8_fast& name = "" );    
    std::optional<JSObject*> CreateProfilerWithOpt( size_t optArgCount, const pfc::string8_fast& name );
    std::optional<uint32_t> DoDragDrop( JsFbMetadbHandleList* handles, uint32_t okEffects );
    std::optional<std::nullptr_t> Exit();
    std::optional<JSObject*> GetClipboardContents( uint32_t hWindow );
    std::optional<pfc::string8_fast> GetDSPPresets();
    std::optional<JSObject*> GetFocusItem( bool force = true );
    std::optional<JSObject*> GetFocusItemWithOpt( size_t optArgCount, bool force );
    std::optional<JSObject*> GetLibraryItems();
    std::optional<pfc::string8_fast> GetLibraryRelativePath( JsFbMetadbHandle* handle );
    std::optional<JSObject*> GetNowPlaying();
    std::optional<pfc::string8_fast> GetOutputDevices();
    std::optional<JSObject*> GetQueryItems( JsFbMetadbHandleList* handles, const pfc::string8_fast& query );
    std::optional<JSObject*> GetSelection();
    std::optional<JSObject*> GetSelections( uint32_t flags = 0 );
    std::optional<JSObject*> GetSelectionsWithOpt( size_t optArgCount, uint32_t flags );
    std::optional<uint32_t> GetSelectionType();
    std::optional<bool> IsLibraryEnabled();
    // TODO: document this
    std::optional<bool> IsMainMenuCommandChecked( const pfc::string8_fast& command );
    std::optional<bool> IsMetadbInMediaLibrary( JsFbMetadbHandle* handle );
    std::optional<std::nullptr_t> LoadPlaylist();
    std::optional<std::nullptr_t> Next();
    std::optional<std::nullptr_t> Pause();
    std::optional<std::nullptr_t> Play();
    std::optional<std::nullptr_t> PlayOrPause();
    std::optional<std::nullptr_t> Prev();
    std::optional<std::nullptr_t> Random();
    std::optional<bool> RunContextCommand( const pfc::string8_fast& command, uint32_t flags = 0);
    std::optional<bool> RunContextCommandWithOpt( size_t optArgCount, const pfc::string8_fast& command, uint32_t flags );
    std::optional<bool> RunContextCommandWithMetadb( const pfc::string8_fast& command, JS::HandleValue handle, uint32_t flags = 0);
    std::optional<bool> RunContextCommandWithMetadbWithOpt( size_t optArgCount, const pfc::string8_fast& command, JS::HandleValue handle, uint32_t flags );
    std::optional<bool> RunMainMenuCommand( const pfc::string8_fast& command );    
    std::optional<std::nullptr_t> SavePlaylist();
    std::optional<std::nullptr_t> SetDSPPreset( uint32_t idx );
    std::optional<std::nullptr_t> SetOutputDevice( const std::wstring& output, const std::wstring& device );
    std::optional<std::nullptr_t> ShowConsole();
    std::optional<std::nullptr_t> ShowLibrarySearchUI( const pfc::string8_fast& query );
    std::optional<std::nullptr_t> ShowPopupMessage( const pfc::string8_fast& msg, const pfc::string8_fast& title = "Spider Monkey Panel" );
    std::optional<std::nullptr_t> ShowPopupMessageWithOpt( size_t optArgCount, const pfc::string8_fast& msg, const pfc::string8_fast& title );
    std::optional<std::nullptr_t> ShowPreferences();
    std::optional<std::nullptr_t> Stop();
    std::optional<JSObject*> TitleFormat( const pfc::string8_fast& expression );
    std::optional<std::nullptr_t> VolumeDown();
    std::optional<std::nullptr_t> VolumeMute();
    std::optional<std::nullptr_t> VolumeUp();

public:
    std::optional<bool> get_AlwaysOnTop();
    std::optional<pfc::string8_fast> get_ComponentPath();
    std::optional<bool> get_CursorFollowPlayback();
    std::optional<pfc::string8_fast> get_FoobarPath();
    std::optional<bool> get_IsPaused();
    std::optional<bool> get_IsPlaying();
    std::optional<bool> get_PlaybackFollowCursor();
    std::optional<double> get_PlaybackLength();
    std::optional<double> get_PlaybackTime();
    std::optional<pfc::string8_fast> get_ProfilePath();
    std::optional<uint32_t> get_ReplaygainMode();
    std::optional<bool> get_StopAfterCurrent();
    std::optional<float> get_Volume();
    std::optional<std::nullptr_t> put_AlwaysOnTop( bool p );
    std::optional<std::nullptr_t> put_CursorFollowPlayback( bool p );
    std::optional<std::nullptr_t> put_PlaybackFollowCursor( bool p );
    std::optional<std::nullptr_t> put_PlaybackTime( double time );
    std::optional<std::nullptr_t> put_ReplaygainMode( uint32_t p );
    std::optional<std::nullptr_t> put_StopAfterCurrent( bool p );
    std::optional<std::nullptr_t> put_Volume( float value );

private:
    JsFbUtils( JSContext* cx );

private:
    JSContext * pJsCtx_ = nullptr;
};

}
