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
	// Methods
	STDMETHOD(InsertPlaylistItems)(UINT playlistIndex, UINT base, __interface IFbMetadbHandleList * handles, [defaultvalue(0)] VARIANT_BOOL select, [out,retval] UINT * outSize);
	STDMETHOD(InsertPlaylistItemsFilter)(UINT playlistIndex, UINT base, __interface IFbMetadbHandleList * handles, [defaultvalue(0)] VARIANT_BOOL select, [out,retval] UINT * outSize);
	STDMETHOD(MovePlaylistSelection)(UINT playlistIndex, int delta);
	STDMETHOD(RemovePlaylistSelection)(UINT playlistIndex, [defaultvalue(0)] VARIANT_BOOL crop);
	STDMETHOD(GetPlaylistSelectedItems)(UINT playlistIndex, [out,retval] __interface IFbMetadbHandleList ** outItems);
	STDMETHOD(GetPlaylistItems)(UINT playlistIndex, [out,retval] __interface IFbMetadbHandleList ** outItems);
	STDMETHOD(SetPlaylistSelectionSingle)(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state);
	STDMETHOD(SetPlaylistSelection)(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state);
	STDMETHOD(ClearPlaylistSelection)(UINT playlistIndex);
	STDMETHOD(UndoBackup)(UINT playlistIndex);
	STDMETHOD(UndoRestore)(UINT playlistIndex);
	STDMETHOD(GetPlaylistFocusItemIndex)(UINT playlistIndex, [out,retval] INT * outPlaylistItemIndex);
	STDMETHOD(GetPlaylistFocusItemHandle)(VARIANT_BOOL force, [out,retval] IFbMetadbHandle ** outItem);
	STDMETHOD(SetPlaylistFocusItem)(UINT playlistIndex, UINT itemIndex);
	STDMETHOD(SetPlaylistFocusItemByHandle)(UINT playlistIndex, IFbMetadbHandle * item);
	STDMETHOD(GetPlaylistName)(UINT playlistIndex, [out,retval] BSTR * outName);
	STDMETHOD(CreatePlaylist)(UINT playlistIndex, BSTR name, [out,retval] UINT * outPlaylistIndex);
	STDMETHOD(RemovePlaylist)(UINT playlistIndex, [out,retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(MovePlaylist)(UINT from, UINT to, [out,retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(RenamePlaylist)(UINT playlistIndex, BSTR name, [out,retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(DuplicatePlaylist)(UINT from, BSTR name, [out,retval] UINT * outPlaylistIndex);
	STDMETHOD(EnsurePlaylistItemVisible)(UINT playlistIndex, UINT itemIndex);
	STDMETHOD(GetPlayingItemLocation)([out,retval] __interface IFbPlayingItemLocation ** outPlayingLocation);
	STDMETHOD(ExecutePlaylistDefaultAction)(UINT playlistIndex, UINT playlistItemIndex, [out,retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(IsPlaylistItemSelected)(UINT playlistIndex, UINT playlistItemIndex, [out,retval] UINT * outSeleted);
	STDMETHOD(SetActivePlaylistContext)();

	STDMETHOD(CreatePlaybackQueueItem)([out,retval] __interface IFbPlaybackQueueItem ** outPlaybackQueueItem);
	STDMETHOD(RemoveItemFromPlaybackQueue)(UINT index);
	STDMETHOD(RemoveItemsFromPlaybackQueue)(VARIANT affectedItems);
	STDMETHOD(AddPlaylistItemToPlaybackQueue)(UINT playlistIndex, UINT playlistItemIndex);
	STDMETHOD(AddItemToPlaybackQueue)(IFbMetadbHandle * handle);
	STDMETHOD(GetPlaybackQueueCount)([out,retval] UINT * outCount);
	STDMETHOD(GetPlaybackQueueContents)([out,retval] VARIANT * outContents);
	STDMETHOD(FindPlaybackQueueItemIndex)(IFbMetadbHandle * handle, UINT playlistIndex, UINT playlistItemIndex, [out,retval] INT * outIndex);
	STDMETHOD(FlushPlaybackQueue)();
	STDMETHOD(IsPlaybackQueueActive)([out,retval] VARIANT_BOOL * outIsActive);

	STDMETHOD(SortByFormat)(UINT playlistIndex, BSTR pattern, [defaultvalue(0)] VARIANT_BOOL selOnly, [out,retval] VARIANT_BOOL * outSuccess);
	STDMETHOD(SortByFormatV2)(UINT playlistIndex, BSTR pattern, [defaultvalue(1)] INT direction,  [out,retval] VARIANT_BOOL * outSuccess);

	// Properties
	[propget] STDMETHOD(PlaybackOrder)([out,retval] UINT * outOrder);
	[propput] STDMETHOD(PlaybackOrder)(UINT order);
	[propget] STDMETHOD(ActivePlaylist)([out,retval] UINT * outPlaylistIndex);
	[propput] STDMETHOD(ActivePlaylist)(UINT playlistIndex);
	[propget] STDMETHOD(PlayingPlaylist)([out,retval] UINT * outPlaylistIndex);
	[propput] STDMETHOD(PlayingPlaylist)(UINT playlistIndex);
	[propget] STDMETHOD(PlaylistCount)([out,retval] UINT * outCount);
	[propget] STDMETHOD(PlaylistItemCount)(UINT playlistIndex, [out,retval] UINT * outCount);
	[propget] STDMETHOD(PlaylistRecyclerManager)([out,retval] __interface IFbPlaylistRecyclerManager ** outRecyclerManager);
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
	// Methods
	STDMETHOD(Equals)(__interface IFbPlaybackQueueItem * item, [out,retval] VARIANT_BOOL * outEquals);

	// Properties
	[propget] STDMETHOD(_ptr)([out,retval] void ** pp);
	[propget] STDMETHOD(Handle)([out,retval] IFbMetadbHandle ** outHandle);
	[propput] STDMETHOD(Handle)(IFbMetadbHandle * handle);
	[propget] STDMETHOD(PlaylistIndex)([out,retval] UINT * outPlaylistIndex);
	[propput] STDMETHOD(PlaylistIndex)(UINT playlistIndex);
	[propget] STDMETHOD(PlaylistItemIndex)([out,retval] UINT * outItemIndex);
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
	[propget] STDMETHOD(IsValid)([out,retval] VARIANT_BOOL * outIsValid);
	[propget] STDMETHOD(PlaylistIndex)([out,retval] UINT * outPlaylistIndex);
	[propget] STDMETHOD(PlaylistItemIndex)([out,retval] UINT * outPlaylistItemIndex);
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
	[propget] STDMETHOD(Count)([out,retval] UINT * outCount);
	[propget] STDMETHOD(Name)(UINT index, [out,retval] BSTR * outName);
	[propget] STDMETHOD(Content)(UINT index, [out,retval] __interface IFbMetadbHandleList ** outContent);
	[propget] STDMETHOD(Id)(UINT index, UINT * outId);

	STDMETHOD(Purge)(VARIANT affectedItems);
	STDMETHOD(Restore)(UINT index);
	STDMETHOD(RestoreById)(UINT id);
	STDMETHOD(FindById)(UINT id, [out,retval] UINT * outId);
};
_COM_SMARTPTR_TYPEDEF(IFbPlaylistRecyclerManager, __uuidof(IFbPlaylistRecyclerManager));
