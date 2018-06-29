#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

/*

class FbUtils : public IDispatchImpl3<IFbUtils>
{
protected:
FbUtils();
virtual ~FbUtils();

public:
STDMETHODIMP AcquireUiSelectionHolder(IFbUiSelectionHolder** outHolder);
STDMETHODIMP AddDirectory();
STDMETHODIMP AddFiles();
STDMETHODIMP CheckClipboardContents(UINT window_id, VARIANT_BOOL* outSuccess);
STDMETHODIMP ClearPlaylist();
STDMETHODIMP CopyHandleListToClipboard(IFbMetadbHandleList* handles, VARIANT_BOOL* outSuccess);
STDMETHODIMP CreateContextMenuManager(IContextMenuManager** pp);
STDMETHODIMP CreateHandleList(IFbMetadbHandleList** pp);
STDMETHODIMP CreateMainMenuManager(IMainMenuManager** pp);
STDMETHODIMP CreateProfiler(BSTR name, IFbProfiler** pp);
STDMETHODIMP DoDragDrop(IFbMetadbHandleList* handles, UINT okEffects, UINT* p);
STDMETHODIMP Exit();
STDMETHODIMP GetClipboardContents(UINT window_id, IFbMetadbHandleList** pp);
STDMETHODIMP GetDSPPresets(BSTR* p);
STDMETHODIMP GetFocusItem(VARIANT_BOOL force, IFbMetadbHandle** pp);
STDMETHODIMP GetLibraryItems(IFbMetadbHandleList** outItems);
STDMETHODIMP GetLibraryRelativePath(IFbMetadbHandle* handle, BSTR* p);
STDMETHODIMP GetNowPlaying(IFbMetadbHandle** pp);
STDMETHODIMP GetOutputDevices(BSTR* p);
STDMETHODIMP GetQueryItems(IFbMetadbHandleList* handles, BSTR query, IFbMetadbHandleList** pp);
STDMETHODIMP GetSelection(IFbMetadbHandle** pp);
STDMETHODIMP GetSelections(UINT flags, IFbMetadbHandleList** pp);
STDMETHODIMP GetSelectionType(UINT* p);
STDMETHODIMP IsLibraryEnabled(VARIANT_BOOL* p);
STDMETHODIMP IsMainMenuCommandChecked(BSTR command, VARIANT_BOOL* p);
STDMETHODIMP IsMetadbInMediaLibrary(IFbMetadbHandle* handle, VARIANT_BOOL* p);
STDMETHODIMP LoadPlaylist();
STDMETHODIMP Next();
STDMETHODIMP Pause();
STDMETHODIMP Play();
STDMETHODIMP PlayOrPause();
STDMETHODIMP Prev();
STDMETHODIMP Random();
STDMETHODIMP RunContextCommand(BSTR command, UINT flags, VARIANT_BOOL* p);
STDMETHODIMP RunContextCommandWithMetadb(BSTR command, VARIANT handle, UINT flags, VARIANT_BOOL* p);
STDMETHODIMP RunMainMenuCommand(BSTR command, VARIANT_BOOL* p);
STDMETHODIMP SaveIndex();
STDMETHODIMP SavePlaylist();
STDMETHODIMP SetDSPPreset(UINT idx);
STDMETHODIMP SetOutputDevice(BSTR output, BSTR device);
STDMETHODIMP ShowConsole();
STDMETHODIMP ShowLibrarySearchUI(BSTR query);
STDMETHODIMP ShowPopupMessage(BSTR msg, BSTR title);
STDMETHODIMP ShowPreferences();
STDMETHODIMP Stop();
STDMETHODIMP TitleFormat(BSTR expression, IFbTitleFormat** pp);
STDMETHODIMP VolumeDown();
STDMETHODIMP VolumeMute();
STDMETHODIMP VolumeUp();
STDMETHODIMP get_AlwaysOnTop(VARIANT_BOOL* p);
STDMETHODIMP get_ComponentPath(BSTR* pp);
STDMETHODIMP get_CursorFollowPlayback(VARIANT_BOOL* p);
STDMETHODIMP get_FoobarPath(BSTR* pp);
STDMETHODIMP get_IsPaused(VARIANT_BOOL* p);
STDMETHODIMP get_IsPlaying(VARIANT_BOOL* p);
STDMETHODIMP get_PlaybackFollowCursor(VARIANT_BOOL* p);
STDMETHODIMP get_PlaybackLength(double* p);
STDMETHODIMP get_PlaybackTime(double* p);
STDMETHODIMP get_ProfilePath(BSTR* pp);
STDMETHODIMP get_ReplaygainMode(UINT* p);
STDMETHODIMP get_StopAfterCurrent(VARIANT_BOOL* p);
STDMETHODIMP get_Volume(float* p);
STDMETHODIMP put_AlwaysOnTop(VARIANT_BOOL p);
STDMETHODIMP put_CursorFollowPlayback(VARIANT_BOOL p);
STDMETHODIMP put_PlaybackFollowCursor(VARIANT_BOOL p);
STDMETHODIMP put_PlaybackTime(double time);
STDMETHODIMP put_ReplaygainMode(UINT p);
STDMETHODIMP put_StopAfterCurrent(VARIANT_BOOL p);
STDMETHODIMP put_Volume(float value);
};


*/
}
