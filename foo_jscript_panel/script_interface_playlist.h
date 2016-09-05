#pragma once

#include <ObjBase.h>

//---
[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("84212840-c0c5-4625-8fc4-2cc20e4bbcc8")
]
__interface IFbPlaylistManager : IDispatch
{
	STDMETHOD(AddItemToPlaybackQueue)(IFbMetadbHandle * handle);
	STDMETHOD(AddLocations)(UINT playlistIndex, VARIANT locations, [defaultvalue(0)] VARIANT_BOOL select);
	STDMETHOD(AddPlaylistItemToPlaybackQueue)(UINT playlistIndex, UINT playlistItemIndex);
	STDMETHOD(ClearPlaylist)(UINT playlistIndex);
	STDMETHOD(ClearPlaylistSelection)(UINT playlistIndex);
	STDMETHOD(CreateAutoPlaylist)(UINT idx, BSTR name, BSTR query, [defaultvalue("")] BSTR sort, [defaultvalue(0)]UINT flags, [out, retval] UINT * p);
	STDMETHOD(CreatePlaybackQueueItem)([out, retval] __interface IFbPlaybackQueueItem ** outPlaybackQueueItem);
	STDMETHOD(CreatePlaylist)(UINT playlistIndex, BSTR name, [out, retval] UINT * outPlaylistIndex);
	STDMETHOD(DuplicatePlaylist)(UINT from, BSTR name, [out, retval] UINT * outPlaylistIndex);
	STDMETHOD(EnsurePlaylistItemVisible)(UINT playlistIndex, UINT itemIndex);
	STDMETHOD(ExecutePlaylistDefaultAction)(UINT playlistIndex, UINT playlistItemIndex, [out, retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(FindPlaybackQueueItemIndex)(IFbMetadbHandle * handle, UINT playlistIndex, UINT playlistItemIndex, [out, retval] INT * outIndex);
	STDMETHOD(FlushPlaybackQueue)();
	STDMETHOD(GetPlaybackQueueContents)([out, retval] VARIANT * outContents);
	STDMETHOD(GetPlaybackQueueCount)([out, retval] UINT * outCount);
	STDMETHOD(GetPlayingItemLocation)([out, retval] __interface IFbPlayingItemLocation ** outPlayingLocation);
	STDMETHOD(GetPlaylistFocusItemIndex)(UINT playlistIndex, [out, retval] INT * outPlaylistItemIndex);
	STDMETHOD(GetPlaylistItems)(UINT playlistIndex, [out, retval] IFbMetadbHandleList ** outItems);
	STDMETHOD(GetPlaylistName)(UINT playlistIndex, [out, retval] BSTR * outName);
	STDMETHOD(GetPlaylistSelectedItems)(UINT playlistIndex, [out, retval] IFbMetadbHandleList ** outItems);
	STDMETHOD(InsertPlaylistItems)(UINT playlistIndex, UINT base, IFbMetadbHandleList * handles, [defaultvalue(0)] VARIANT_BOOL select, [out, retval] UINT * outSize);
	STDMETHOD(InsertPlaylistItemsFilter)(UINT playlistIndex, UINT base, IFbMetadbHandleList * handles, [defaultvalue(0)] VARIANT_BOOL select, [out, retval] UINT * outSize);
	STDMETHOD(IsAutoPlaylist)(UINT idx, [out, retval] VARIANT_BOOL * p);
	STDMETHOD(IsPlaybackQueueActive)([out, retval] VARIANT_BOOL * outIsActive);
	STDMETHOD(IsPlaylistItemSelected)(UINT playlistIndex, UINT playlistItemIndex, [out, retval] UINT * outSelected);
	STDMETHOD(MovePlaylist)(UINT from, UINT to, [out, retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(MovePlaylistSelection)(UINT playlistIndex, int delta);
	STDMETHOD(RemoveItemFromPlaybackQueue)(UINT index);
	STDMETHOD(RemoveItemsFromPlaybackQueue)(VARIANT affectedItems);
	STDMETHOD(RemovePlaylist)(UINT playlistIndex, [out, retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(RemovePlaylistSelection)(UINT playlistIndex, [defaultvalue(0)] VARIANT_BOOL crop);
	STDMETHOD(RenamePlaylist)(UINT playlistIndex, BSTR name, [out, retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(SetActivePlaylistContext)();
	STDMETHOD(SetPlaylistFocusItem)(UINT playlistIndex, UINT itemIndex);
	STDMETHOD(SetPlaylistFocusItemByHandle)(UINT playlistIndex, IFbMetadbHandle * item);
	STDMETHOD(SetPlaylistSelection)(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state);
	STDMETHOD(SetPlaylistSelectionSingle)(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state);
	STDMETHOD(ShowAutoPlaylistUI)(UINT idx, [out, retval] VARIANT_BOOL * p);
	STDMETHOD(SortByFormat)(UINT playlistIndex, BSTR pattern, [defaultvalue(0)] VARIANT_BOOL selOnly, [out, retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(SortByFormatV2)(UINT playlistIndex, BSTR pattern, [defaultvalue(1)] INT direction, [out, retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(UndoBackup)(UINT playlistIndex);
	STDMETHOD(UndoRestore)(UINT playlistIndex);
	[propget] STDMETHOD(ActivePlaylist)([out, retval] UINT * outPlaylistIndex);
	[propget] STDMETHOD(PlaybackOrder)([out, retval] UINT * outOrder);
	[propget] STDMETHOD(PlayingPlaylist)([out, retval] UINT * outPlaylistIndex);
	[propget] STDMETHOD(PlaylistCount)([out, retval] UINT * outCount);
	[propget] STDMETHOD(PlaylistItemCount)(UINT playlistIndex, [out, retval] UINT * outCount);
	[propget] STDMETHOD(PlaylistRecyclerManager)([out, retval] __interface IFbPlaylistRecyclerManager ** outRecyclerManager);
	[propput] STDMETHOD(ActivePlaylist)(UINT playlistIndex);
	[propput] STDMETHOD(PlaybackOrder)(UINT order);
	[propput] STDMETHOD(PlayingPlaylist)(UINT playlistIndex);
};
_COM_SMARTPTR_TYPEDEF(IFbPlaylistManager, __uuidof(IFbPlaylistManager));

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("e6d4354c-9a79-4062-b4d7-714b13539500")
]
__interface IFbPlaybackQueueItem : IDisposable
{
	STDMETHOD(Equals)(__interface IFbPlaybackQueueItem * item, [out, retval] VARIANT_BOOL * outEquals);
	[propget] STDMETHOD(Handle)([out, retval] IFbMetadbHandle ** outHandle);
	[propget] STDMETHOD(PlaylistIndex)([out, retval] UINT * outPlaylistIndex);
	[propget] STDMETHOD(PlaylistItemIndex)([out, retval] UINT * outItemIndex);
	[propget] STDMETHOD(_ptr)([out, retval] void ** pp);
	[propput] STDMETHOD(Handle)(IFbMetadbHandle * handle);
	[propput] STDMETHOD(PlaylistIndex)(UINT playlistIndex);
	[propput] STDMETHOD(PlaylistItemIndex)(UINT itemIndex);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("0f54464f-0b86-4419-83c0-b6f612d85fb0")
]
__interface IFbPlayingItemLocation : IDispatch
{
	[propget] STDMETHOD(IsValid)([out, retval] VARIANT_BOOL * outIsValid);
	[propget] STDMETHOD(PlaylistIndex)([out, retval] UINT * outPlaylistIndex);
	[propget] STDMETHOD(PlaylistItemIndex)([out, retval] UINT * outPlaylistItemIndex);
};

[
	object,
	dual,
	pointer_default(unique),
	library_block,
	uuid("0bc36d7f-3fcb-4157-8b90-db1281423e81")
]
__interface IFbPlaylistRecyclerManager : IDispatch
{
	STDMETHOD(FindById)(UINT id, [out, retval] UINT * outId);
	STDMETHOD(Purge)(VARIANT affectedItems);
	STDMETHOD(Restore)(UINT index);
	STDMETHOD(RestoreById)(UINT id);
	[propget] STDMETHOD(Content)(UINT index, [out, retval] IFbMetadbHandleList ** outContent);
	[propget] STDMETHOD(Count)([out, retval] UINT * outCount);
	[propget] STDMETHOD(Id)(UINT index, UINT * outId);
	[propget] STDMETHOD(Name)(UINT index, [out, retval] BSTR * outName);
};
_COM_SMARTPTR_TYPEDEF(IFbPlaylistRecyclerManager, __uuidof(IFbPlaylistRecyclerManager));
