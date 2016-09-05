#pragma once

#include "script_interface_playlist.h"
#include "com_tools.h"

// NOTE: Do not use com_object_impl_t<> to initialize, use com_object_singleton_t<> instead.
class FbPlaylistManager : public IDispatchImpl3<IFbPlaylistManager>
{
private:
	IFbPlaylistRecyclerManagerPtr m_fbPlaylistRecyclerManager;

protected:
	FbPlaylistManager() : m_fbPlaylistRecyclerManager(NULL)
	{
	}

public:
	STDMETHODIMP AddItemToPlaybackQueue(IFbMetadbHandle * handle);
	STDMETHODIMP AddLocations(UINT playlistIndex, VARIANT locations, VARIANT_BOOL select);
	STDMETHODIMP AddPlaylistItemToPlaybackQueue(UINT playlistIndex, UINT playlistItemIndex);
	STDMETHODIMP ClearPlaylist(UINT playlistIndex);
	STDMETHODIMP ClearPlaylistSelection(UINT playlistIndex);
	STDMETHODIMP CreateAutoPlaylist(UINT idx, BSTR name, BSTR query, BSTR sort, UINT flags, UINT * p);
	STDMETHODIMP CreatePlaybackQueueItem(IFbPlaybackQueueItem ** outPlaybackQueueItem);
	STDMETHODIMP CreatePlaylist(UINT playlistIndex, BSTR name, UINT * outPlaylistIndex);
	STDMETHODIMP DuplicatePlaylist(UINT from, BSTR name, UINT * outPlaylistIndex);
	STDMETHODIMP EnsurePlaylistItemVisible(UINT playlistIndex, UINT itemIndex);
	STDMETHODIMP ExecutePlaylistDefaultAction(UINT playlistIndex, UINT playlistItemIndex, VARIANT_BOOL * outSuccess);
	STDMETHODIMP FindPlaybackQueueItemIndex(IFbMetadbHandle * handle, UINT playlistIndex, UINT playlistItemIndex, INT * outIndex);
	STDMETHODIMP FlushPlaybackQueue();
	STDMETHODIMP GetPlaybackQueueContents(VARIANT * outContents);
	STDMETHODIMP GetPlaybackQueueCount(UINT * outCount);
	STDMETHODIMP GetPlayingItemLocation(IFbPlayingItemLocation ** outPlayingLocation);
	STDMETHODIMP GetPlaylistFocusItemIndex(UINT playlistIndex, INT * outPlaylistItemIndex);
	STDMETHODIMP GetPlaylistItems(UINT playlistIndex, IFbMetadbHandleList ** outItems);
	STDMETHODIMP GetPlaylistName(UINT playlistIndex, BSTR * outName);
	STDMETHODIMP GetPlaylistSelectedItems(UINT playlistIndex, IFbMetadbHandleList ** outItems);
	STDMETHODIMP InsertPlaylistItems(UINT playlistIndex, UINT base, IFbMetadbHandleList * handles, VARIANT_BOOL select, UINT * outSize);
	STDMETHODIMP InsertPlaylistItemsFilter(UINT playlistIndex, UINT base, IFbMetadbHandleList * handles, VARIANT_BOOL select, UINT * outSize);
	STDMETHODIMP IsAutoPlaylist(UINT idx, VARIANT_BOOL * p);
	STDMETHODIMP IsPlaybackQueueActive(VARIANT_BOOL * outIsActive);
	STDMETHODIMP IsPlaylistItemSelected(UINT playlistIndex, UINT playlistItemIndex, UINT * outSelected);
	STDMETHODIMP MovePlaylist(UINT from, UINT to, VARIANT_BOOL * outSuccess);
	STDMETHODIMP MovePlaylistSelection(UINT playlistIndex, int delta);
	STDMETHODIMP RemoveItemFromPlaybackQueue(UINT index);
	STDMETHODIMP RemoveItemsFromPlaybackQueue(VARIANT affectedItems);
	STDMETHODIMP RemovePlaylist(UINT playlistIndex, VARIANT_BOOL * outSuccess);
	STDMETHODIMP RemovePlaylistSelection(UINT playlistIndex, VARIANT_BOOL crop);
	STDMETHODIMP RenamePlaylist(UINT playlistIndex, BSTR name, VARIANT_BOOL * outSuccess);
	STDMETHODIMP SetActivePlaylistContext();
	STDMETHODIMP SetPlaylistFocusItem(UINT playlistIndex, UINT itemIndex);
	STDMETHODIMP SetPlaylistFocusItemByHandle(UINT playlistIndex, IFbMetadbHandle * item);
	STDMETHODIMP SetPlaylistSelection(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state);
	STDMETHODIMP SetPlaylistSelectionSingle(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state);
	STDMETHODIMP ShowAutoPlaylistUI(UINT idx, VARIANT_BOOL * p);
	STDMETHODIMP SortByFormat(UINT playlistIndex, BSTR pattern, VARIANT_BOOL selOnly, VARIANT_BOOL * outSuccess);
	STDMETHODIMP SortByFormatV2(UINT playlistIndex, BSTR pattern, INT direction, VARIANT_BOOL * outSuccess);
	STDMETHODIMP UndoBackup(UINT playlistIndex);
	STDMETHODIMP UndoRestore(UINT playlistIndex);
	STDMETHODIMP get_ActivePlaylist(UINT * outPlaylistIndex);
	STDMETHODIMP get_PlaybackOrder(UINT * outOrder);
	STDMETHODIMP get_PlayingPlaylist(UINT * outPlaylistIndex);
	STDMETHODIMP get_PlaylistCount(UINT * outCount);
	STDMETHODIMP get_PlaylistItemCount(UINT playlistIndex, UINT * outCount);
	STDMETHODIMP get_PlaylistRecyclerManager(__interface IFbPlaylistRecyclerManager ** outRecyclerManager);
	STDMETHODIMP put_ActivePlaylist(UINT playlistIndex);
	STDMETHODIMP put_PlaybackOrder(UINT order);
	STDMETHODIMP put_PlayingPlaylist(UINT playlistIndex);
};

