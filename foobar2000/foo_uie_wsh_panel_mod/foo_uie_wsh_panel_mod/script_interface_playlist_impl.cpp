#include "stdafx.h"
#include "script_interface_impl.h"
#include "script_interface_playlist_impl.h"
#include "helpers.h"
#include "com_array.h"

STDMETHODIMP FbPlaylistManagerTemplate::CreateAutoPlaylist(UINT idx, BSTR name, BSTR query, BSTR sort, UINT flags, UINT * p)
{
	TRACK_FUNCTION();

	if (!name || !query) return E_INVALIDARG;
	if (!p) return E_POINTER;

	UINT pos = 0;
	HRESULT hr = CreatePlaylist(idx, name, &pos);

	if (FAILED(hr)) return hr;

	pfc::stringcvt::string_utf8_from_wide wquery(query);
	pfc::stringcvt::string_utf8_from_wide wsort(sort);

	try
	{
		*p = pos;
		static_api_ptr_t<autoplaylist_manager>()->add_client_simple(wquery, wsort, pos, flags);
	}
	catch (...)
	{
		*p = pfc_infinite;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistManagerTemplate::CreatePlaylist(UINT playlistIndex, BSTR name, UINT * outPlaylistIndex)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;
	if (!outPlaylistIndex) return E_POINTER;

	if (*name)
	{
		pfc::stringcvt::string_utf8_from_wide uname(name);

		*outPlaylistIndex = static_api_ptr_t<playlist_manager>()->create_playlist(uname, uname.length(), playlistIndex);
	}
	else
	{
		*outPlaylistIndex = static_api_ptr_t<playlist_manager>()->create_playlist_autoname(playlistIndex);
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistManagerTemplate::GetPlaylistFocusItemHandle(VARIANT_BOOL force, IFbMetadbHandle ** outItem)
{
	TRACK_FUNCTION();

	if (!outItem) return E_POINTER;

	metadb_handle_ptr metadb;

	try
	{
		static_api_ptr_t<playlist_manager>()->activeplaylist_get_focus_item_handle(metadb);
		if (force && metadb.is_empty())
		{
			static_api_ptr_t<playlist_manager>()->activeplaylist_get_item_handle(metadb, 0);
		}
		if (metadb.is_empty())
		{
			(*outItem) = NULL;
			return S_OK;
		}
	}
	catch (std::exception &) {}

	(*outItem) = new com_object_impl_t<FbMetadbHandle>(metadb);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::CreateAutoPlaylist(UINT idx, BSTR name, BSTR query, BSTR sort, UINT flags, UINT * p)
{
	return FbPlaylistManagerTemplate::CreateAutoPlaylist(idx, name, query, sort, flags, p);
}

STDMETHODIMP FbPlaylistManager::CreatePlaylist(UINT playlistIndex, BSTR name, UINT * outPlaylistIndex)
{
	return FbPlaylistManagerTemplate::CreatePlaylist(playlistIndex, name, outPlaylistIndex);
}

STDMETHODIMP FbPlaylistManager::GetPlaylistFocusItemHandle(VARIANT_BOOL force, IFbMetadbHandle ** outItem)
{
	return FbPlaylistManagerTemplate::GetPlaylistFocusItemHandle(force, outItem);
}

//---

STDMETHODIMP FbPlaylistManager::GetQueryItems(IFbMetadbHandleList * items, BSTR query, IFbMetadbHandleList ** pp)
{
	TRACK_FUNCTION();

	if (!pp)return E_POINTER;
	if (!query)return E_INVALIDARG;

	metadb_handle_list *srclist_ptr, dst_list;

	items->get__ptr((void **)&srclist_ptr);

	dst_list = *srclist_ptr;
	pfc::stringcvt::string_utf8_from_wide query8(query);

	try
	{
		pfc::array_t<bool> mask;
		mask.set_size(dst_list.get_size());
		search_filter::ptr ptr = static_api_ptr_t<search_filter_manager>()->create(query8);
		ptr->test_multi(dst_list, mask.get_ptr());
		dst_list.filter_mask(mask.get_ptr());
	}
	catch (...)
	{
		return E_FAIL;
	}

	(*pp) = new com_object_impl_t<FbMetadbHandleList>(dst_list);

	return S_OK;
}

STDMETHODIMP FbPlaylistManager::IsAutoPlaylist(UINT idx, VARIANT_BOOL * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	try
	{
		*p = TO_VARIANT_BOOL(static_api_ptr_t<autoplaylist_manager>()->is_client_present(idx));
	}
	catch (...)
	{
		*p = VARIANT_FALSE;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistManager::ClearPlaylist()
{
	TRACK_FUNCTION();

	standard_commands::main_clear_playlist();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlaybackOrder(UINT * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	(*p) = static_api_ptr_t<playlist_manager>()->playback_order_get_active();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::put_PlaybackOrder(UINT p)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->playback_order_set_active(p);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::InsertPlaylistItems(UINT playlistIndex, UINT base, __interface IFbMetadbHandleList * handles, VARIANT_BOOL select, UINT * outSize)
{
	TRACK_FUNCTION();
	
	if (!outSize) return E_POINTER;
	if (!handles) return E_INVALIDARG;
	
	metadb_handle_list * metadbHandles = NULL;
	handles->get__ptr((void**)&metadbHandles);
	if (!metadbHandles) return E_INVALIDARG;

	bit_array_val selection(select == VARIANT_TRUE);
	(*outSize) = static_api_ptr_t<playlist_manager>()->playlist_insert_items(playlistIndex, base, *metadbHandles, selection);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::InsertPlaylistItemsFilter(UINT playlistIndex, UINT base, __interface IFbMetadbHandleList * handles, VARIANT_BOOL select, UINT * outSize)
{
	TRACK_FUNCTION();

	if (!outSize) return E_POINTER;
	if (!handles) return E_INVALIDARG;

	metadb_handle_list * metadbHandles = NULL;
	handles->get__ptr((void**)&metadbHandles);
	if (!metadbHandles) return E_INVALIDARG;

	(*outSize) = static_api_ptr_t<playlist_manager>()->playlist_insert_items_filter(playlistIndex, base, *metadbHandles, select == VARIANT_TRUE);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaylistSelectedItems(UINT playlistIndex, __interface IFbMetadbHandleList ** outItems)
{
	TRACK_FUNCTION();

	if (!outItems) return E_POINTER;

	metadb_handle_list items;
	static_api_ptr_t<playlist_manager>()->playlist_get_selected_items(playlistIndex, items);
	(*outItems) = new com_object_impl_t<FbMetadbHandleList>(items);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaylistItems(UINT playlistIndex, IFbMetadbHandleList ** outItems)
{
	TRACK_FUNCTION();

	if (!outItems) return E_POINTER;

	metadb_handle_list items;
	static_api_ptr_t<playlist_manager>()->playlist_get_all_items(playlistIndex, items);
	(*outItems) = new com_object_impl_t<FbMetadbHandleList>(items);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetPlaylistSelectionSingle(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->playlist_set_selection_single(playlistIndex, itemIndex, state == VARIANT_TRUE);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetPlaylistSelection(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state)
{
	TRACK_FUNCTION();

	unsigned bitArrayCount;
	bool empty;
	static_api_ptr_t<playlist_manager> plm;
	bit_array_bittable affected;
	bitArrayCount = plm->playlist_get_item_count(playlistIndex);
	if (!helpers::com_array_to_bitarray::convert(affectedItems, bitArrayCount, affected, empty)) return E_INVALIDARG;
	if (empty) return S_OK;

	bit_array_val status(state == VARIANT_TRUE);
	static_api_ptr_t<playlist_manager>()->playlist_set_selection(playlistIndex, affected, status);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::ClearPlaylistSelection(UINT playlistIndex)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->playlist_clear_selection(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::UndoBackup(UINT playlistIndex)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->playlist_undo_backup(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::UndoRestore(UINT playlistIndex)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->playlist_undo_restore(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaylistFocusItemIndex(UINT playlistIndex, INT * outPlaylistItemIndex)
{
	TRACK_FUNCTION();

	if (!outPlaylistItemIndex) return E_POINTER;
	(*outPlaylistItemIndex) = static_api_ptr_t<playlist_manager>()->playlist_get_focus_item(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetPlaylistFocusItem(UINT playlistIndex, UINT itemIndex)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->playlist_set_focus_item(playlistIndex, itemIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetPlaylistFocusItemByHandle(UINT playlistIndex, IFbMetadbHandle * item)
{
	TRACK_FUNCTION();

	if (!item) return E_INVALIDARG;

	metadb_handle * ptr = NULL;
	item->get__ptr((void**)&ptr);

	static_api_ptr_t<playlist_manager>()->playlist_set_focus_by_handle(playlistIndex, ptr);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaylistName(UINT playlistIndex, BSTR * outName)
{
	TRACK_FUNCTION();

	if (!outName) return E_POINTER;

	pfc::string8_fast temp;

	static_api_ptr_t<playlist_manager>()->playlist_get_name(playlistIndex, temp);
	*outName = SysAllocString(pfc::stringcvt::string_wide_from_utf8(temp));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RemovePlaylist(UINT playlistIndex, VARIANT_BOOL * outSuccess)
{
	TRACK_FUNCTION();

	if (!outSuccess) return E_POINTER;

	*outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->remove_playlist(playlistIndex));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::MovePlaylist(UINT from, UINT to, VARIANT_BOOL * outSuccess)
{
	TRACK_FUNCTION();

	if (!outSuccess) return E_POINTER;

	static_api_ptr_t<playlist_manager> pm;
	order_helper order(pm->get_playlist_count());

	if ((from >= order.get_count()) || (to >= order.get_count()))
	{
		*outSuccess = VARIANT_FALSE;
		return S_OK;
	}

	int inc = (from < to) ? 1 : -1;

	for (t_size i = from; i != to; i += inc)
	{
		order[i] = order[i + inc];
	}

	order[to] = from;

	*outSuccess = TO_VARIANT_BOOL(pm->reorder(order.get_ptr(), order.get_count()));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RenamePlaylist(UINT playlistIndex, BSTR name, VARIANT_BOOL * outSuccess)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;
	if (!outSuccess) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide uname(name);

	*outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_rename(playlistIndex, uname, uname.length()));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::DuplicatePlaylist(UINT from, BSTR name, UINT * outPlaylistIndex)
{
	TRACK_FUNCTION();

	if (!outPlaylistIndex) return E_POINTER;

	static_api_ptr_t<playlist_manager_v4> manager;
	metadb_handle_list contents;
	pfc::string8_fast name_utf8;

	if (from >= manager->get_playlist_count()) return E_INVALIDARG;

	manager->playlist_get_all_items(from, contents);

	if (!name || !*name)
		// If no name specified, create a playlistIndex which will have the same name
		manager->playlist_get_name(from, name_utf8);
	else
		name_utf8 = pfc::stringcvt::string_utf8_from_wide(name);

	stream_reader_dummy dummy_reader;
	abort_callback_dummy dummy_callback;

	t_size idx = manager->create_playlist_ex(name_utf8.get_ptr(), name_utf8.get_length(), from + 1, contents, &dummy_reader, dummy_callback);
	*outPlaylistIndex = idx;
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_ActivePlaylist(UINT * outPlaylistIndex)
{
	TRACK_FUNCTION();

	if (!outPlaylistIndex) return E_POINTER;

	*outPlaylistIndex = static_api_ptr_t<playlist_manager>()->get_active_playlist();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::put_ActivePlaylist(UINT playlistIndex)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager> pm;
	t_size index = (playlistIndex < pm->get_playlist_count()) ? playlistIndex : pfc::infinite_size;

	pm->set_active_playlist(index);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlayingPlaylist(UINT * outPlaylistIndex)
{
	TRACK_FUNCTION();

	if (!outPlaylistIndex) return E_POINTER;

	(*outPlaylistIndex) = static_api_ptr_t<playlist_manager>()->get_playing_playlist();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::put_PlayingPlaylist(UINT playlistIndex)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager> pm;
	t_size index = (playlistIndex < pm->get_playlist_count()) ? playlistIndex : pfc::infinite_size;

	pm->set_playing_playlist(index);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlaylistCount(UINT * outCount)
{
	TRACK_FUNCTION();

	if (!outCount) return E_POINTER;

	*outCount = static_api_ptr_t<playlist_manager>()->get_playlist_count();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlaylistItemCount(UINT playlistIndex, UINT * outCount)
{
	TRACK_FUNCTION();

	if (!outCount) return E_POINTER;

	*outCount = static_api_ptr_t<playlist_manager>()->playlist_get_item_count(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::CreatePlaybackQueueItem(IFbPlaybackQueueItem ** outPlaybackQueueItem)
{
	TRACK_FUNCTION();

	if (!outPlaybackQueueItem) return E_POINTER;

	(*outPlaybackQueueItem) = new com_object_impl_t<FbPlaybackQueueItem>();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RemoveItemFromPlaybackQueue(UINT index)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->queue_remove_mask(bit_array_one(index));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RemoveItemsFromPlaybackQueue(VARIANT affectedItems)
{
	TRACK_FUNCTION();

	unsigned bitArrayCount;
	bool empty;
	static_api_ptr_t<playlist_manager> plm;
	bit_array_bittable affected;
	bitArrayCount = plm->queue_get_count();
	if (!helpers::com_array_to_bitarray::convert(affectedItems, bitArrayCount, affected, empty)) return E_INVALIDARG;
	if (empty) return S_OK;

	plm->queue_remove_mask(affected);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::AddPlaylistItemToPlaybackQueue(UINT playlistIndex, UINT playlistItemIndex)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->queue_add_item_playlist(playlistIndex, playlistItemIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::AddItemToPlaybackQueue(IFbMetadbHandle * handle)
{
	TRACK_FUNCTION();

	metadb_handle * ptrHandle = NULL;
	handle->get__ptr((void **)&ptrHandle);
	if (!ptrHandle) return E_INVALIDARG;

	static_api_ptr_t<playlist_manager>()->queue_add_item(ptrHandle);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaybackQueueCount(UINT * outCount)
{
	TRACK_FUNCTION();

	if (!outCount) return E_POINTER;

	(*outCount) = static_api_ptr_t<playlist_manager>()->queue_get_count();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaybackQueueContents(VARIANT * outContents)
{
	TRACK_FUNCTION();

	if (!outContents) return E_POINTER;

	pfc::list_t<t_playback_queue_item> contents;
	helpers::com_array_writer<> arrayWriter;

	static_api_ptr_t<playlist_manager>()->queue_get_contents(contents);
	
	if (!arrayWriter.create(contents.get_count()))
	{
		return E_OUTOFMEMORY;
	}

	for (t_size i = 0; i < contents.get_count(); ++i)
	{
		_variant_t var;
		var.vt = VT_DISPATCH;
		var.pdispVal = new com_object_impl_t<FbPlaybackQueueItem>(contents[i]);

		if (FAILED(arrayWriter.put(i, var)))
		{
			// deep destroy
			arrayWriter.reset();
			return E_OUTOFMEMORY;
		}
	}

	outContents->vt = VT_ARRAY | VT_VARIANT;
	outContents->parray = arrayWriter.get_ptr();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::FindPlaybackQueueItemIndex(IFbMetadbHandle * handle, UINT playlistIndex, UINT playlistItemIndex, INT * outIndex)
{
	TRACK_FUNCTION();

	if (!outIndex) return E_POINTER;
	if (!handle) return E_INVALIDARG;

	metadb_handle * ptrHandle = NULL;
	handle->get__ptr((void **)&ptrHandle);

	t_playback_queue_item item;
	item.m_handle = ptrHandle;
	item.m_playlist = playlistIndex;
	item.m_item = playlistItemIndex;
	(*outIndex) = static_api_ptr_t<playlist_manager>()->queue_find_index(item);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::FlushPlaybackQueue()
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->queue_flush();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::IsPlaybackQueueActive(VARIANT_BOOL * outIsActive)
{
	TRACK_FUNCTION();

	if (!outIsActive) return E_POINTER;

	(*outIsActive) = static_api_ptr_t<playlist_manager>()->queue_is_active();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SortByFormat(UINT playlistIndex, BSTR pattern, VARIANT_BOOL selOnly, VARIANT_BOOL * outSuccess)
{
	TRACK_FUNCTION();

	if (!pattern) return E_INVALIDARG;
	if (!outSuccess) return E_POINTER;

	bool sel_only = (selOnly == VARIANT_TRUE);
	pfc::stringcvt::string_utf8_from_wide string_conv;
	const char * pattern_ptr = NULL;

	if (*pattern) {
		string_conv.convert(pattern);
		pattern_ptr = string_conv.get_ptr();
	}


	*outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_sort_by_format(playlistIndex, pattern_ptr, sel_only));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SortByFormatV2(UINT playlistIndex, BSTR pattern, INT direction, VARIANT_BOOL * outSuccess)
{
	TRACK_FUNCTION();

	if (!pattern) return E_INVALIDARG;
	if (!outSuccess) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide spec(pattern);
	service_ptr_t<titleformat_object> script;
	metadb_handle_list metadb_handles;
	pfc::array_t<size_t> order;

	if (static_api_ptr_t<titleformat_compiler>()->compile(script, spec)) {
		static_api_ptr_t<playlist_manager> api;

		// Get metadb_handle_list for playlist specified.
		api->playlist_get_all_items(playlistIndex, metadb_handles);
		order.set_count(metadb_handles.get_count());
		// Reorder metadb handles
		metadb_handle_list_helper::sort_by_format_get_order(metadb_handles, order.get_ptr(), script, NULL, direction);
		// Reorder the playlist
		*outSuccess = TO_VARIANT_BOOL(api->playlist_reorder_items(playlistIndex, order.get_ptr(), order.get_count()));
	} else {
		*outSuccess = VARIANT_FALSE;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistManager::EnsurePlaylistItemVisible(UINT playlistIndex, UINT itemIndex)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->playlist_ensure_visible(playlistIndex, itemIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlayingItemLocation(IFbPlayingItemLocation ** outPlayingLocation)
{
	TRACK_FUNCTION();

	if (!outPlayingLocation) return E_POINTER;

	t_size playlistIndex = -1;
	t_size itemIndex = -1;
	bool isValid = static_api_ptr_t<playlist_manager>()->get_playing_item_location(&playlistIndex, &itemIndex);
	(*outPlayingLocation) = new com_object_impl_t<FbPlayingItemLocation>(isValid, playlistIndex, itemIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::ExecutePlaylistDefaultAction(UINT playlistIndex, UINT playlistItemIndex, VARIANT_BOOL * outSuccess)
{
	TRACK_FUNCTION();

	if (!outSuccess) return E_POINTER;

	(*outSuccess) = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_execute_default_action(playlistIndex, playlistItemIndex));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::IsPlaylistItemSelected(UINT playlistIndex, UINT playlistItemIndex, UINT * outSeleted)
{
	TRACK_FUNCTION();

	if (!outSeleted) return E_POINTER;

	(*outSeleted) = TO_VARIANT_BOOL(
		static_api_ptr_t<playlist_manager>()->playlist_is_item_selected(playlistIndex, playlistItemIndex));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::MovePlaylistSelection(UINT playlistIndex, int delta)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->playlist_move_selection(playlistIndex, delta);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RemovePlaylistSelection(UINT playlistIndex, VARIANT_BOOL crop)
{
	TRACK_FUNCTION();

	static_api_ptr_t<playlist_manager>()->playlist_remove_selection(playlistIndex, crop == VARIANT_TRUE);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetActivePlaylistContext()
{
	TRACK_FUNCTION();

	static_api_ptr_t<ui_edit_context_manager>()->set_context_active_playlist();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlaylistRecyclerManager(__interface IFbPlaylistRecyclerManager ** outRecyclerManagerManager)
{
	TRACK_FUNCTION();
	
	try
	{
		if (!m_fbPlaylistRecyclerManager)
		{
			m_fbPlaylistRecyclerManager.Attach(new com_object_impl_t<FbPlaylistRecyclerManager>());
		}

		(*outRecyclerManagerManager) = m_fbPlaylistRecyclerManager;
		(*outRecyclerManagerManager)->AddRef();
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}

FbPlaybackQueueItem::FbPlaybackQueueItem(const t_playback_queue_item & playbackQueueItem)
{
	m_playback_queue_item.m_handle = playbackQueueItem.m_handle;
	m_playback_queue_item.m_playlist = playbackQueueItem.m_playlist;
	m_playback_queue_item.m_item = playbackQueueItem.m_item;
}

FbPlaybackQueueItem::~FbPlaybackQueueItem()
{

}

void FbPlaybackQueueItem::FinalRelease()
{
	m_playback_queue_item.m_handle.release();
	m_playback_queue_item.m_playlist = 0;
	m_playback_queue_item.m_item = 0;
}

STDMETHODIMP FbPlaybackQueueItem::Equals(IFbPlaybackQueueItem * item, VARIANT_BOOL * outEquals)
{
	TRACK_FUNCTION();

	if (!item) return E_INVALIDARG;

	t_playback_queue_item * ptrQueueItem = NULL;
	item->get__ptr((void **)&ptrQueueItem);
	if (!ptrQueueItem) return E_INVALIDARG;

	(*outEquals) = TO_VARIANT_BOOL(m_playback_queue_item == *ptrQueueItem);
	return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::get__ptr(void ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	(*pp) = &m_playback_queue_item;
	return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::get_Handle(IFbMetadbHandle ** outHandle)
{
	TRACK_FUNCTION();

	if (!outHandle) return E_POINTER;

	(*outHandle) = new com_object_impl_t<FbMetadbHandle>(m_playback_queue_item.m_handle);
	return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::put_Handle(IFbMetadbHandle * handle)
{
	TRACK_FUNCTION();

	if (!handle) return E_INVALIDARG;
	metadb_handle * ptrHandle = NULL;
	handle->get__ptr((void **)&ptrHandle);

	m_playback_queue_item.m_handle = ptrHandle;
	return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::get_PlaylistIndex(UINT * outPlaylistIndex)
{
	TRACK_FUNCTION();

	if (!outPlaylistIndex) return E_POINTER;

	(*outPlaylistIndex) = m_playback_queue_item.m_playlist;
	return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::put_PlaylistIndex(UINT playlistIndex)
{
	TRACK_FUNCTION();

	m_playback_queue_item.m_playlist = playlistIndex;
	return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::get_PlaylistItemIndex(UINT * outItemIndex)
{
	TRACK_FUNCTION();

	if (!outItemIndex) return E_POINTER;

	(*outItemIndex) = m_playback_queue_item.m_item;
	return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::put_PlaylistItemIndex(UINT itemIndex)
{
	TRACK_FUNCTION();

	m_playback_queue_item.m_item = itemIndex;
	return S_OK;
}

STDMETHODIMP FbPlayingItemLocation::get_IsValid(VARIANT_BOOL * outIsValid)
{
	TRACK_FUNCTION();

	if (!outIsValid) return E_POINTER;
	(*outIsValid) = TO_VARIANT_BOOL(m_isValid);
	return S_OK;
}

STDMETHODIMP FbPlayingItemLocation::get_PlaylistIndex(UINT * outPlaylistIndex)
{
	TRACK_FUNCTION();

	if (!outPlaylistIndex) return E_POINTER;
	(*outPlaylistIndex) = m_playlistIndex;
	return S_OK;
}

STDMETHODIMP FbPlayingItemLocation::get_PlaylistItemIndex(UINT * outPlaylistItemIndex)
{
	TRACK_FUNCTION();

	if (!outPlaylistItemIndex) return E_POINTER;
	(*outPlaylistItemIndex) = m_itemIndex;
	return S_OK;
}


STDMETHODIMP FbPlaylistRecyclerManager::get_Count(UINT * outCount)
{
	TRACK_FUNCTION();

	if (!outCount) return E_POINTER;

	try
	{
		(*outCount) = static_api_ptr_t<playlist_manager_v3>()->recycler_get_count();
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::get_Name(UINT index, BSTR * outName)
{
	TRACK_FUNCTION();

	if (!outName) return E_POINTER;

	try
	{
		pfc::string8_fast name;
		static_api_ptr_t<playlist_manager_v3>()->recycler_get_name(index, name);
		(*outName) = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(name));
	}
	catch (pfc::exception_invalid_params&)
	{
		return E_INVALIDARG;
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::get_Content(UINT index, __interface IFbMetadbHandleList ** outContent)
{
	TRACK_FUNCTION();

	if (!outContent) return E_POINTER;

	try
	{
		metadb_handle_list handles;
		static_api_ptr_t<playlist_manager_v3>()->recycler_get_content(index, handles);
		(*outContent) = new com_object_impl_t<FbMetadbHandleList>(handles);
	}
	catch (pfc::exception_invalid_params&)
	{
		return E_INVALIDARG;
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::get_Id(UINT index, UINT * outId)
{
	TRACK_FUNCTION();

	if (!outId) return E_POINTER;

	try
	{
		(*outId) = static_api_ptr_t<playlist_manager_v3>()->recycler_get_id(index);
	}
	catch (pfc::exception_invalid_params&)
	{
		return E_INVALIDARG;
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::Purge(VARIANT affectedItems)
{
	TRACK_FUNCTION();

	try
	{
		unsigned bitArrayCount;
		bool empty;
		static_api_ptr_t<playlist_manager_v3> plm;
		bit_array_bittable mask;
		bitArrayCount = plm->recycler_get_count();
		if (!helpers::com_array_to_bitarray::convert(affectedItems, bitArrayCount, mask, empty)) return E_INVALIDARG;
		if (empty) return S_OK;

		plm->recycler_purge(mask);
	}
	catch (pfc::exception_invalid_params&)
	{
		return E_INVALIDARG;
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::Restore(UINT index)
{
	TRACK_FUNCTION();

	try
	{
		static_api_ptr_t<playlist_manager_v3>()->recycler_restore(index);
	}
	catch (pfc::exception_invalid_params&)
	{
		return E_INVALIDARG;
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::RestoreById(UINT id)
{
	TRACK_FUNCTION();

	try
	{
		static_api_ptr_t<playlist_manager_v3>()->recycler_restore_by_id(id);
	}
	catch (pfc::exception_invalid_params&)
	{
		return E_INVALIDARG;
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::FindById(UINT id, UINT * outId)
{
	TRACK_FUNCTION();

	if (!outId) return E_POINTER;

	try
	{
		(*outId) = static_api_ptr_t<playlist_manager_v3>()->recycler_find_by_id(id);
	}
	catch (pfc::exception_invalid_params&)
	{
		return E_INVALIDARG;
	}
	catch (...)
	{
		return E_FAIL;
	}

	return S_OK;
}
