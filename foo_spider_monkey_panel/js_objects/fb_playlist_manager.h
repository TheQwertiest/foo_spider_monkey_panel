#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

/*


class FbPlaylistManager : public IDispatchImpl3<IFbPlaylistManager>
{
protected:
	FbPlaylistManager();

public:
	STDMETHODIMP AddItemToPlaybackQueue(IFbMetadbHandle* handle);
	STDMETHODIMP AddLocations(UINT playlistIndex, VARIANT locations, VARIANT_BOOL select);
	STDMETHODIMP AddPlaylistItemToPlaybackQueue(UINT playlistIndex, UINT playlistItemIndex);
	STDMETHODIMP ClearPlaylist(UINT playlistIndex);
	STDMETHODIMP ClearPlaylistSelection(UINT playlistIndex);
	STDMETHODIMP CreateAutoPlaylist(UINT playlistIndex, BSTR name, BSTR query, BSTR sort, UINT flags, int* outPlaylistIndex);
	STDMETHODIMP CreatePlaylist(UINT playlistIndex, BSTR name, int* outPlaylistIndex);
	STDMETHODIMP DuplicatePlaylist(UINT from, BSTR name, UINT* outPlaylistIndex);
	STDMETHODIMP EnsurePlaylistItemVisible(UINT playlistIndex, UINT playlistItemIndex);
	STDMETHODIMP ExecutePlaylistDefaultAction(UINT playlistIndex, UINT playlistItemIndex, VARIANT_BOOL* outSuccess);
	STDMETHODIMP FindOrCreatePlaylist(BSTR name, VARIANT_BOOL unlocked, int* outPlaylistIndex);
	STDMETHODIMP FindPlaybackQueueItemIndex(IFbMetadbHandle* handle, UINT playlistIndex, UINT playlistItemIndex, int* outIndex);
	STDMETHODIMP FindPlaylist(BSTR name, int* outPlaylistIndex);
	STDMETHODIMP FlushPlaybackQueue();
	STDMETHODIMP GetPlaybackQueueContents(VARIANT* outContents);
	STDMETHODIMP GetPlaybackQueueHandles(IFbMetadbHandleList** outItems);
	STDMETHODIMP GetPlayingItemLocation(IFbPlayingItemLocation** outPlayingLocation);
	STDMETHODIMP GetPlaylistFocusItemIndex(UINT playlistIndex, int* outPlaylistItemIndex);
	STDMETHODIMP GetPlaylistItems(UINT playlistIndex, IFbMetadbHandleList** outItems);
	STDMETHODIMP GetPlaylistName(UINT playlistIndex, BSTR* outName);
	STDMETHODIMP GetPlaylistSelectedItems(UINT playlistIndex, IFbMetadbHandleList** outItems);
	STDMETHODIMP InsertPlaylistItems(UINT playlistIndex, UINT base, IFbMetadbHandleList* handles, VARIANT_BOOL select);
	STDMETHODIMP InsertPlaylistItemsFilter(UINT playlistIndex, UINT base, IFbMetadbHandleList* handles, VARIANT_BOOL select);
	STDMETHODIMP IsAutoPlaylist(UINT playlistIndex, VARIANT_BOOL* p);
	STDMETHODIMP IsPlaylistItemSelected(UINT playlistIndex, UINT playlistItemIndex, VARIANT_BOOL* outSelected);
	STDMETHODIMP IsPlaylistLocked(UINT playlistIndex, VARIANT_BOOL* p);
	STDMETHODIMP MovePlaylist(UINT from, UINT to, VARIANT_BOOL* outSuccess);
	STDMETHODIMP MovePlaylistSelection(UINT playlistIndex, int delta, VARIANT_BOOL* outSuccess);
	STDMETHODIMP PlaylistItemCount(UINT playlistIndex, UINT* outCount);
	STDMETHODIMP RemoveItemFromPlaybackQueue(UINT index);
	STDMETHODIMP RemoveItemsFromPlaybackQueue(VARIANT affectedItems);
	STDMETHODIMP RemovePlaylist(UINT playlistIndex, VARIANT_BOOL* outSuccess);
	STDMETHODIMP RemovePlaylistSelection(UINT playlistIndex, VARIANT_BOOL crop);
	STDMETHODIMP RemovePlaylistSwitch(UINT playlistIndex, VARIANT_BOOL* outSuccess);
	STDMETHODIMP RenamePlaylist(UINT playlistIndex, BSTR name, VARIANT_BOOL* outSuccess);
	STDMETHODIMP SetActivePlaylistContext();
	STDMETHODIMP SetPlaylistFocusItem(UINT playlistIndex, UINT playlistItemIndex);
	STDMETHODIMP SetPlaylistFocusItemByHandle(UINT playlistIndex, IFbMetadbHandle* handle);
	STDMETHODIMP SetPlaylistSelection(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state);
	STDMETHODIMP SetPlaylistSelectionSingle(UINT playlistIndex, UINT playlistItemIndex, VARIANT_BOOL state);
	STDMETHODIMP ShowAutoPlaylistUI(UINT playlistIndex, VARIANT_BOOL* outSuccess);
	STDMETHODIMP SortByFormat(UINT playlistIndex, BSTR pattern, VARIANT_BOOL selOnly, VARIANT_BOOL* outSuccess);
	STDMETHODIMP SortByFormatV2(UINT playlistIndex, BSTR pattern, int direction, VARIANT_BOOL* outSuccess);
	STDMETHODIMP SortPlaylistsByName(int direction);
	STDMETHODIMP UndoBackup(UINT playlistIndex);
	STDMETHODIMP get_ActivePlaylist(int* outPlaylistIndex);
	STDMETHODIMP get_PlaybackOrder(UINT* outOrder);
	STDMETHODIMP get_PlayingPlaylist(int* outPlaylistIndex);
	STDMETHODIMP get_PlaylistCount(UINT* outCount);
	STDMETHODIMP get_PlaylistRecyclerManager(__interface IFbPlaylistRecyclerManager** outRecyclerManager);
	STDMETHODIMP put_ActivePlaylist(int playlistIndex);
	STDMETHODIMP put_PlaybackOrder(UINT order);
	STDMETHODIMP put_PlayingPlaylist(int playlistIndex);

private:
	IFbPlaylistRecyclerManagerPtr m_fbPlaylistRecyclerManager;
};

*/

}