class FbPlaybackQueueItem : public IDisposableImpl4<IFbPlaybackQueueItem>
{
protected:
	t_playback_queue_item m_playback_queue_item;

	FbPlaybackQueueItem() {}
	FbPlaybackQueueItem(const t_playback_queue_item & playbackQueueItem);
	virtual ~FbPlaybackQueueItem();
	virtual void FinalRelease();

public:
	STDMETHODIMP Equals(IFbPlaybackQueueItem * item, VARIANT_BOOL * outEquals);
	STDMETHODIMP get_Handle(IFbMetadbHandle ** outHandle);
	STDMETHODIMP get_PlaylistIndex(UINT * outPlaylistIndex);
	STDMETHODIMP get_PlaylistItemIndex(UINT * outItemIndex);
	STDMETHODIMP get__ptr(void ** pp);
	STDMETHODIMP put_Handle(IFbMetadbHandle * handle);
	STDMETHODIMP put_PlaylistIndex(UINT playlistIndex);
	STDMETHODIMP put_PlaylistItemIndex(UINT itemIndex);
};

class FbPlayingItemLocation : public IDispatchImpl3<IFbPlayingItemLocation>
{
protected:
	bool m_isValid;
	t_size m_playlistIndex;
	t_size m_itemIndex;

	FbPlayingItemLocation(bool isValid, t_size playlistIndex, t_size itemInex)
		: m_isValid(isValid), m_playlistIndex(playlistIndex), m_itemIndex(itemInex)
	{
	}

public:
	STDMETHODIMP get_IsValid(VARIANT_BOOL * outIsValid);
	STDMETHODIMP get_PlaylistIndex(UINT * outPlaylistIndex);
	STDMETHODIMP get_PlaylistItemIndex(UINT * outPlaylistItemIndex);
};

class FbPlaylistRecyclerManager : public IDispatchImpl3<IFbPlaylistRecyclerManager>
{
public:
	STDMETHODIMP FindById(UINT id, UINT * outId);
	STDMETHODIMP Purge(VARIANT affectedItems);
	STDMETHODIMP Restore(UINT index);
	STDMETHODIMP RestoreById(UINT id);
	STDMETHODIMP get_Content(UINT index, IFbMetadbHandleList ** outContent);
	STDMETHODIMP get_Count(UINT * outCount);
	STDMETHODIMP get_Id(UINT index, UINT * outId);
	STDMETHODIMP get_Name(UINT index, BSTR * outName);
};
