#include "stdafx.h"
#include "script_interface_impl.h"
#include "stackblur.h"
#include "popup_msg.h"
#include "stats.h"
#include <map>
#include <vector>
#include <algorithm>

ContextMenuManager::ContextMenuManager()
{
}

ContextMenuManager::~ContextMenuManager()
{
}

void ContextMenuManager::FinalRelease()
{
	m_cm.release();
}

STDMETHODIMP ContextMenuManager::BuildMenu(IMenuObj* p, int base_id, int max_id)
{
	if (m_cm.is_empty()) return E_POINTER;

	UINT menuid;
	p->get_ID(&menuid);
	contextmenu_node* parent = m_cm->get_root();
	m_cm->win32_build_menu((HMENU)menuid, parent, base_id, max_id);
	return S_OK;
}

STDMETHODIMP ContextMenuManager::ExecuteByID(UINT id, VARIANT_BOOL* p)
{
	if (m_cm.is_empty() || !p) return E_POINTER;

	*p = TO_VARIANT_BOOL(m_cm->execute_by_id(id));
	return S_OK;
}

STDMETHODIMP ContextMenuManager::InitContext(IFbMetadbHandleList* handles)
{
	metadb_handle_list* handles_ptr = NULL;
	handles->get__ptr((void**)&handles_ptr);
	contextmenu_manager::g_create(m_cm);
	m_cm->init_context(*handles_ptr, contextmenu_manager::flag_show_shortcuts);
	return S_OK;
}

STDMETHODIMP ContextMenuManager::InitNowPlaying()
{
	contextmenu_manager::g_create(m_cm);
	m_cm->init_context_now_playing(contextmenu_manager::flag_show_shortcuts);
	return S_OK;
}

DropSourceAction::DropSourceAction()
{
	Reset();
}

DropSourceAction::~DropSourceAction()
{
}

void DropSourceAction::FinalRelease()
{
}

STDMETHODIMP DropSourceAction::ToPlaylist()
{
	m_action_mode = kActionModePlaylist;
	return S_OK;
}

STDMETHODIMP DropSourceAction::get_Mode(int* mode)
{
	if (!mode) return E_POINTER;

	*mode = m_action_mode;
	return S_OK;
}

STDMETHODIMP DropSourceAction::get_Parsable(VARIANT_BOOL* parsable)
{
	if (!parsable) return E_POINTER;

	*parsable = TO_VARIANT_BOOL(m_parsable);
	return S_OK;
}

STDMETHODIMP DropSourceAction::get_Playlist(int* id)
{
	if (!id) return E_POINTER;

	*id = m_playlist_idx;
	return S_OK;
}

STDMETHODIMP DropSourceAction::get_ToSelect(VARIANT_BOOL* select)
{
	if (!select) return E_POINTER;

	*select = TO_VARIANT_BOOL(m_to_select);
	return S_OK;
}

STDMETHODIMP DropSourceAction::put_Parsable(VARIANT_BOOL parsable)
{
	m_parsable = parsable == VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP DropSourceAction::put_Playlist(int id)
{
	m_playlist_idx = id;
	return S_OK;
}

STDMETHODIMP DropSourceAction::put_ToSelect(VARIANT_BOOL select)
{
	m_to_select = (select == VARIANT_TRUE);
	return S_OK;
}

FbFileInfo::FbFileInfo(file_info_impl* p_info_ptr) : m_info_ptr(p_info_ptr)
{
}

FbFileInfo::~FbFileInfo()
{
}

void FbFileInfo::FinalRelease()
{
	if (m_info_ptr)
	{
		delete m_info_ptr;
		m_info_ptr = NULL;
	}
}

STDMETHODIMP FbFileInfo::InfoFind(BSTR name, int* p)
{
	if (!m_info_ptr || !p) return E_POINTER;

	*p = m_info_ptr->info_find(pfc::stringcvt::string_utf8_from_wide(name));
	return S_OK;
}

STDMETHODIMP FbFileInfo::InfoName(UINT idx, BSTR* pp)
{
	if (!m_info_ptr || !pp) return E_POINTER;

	*pp = NULL;

	if (idx < m_info_ptr->info_get_count())
	{
		pfc::stringcvt::string_wide_from_utf8_fast ucs = m_info_ptr->info_enum_name(idx);
		*pp = SysAllocString(ucs);
	}

	return S_OK;
}

STDMETHODIMP FbFileInfo::InfoValue(UINT idx, BSTR* pp)
{
	if (!m_info_ptr || !pp) return E_POINTER;

	*pp = NULL;

	if (idx < m_info_ptr->info_get_count())
	{
		pfc::stringcvt::string_wide_from_utf8_fast ucs = m_info_ptr->info_enum_value(idx);
		*pp = SysAllocString(ucs);
	}

	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaFind(BSTR name, int* p)
{
	if (!m_info_ptr || !p) return E_POINTER;

	*p = m_info_ptr->meta_find(pfc::stringcvt::string_utf8_from_wide(name));
	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaName(UINT idx, BSTR* pp)
{
	if (!m_info_ptr || !pp) return E_POINTER;

	*pp = NULL;

	if (idx < m_info_ptr->meta_get_count())
	{
		pfc::stringcvt::string_wide_from_utf8_fast ucs = m_info_ptr->meta_enum_name(idx);
		*pp = SysAllocString(ucs);
	}

	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaValue(UINT idx, UINT vidx, BSTR* pp)
{
	if (!m_info_ptr || !pp) return E_POINTER;

	*pp = NULL;

	if (idx < m_info_ptr->meta_get_count() && vidx < m_info_ptr->meta_enum_value_count(idx))
	{
		pfc::stringcvt::string_wide_from_utf8_fast ucs = m_info_ptr->meta_enum_value(idx, vidx);
		*pp = SysAllocString(ucs);
	}

	return S_OK;
}

STDMETHODIMP FbFileInfo::MetaValueCount(UINT idx, UINT* p)
{
	if (!m_info_ptr || !p) return E_POINTER;

	*p = (UINT)m_info_ptr->meta_enum_value_count(idx);
	return S_OK;
}

STDMETHODIMP FbFileInfo::get_InfoCount(UINT* p)
{
	if (!m_info_ptr || !p) return E_POINTER;

	*p = (UINT)m_info_ptr->info_get_count();
	return S_OK;
}

STDMETHODIMP FbFileInfo::get_MetaCount(UINT* p)
{
	if (!m_info_ptr || !p) return E_POINTER;

	*p = (UINT)m_info_ptr->meta_get_count();
	return S_OK;
}

STDMETHODIMP FbFileInfo::get__ptr(void** pp)
{
	if (!pp) return E_POINTER;

	*pp = m_info_ptr;
	return S_OK;
}

FbMetadbHandle::FbMetadbHandle(const metadb_handle_ptr& src) : m_handle(src)
{
}

FbMetadbHandle::FbMetadbHandle(metadb_handle* src) : m_handle(src)
{
}

FbMetadbHandle::~FbMetadbHandle()
{
}

void FbMetadbHandle::FinalRelease()
{
	m_handle.release();
}

STDMETHODIMP FbMetadbHandle::Compare(IFbMetadbHandle* handle, VARIANT_BOOL* p)
{
	if (m_handle.is_empty() || !p) return E_POINTER;

	*p = VARIANT_FALSE;

	if (handle)
	{
		metadb_handle* ptr = NULL;
		handle->get__ptr((void**)&ptr);

		*p = TO_VARIANT_BOOL(ptr == m_handle.get_ptr());
	}

	return S_OK;
}

STDMETHODIMP FbMetadbHandle::GetFileInfo(IFbFileInfo** pp)
{
	if (m_handle.is_empty() || !pp) return E_POINTER;

	file_info_impl* info_ptr = new file_info_impl;

	m_handle->get_info(*info_ptr);
	*pp = new com_object_impl_t<FbFileInfo>(info_ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::SetLoved(UINT loved)
{
	if (m_handle.is_empty()) return E_POINTER;

	metadb_index_hash hash;
	if (stats::g_client->hashHandle(m_handle, hash))
	{
		stats::set_loved(hash, loved);
	}
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::SetPlaycount(UINT playcount)
{
	if (m_handle.is_empty()) return E_POINTER;

	metadb_index_hash hash;
	if (stats::g_client->hashHandle(m_handle, hash))
	{
		stats::set_playcount(hash, playcount);
	}
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_FileSize(LONGLONG* p)
{
	if (m_handle.is_empty() || !p) return E_POINTER;

	*p = m_handle->get_filesize();
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_Length(double* p)
{
	if (m_handle.is_empty() || !p) return E_POINTER;

	*p = m_handle->get_length();
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_Path(BSTR* pp)
{
	if (m_handle.is_empty() || !pp) return E_POINTER;

	pfc::stringcvt::string_wide_from_utf8_fast ucs = file_path_display(m_handle->get_path());

	*pp = SysAllocString(ucs);
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_RawPath(BSTR* pp)
{
	if (m_handle.is_empty() || !pp) return E_POINTER;

	pfc::stringcvt::string_wide_from_utf8_fast ucs = m_handle->get_path();

	*pp = SysAllocString(ucs);
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get_SubSong(UINT* p)
{
	if (m_handle.is_empty() || !p) return E_POINTER;

	*p = m_handle->get_subsong_index();
	return S_OK;
}

STDMETHODIMP FbMetadbHandle::get__ptr(void** pp)
{
	if (!pp) return E_POINTER;

	*pp = m_handle.get_ptr();
	return S_OK;
}

FbMetadbHandleList::FbMetadbHandleList(metadb_handle_list_cref handles) : m_handles(handles)
{
}

FbMetadbHandleList::~FbMetadbHandleList()
{
}

void FbMetadbHandleList::FinalRelease()
{
	m_handles.remove_all();
}

STDMETHODIMP FbMetadbHandleList::Add(IFbMetadbHandle* handle)
{
	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	m_handles.add_item(ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::AddRange(IFbMetadbHandleList* handles)
{
	metadb_handle_list* handles_ptr = NULL;
	handles->get__ptr((void**)&handles_ptr);
	m_handles.add_items(*handles_ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::BSearch(IFbMetadbHandle* handle, int* p)
{
	if (!p) return E_POINTER;

	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	*p = m_handles.bsearch_by_pointer(ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::CalcTotalDuration(double* p)
{
	if (!p) return E_POINTER;

	*p = m_handles.calc_total_duration();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::CalcTotalSize(LONGLONG* p)
{
	if (!p) return E_POINTER;

	*p = metadb_handle_list_helper::calc_total_size(m_handles, true);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Clone(IFbMetadbHandleList** pp)
{
	if (!pp) return E_POINTER;

	*pp = new com_object_impl_t<FbMetadbHandleList>(m_handles);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Find(IFbMetadbHandle* handle, int* p)
{
	if (!p) return E_POINTER;

	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	*p = m_handles.find_item(ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Insert(UINT index, IFbMetadbHandle* handle)
{
	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	m_handles.insert_item(ptr, index);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::InsertRange(UINT index, IFbMetadbHandleList* handles)
{
	metadb_handle_list* handles_ptr = NULL;
	handles->get__ptr((void**)&handles_ptr);
	m_handles.insert_items(*handles_ptr, index);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::MakeDifference(IFbMetadbHandleList* handles)
{
	metadb_handle_list* handles_ptr = NULL;
	handles->get__ptr((void**)&handles_ptr);

	metadb_handle_list_ref handles_ref = *handles_ptr;
	metadb_handle_list result;
	t_size walk1 = 0;
	t_size walk2 = 0;
	t_size last1 = m_handles.get_count();
	t_size last2 = handles_ptr->get_count();

	while (walk1 != last1 && walk2 != last2)
	{
		if (m_handles[walk1] < handles_ref[walk2])
		{
			result.add_item(m_handles[walk1]);
			++walk1;
		}
		else if (handles_ref[walk2] < m_handles[walk1])
		{
			++walk2;
		}
		else
		{
			++walk1;
			++walk2;
		}
	}

	m_handles = result;
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::MakeIntersection(IFbMetadbHandleList* handles)
{
	metadb_handle_list* handles_ptr = NULL;
	handles->get__ptr((void**)&handles_ptr);

	metadb_handle_list_ref handles_ref = *handles_ptr;
	metadb_handle_list result;
	t_size walk1 = 0;
	t_size walk2 = 0;
	t_size last1 = m_handles.get_count();
	t_size last2 = handles_ptr->get_count();

	while (walk1 != last1 && walk2 != last2)
	{
		if (m_handles[walk1] < handles_ref[walk2])
			++walk1;
		else if (handles_ref[walk2] < m_handles[walk1])
			++walk2;
		else
		{
			result.add_item(m_handles[walk1]);
			++walk1;
			++walk2;
		}
	}

	m_handles = result;
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::MakeUnion(IFbMetadbHandleList* handles)
{
	metadb_handle_list* handles_ptr = NULL;
	handles->get__ptr((void**)&handles_ptr);

	m_handles.add_items(*handles_ptr);
	m_handles.sort_by_pointer_remove_duplicates();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::OrderByFormat(__interface IFbTitleFormat* script, int direction)
{
	titleformat_object* obj = NULL;
	script->get__ptr((void**)&obj);
	m_handles.sort_by_format(obj, NULL, direction);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::OrderByPath()
{
	m_handles.sort_by_path();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::OrderByRelativePath()
{
	m_handles.sort_by_relative_path();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Remove(IFbMetadbHandle* handle)
{
	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	m_handles.remove_item(ptr);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::RemoveAll()
{
	m_handles.remove_all();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::RemoveById(UINT index)
{
	if (index < m_handles.get_count())
	{
		m_handles.remove_by_idx(index);
		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}
}

STDMETHODIMP FbMetadbHandleList::RemoveRange(UINT from, UINT count)
{
	m_handles.remove_from_idx(from, count);
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::Sort()
{
	m_handles.sort_by_pointer_remove_duplicates();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::UpdateFileInfoFromJSON(BSTR str)
{
	t_size count = m_handles.get_count();
	if (count == 0) return E_POINTER;

	json o;
	bool is_array;

	try
	{
		pfc::stringcvt::string_utf8_from_wide js(str);
		o = json::parse(js.get_ptr());
		if (o.is_array())
		{
			if (o.size() != count) return E_INVALIDARG;
			is_array = true;
		}
		else if (o.is_object())
		{
			if (o.size() == 0) return E_INVALIDARG;
			is_array = false;
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	catch (...)
	{
		return E_INVALIDARG;
	}

	pfc::list_t<file_info_impl> info;
	info.set_size(count);
	
	for (t_size i = 0; i < count; i++)
	{
		json obj = is_array ? o[i] : o;
		if (!obj.is_object() || obj.size() == 0) return E_INVALIDARG;

		metadb_handle_ptr item = m_handles.get_item(i);
		item->get_info(info[i]);

		for (json::iterator it = obj.begin(); it != obj.end(); ++it)
		{
			std::string key = it.key();
			pfc::string8 key8 = key.c_str();
			if (key8.is_empty()) return E_INVALIDARG;

			info[i].meta_remove_field(key8);

			if (it.value().is_array())
			{
				for (json::iterator ita = it.value().begin(); ita != it.value().end(); ++ita)
				{
					pfc::string8 value = helpers::iterator_to_string8(ita);
					if (!value.is_empty())
						info[i].meta_add(key8, value);
				}
			}
			else
			{
				pfc::string8 value = helpers::iterator_to_string8(it);
				if (!value.is_empty())
					info[i].meta_set(key8, value);
			}
		}
	}

	static_api_ptr_t<metadb_io_v2>()->update_info_async_simple(
		m_handles,
		pfc::ptr_list_const_array_t<const file_info, file_info_impl *>(info.get_ptr(), info.get_count()),
		core_api::get_main_window(),
		metadb_io_v2::op_flag_delay_ui,
		NULL
	);

	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::get_Count(UINT* p)
{
	if (!p) return E_POINTER;

	*p = m_handles.get_count();
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::get_Item(UINT index, IFbMetadbHandle** pp)
{
	if (!pp) return E_POINTER;

	if (index < m_handles.get_count())
	{
		*pp = new com_object_impl_t<FbMetadbHandle>(m_handles.get_item_ref(index));
		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}

}

STDMETHODIMP FbMetadbHandleList::get__ptr(void** pp)
{
	if (!pp) return E_POINTER;

	*pp = &m_handles;
	return S_OK;
}

STDMETHODIMP FbMetadbHandleList::put_Item(UINT index, IFbMetadbHandle* handle)
{
	if (index < m_handles.get_count())
	{
		metadb_handle* ptr = NULL;
		handle->get__ptr((void**)&ptr);
		m_handles.replace_item(index, ptr);
		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}
}

FbPlaylistManager::FbPlaylistManager() : m_fbPlaylistRecyclerManager(NULL)
{
}

STDMETHODIMP FbPlaylistManager::AddItemToPlaybackQueue(IFbMetadbHandle* handle)
{
	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	static_api_ptr_t<playlist_manager>()->queue_add_item(ptr);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::AddLocations(UINT playlistIndex, VARIANT locations, VARIANT_BOOL select)
{
	bool toSelect = (select == VARIANT_TRUE);
	helpers::com_array_reader helper;

	if (!helper.convert(&locations)) return E_INVALIDARG;
	pfc::list_t<pfc::string8> locations2;

	for (long i = 0; i < static_cast<long>(helper.get_count()); ++i)
	{
		_variant_t varUrl;

		helper.get_item(i, varUrl);

		if (FAILED(VariantChangeType(&varUrl, &varUrl, 0, VT_BSTR))) return E_INVALIDARG;

		locations2.add_item(pfc::string8(pfc::stringcvt::string_utf8_from_wide(varUrl.bstrVal)));
	}

	pfc::list_const_array_t<const char*, pfc::list_t<pfc::string8>> locations3(locations2, locations2.get_count());

	static_api_ptr_t<playlist_incoming_item_filter_v2>()->process_locations_async(
		locations3,
		playlist_incoming_item_filter_v2::op_flag_no_filter | playlist_incoming_item_filter_v2::op_flag_delay_ui,
		NULL,
		NULL,
		NULL,
		new service_impl_t<helpers::js_process_locations>(playlistIndex, toSelect));

	return S_OK;
}

STDMETHODIMP FbPlaylistManager::AddPlaylistItemToPlaybackQueue(UINT playlistIndex, UINT playlistItemIndex)
{
	static_api_ptr_t<playlist_manager>()->queue_add_item_playlist(playlistIndex, playlistItemIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::ClearPlaylist(UINT playlistIndex)
{
	static_api_ptr_t<playlist_manager>()->playlist_clear(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::ClearPlaylistSelection(UINT playlistIndex)
{
	static_api_ptr_t<playlist_manager>()->playlist_clear_selection(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::CreateAutoPlaylist(UINT idx, BSTR name, BSTR query, BSTR sort, UINT flags, int* p)
{
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

STDMETHODIMP FbPlaylistManager::CreatePlaylist(UINT playlistIndex, BSTR name, UINT* outPlaylistIndex)
{
	if (!outPlaylistIndex) return E_POINTER;

	static_api_ptr_t<playlist_manager> api;

	if (*name)
	{
		pfc::stringcvt::string_utf8_from_wide uname(name);

		*outPlaylistIndex = api->create_playlist(uname, uname.length(), playlistIndex);
	}
	else
	{
		*outPlaylistIndex = api->create_playlist_autoname(playlistIndex);
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistManager::DuplicatePlaylist(UINT from, BSTR name, UINT* outPlaylistIndex)
{
	if (!outPlaylistIndex) return E_POINTER;

	static_api_ptr_t<playlist_manager_v4> manager;
	metadb_handle_list contents;
	pfc::string8_fast name_utf8;

	if (from < manager->get_playlist_count())
	{
		manager->playlist_get_all_items(from, contents);

		if (!name || !*name)
			manager->playlist_get_name(from, name_utf8);
		else
			name_utf8 = pfc::stringcvt::string_utf8_from_wide(name);

		stream_reader_dummy dummy_reader;
		abort_callback_dummy dummy_callback;
		*outPlaylistIndex = manager->create_playlist_ex(name_utf8.get_ptr(), name_utf8.get_length(), from + 1, contents, &dummy_reader, dummy_callback);
		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}
}

STDMETHODIMP FbPlaylistManager::EnsurePlaylistItemVisible(UINT playlistIndex, UINT itemIndex)
{
	static_api_ptr_t<playlist_manager>()->playlist_ensure_visible(playlistIndex, itemIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::ExecutePlaylistDefaultAction(UINT playlistIndex, UINT playlistItemIndex, VARIANT_BOOL* outSuccess)
{
	if (!outSuccess) return E_POINTER;

	*outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_execute_default_action(playlistIndex, playlistItemIndex));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::FindPlaybackQueueItemIndex(IFbMetadbHandle* handle, UINT playlistIndex, UINT playlistItemIndex, int* outIndex)
{
	if (!outIndex) return E_POINTER;

	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);

	t_playback_queue_item item;
	item.m_handle = ptr;
	item.m_playlist = playlistIndex;
	item.m_item = playlistItemIndex;
	*outIndex = static_api_ptr_t<playlist_manager>()->queue_find_index(item);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::FlushPlaybackQueue()
{
	static_api_ptr_t<playlist_manager>()->queue_flush();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaybackQueueHandles(IFbMetadbHandleList** outItems)
{
	if (!outItems) return E_POINTER;

	metadb_handle_list items;
	pfc::list_t<t_playback_queue_item> contents;
	static_api_ptr_t<playlist_manager>()->queue_get_contents(contents);
	for (t_size i = 0; i < contents.get_count(); ++i)
	{
		items.add_item(contents[i].m_handle);
	}
	*outItems = new com_object_impl_t<FbMetadbHandleList>(items);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlayingItemLocation(IFbPlayingItemLocation** outPlayingLocation)
{
	if (!outPlayingLocation) return E_POINTER;

	t_size playlistIndex = -1;
	t_size itemIndex = -1;
	bool isValid = static_api_ptr_t<playlist_manager>()->get_playing_item_location(&playlistIndex, &itemIndex);
	*outPlayingLocation = new com_object_impl_t<FbPlayingItemLocation>(isValid, playlistIndex, itemIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaylistFocusItemIndex(UINT playlistIndex, int* outPlaylistItemIndex)
{
	if (!outPlaylistItemIndex) return E_POINTER;

	*outPlaylistItemIndex = static_api_ptr_t<playlist_manager>()->playlist_get_focus_item(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaylistItems(UINT playlistIndex, IFbMetadbHandleList** outItems)
{
	if (!outItems) return E_POINTER;

	metadb_handle_list items;
	static_api_ptr_t<playlist_manager>()->playlist_get_all_items(playlistIndex, items);
	*outItems = new com_object_impl_t<FbMetadbHandleList>(items);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaylistName(UINT playlistIndex, BSTR* outName)
{
	if (!outName) return E_POINTER;

	pfc::string8_fast temp;
	static_api_ptr_t<playlist_manager>()->playlist_get_name(playlistIndex, temp);
	*outName = SysAllocString(pfc::stringcvt::string_wide_from_utf8(temp));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::GetPlaylistSelectedItems(UINT playlistIndex, IFbMetadbHandleList** outItems)
{
	if (!outItems) return E_POINTER;

	metadb_handle_list items;
	static_api_ptr_t<playlist_manager>()->playlist_get_selected_items(playlistIndex, items);
	*outItems = new com_object_impl_t<FbMetadbHandleList>(items);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::InsertPlaylistItems(UINT playlistIndex, UINT base, IFbMetadbHandleList* handles, VARIANT_BOOL select)
{
	metadb_handle_list* handles_ptr = NULL;
	handles->get__ptr((void**)&handles_ptr);
	bit_array_val selection(select == VARIANT_TRUE);
	static_api_ptr_t<playlist_manager>()->playlist_insert_items(playlistIndex, base, *handles_ptr, selection);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::InsertPlaylistItemsFilter(UINT playlistIndex, UINT base, IFbMetadbHandleList* handles, VARIANT_BOOL select)
{
	metadb_handle_list* handles_ptr = NULL;
	handles->get__ptr((void**)&handles_ptr);
	static_api_ptr_t<playlist_manager>()->playlist_insert_items_filter(playlistIndex, base, *handles_ptr, select == VARIANT_TRUE);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::IsAutoPlaylist(UINT idx, VARIANT_BOOL* p)
{
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

STDMETHODIMP FbPlaylistManager::IsPlaylistItemSelected(UINT playlistIndex, UINT playlistItemIndex, VARIANT_BOOL* outSelected)
{
	if (!outSelected) return E_POINTER;

	*outSelected = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_is_item_selected(playlistIndex, playlistItemIndex));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::IsPlaylistLocked(UINT playlistIndex, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_lock_is_present(playlistIndex));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::MovePlaylist(UINT from, UINT to, VARIANT_BOOL* outSuccess)
{
	if (!outSuccess) return E_POINTER;

	static_api_ptr_t<playlist_manager> api;
	order_helper order(api->get_playlist_count());

	if (from < order.get_count() && to < order.get_count())
	{
		int inc = (from < to) ? 1 : -1;

		for (t_size i = from; i != to; i += inc)
		{
			order[i] = order[i + inc];
		}

		order[to] = from;

		*outSuccess = TO_VARIANT_BOOL(api->reorder(order.get_ptr(), order.get_count()));
	}
	else
	{
		*outSuccess = VARIANT_FALSE;
	}
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::MovePlaylistSelection(UINT playlistIndex, int delta, VARIANT_BOOL* outSuccess)
{
	if (!outSuccess) return E_POINTER;

	*outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_move_selection(playlistIndex, delta));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RemoveItemFromPlaybackQueue(UINT index)
{
	static_api_ptr_t<playlist_manager>()->queue_remove_mask(bit_array_one(index));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RemoveItemsFromPlaybackQueue(VARIANT affectedItems)
{
	unsigned bitArrayCount;
	bool empty;
	static_api_ptr_t<playlist_manager> api;
	bit_array_bittable affected;
	bitArrayCount = api->queue_get_count();
	if (!helpers::com_array_to_bitarray::convert(affectedItems, bitArrayCount, affected, empty)) return E_INVALIDARG;
	if (!empty)
	{
		api->queue_remove_mask(affected);
	}
	
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RemovePlaylist(UINT playlistIndex, VARIANT_BOOL* outSuccess)
{
	if (!outSuccess) return E_POINTER;

	*outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->remove_playlist(playlistIndex));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RemovePlaylistSelection(UINT playlistIndex, VARIANT_BOOL crop)
{
	static_api_ptr_t<playlist_manager>()->playlist_remove_selection(playlistIndex, crop == VARIANT_TRUE);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::RenamePlaylist(UINT playlistIndex, BSTR name, VARIANT_BOOL* outSuccess)
{
	if (!outSuccess) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide uname(name);
	*outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_rename(playlistIndex, uname, uname.length()));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetActivePlaylistContext()
{
	static_api_ptr_t<ui_edit_context_manager>()->set_context_active_playlist();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetPlaylistFocusItem(UINT playlistIndex, UINT itemIndex)
{
	static_api_ptr_t<playlist_manager>()->playlist_set_focus_item(playlistIndex, itemIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetPlaylistFocusItemByHandle(UINT playlistIndex, IFbMetadbHandle* handle)
{
	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	static_api_ptr_t<playlist_manager>()->playlist_set_focus_by_handle(playlistIndex, ptr);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetPlaylistSelection(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state)
{
	unsigned bitArrayCount;
	bool empty;
	static_api_ptr_t<playlist_manager> api;
	bit_array_bittable affected;
	bitArrayCount = api->playlist_get_item_count(playlistIndex);
	if (!helpers::com_array_to_bitarray::convert(affectedItems, bitArrayCount, affected, empty)) return E_INVALIDARG;
	if (!empty)
	{
		bit_array_val status(state == VARIANT_TRUE);
		api->playlist_set_selection(playlistIndex, affected, status);
	}
	
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SetPlaylistSelectionSingle(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state)
{
	static_api_ptr_t<playlist_manager>()->playlist_set_selection_single(playlistIndex, itemIndex, state == VARIANT_TRUE);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::ShowAutoPlaylistUI(UINT idx, VARIANT_BOOL* p)
{
	*p = VARIANT_FALSE;
	static_api_ptr_t<autoplaylist_manager> manager;

	try
	{
		if (manager->is_client_present(idx))
		{
			autoplaylist_client_ptr client = manager->query_client(idx);
			client->show_ui(idx);
			*p = VARIANT_TRUE;
		}
	}
	catch (...)
	{
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SortByFormat(UINT playlistIndex, BSTR pattern, VARIANT_BOOL selOnly, VARIANT_BOOL* outSuccess)
{
	if (!outSuccess) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide string_conv;
	const char* pattern_ptr = NULL;

	if (*pattern)
	{
		string_conv.convert(pattern);
		pattern_ptr = string_conv.get_ptr();
	}

	*outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_sort_by_format(playlistIndex, pattern_ptr, selOnly == VARIANT_TRUE));
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::SortByFormatV2(UINT playlistIndex, BSTR pattern, int direction, VARIANT_BOOL* outSuccess)
{
	if (!outSuccess) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide spec(pattern);
	service_ptr_t<titleformat_object> script;
	metadb_handle_list metadb_handles;
	pfc::array_t<size_t> order;

	if (static_api_ptr_t<titleformat_compiler>()->compile(script, spec))
	{
		static_api_ptr_t<playlist_manager> api;

		// Get metadb_handle_list for playlist specified.
		api->playlist_get_all_items(playlistIndex, metadb_handles);
		order.set_count(metadb_handles.get_count());
		// Reorder metadb handles
		metadb_handle_list_helper::sort_by_format_get_order(metadb_handles, order.get_ptr(), script, NULL, direction);
		// Reorder the playlist
		*outSuccess = TO_VARIANT_BOOL(api->playlist_reorder_items(playlistIndex, order.get_ptr(), order.get_count()));
	}
	else
	{
		*outSuccess = VARIANT_FALSE;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistManager::UndoBackup(UINT playlistIndex)
{
	static_api_ptr_t<playlist_manager>()->playlist_undo_backup(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::UndoRestore(UINT playlistIndex)
{
	static_api_ptr_t<playlist_manager>()->playlist_undo_restore(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_ActivePlaylist(int* outPlaylistIndex)
{
	if (!outPlaylistIndex) return E_POINTER;

	*outPlaylistIndex = static_api_ptr_t<playlist_manager>()->get_active_playlist();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlaybackOrder(UINT* p)
{
	if (!p) return E_POINTER;

	*p = static_api_ptr_t<playlist_manager>()->playback_order_get_active();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlayingPlaylist(int* outPlaylistIndex)
{
	if (!outPlaylistIndex) return E_POINTER;

	*outPlaylistIndex = static_api_ptr_t<playlist_manager>()->get_playing_playlist();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlaylistCount(UINT* outCount)
{
	if (!outCount) return E_POINTER;

	*outCount = static_api_ptr_t<playlist_manager>()->get_playlist_count();
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlaylistItemCount(UINT playlistIndex, UINT* outCount)
{
	if (!outCount) return E_POINTER;

	*outCount = static_api_ptr_t<playlist_manager>()->playlist_get_item_count(playlistIndex);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::get_PlaylistRecyclerManager(__interface IFbPlaylistRecyclerManager** outRecyclerManagerManager)
{
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

STDMETHODIMP FbPlaylistManager::put_ActivePlaylist(int playlistIndex)
{
	static_api_ptr_t<playlist_manager> api;
	t_size index = playlistIndex > -1 ? playlistIndex : pfc::infinite_size;
	api->set_active_playlist(index);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::put_PlaybackOrder(UINT p)
{
	static_api_ptr_t<playlist_manager>()->playback_order_set_active(p);
	return S_OK;
}

STDMETHODIMP FbPlaylistManager::put_PlayingPlaylist(int playlistIndex)
{
	static_api_ptr_t<playlist_manager> api;
	t_size index = playlistIndex > -1 ? playlistIndex : pfc::infinite_size;
	api->set_playing_playlist(index);
	return S_OK;
}

FbPlayingItemLocation::FbPlayingItemLocation(bool isValid, t_size playlistIndex, t_size itemIndex) : m_isValid(isValid), m_playlistIndex(playlistIndex), m_itemIndex(itemIndex)
{
}

STDMETHODIMP FbPlayingItemLocation::get_IsValid(VARIANT_BOOL* outIsValid)
{
	if (!outIsValid) return E_POINTER;

	*outIsValid = TO_VARIANT_BOOL(m_isValid);
	return S_OK;
}

STDMETHODIMP FbPlayingItemLocation::get_PlaylistIndex(UINT* outPlaylistIndex)
{
	if (!outPlaylistIndex) return E_POINTER;

	*outPlaylistIndex = m_playlistIndex;
	return S_OK;
}

STDMETHODIMP FbPlayingItemLocation::get_PlaylistItemIndex(UINT* outPlaylistItemIndex)
{
	if (!outPlaylistItemIndex) return E_POINTER;

	*outPlaylistItemIndex = m_itemIndex;
	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::Purge(VARIANT affectedItems)
{
	try
	{
		unsigned bitArrayCount;
		bool empty;
		static_api_ptr_t<playlist_manager_v3> plm;
		bit_array_bittable mask;
		bitArrayCount = plm->recycler_get_count();
		if (!helpers::com_array_to_bitarray::convert(affectedItems, bitArrayCount, mask, empty)) return E_INVALIDARG;
		if (!empty)
		{
			plm->recycler_purge(mask);
		}
	}
	catch (...)
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::Restore(UINT index)
{
	try
	{
		static_api_ptr_t<playlist_manager_v3>()->recycler_restore(index);
	}
	catch (...)
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::get_Content(UINT index, IFbMetadbHandleList** outContent)
{
	if (!outContent) return E_POINTER;

	try
	{
		metadb_handle_list handles;
		static_api_ptr_t<playlist_manager_v3>()->recycler_get_content(index, handles);
		*outContent = new com_object_impl_t<FbMetadbHandleList>(handles);
	}
	catch (...)
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::get_Count(UINT* outCount)
{
	if (!outCount) return E_POINTER;

	*outCount = static_api_ptr_t<playlist_manager_v3>()->recycler_get_count();
	return S_OK;
}

STDMETHODIMP FbPlaylistRecyclerManager::get_Name(UINT index, BSTR* outName)
{
	if (!outName) return E_POINTER;

	try
	{
		pfc::string8_fast name;
		static_api_ptr_t<playlist_manager_v3>()->recycler_get_name(index, name);
		*outName = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(name));
	}
	catch (...)
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

FbProfiler::FbProfiler(const char* p_name) : m_name(p_name)
{
	m_timer.start();
}

FbProfiler::~FbProfiler()
{
}

STDMETHODIMP FbProfiler::Print()
{
	console::formatter() << JSP_NAME ": FbProfiler (" << m_name << "): " << (int)(m_timer.query() * 1000) << " ms";
	return S_OK;
}

STDMETHODIMP FbProfiler::Reset()
{
	m_timer.start();
	return S_OK;
}

STDMETHODIMP FbProfiler::get_Time(INT* p)
{
	if (!p) return E_POINTER;

	*p = (int)(m_timer.query() * 1000);
	return S_OK;
}

FbTitleFormat::FbTitleFormat(BSTR expr)
{
	pfc::stringcvt::string_utf8_from_wide utf8 = expr;
	static_api_ptr_t<titleformat_compiler>()->compile_safe(m_obj, utf8);
}

FbTitleFormat::~FbTitleFormat()
{
}

void FbTitleFormat::FinalRelease()
{
	m_obj.release();
}

STDMETHODIMP FbTitleFormat::Eval(VARIANT_BOOL force, BSTR* pp)
{
	if (m_obj.is_empty() || !pp) return E_POINTER;

	pfc::string8_fast text;

	if (!static_api_ptr_t<playback_control>()->playback_format_title(NULL, text, m_obj, NULL, playback_control::display_level_all) && force)
	{
		metadb_handle_ptr handle;

		if (!metadb::g_get_random_handle(handle))
		{
			// HACK: A fake file handle should be okay
			 static_api_ptr_t<metadb>()->handle_create(handle, make_playable_location("file://C:\\________.ogg", 0));
		}

		handle->format_title(NULL, text, m_obj, NULL);
	}

	*pp = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(text));
	return S_OK;
}

STDMETHODIMP FbTitleFormat::EvalWithMetadb(IFbMetadbHandle* handle, BSTR* pp)
{
	if (m_obj.is_empty() || !pp) return E_POINTER;

	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	pfc::string8_fast text;
	ptr->format_title(NULL, text, m_obj, NULL);
	*pp = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(text));
	return S_OK;
}

STDMETHODIMP FbTitleFormat::get__ptr(void** pp)
{
	if (!pp) return E_POINTER;

	*pp = m_obj.get_ptr();
	return S_OK;
}

FbTooltip::FbTooltip(HWND p_wndparent, const panel_tooltip_param_ptr& p_param_ptr) : m_wndparent(p_wndparent), m_panel_tooltip_param_ptr(p_param_ptr), m_tip_buffer(SysAllocString(PFC_WIDESTRING(JSP_NAME)))
{
	m_wndtooltip = CreateWindowEx(
		WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		p_wndparent,
		NULL,
		core_api::get_my_instance(),
		NULL);

	// Original position
	SetWindowPos(m_wndtooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	// Set up tooltip information.
	memset(&m_ti, 0, sizeof(m_ti));

	m_ti.cbSize = sizeof(m_ti);
	m_ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
	m_ti.hinst = core_api::get_my_instance();
	m_ti.hwnd = p_wndparent;
	m_ti.uId = (UINT_PTR)p_wndparent;
	m_ti.lpszText = m_tip_buffer;

	HFONT font = CreateFont(
		-(INT)m_panel_tooltip_param_ptr->font_size,
		0,
		0,
		0,
		(m_panel_tooltip_param_ptr->font_style & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL,
		(m_panel_tooltip_param_ptr->font_style & Gdiplus::FontStyleItalic) ? TRUE : FALSE,
		(m_panel_tooltip_param_ptr->font_style & Gdiplus::FontStyleUnderline) ? TRUE : FALSE,
		(m_panel_tooltip_param_ptr->font_style & Gdiplus::FontStyleStrikeout) ? TRUE : FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		m_panel_tooltip_param_ptr->font_name);

	SendMessage(m_wndtooltip, TTM_ADDTOOL, 0, (LPARAM)&m_ti);
	SendMessage(m_wndtooltip, TTM_ACTIVATE, FALSE, 0);
	SendMessage(m_wndtooltip, WM_SETFONT, (WPARAM)font, MAKELPARAM(FALSE, 0));

	m_panel_tooltip_param_ptr->tooltip_hwnd = m_wndtooltip;
	m_panel_tooltip_param_ptr->tooltip_size.cx = -1;
	m_panel_tooltip_param_ptr->tooltip_size.cy = -1;
}

FbTooltip::~FbTooltip()
{
}

void FbTooltip::FinalRelease()
{
	if (m_wndtooltip && IsWindow(m_wndtooltip))
	{
		DestroyWindow(m_wndtooltip);
		m_wndtooltip = NULL;
	}

	if (m_tip_buffer)
	{
		SysFreeString(m_tip_buffer);
		m_tip_buffer = NULL;
	}
}

STDMETHODIMP FbTooltip::get_Text(BSTR* pp)
{
	if (!pp) return E_POINTER;

	*pp = SysAllocString(m_tip_buffer);
	return S_OK;
}

STDMETHODIMP FbTooltip::put_Text(BSTR text)
{
	SysReAllocString(&m_tip_buffer, text);
	m_ti.lpszText = m_tip_buffer;
	SendMessage(m_wndtooltip, TTM_SETTOOLINFO, 0, (LPARAM)&m_ti);
	return S_OK;
}

STDMETHODIMP FbTooltip::put_TrackActivate(VARIANT_BOOL activate)
{
	if (activate)
	{
		m_ti.uFlags |= TTF_TRACK | TTF_ABSOLUTE;
	}
	else
	{
		m_ti.uFlags &= ~(TTF_TRACK | TTF_ABSOLUTE);
	}

	SendMessage(m_wndtooltip, TTM_TRACKACTIVATE, activate ? TRUE : FALSE, (LPARAM)&m_ti);
	return S_OK;
}

STDMETHODIMP FbTooltip::Activate()
{
	SendMessage(m_wndtooltip, TTM_ACTIVATE, TRUE, 0);
	return S_OK;
}

STDMETHODIMP FbTooltip::Deactivate()
{
	SendMessage(m_wndtooltip, TTM_ACTIVATE, FALSE, 0);
	return S_OK;
}

STDMETHODIMP FbTooltip::SetMaxWidth(int width)
{
	SendMessage(m_wndtooltip, TTM_SETMAXTIPWIDTH, 0, width);
	return S_OK;
}

STDMETHODIMP FbTooltip::GetDelayTime(int type, int* p)
{
	if (!p) return E_POINTER;
	if (type < TTDT_AUTOMATIC || type > TTDT_INITIAL) return E_INVALIDARG;

	*p = SendMessage(m_wndtooltip, TTM_GETDELAYTIME, type, 0);
	return S_OK;
}

STDMETHODIMP FbTooltip::SetDelayTime(int type, int time)
{
	if (type < TTDT_AUTOMATIC || type > TTDT_INITIAL) return E_INVALIDARG;

	SendMessage(m_wndtooltip, TTM_SETDELAYTIME, type, time);
	return S_OK;
}

STDMETHODIMP FbTooltip::TrackPosition(int x, int y)
{
	POINT pt = { x, y };
	ClientToScreen(m_wndparent, &pt);
	SendMessage(m_wndtooltip, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
	return S_OK;
}

FbUiSelectionHolder::FbUiSelectionHolder(const ui_selection_holder::ptr& holder) : m_holder(holder)
{
}

FbUiSelectionHolder::~FbUiSelectionHolder()
{
}

void FbUiSelectionHolder::FinalRelease()
{
	m_holder.release();
}

STDMETHODIMP FbUiSelectionHolder::SetPlaylistSelectionTracking()
{
	m_holder->set_playlist_selection_tracking();
	return S_OK;
}

STDMETHODIMP FbUiSelectionHolder::SetPlaylistTracking()
{
	m_holder->set_playlist_tracking();
	return S_OK;
}

STDMETHODIMP FbUiSelectionHolder::SetSelection(IFbMetadbHandleList* handles)
{
	metadb_handle_list* handles_ptr = NULL;
	handles->get__ptr((void**)&handles_ptr);
	m_holder->set_selection(*handles_ptr);
	return S_OK;
}

FbUtils::FbUtils()
{
}

FbUtils::~FbUtils()
{
}

STDMETHODIMP FbUtils::AcquireUiSelectionHolder(IFbUiSelectionHolder** outHolder)
{
	if (!outHolder) return E_POINTER;

	ui_selection_holder::ptr holder = static_api_ptr_t<ui_selection_manager>()->acquire();
	*outHolder = new com_object_impl_t<FbUiSelectionHolder>(holder);
	return S_OK;
}

STDMETHODIMP FbUtils::AddDirectory()
{
	standard_commands::main_add_directory();
	return S_OK;
}

STDMETHODIMP FbUtils::AddFiles()
{
	standard_commands::main_add_files();
	return S_OK;
}

STDMETHODIMP FbUtils::ClearPlaylist()
{
	standard_commands::main_clear_playlist();
	return S_OK;
}

STDMETHODIMP FbUtils::CreateContextMenuManager(IContextMenuManager** pp)
{
	if (!pp) return E_POINTER;

	*pp = new com_object_impl_t<ContextMenuManager>();
	return S_OK;
}

STDMETHODIMP FbUtils::CreateHandleList(IFbMetadbHandleList** pp)
{
	if (!pp) return E_POINTER;

	metadb_handle_list items;
	*pp = new com_object_impl_t<FbMetadbHandleList>(items);

	return S_OK;
}

STDMETHODIMP FbUtils::CreateMainMenuManager(IMainMenuManager** pp)
{
	if (!pp) return E_POINTER;

	*pp = new com_object_impl_t<MainMenuManager>();
	return S_OK;
}

STDMETHODIMP FbUtils::CreateProfiler(BSTR name, IFbProfiler** pp)
{
	if (!pp) return E_POINTER;

	*pp = new com_object_impl_t<FbProfiler>(pfc::stringcvt::string_utf8_from_wide(name));
	return S_OK;
}

STDMETHODIMP FbUtils::Exit()
{
	standard_commands::main_exit();
	return S_OK;
}

STDMETHODIMP FbUtils::GetFocusItem(VARIANT_BOOL force, IFbMetadbHandle** pp)
{
	if (!pp) return E_POINTER;

	metadb_handle_ptr metadb;
	static_api_ptr_t<playlist_manager> api;

	try
	{
		api->activeplaylist_get_focus_item_handle(metadb);
		if (force && metadb.is_empty())
		{
			api->activeplaylist_get_item_handle(metadb, 0);
		}
		if (metadb.is_empty())
		{
			*pp = NULL;
			return S_OK;
		}
	}
	catch (std::exception&)
	{
	}

	*pp = new com_object_impl_t<FbMetadbHandle>(metadb);
	return S_OK;
}

STDMETHODIMP FbUtils::GetLibraryItems(IFbMetadbHandleList** outItems)
{
	if (!outItems) return E_POINTER;

	metadb_handle_list items;
	static_api_ptr_t<library_manager>()->get_all_items(items);
	*outItems = new com_object_impl_t<FbMetadbHandleList>(items);
	return S_OK;
}

STDMETHODIMP FbUtils::GetLibraryRelativePath(IFbMetadbHandle* handle, BSTR* p)
{
	if (!p) return E_POINTER;

	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	pfc::string8_fast temp;
	if (!static_api_ptr_t<library_manager>()->get_relative_path(ptr, temp))
	{
		temp = "";
	}

	*p = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(temp));
	return S_OK;
}

STDMETHODIMP FbUtils::GetNowPlaying(IFbMetadbHandle** pp)
{
	if (!pp) return E_POINTER;

	metadb_handle_ptr metadb;

	if (static_api_ptr_t<playback_control>()->get_now_playing(metadb))
	{
		*pp = new com_object_impl_t<FbMetadbHandle>(metadb);
	}
	else
	{
		*pp = NULL;
	}

	return S_OK;
}

STDMETHODIMP FbUtils::GetQueryItems(IFbMetadbHandleList* items, BSTR query, IFbMetadbHandleList** pp)
{
	if (!pp) return E_POINTER;

	metadb_handle_list *srclist_ptr, dst_list;
	search_filter_v2::ptr filter;

	items->get__ptr((void**)&srclist_ptr);
	dst_list = *srclist_ptr;
	pfc::stringcvt::string_utf8_from_wide query8(query);

	try
	{
		filter = static_api_ptr_t<search_filter_manager_v2>()->create_ex(query8, new service_impl_t<completion_notify_dummy>(), search_filter_manager_v2::KFlagSuppressNotify);
	}
	catch (...)
	{
		return E_FAIL;
	}

	pfc::array_t<bool> mask;
	mask.set_size(dst_list.get_count());
	filter->test_multi(dst_list, mask.get_ptr());
	dst_list.filter_mask(mask.get_ptr());

	*pp = new com_object_impl_t<FbMetadbHandleList>(dst_list);

	return S_OK;
}

STDMETHODIMP FbUtils::GetSelection(IFbMetadbHandle** pp)
{
	if (!pp) return E_POINTER;

	metadb_handle_list items;
	static_api_ptr_t<ui_selection_manager>()->get_selection(items);

	if (items.get_count() > 0)
	{
		*pp = new com_object_impl_t<FbMetadbHandle>(items[0]);
	}
	else
	{
		*pp = NULL;
	}

	return S_OK;
}

STDMETHODIMP FbUtils::GetSelectionType(UINT* p)
{
	if (!p) return E_POINTER;

	const GUID* guids[] = {
		&contextmenu_item::caller_undefined,
		&contextmenu_item::caller_active_playlist_selection,
		&contextmenu_item::caller_active_playlist,
		&contextmenu_item::caller_playlist_manager,
		&contextmenu_item::caller_now_playing,
		&contextmenu_item::caller_keyboard_shortcut_list,
		&contextmenu_item::caller_media_library_viewer,
	};

	*p = 0;
	GUID type = static_api_ptr_t<ui_selection_manager_v2>()->get_selection_type(0);

	for (t_size i = 0; i < _countof(guids); ++i)
	{
		if (*guids[i] == type)
		{
			*p = i;
			break;
		}
	}

	return S_OK;
}

STDMETHODIMP FbUtils::GetSelections(UINT flags, IFbMetadbHandleList** pp)
{
	if (!pp) return E_POINTER;

	metadb_handle_list items;
	static_api_ptr_t<ui_selection_manager_v2>()->get_selection(items, flags);
	*pp = new com_object_impl_t<FbMetadbHandleList>(items);
	return S_OK;
}

STDMETHODIMP FbUtils::IsLibraryEnabled(VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(static_api_ptr_t<library_manager>()->is_library_enabled());
	return S_OK;
}

STDMETHODIMP FbUtils::IsMetadbInMediaLibrary(IFbMetadbHandle* handle, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	*p = TO_VARIANT_BOOL(static_api_ptr_t<library_manager>()->is_item_in_library(ptr));
	return S_OK;
}

STDMETHODIMP FbUtils::LoadPlaylist()
{
	standard_commands::main_load_playlist();
	return S_OK;
}

STDMETHODIMP FbUtils::Next()
{
	standard_commands::main_next();
	return S_OK;
}

STDMETHODIMP FbUtils::Pause()
{
	standard_commands::main_pause();
	return S_OK;
}

STDMETHODIMP FbUtils::Play()
{
	standard_commands::main_play();
	return S_OK;
}

STDMETHODIMP FbUtils::PlayOrPause()
{
	standard_commands::main_play_or_pause();
	return S_OK;
}

STDMETHODIMP FbUtils::Prev()
{
	standard_commands::main_previous();
	return S_OK;
}

STDMETHODIMP FbUtils::Random()
{
	standard_commands::main_random();
	return S_OK;
}

STDMETHODIMP FbUtils::RunContextCommand(BSTR command, UINT flags, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide name(command);
	*p = TO_VARIANT_BOOL(helpers::execute_context_command_by_name_SEH(name, metadb_handle_list(), flags));
	return S_OK;
}

STDMETHODIMP FbUtils::RunContextCommandWithMetadb(BSTR command, VARIANT handle, UINT flags, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;
	if (handle.vt != VT_DISPATCH || !handle.pdispVal) return E_INVALIDARG;

	metadb_handle_list handle_list;
	IDispatch* temp = NULL;
	IDispatchPtr handle_s = NULL;
	void* ptr = NULL;

	if (SUCCEEDED(handle.pdispVal->QueryInterface(__uuidof(IFbMetadbHandle), (void**)&temp)))
	{
		handle_s = temp;
		reinterpret_cast<IFbMetadbHandle *>(handle_s.GetInterfacePtr())->get__ptr(&ptr);
		if (!ptr) return E_INVALIDARG;
		handle_list = pfc::list_single_ref_t<metadb_handle_ptr>(reinterpret_cast<metadb_handle *>(ptr));
	}
	else if (SUCCEEDED(handle.pdispVal->QueryInterface(__uuidof(IFbMetadbHandleList), (void**)&temp)))
	{
		handle_s = temp;
		reinterpret_cast<IFbMetadbHandleList *>(handle_s.GetInterfacePtr())->get__ptr(&ptr);
		if (!ptr) return E_INVALIDARG;
		handle_list = *reinterpret_cast<metadb_handle_list *>(ptr);
	}
	else
	{
		return E_INVALIDARG;
	}

	pfc::stringcvt::string_utf8_from_wide name(command);
	*p = TO_VARIANT_BOOL(helpers::execute_context_command_by_name_SEH(name, handle_list, flags));
	return S_OK;
}

STDMETHODIMP FbUtils::RunMainMenuCommand(BSTR command, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	pfc::stringcvt::string_utf8_from_wide name(command);
	*p = TO_VARIANT_BOOL(helpers::execute_mainmenu_command_by_name_SEH(name));
	return S_OK;
}

STDMETHODIMP FbUtils::SavePlaylist()
{
	standard_commands::main_save_playlist();
	return S_OK;
}

STDMETHODIMP FbUtils::ShowConsole()
{
	const GUID guid_main_show_console = { 0x5b652d25, 0xce44, 0x4737, {0x99, 0xbb, 0xa3, 0xcf, 0x2a, 0xeb, 0x35, 0xcc} };
	standard_commands::run_main(guid_main_show_console);
	return S_OK;
}

STDMETHODIMP FbUtils::ShowLibrarySearchUI(BSTR query)
{
	if (!query) return E_INVALIDARG;

	pfc::stringcvt::string_utf8_from_wide wquery(query);
	static_api_ptr_t<library_search_ui>()->show(wquery);
	return S_OK;
}

STDMETHODIMP FbUtils::ShowPopupMessage(BSTR msg, BSTR title, int iconid)
{
	popup_msg::g_show(pfc::stringcvt::string_utf8_from_wide(msg), pfc::stringcvt::string_utf8_from_wide(title), (popup_message::t_icon)iconid);
	return S_OK;
}

STDMETHODIMP FbUtils::ShowPreferences()
{
	standard_commands::main_preferences();
	return S_OK;
}

STDMETHODIMP FbUtils::Stop()
{
	standard_commands::main_stop();
	return S_OK;
}

STDMETHODIMP FbUtils::TitleFormat(BSTR expression, IFbTitleFormat** pp)
{
	if (!pp) return E_POINTER;

	*pp = new com_object_impl_t<FbTitleFormat>(expression);
	return S_OK;
}

STDMETHODIMP FbUtils::VolumeDown()
{
	standard_commands::main_volume_down();
	return S_OK;
}

STDMETHODIMP FbUtils::VolumeMute()
{
	standard_commands::main_volume_mute();
	return S_OK;
}

STDMETHODIMP FbUtils::VolumeUp()
{
	standard_commands::main_volume_up();
	return S_OK;
}

STDMETHODIMP FbUtils::get_AlwaysOnTop(VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(config_object::g_get_data_bool_simple(standard_config_objects::bool_ui_always_on_top, false));
	return S_OK;
}

STDMETHODIMP FbUtils::get_ComponentPath(BSTR* pp)
{
	if (!pp) return E_POINTER;

	static pfc::stringcvt::string_wide_from_utf8 path(helpers::get_fb2k_component_path());
	*pp = SysAllocString(path.get_ptr());
	return S_OK;
}

STDMETHODIMP FbUtils::get_CursorFollowPlayback(VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(config_object::g_get_data_bool_simple(standard_config_objects::bool_cursor_follows_playback, false));
	return S_OK;
}

STDMETHODIMP FbUtils::get_FoobarPath(BSTR* pp)
{
	if (!pp) return E_POINTER;

	static pfc::stringcvt::string_wide_from_utf8 path(helpers::get_fb2k_path());

	*pp = SysAllocString(path.get_ptr());
	return S_OK;
}

STDMETHODIMP FbUtils::get_IsPaused(VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(static_api_ptr_t<playback_control>()->is_paused());
	return S_OK;
}

STDMETHODIMP FbUtils::get_IsPlaying(VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(static_api_ptr_t<playback_control>()->is_playing());
	return S_OK;
}

STDMETHODIMP FbUtils::get_PlaybackFollowCursor(VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(config_object::g_get_data_bool_simple(standard_config_objects::bool_playback_follows_cursor, false));
	return S_OK;
}

STDMETHODIMP FbUtils::get_PlaybackLength(double* p)
{
	if (!p) return E_POINTER;

	*p = static_api_ptr_t<playback_control>()->playback_get_length();
	return S_OK;
}

STDMETHODIMP FbUtils::get_PlaybackTime(double* p)
{
	if (!p) return E_POINTER;

	*p = static_api_ptr_t<playback_control>()->playback_get_position();
	return S_OK;
}

STDMETHODIMP FbUtils::get_ProfilePath(BSTR* pp)
{
	if (!pp) return E_POINTER;

	static pfc::stringcvt::string_wide_from_utf8 path(helpers::get_profile_path());
	*pp = SysAllocString(path.get_ptr());
	return S_OK;
}

STDMETHODIMP FbUtils::get_ReplaygainMode(UINT* p)
{
	if (!p) return E_POINTER;

	t_replaygain_config rg;
	static_api_ptr_t<replaygain_manager>()->get_core_settings(rg);
	*p = rg.m_source_mode;
	return S_OK;
}

STDMETHODIMP FbUtils::get_StopAfterCurrent(VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = static_api_ptr_t<playback_control>()->get_stop_after_current();
	return S_OK;
}

STDMETHODIMP FbUtils::get_Volume(float* p)
{
	if (!p) return E_POINTER;

	*p = static_api_ptr_t<playback_control>()->get_volume();
	return S_OK;
}

STDMETHODIMP FbUtils::put_AlwaysOnTop(VARIANT_BOOL p)
{
	config_object::g_set_data_bool(standard_config_objects::bool_ui_always_on_top, (p != VARIANT_FALSE));
	return S_OK;
}

STDMETHODIMP FbUtils::put_CursorFollowPlayback(VARIANT_BOOL p)
{
	config_object::g_set_data_bool(standard_config_objects::bool_cursor_follows_playback, (p != VARIANT_FALSE));
	return S_OK;
}

STDMETHODIMP FbUtils::put_PlaybackFollowCursor(VARIANT_BOOL p)
{
	config_object::g_set_data_bool(standard_config_objects::bool_playback_follows_cursor, (p != VARIANT_FALSE));
	return S_OK;
}

STDMETHODIMP FbUtils::put_PlaybackTime(double time)
{
	static_api_ptr_t<playback_control>()->playback_seek(time);
	return S_OK;
}

STDMETHODIMP FbUtils::put_ReplaygainMode(UINT p)
{
	switch (p)
	{
	case 0:
		standard_commands::main_rg_disable();
		break;
	case 1:
		standard_commands::main_rg_set_track();
		break;
	case 2:
		standard_commands::main_rg_set_album();
		break;
	case 3:
		standard_commands::run_main(standard_commands::guid_main_rg_byorder);
		break;
	default:
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP FbUtils::put_StopAfterCurrent(VARIANT_BOOL p)
{
	static_api_ptr_t<playback_control>()->set_stop_after_current(p != VARIANT_FALSE);
	return S_OK;
}

STDMETHODIMP FbUtils::put_Volume(float value)
{
	static_api_ptr_t<playback_control>()->set_volume(value);
	return S_OK;
}

GdiBitmap::GdiBitmap(Gdiplus::Bitmap* p) : GdiObj<IGdiBitmap, Gdiplus::Bitmap>(p)
{
}

STDMETHODIMP GdiBitmap::ApplyAlpha(BYTE alpha, IGdiBitmap** pp)
{
	if (!m_ptr || !pp) return E_POINTER;

	UINT width = m_ptr->GetWidth();
	UINT height = m_ptr->GetHeight();
	Gdiplus::Bitmap* out = new Gdiplus::Bitmap(width, height, PixelFormat32bppPARGB);
	Gdiplus::Graphics g(out);
	Gdiplus::ImageAttributes ia;
	Gdiplus::ColorMatrix cm = { 0.0 };
	Gdiplus::Rect rc;

	cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0;
	cm.m[3][3] = static_cast<float>(alpha) / 255;
	ia.SetColorMatrix(&cm);

	rc.X = rc.Y = 0;
	rc.Width = width;
	rc.Height = height;

	g.DrawImage(m_ptr, rc, 0, 0, width, height, Gdiplus::UnitPixel, &ia);

	*pp = new com_object_impl_t<GdiBitmap>(out);
	return S_OK;
}

STDMETHODIMP GdiBitmap::ApplyMask(IGdiBitmap* mask, VARIANT_BOOL* p)
{
	if (!m_ptr || !p) return E_POINTER;

	*p = VARIANT_FALSE;
	Gdiplus::Bitmap* bitmap_mask = NULL;
	mask->get__ptr((void**)&bitmap_mask);

	if (!bitmap_mask || bitmap_mask->GetHeight() != m_ptr->GetHeight() || bitmap_mask->GetWidth() != m_ptr->GetWidth())
	{
		return E_INVALIDARG;
	}

	Gdiplus::Rect rect(0, 0, m_ptr->GetWidth(), m_ptr->GetHeight());
	Gdiplus::BitmapData bmpdata_mask = { 0 }, bmpdata_dst = { 0 };

	if (bitmap_mask->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata_mask) != Gdiplus::Ok)
	{
		return S_OK;
	}

	if (m_ptr->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bmpdata_dst) != Gdiplus::Ok)
	{
		bitmap_mask->UnlockBits(&bmpdata_mask);
		return S_OK;
	}

	const int width = rect.Width;
	const int height = rect.Height;
	const int size = width * height;
	//const int size_threshold = 512;
	t_uint32* p_mask = reinterpret_cast<t_uint32 *>(bmpdata_mask.Scan0);
	t_uint32* p_dst = reinterpret_cast<t_uint32 *>(bmpdata_dst.Scan0);
	const t_uint32* p_mask_end = p_mask + rect.Width * rect.Height;
	t_uint32 alpha;

	while (p_mask < p_mask_end)
	{
		// Method 1:
		//alpha = (~*p_mask & 0xff) * (*p_dst >> 24) + 0x80;
		//*p_dst = ((((alpha >> 8) + alpha) & 0xff00) << 16) | (*p_dst & 0xffffff);
		// Method 2
		alpha = (((~*p_mask & 0xff) * (*p_dst >> 24)) << 16) & 0xff000000;
		*p_dst = alpha | (*p_dst & 0xffffff);

		++p_mask;
		++p_dst;
	}

	m_ptr->UnlockBits(&bmpdata_dst);
	bitmap_mask->UnlockBits(&bmpdata_mask);

	*p = VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP GdiBitmap::Clone(float x, float y, float w, float h, IGdiBitmap** pp)
{
	if (!m_ptr || !pp) return E_POINTER;

	Gdiplus::Bitmap* img = m_ptr->Clone(x, y, w, h, PixelFormat32bppPARGB);
	if (helpers::ensure_gdiplus_object(img))
	{
		*pp = new com_object_impl_t<GdiBitmap>(img);
	}
	else
	{
		if (img) delete img;
		*pp = NULL;
	}

	return S_OK;
}

STDMETHODIMP GdiBitmap::CreateRawBitmap(IGdiRawBitmap** pp)
{
	if (!m_ptr || !pp) return E_POINTER;

	*pp = new com_object_impl_t<GdiRawBitmap>(m_ptr);
	return S_OK;
}

STDMETHODIMP GdiBitmap::GetColourScheme(UINT count, VARIANT* outArray)
{
	if (!m_ptr || !outArray) return E_POINTER;

	Gdiplus::BitmapData bmpdata;
	Gdiplus::Rect rect(0, 0, m_ptr->GetWidth(), m_ptr->GetHeight());

	if (m_ptr->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpdata) != Gdiplus::Ok)
		return E_POINTER;

	std::map<unsigned, int> color_counters;
	const unsigned colors_length = bmpdata.Width * bmpdata.Height;
	const t_uint32* colors = (const t_uint32 *)bmpdata.Scan0;

	for (unsigned i = 0; i < colors_length; i++)
	{
		// format: 0xaarrggbb
		unsigned color = colors[i];
		unsigned r = (color >> 16) & 0xff;
		unsigned g = (color >> 8) & 0xff;
		unsigned b = (color) & 0xff;

		// Round colors
		r = (r + 15) & 0xffffffe0;
		g = (g + 15) & 0xffffffe0;
		b = (b + 15) & 0xffffffe0;

		if (r > 0xff) r = 0xf0;
		if (g > 0xff) g = 0xf0;
		if (b > 0xff) b = 0xf0;

		++color_counters[Gdiplus::Color::MakeARGB(0xff, r, g, b)];
	}

	m_ptr->UnlockBits(&bmpdata);

	// Sorting
	typedef std::pair<unsigned, int> sort_vec_pair_t;
	std::vector<sort_vec_pair_t> sort_vec(color_counters.begin(), color_counters.end());
	color_counters.clear();
	count = min(count, sort_vec.size());
	std::partial_sort(
		sort_vec.begin(),
		sort_vec.begin() + count,
		sort_vec.end(),
		[](const sort_vec_pair_t& a, const sort_vec_pair_t& b)
	{
		return a.second > b.second;
	});

	helpers::com_array_writer<> helper;
	if (!helper.create(count)) return E_OUTOFMEMORY;

	for (long i = 0; i < helper.get_count(); ++i)
	{
		_variant_t var;
		var.vt = VT_UI4;
		var.ulVal = sort_vec[i].first;

		if (FAILED(helper.put(i, var)))
		{
			helper.reset();
			return E_OUTOFMEMORY;
		}
	}

	outArray->vt = VT_ARRAY | VT_VARIANT;
	outArray->parray = helper.get_ptr();
	return S_OK;
}

STDMETHODIMP GdiBitmap::GetGraphics(IGdiGraphics** pp)
{
	if (!m_ptr || !pp) return E_POINTER;

	Gdiplus::Graphics* g = new Gdiplus::Graphics(m_ptr);
	*pp = new com_object_impl_t<GdiGraphics>();
	(*pp)->put__ptr(g);
	return S_OK;
}

STDMETHODIMP GdiBitmap::ReleaseGraphics(IGdiGraphics* p)
{
	if (p)
	{
		Gdiplus::Graphics* g = NULL;
		p->get__ptr((void**)&g);
		p->put__ptr(NULL);
		if (g) delete g;
	}

	return S_OK;
}

STDMETHODIMP GdiBitmap::Resize(UINT w, UINT h, int interpolationMode, IGdiBitmap** pp)
{
	if (!m_ptr || !pp) return E_POINTER;

	Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(w, h, PixelFormat32bppPARGB);
	Gdiplus::Graphics g(bitmap);
	g.SetInterpolationMode((Gdiplus::InterpolationMode)interpolationMode);
	g.DrawImage(m_ptr, 0, 0, w, h);
	*pp = new com_object_impl_t<GdiBitmap>(bitmap);
	return S_OK;
}

STDMETHODIMP GdiBitmap::RotateFlip(UINT mode)
{
	if (!m_ptr) return E_POINTER;

	m_ptr->RotateFlip((Gdiplus::RotateFlipType)mode);
	return S_OK;
}

STDMETHODIMP GdiBitmap::SaveAs(BSTR path, BSTR format, VARIANT_BOOL* p)
{
	if (!p || !m_ptr) return E_POINTER;

	CLSID clsid_encoder;
	int ret = helpers::get_encoder_clsid(format, &clsid_encoder);

	if (ret > -1)
	{
		m_ptr->Save(path, &clsid_encoder);
		*p = TO_VARIANT_BOOL(m_ptr->GetLastStatus() == Gdiplus::Ok);
	}
	else
	{
		*p = VARIANT_FALSE;
	}

	return S_OK;
}

STDMETHODIMP GdiBitmap::StackBlur(int radius)
{
	if (!m_ptr) return E_POINTER;

	t_size threads = pfc::getOptimalWorkerThreadCount();
	stack_blur_filter(*m_ptr, radius, threads);
	return S_OK;
}

STDMETHODIMP GdiBitmap::get_Height(UINT* p)
{
	if (!m_ptr || !p) return E_POINTER;

	*p = m_ptr->GetHeight();
	return S_OK;
}

STDMETHODIMP GdiBitmap::get_Width(UINT* p)
{
	if (!m_ptr || !p) return E_POINTER;

	*p = m_ptr->GetWidth();
	return S_OK;
}

GdiFont::GdiFont(Gdiplus::Font* p, HFONT hFont, bool managed) : GdiObj<IGdiFont, Gdiplus::Font>(p), m_hFont(hFont), m_managed(managed)
{
}

GdiFont:: ~GdiFont()
{
}

void GdiFont::FinalRelease()
{
	if (m_hFont && m_managed)
	{
		DeleteFont(m_hFont);
		m_hFont = NULL;
	}

	// call parent
	GdiObj<IGdiFont, Gdiplus::Font>::FinalRelease();
}

STDMETHODIMP GdiFont::get_HFont(UINT* p)
{
	if (!m_ptr || !p) return E_POINTER;

	*p = (UINT)m_hFont;
	return S_OK;
}

STDMETHODIMP GdiFont::get_Height(UINT* p)
{
	if (!m_ptr || !p) return E_POINTER;

	Gdiplus::Bitmap img(1, 1, PixelFormat32bppPARGB);
	Gdiplus::Graphics g(&img);
	*p = (UINT)m_ptr->GetHeight(&g);
	return S_OK;
}

STDMETHODIMP GdiFont::get_Name(LANGID langId, BSTR* outName)
{
	if (!m_ptr || !outName) return E_POINTER;

	Gdiplus::FontFamily fontFamily;
	WCHAR name[LF_FACESIZE] = { 0 };
	m_ptr->GetFamily(&fontFamily);
	fontFamily.GetFamilyName(name, langId);
	*outName = SysAllocString(name);
	return S_OK;
}

STDMETHODIMP GdiFont::get_Size(float* outSize)
{
	if (!m_ptr || !outSize) return E_POINTER;

	*outSize = m_ptr->GetSize();
	return S_OK;
}

STDMETHODIMP GdiFont::get_Style(INT* outStyle)
{
	if (!m_ptr || !outStyle) return E_POINTER;

	*outStyle = m_ptr->GetStyle();
	return S_OK;
}

GdiGraphics::GdiGraphics() : GdiObj<IGdiGraphics, Gdiplus::Graphics>(NULL)
{
}

void GdiGraphics::GetRoundRectPath(Gdiplus::GraphicsPath& gp, Gdiplus::RectF& rect, float arc_width, float arc_height)
{
	float arc_dia_w = arc_width * 2;
	float arc_dia_h = arc_height * 2;
	Gdiplus::RectF corner(rect.X, rect.Y, arc_dia_w, arc_dia_h);

	gp.Reset();

	// top left
	gp.AddArc(corner, 180, 90);

	// top right
	corner.X += (rect.Width - arc_dia_w);
	gp.AddArc(corner, 270, 90);

	// bottom right
	corner.Y += (rect.Height - arc_dia_h);
	gp.AddArc(corner, 0, 90);

	// bottom left
	corner.X -= (rect.Width - arc_dia_w);
	gp.AddArc(corner, 90, 90);

	gp.CloseFigure();
}

STDMETHODIMP GdiGraphics::CalcTextHeight(BSTR str, IGdiFont* font, UINT* p)
{
	if (!m_ptr || !p) return E_POINTER;

	HFONT hFont = NULL;
	font->get_HFont((UINT *)&hFont);
	HFONT oldfont;
	HDC dc = m_ptr->GetHDC();
	oldfont = SelectFont(dc, hFont);
	*p = helpers::get_text_height(dc, str, SysStringLen(str));
	SelectFont(dc, oldfont);
	m_ptr->ReleaseHDC(dc);
	return S_OK;
}

STDMETHODIMP GdiGraphics::CalcTextWidth(BSTR str, IGdiFont* font, UINT* p)
{
	if (!m_ptr || !p) return E_POINTER;

	HFONT hFont = NULL;
	font->get_HFont((UINT *)&hFont);
	HFONT oldfont;
	HDC dc = m_ptr->GetHDC();
	oldfont = SelectFont(dc, hFont);
	*p = helpers::get_text_width(dc, str, SysStringLen(str));
	SelectFont(dc, oldfont);
	m_ptr->ReleaseHDC(dc);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawEllipse(float x, float y, float w, float h, float line_width, VARIANT colour)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::Pen pen(helpers::get_colour_from_variant(colour), line_width);
	m_ptr->DrawEllipse(&pen, x, y, w, h);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawImage(IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, float angle, BYTE alpha)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::Bitmap* img = NULL;
	image->get__ptr((void**)&img);
	Gdiplus::Matrix old_m;

	if (angle != 0.0)
	{
		Gdiplus::Matrix m;
		Gdiplus::RectF rect;
		Gdiplus::PointF pt;

		pt.X = dstX + dstW / 2;
		pt.Y = dstY + dstH / 2;
		m.RotateAt(angle, pt);
		m_ptr->GetTransform(&old_m);
		m_ptr->SetTransform(&m);
	}

	if (alpha != (BYTE)~0)
	{
		Gdiplus::ImageAttributes ia;
		Gdiplus::ColorMatrix cm = { 0.0f };

		cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0f;
		cm.m[3][3] = static_cast<float>(alpha) / 255;

		ia.SetColorMatrix(&cm);

		m_ptr->DrawImage(img, Gdiplus::RectF(dstX, dstY, dstW, dstH), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel, &ia);
	}
	else
	{
		m_ptr->DrawImage(img, Gdiplus::RectF(dstX, dstY, dstW, dstH), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel);
	}

	if (angle != 0.0)
	{
		m_ptr->SetTransform(&old_m);
	}

	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawLine(float x1, float y1, float x2, float y2, float line_width, VARIANT colour)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::Pen pen(helpers::get_colour_from_variant(colour), line_width);
	m_ptr->DrawLine(&pen, x1, y1, x2, y2);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawPolygon(VARIANT colour, float line_width, VARIANT points)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::SolidBrush br(helpers::get_colour_from_variant(colour));
	helpers::com_array_reader helper;
	pfc::array_t<Gdiplus::PointF> point_array;

	if (!helper.convert(&points)) return E_INVALIDARG;
	if ((helper.get_count() % 2) != 0) return E_INVALIDARG;

	point_array.set_count(helper.get_count() >> 1);

	for (long i = 0; i < static_cast<long>(point_array.get_count()); ++i)
	{
		_variant_t varX, varY;

		helper.get_item(i * 2, varX);
		helper.get_item(i * 2 + 1, varY);

		if (FAILED(VariantChangeType(&varX, &varX, 0, VT_R4))) return E_INVALIDARG;
		if (FAILED(VariantChangeType(&varY, &varY, 0, VT_R4))) return E_INVALIDARG;

		point_array[i].X = varX.fltVal;
		point_array[i].Y = varY.fltVal;
	}

	Gdiplus::Pen pen(helpers::get_colour_from_variant(colour), line_width);
	m_ptr->DrawPolygon(&pen, point_array.get_ptr(), point_array.get_count());
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawRect(float x, float y, float w, float h, float line_width, VARIANT colour)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::Pen pen(helpers::get_colour_from_variant(colour), line_width);
	m_ptr->DrawRectangle(&pen, x, y, w, h);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, float line_width, VARIANT colour)
{
	if (!m_ptr) return E_POINTER;

	if (2 * arc_width > w || 2 * arc_height > h) return E_INVALIDARG;

	Gdiplus::Pen pen(helpers::get_colour_from_variant(colour), line_width);
	Gdiplus::GraphicsPath gp;
	Gdiplus::RectF rect(x, y, w, h);
	GetRoundRectPath(gp, rect, arc_width, arc_height);
	pen.SetStartCap(Gdiplus::LineCapRound);
	pen.SetEndCap(Gdiplus::LineCapRound);
	m_ptr->DrawPath(&pen, &gp);
	return S_OK;
}

STDMETHODIMP GdiGraphics::DrawString(BSTR str, IGdiFont* font, VARIANT colour, float x, float y, float w, float h, int flags)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::Font* fn = NULL;
	font->get__ptr((void**)&fn);
	Gdiplus::SolidBrush br(helpers::get_colour_from_variant(colour));
	Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());

	if (flags != 0)
	{
		fmt.SetAlignment((Gdiplus::StringAlignment)((flags >> 28) & 0x3)); //0xf0000000
		fmt.SetLineAlignment((Gdiplus::StringAlignment)((flags >> 24) & 0x3)); //0x0f000000
		fmt.SetTrimming((Gdiplus::StringTrimming)((flags >> 20) & 0x7)); //0x00f00000
		fmt.SetFormatFlags((Gdiplus::StringFormatFlags)(flags & 0x7FFF)); //0x0000ffff
	}

	m_ptr->DrawString(str, -1, fn, Gdiplus::RectF(x, y, w, h), &fmt, &br);
	return S_OK;
}

STDMETHODIMP GdiGraphics::EstimateLineWrap(BSTR str, IGdiFont* font, int max_width, VARIANT* p)
{
	if (!m_ptr || !p) return E_POINTER;

	HFONT hFont = NULL;
	font->get_HFont((UINT *)&hFont);
	HFONT oldfont;
	HDC dc = m_ptr->GetHDC();
	pfc::list_t<helpers::wrapped_item> result;
	oldfont = SelectFont(dc, hFont);
	estimate_line_wrap(dc, str, SysStringLen(str), max_width, result);
	SelectFont(dc, oldfont);
	m_ptr->ReleaseHDC(dc);

	helpers::com_array_writer<> helper;

	if (!helper.create(result.get_count() * 2))
	{
		return E_OUTOFMEMORY;
	}

	for (long i = 0; i < helper.get_count() / 2; ++i)
	{
		_variant_t var1, var2;

		var1.vt = VT_BSTR;
		var1.bstrVal = result[i].text;
		var2.vt = VT_I4;
		var2.lVal = result[i].width;

		helper.put(i * 2, var1);
		helper.put(i * 2 + 1, var2);
	}

	p->vt = VT_ARRAY | VT_VARIANT;
	p->parray = helper.get_ptr();
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillEllipse(float x, float y, float w, float h, VARIANT colour)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::SolidBrush br(helpers::get_colour_from_variant(colour));
	m_ptr->FillEllipse(&br, x, y, w, h);
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillGradRect(float x, float y, float w, float h, float angle, VARIANT colour1, VARIANT colour2, float focus)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::RectF rect(x, y, w, h);
	Gdiplus::LinearGradientBrush brush(rect, helpers::get_colour_from_variant(colour1), helpers::get_colour_from_variant(colour2), angle, TRUE);
	brush.SetBlendTriangularShape(focus);
	m_ptr->FillRectangle(&brush, rect);
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillPolygon(VARIANT colour, int fillmode, VARIANT points)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::SolidBrush br(helpers::get_colour_from_variant(colour));
	helpers::com_array_reader helper;
	pfc::array_t<Gdiplus::PointF> point_array;

	if (!helper.convert(&points)) return E_INVALIDARG;
	if ((helper.get_count() % 2) != 0) return E_INVALIDARG;

	point_array.set_count(helper.get_count() >> 1);

	for (long i = 0; i < static_cast<long>(point_array.get_count()); ++i)
	{
		_variant_t varX, varY;

		helper.get_item(i * 2, varX);
		helper.get_item(i * 2 + 1, varY);

		if (FAILED(VariantChangeType(&varX, &varX, 0, VT_R4))) return E_INVALIDARG;
		if (FAILED(VariantChangeType(&varY, &varY, 0, VT_R4))) return E_INVALIDARG;

		point_array[i].X = varX.fltVal;
		point_array[i].Y = varY.fltVal;
	}

	m_ptr->FillPolygon(&br, point_array.get_ptr(), point_array.get_count(), (Gdiplus::FillMode)fillmode);
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, VARIANT colour)
{
	if (!m_ptr) return E_POINTER;

	if (2 * arc_width > w || 2 * arc_height > h) return E_INVALIDARG;

	Gdiplus::SolidBrush br(helpers::get_colour_from_variant(colour));
	Gdiplus::GraphicsPath gp;
	Gdiplus::RectF rect(x, y, w, h);
	GetRoundRectPath(gp, rect, arc_width, arc_height);
	m_ptr->FillPath(&br, &gp);
	return S_OK;
}

STDMETHODIMP GdiGraphics::FillSolidRect(float x, float y, float w, float h, VARIANT colour)
{
	if (!m_ptr) return E_POINTER;

	Gdiplus::SolidBrush brush(helpers::get_colour_from_variant(colour));
	m_ptr->FillRectangle(&brush, x, y, w, h);
	return S_OK;
}

STDMETHODIMP GdiGraphics::GdiAlphaBlend(IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, BYTE alpha)
{
	if (!m_ptr) return E_POINTER;

	HDC src_dc = NULL;
	bitmap->get__Handle(&src_dc);
	HDC dc = m_ptr->GetHDC();
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA };

	::GdiAlphaBlend(dc, dstX, dstY, dstW, dstH, src_dc, srcX, srcY, srcW, srcH, bf);
	m_ptr->ReleaseHDC(dc);
	return S_OK;
}

STDMETHODIMP GdiGraphics::GdiDrawBitmap(IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH)
{
	if (!m_ptr) return E_POINTER;

	HDC src_dc = NULL;
	bitmap->get__Handle(&src_dc);
	HDC dc = m_ptr->GetHDC();

	if (dstW == srcW && dstH == srcH)
	{
		BitBlt(dc, dstX, dstY, dstW, dstH, src_dc, srcX, srcY, SRCCOPY);
	}
	else
	{
		SetStretchBltMode(dc, HALFTONE);
		SetBrushOrgEx(dc, 0, 0, NULL);
		StretchBlt(dc, dstX, dstY, dstW, dstH, src_dc, srcX, srcY, srcW, srcH, SRCCOPY);
	}

	m_ptr->ReleaseHDC(dc);
	return S_OK;
}

STDMETHODIMP GdiGraphics::GdiDrawText(BSTR str, IGdiFont* font, VARIANT colour, int x, int y, int w, int h, int format, VARIANT* p)
{
	if (!m_ptr || !p) return E_POINTER;

	HFONT hFont = NULL;
	font->get_HFont((UINT *)&hFont);
	HFONT oldfont;
	HDC dc = m_ptr->GetHDC();
	RECT rc = { x, y, x + w, y + h };
	DRAWTEXTPARAMS dpt = { sizeof(DRAWTEXTPARAMS), 4, 0, 0, 0 };

	oldfont = SelectFont(dc, hFont);
	SetTextColor(dc, helpers::convert_argb_to_colorref(helpers::get_colour_from_variant(colour)));
	SetBkMode(dc, TRANSPARENT);
	SetTextAlign(dc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

	// Remove DT_MODIFYSTRING flag
	if (format & DT_MODIFYSTRING)
		format &= ~DT_MODIFYSTRING;

	// Well, magic :P
	if (format & DT_CALCRECT)
	{
		RECT rc_calc = { 0 }, rc_old = { 0 };

		memcpy(&rc_calc, &rc, sizeof(RECT));
		memcpy(&rc_old, &rc, sizeof(RECT));

		DrawText(dc, str, -1, &rc_calc, format);

		format &= ~DT_CALCRECT;

		// adjust vertical align
		if (format & DT_VCENTER)
		{
			rc.top = rc_old.top + (((rc_old.bottom - rc_old.top) - (rc_calc.bottom - rc_calc.top)) >> 1);
			rc.bottom = rc.top + (rc_calc.bottom - rc_calc.top);
		}
		else if (format & DT_BOTTOM)
		{
			rc.top = rc_old.bottom - (rc_calc.bottom - rc_calc.top);
		}
	}

	DrawTextEx(dc, str, -1, &rc, format, &dpt);

	SelectFont(dc, oldfont);
	m_ptr->ReleaseHDC(dc);

	// Returns an VBArray:
	//   [0] left   (DT_CALCRECT)
	//   [1] top    (DT_CALCRECT)
	//   [2] right  (DT_CALCRECT)
	//   [3] bottom (DT_CALCRECT)
	//   [4] characters drawn
	const int elements[] =
	{
		rc.left,
		rc.top,
		rc.right,
		rc.bottom,
		(int)dpt.uiLengthDrawn
	};

	helpers::com_array_writer<> helper;

	if (!helper.create(_countof(elements)))
		return E_OUTOFMEMORY;

	for (long i = 0; i < helper.get_count(); ++i)
	{
		_variant_t var;
		var.vt = VT_I4;
		var.lVal = elements[i];

		if (FAILED(helper.put(i, var)))
		{
			helper.reset();
			return E_OUTOFMEMORY;
		}
	}

	p->vt = VT_ARRAY | VT_VARIANT;
	p->parray = helper.get_ptr();
	return S_OK;
}

STDMETHODIMP GdiGraphics::MeasureString(BSTR str, IGdiFont* font, float x, float y, float w, float h, int flags, IMeasureStringInfo** pp)
{
	if (!m_ptr || !pp) return E_POINTER;

	Gdiplus::Font* fn = NULL;
	font->get__ptr((void**)&fn);

	Gdiplus::StringFormat fmt = Gdiplus::StringFormat::GenericTypographic();

	if (flags != 0)
	{
		fmt.SetAlignment((Gdiplus::StringAlignment)((flags >> 28) & 0x3)); //0xf0000000
		fmt.SetLineAlignment((Gdiplus::StringAlignment)((flags >> 24) & 0x3)); //0x0f000000
		fmt.SetTrimming((Gdiplus::StringTrimming)((flags >> 20) & 0x7)); //0x00f00000
		fmt.SetFormatFlags((Gdiplus::StringFormatFlags)(flags & 0x7FFF)); //0x0000ffff
	}

	Gdiplus::RectF bound;
	int chars, lines;

	m_ptr->MeasureString(str, -1, fn, Gdiplus::RectF(x, y, w, h), &fmt, &bound, &chars, &lines);

	*pp = new com_object_impl_t<MeasureStringInfo>(bound.X, bound.Y, bound.Width, bound.Height, lines, chars);
	return S_OK;
}

STDMETHODIMP GdiGraphics::SetInterpolationMode(INT mode)
{
	if (!m_ptr) return E_POINTER;

	m_ptr->SetInterpolationMode((Gdiplus::InterpolationMode)mode);
	return S_OK;
}

STDMETHODIMP GdiGraphics::SetSmoothingMode(INT mode)
{
	if (!m_ptr) return E_POINTER;

	m_ptr->SetSmoothingMode((Gdiplus::SmoothingMode)mode);
	return S_OK;
}

STDMETHODIMP GdiGraphics::SetTextRenderingHint(UINT mode)
{
	if (!m_ptr) return E_POINTER;

	m_ptr->SetTextRenderingHint((Gdiplus::TextRenderingHint)mode);
	return S_OK;
}

STDMETHODIMP GdiGraphics::put__ptr(void* p)
{
	m_ptr = (Gdiplus::Graphics *)p;
	return S_OK;
}

GdiRawBitmap::GdiRawBitmap(Gdiplus::Bitmap* p_bmp)
{
	PFC_ASSERT(p_bmp != NULL);
	m_width = p_bmp->GetWidth();
	m_height = p_bmp->GetHeight();

	m_hdc = CreateCompatibleDC(NULL);
	m_hbmp = helpers::create_hbitmap_from_gdiplus_bitmap(p_bmp);
	m_hbmpold = SelectBitmap(m_hdc, m_hbmp);
}

GdiRawBitmap::~GdiRawBitmap()
{
}

void GdiRawBitmap::FinalRelease()
{
	if (m_hdc)
	{
		SelectBitmap(m_hdc, m_hbmpold);
		DeleteDC(m_hdc);
		m_hdc = NULL;
	}

	if (m_hbmp)
	{
		DeleteBitmap(m_hbmp);
		m_hbmp = NULL;
	}
}

STDMETHODIMP GdiRawBitmap::get_Height(UINT* p)
{
	if (!m_hdc || !p) return E_POINTER;

	*p = m_height;
	return S_OK;
}

STDMETHODIMP GdiRawBitmap::get_Width(UINT* p)
{
	if (!m_hdc || !p) return E_POINTER;

	*p = m_width;
	return S_OK;
}

STDMETHODIMP GdiRawBitmap::get__Handle(HDC* p)
{
	if (!m_hdc || !p) return E_POINTER;

	*p = m_hdc;
	return S_OK;
}

GdiUtils::GdiUtils()
{
}

GdiUtils::~GdiUtils()
{
}

STDMETHODIMP GdiUtils::CreateImage(int w, int h, IGdiBitmap** pp)
{
	if (!pp) return E_POINTER;

	Gdiplus::Bitmap* img = new Gdiplus::Bitmap(w, h, PixelFormat32bppPARGB);
	if (helpers::ensure_gdiplus_object(img))
	{
		*pp = new com_object_impl_t<GdiBitmap>(img);
	}
	else
	{
		if (img) delete img;
		*pp = NULL;
	}

	return S_OK;
}

STDMETHODIMP GdiUtils::Font(BSTR name, float pxSize, int style, IGdiFont** pp)
{
	if (!pp) return E_POINTER;

	Gdiplus::Font* font = new Gdiplus::Font(name, pxSize, style, Gdiplus::UnitPixel);
	if (helpers::ensure_gdiplus_object(font))
	{
		// Generate HFONT
		// The benefit of replacing Gdiplus::Font::GetLogFontW is that you can get it work with CCF/OpenType fonts.
		HFONT hFont = CreateFont(
			-(int)pxSize,
			0,
			0,
			0,
			(style & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL,
			(style & Gdiplus::FontStyleItalic) ? TRUE : FALSE,
			(style & Gdiplus::FontStyleUnderline) ? TRUE : FALSE,
			(style & Gdiplus::FontStyleStrikeout) ? TRUE : FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			name);
		*pp = new com_object_impl_t<GdiFont>(font, hFont);
	}
	else
	{
		if (font) delete font;
		*pp = NULL;
	}

	return S_OK;
}

STDMETHODIMP GdiUtils::Image(BSTR path, IGdiBitmap** pp)
{
	if (!pp) return E_POINTER;
	*pp = NULL;

	// Since using Gdiplus::Bitmap(path) will result locking file, so use IStream instead to prevent it.
	IStreamPtr pStream;
	HRESULT hr = SHCreateStreamOnFileEx(path, STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, FALSE, NULL, &pStream);
	if (SUCCEEDED(hr))
	{
		Gdiplus::Bitmap* img = new Gdiplus::Bitmap(pStream, PixelFormat32bppPARGB);
		if (helpers::ensure_gdiplus_object(img))
		{
			*pp = new com_object_impl_t<GdiBitmap>(img);
		}
		else
		{
			if (img) delete img;	
		}
	}
	
	return S_OK;
}

STDMETHODIMP GdiUtils::LoadImageAsync(UINT window_id, BSTR path, UINT* p)
{
	if (!p) return E_POINTER;

	unsigned cookie = 0;

	try
	{
		helpers::load_image_async* task = new helpers::load_image_async((HWND)window_id, path);

		if (simple_thread_pool::instance().enqueue(task))
			cookie = reinterpret_cast<unsigned>(task);
		else
			delete task;
	}
	catch (std::exception&)
	{
	}

	*p = cookie;
	return S_OK;
}

JSConsole::JSConsole()
{
}

JSConsole::~JSConsole()
{
}

STDMETHODIMP JSConsole::Log(SAFEARRAY* p)
{
	pfc::string8_fast str;
	LONG nLBound = 0, nUBound = -1;
	HRESULT hr;

	if (FAILED(hr = SafeArrayGetLBound(p, 1, &nLBound)))
		return hr;

	if (FAILED(hr = SafeArrayGetUBound(p, 1, &nUBound)))
		return hr;

	for (LONG i = nLBound; i <= nUBound; ++i)
	{
		_variant_t var;
		LONG n = i;

		if (FAILED(SafeArrayGetElement(p, &n, &var)))
			continue;

		if (FAILED(hr = VariantChangeType(&var, &var, VARIANT_ALPHABOOL, VT_BSTR)))
			continue;

		str.add_string(pfc::stringcvt::string_utf8_from_wide(var.bstrVal));

		if (i < nUBound)
		{
			str.add_byte(' ');
		}
	}

	console::info(str);
	return S_OK;
}

JSUtils::JSUtils()
{
}

JSUtils::~JSUtils()
{
}

STDMETHODIMP JSUtils::CheckComponent(BSTR name, VARIANT_BOOL is_dll, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	service_enum_t<componentversion> e;
	service_ptr_t<componentversion> ptr;
	pfc::string8_fast uname = pfc::stringcvt::string_utf8_from_wide(name);
	pfc::string8_fast temp;

	*p = VARIANT_FALSE;

	while (e.next(ptr))
	{
		if (is_dll)
			ptr->get_file_name(temp);
		else
			ptr->get_component_name(temp);

		if (!_stricmp(temp, uname))
		{
			*p = VARIANT_TRUE;
			break;
		}
	}

	return S_OK;
}

STDMETHODIMP JSUtils::CheckFont(BSTR name, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	WCHAR family_name_eng[LF_FACESIZE] = { 0 };
	WCHAR family_name_loc[LF_FACESIZE] = { 0 };
	Gdiplus::InstalledFontCollection font_collection;
	Gdiplus::FontFamily* font_families;
	int count = font_collection.GetFamilyCount();
	int recv;

	*p = VARIANT_FALSE;
	font_families = new Gdiplus::FontFamily[count];
	font_collection.GetFamilies(count, font_families, &recv);

	if (recv == count)
	{
		// Find
		for (int i = 0; i < count; ++i)
		{
			font_families[i].GetFamilyName(family_name_eng, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			font_families[i].GetFamilyName(family_name_loc);

			if (_wcsicmp(name, family_name_loc) == 0 || _wcsicmp(name, family_name_eng) == 0)
			{
				*p = VARIANT_TRUE;
				break;
			}
		}
	}

	delete[] font_families;
	return S_OK;
}

STDMETHODIMP JSUtils::ColourPicker(UINT window_id, int default_colour, int* out_colour)
{
	if (!out_colour) return E_POINTER;

	COLORREF COLOR = helpers::convert_argb_to_colorref(default_colour);
	COLORREF COLORS[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uChooseColor(&COLOR, (HWND)window_id, &COLORS[0]);
	*out_colour = helpers::convert_colorref_to_argb(COLOR);
	return S_OK;
}

STDMETHODIMP JSUtils::FileTest(BSTR path, BSTR mode, VARIANT* p)
{
	if (!p) return E_POINTER;

	if (wcscmp(mode, L"e") == 0) // exists
	{
		p->vt = VT_BOOL;
		p->boolVal = TO_VARIANT_BOOL(PathFileExists(path));
	}
	else if (wcscmp(mode, L"s") == 0)
	{
		HANDLE fh = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		LARGE_INTEGER size = { 0 };

		if (fh != INVALID_HANDLE_VALUE)
		{
			GetFileSizeEx(fh, &size);
			CloseHandle(fh);
		}

		// Only 32bit integers...
		p->vt = VT_UI4;
		p->ulVal = (size.HighPart) ? pfc::infinite32 : size.LowPart;
	}
	else if (wcscmp(mode, L"d") == 0)
	{
		p->vt = VT_BOOL;
		p->boolVal = TO_VARIANT_BOOL(PathIsDirectory(path));
	}
	else if (wcscmp(mode, L"split") == 0)
	{
		const wchar_t* fn = PathFindFileName(path);
		const wchar_t* ext = PathFindExtension(fn);
		wchar_t dir[MAX_PATH] = { 0 };
		helpers::com_array_writer<> helper;
		_variant_t vars[3];

		if (!helper.create(_countof(vars))) return E_OUTOFMEMORY;

		vars[0].vt = VT_BSTR;
		vars[0].bstrVal = NULL;
		vars[1].vt = VT_BSTR;
		vars[1].bstrVal = NULL;
		vars[2].vt = VT_BSTR;
		vars[2].bstrVal = NULL;

		if (PathIsFileSpec(fn))
		{
			StringCchCopyN(dir, _countof(dir), path, fn - path);
			PathAddBackslash(dir);

			vars[0].bstrVal = SysAllocString(dir);
			vars[1].bstrVal = SysAllocStringLen(fn, ext - fn);
			vars[2].bstrVal = SysAllocString(ext);
		}
		else
		{
			StringCchCopy(dir, _countof(dir), path);
			PathAddBackslash(dir);

			vars[0].bstrVal = SysAllocString(dir);
		}

		for (long i = 0; i < helper.get_count(); ++i)
		{
			if (FAILED(helper.put(i, vars[i])))
			{
				helper.reset();
				return E_OUTOFMEMORY;
			}
		}

		p->vt = VT_VARIANT | VT_ARRAY;
		p->parray = helper.get_ptr();
	}
	else if (wcscmp(mode, L"chardet") == 0)
	{
		p->vt = VT_UI4;
		p->ulVal = helpers::detect_charset(pfc::stringcvt::string_utf8_from_wide(path));
	}
	else
	{
		return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP JSUtils::FormatDuration(double p, BSTR* pp)
{
	if (!pp) return E_POINTER;

	pfc::string8_fast str;
	str = pfc::format_time_ex(p, 0);
	*pp = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(str));
	return S_OK;
}

STDMETHODIMP JSUtils::FormatFileSize(LONGLONG p, BSTR* pp)
{
	if (!pp) return E_POINTER;

	pfc::string8_fast str;
	str = pfc::format_file_size_short(p);
	*pp = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(str));
	return S_OK;
}

STDMETHODIMP JSUtils::GetAlbumArtAsync(UINT window_id, IFbMetadbHandle* handle, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL only_embed, VARIANT_BOOL no_load, UINT* p)
{
	if (!p) return E_POINTER;

	unsigned cookie = 0;
	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);

	if (ptr)
	{
		try
		{
			helpers::album_art_async* task = new helpers::album_art_async((HWND)window_id, ptr, art_id, need_stub, only_embed, no_load);

			if (simple_thread_pool::instance().enqueue(task))
				cookie = reinterpret_cast<unsigned>(task);
			else
				delete task;
		}
		catch (std::exception&)
		{
			cookie = 0;
		}
	}
	else
	{
		cookie = 0;
	}

	*p = cookie;
	return S_OK;
}

STDMETHODIMP JSUtils::GetAlbumArtEmbedded(BSTR rawpath, int art_id, IGdiBitmap** pp)
{
	if (!pp) return E_POINTER;

	return helpers::get_album_art_embedded(rawpath, pp, art_id);
}

STDMETHODIMP JSUtils::GetAlbumArtV2(IFbMetadbHandle* handle, int art_id, VARIANT_BOOL need_stub, IGdiBitmap** pp)
{
	if (!pp) return E_POINTER;

	metadb_handle* ptr = NULL;
	handle->get__ptr((void**)&ptr);
	return helpers::get_album_art_v2(ptr, pp, art_id, need_stub);
}

STDMETHODIMP JSUtils::GetSysColour(UINT index, int* p)
{
	if (!p) return E_POINTER;

	int ret = ::GetSysColor(index);

	if (!ret)
	{
		if (!::GetSysColorBrush(index))
		{
			// invalid
			*p = 0;
			return S_OK;
		}
	}

	*p = helpers::convert_colorref_to_argb(ret);
	return S_OK;
}

STDMETHODIMP JSUtils::GetSystemMetrics(UINT index, int* p)
{
	if (!p) return E_POINTER;

	*p = ::GetSystemMetrics(index);
	return S_OK;
}

STDMETHODIMP JSUtils::Glob(BSTR pattern, UINT exc_mask, UINT inc_mask, VARIANT* p)
{
	if (!p) return E_POINTER;

	pfc::string8_fast path = pfc::stringcvt::string_utf8_from_wide(pattern);
	const char* fn = path.get_ptr() + path.scan_filename();
	pfc::string8_fast dir(path.get_ptr(), fn - path.get_ptr());
	puFindFile ff = uFindFirstFile(path.get_ptr());

	pfc::string_list_impl files;

	if (ff)
	{
		do
		{
			DWORD attr = ff->GetAttributes();

			if ((attr & inc_mask) && !(attr & exc_mask))
			{
				pfc::string8_fast fullpath = dir;
				fullpath.add_string(ff->GetFileName());
				files.add_item(fullpath.get_ptr());
			}
		} while (ff->FindNext());
	}

	delete ff;
	ff = NULL;

	helpers::com_array_writer<> helper;

	if (!helper.create(files.get_count())) return E_OUTOFMEMORY;

	for (long i = 0; i < helper.get_count(); ++i)
	{
		_variant_t var;
		var.vt = VT_BSTR;
		var.bstrVal = SysAllocString(pfc::stringcvt::string_wide_from_utf8_fast(files[i]).get_ptr());

		if (FAILED(helper.put(i, var)))
		{
			// deep destroy
			helper.reset();
			return E_OUTOFMEMORY;
		}
	}

	p->vt = VT_ARRAY | VT_VARIANT;
	p->parray = helper.get_ptr();
	return S_OK;
}

STDMETHODIMP JSUtils::IsKeyPressed(UINT vkey, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(::IsKeyPressed(vkey));
	return S_OK;
}

STDMETHODIMP JSUtils::MapString(BSTR str, UINT lcid, UINT flags, BSTR* pp)
{
	if (!pp) return E_POINTER;

	int r = ::LCMapStringW(lcid, flags, str, wcslen(str) + 1, NULL, 0);
	if (!r) return E_FAIL;
	wchar_t* dst = new wchar_t[r];
	r = ::LCMapStringW(lcid, flags, str, wcslen(str) + 1, dst, r);
	if (r) *pp = SysAllocString(dst);
	delete dst;
	return S_OK;
}

STDMETHODIMP JSUtils::PathWildcardMatch(BSTR pattern, BSTR str, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(PathMatchSpec(str, pattern));
	return S_OK;
}

STDMETHODIMP JSUtils::ReadINI(BSTR filename, BSTR section, BSTR key, VARIANT defaultval, BSTR* pp)
{
	if (!pp) return E_POINTER;

	enum
	{
		BUFFER_LEN = 255
	};
	TCHAR buff[BUFFER_LEN] = { 0 };

	GetPrivateProfileString(section, key, NULL, buff, BUFFER_LEN, filename);

	if (!buff[0])
	{
		_variant_t var;

		if (SUCCEEDED(VariantChangeType(&var, &defaultval, 0, VT_BSTR)))
		{
			*pp = SysAllocString(var.bstrVal);
			return S_OK;
		}
	}

	*pp = SysAllocString(buff);
	return S_OK;
}

STDMETHODIMP JSUtils::ReadTextFile(BSTR filename, UINT codepage, BSTR* pp)
{
	if (!pp) return E_POINTER;

	pfc::array_t<wchar_t> content;
	*pp = NULL;

	if (helpers::read_file_wide(codepage, filename, content))
	{
		*pp = SysAllocString(content.get_ptr());
	}

	return S_OK;
}

STDMETHODIMP JSUtils::WriteINI(BSTR filename, BSTR section, BSTR key, VARIANT val, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	_variant_t var;
	HRESULT hr;
	if (FAILED(hr = VariantChangeType(&var, &val, 0, VT_BSTR))) return hr;
	*p = TO_VARIANT_BOOL(WritePrivateProfileString(section, key, var.bstrVal, filename));
	return S_OK;
}

STDMETHODIMP JSUtils::WriteTextFile(BSTR filename, BSTR content, VARIANT_BOOL* p)
{
	if (!p) return E_POINTER;

	try
	{
		pfc::string8_fast filename8 = pfc::stringcvt::string_utf8_from_wide(filename);
		pfc::string8_fast content8 = pfc::stringcvt::string_utf8_from_wide(content);
		*p = TO_VARIANT_BOOL(helpers::write_file(filename8, content8));
	}
	catch (...)
	{
		*p = VARIANT_FALSE;
	}
	return S_OK;
}

STDMETHODIMP JSUtils::get_Version(UINT* v)
{
	if (!v) return E_POINTER;

	*v = 2000;
	return S_OK;
}

MainMenuManager::MainMenuManager()
{
}

MainMenuManager::~MainMenuManager()
{
}

void MainMenuManager::FinalRelease()
{
	m_mm.release();
}

STDMETHODIMP MainMenuManager::BuildMenu(IMenuObj* p, int base_id, int count)
{
	if (m_mm.is_empty()) return E_POINTER;

	UINT menuid;
	p->get_ID(&menuid);

	// HACK: workaround for foo_menu_addons
	try
	{
		m_mm->generate_menu_win32((HMENU)menuid, base_id, count, mainmenu_manager::flag_show_shortcuts);
	}
	catch (...)
	{
	}

	return S_OK;
}

STDMETHODIMP MainMenuManager::ExecuteByID(UINT id, VARIANT_BOOL* p)
{
	if (m_mm.is_empty() || !p) return E_POINTER;

	*p = TO_VARIANT_BOOL(m_mm->execute_command(id));
	return S_OK;
}

STDMETHODIMP MainMenuManager::Init(BSTR root_name)
{
	struct t_valid_root_name
	{
		const wchar_t* name;
		const GUID* guid;
	};

	// In mainmenu_groups:
	// static const GUID file,view,edit,playback,library,help;
	const t_valid_root_name valid_root_names[] =
	{
		{L"file", &mainmenu_groups::file},
		{L"view", &mainmenu_groups::view},
		{L"edit", &mainmenu_groups::edit},
		{L"playback", &mainmenu_groups::playback},
		{L"library", &mainmenu_groups::library},
		{L"help", &mainmenu_groups::help},
	};

	// Find
	for (int i = 0; i < _countof(valid_root_names); ++i)
	{
		if (_wcsicmp(root_name, valid_root_names[i].name) == 0)
		{
			// found
			m_mm = standard_api_create_t<mainmenu_manager>();
			m_mm->instantiate(*valid_root_names[i].guid);
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

MeasureStringInfo::MeasureStringInfo(float x, float y, float w, float h, int l, int c) : m_x(x), m_y(y), m_w(w), m_h(h), m_l(l), m_c(c)
{
}

MeasureStringInfo::~MeasureStringInfo()
{
}

STDMETHODIMP MeasureStringInfo::get_chars(int* p)
{
	if (!p) return E_POINTER;

	*p = m_c;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_height(float* p)
{
	if (!p) return E_POINTER;

	*p = m_h;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_lines(int* p)
{
	if (!p) return E_POINTER;

	*p = m_l;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_width(float* p)
{
	if (!p) return E_POINTER;

	*p = m_w;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_x(float* p)
{
	if (!p) return E_POINTER;

	*p = m_x;
	return S_OK;
}

STDMETHODIMP MeasureStringInfo::get_y(float* p)
{
	if (!p) return E_POINTER;

	*p = m_y;
	return S_OK;
}

MenuObj::MenuObj(HWND wnd_parent) : m_wnd_parent(wnd_parent), m_has_detached(false)
{
	m_hMenu = ::CreatePopupMenu();
}

MenuObj::~MenuObj()
{
}

void MenuObj::FinalRelease()
{
	if (!m_has_detached && m_hMenu && IsMenu(m_hMenu))
	{
		DestroyMenu(m_hMenu);
		m_hMenu = NULL;
	}
}

STDMETHODIMP MenuObj::AppendMenuItem(UINT flags, UINT item_id, BSTR text)
{
	if (!m_hMenu) return E_POINTER;
	if ((flags & MF_STRING) && !text) return E_INVALIDARG;
	if (flags & MF_POPUP) return E_INVALIDARG;

	::AppendMenu(m_hMenu, flags, item_id, text);
	return S_OK;
}

STDMETHODIMP MenuObj::AppendMenuSeparator()
{
	if (!m_hMenu) return E_POINTER;

	::AppendMenu(m_hMenu, MF_SEPARATOR, 0, 0);
	return S_OK;
}

STDMETHODIMP MenuObj::AppendTo(IMenuObj* parent, UINT flags, BSTR text)
{
	if (!m_hMenu) return E_POINTER;

	MenuObj* pMenuParent = static_cast<MenuObj *>(parent);
	if (::AppendMenu(pMenuParent->m_hMenu, flags | MF_STRING | MF_POPUP, UINT_PTR(m_hMenu), text))
	{
		m_has_detached = true;
	}
	return S_OK;
}

STDMETHODIMP MenuObj::CheckMenuItem(UINT id_or_pos, VARIANT_BOOL check, VARIANT_BOOL bypos)
{
	if (!m_hMenu) return E_POINTER;

	UINT ucheck = bypos ? MF_BYPOSITION : MF_BYCOMMAND;
	if (check)
	{
		ucheck = MF_CHECKED;
	}
	::CheckMenuItem(m_hMenu, id_or_pos, ucheck);
	return S_OK;
}

STDMETHODIMP MenuObj::CheckMenuRadioItem(UINT first, UINT last, UINT check, VARIANT_BOOL bypos)
{
	if (!m_hMenu) return E_POINTER;

	::CheckMenuRadioItem(m_hMenu, first, last, check, bypos ? MF_BYPOSITION : MF_BYCOMMAND);
	return S_OK;
}

STDMETHODIMP MenuObj::EnableMenuItem(UINT id_or_pos, UINT enable, VARIANT_BOOL bypos)
{
	if (!m_hMenu) return E_POINTER;

	enable &= ~(MF_BYPOSITION | MF_BYCOMMAND);
	enable |= bypos ? MF_BYPOSITION : MF_BYCOMMAND;

	::EnableMenuItem(m_hMenu, id_or_pos, enable);
	return S_OK;
}

STDMETHODIMP MenuObj::TrackPopupMenu(int x, int y, UINT flags, UINT* item_id)
{
	if (!m_hMenu || !item_id) return E_POINTER;

	POINT pt = { x, y };

	// Only include specified flags
	flags |= TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON;
	flags &= ~TPM_RECURSE;

	ClientToScreen(m_wnd_parent, &pt);
	*item_id = ::TrackPopupMenu(m_hMenu, flags, pt.x, pt.y, 0, m_wnd_parent, 0);
	return S_OK;
}

STDMETHODIMP MenuObj::get_ID(UINT* p)
{
	if (!m_hMenu || !p) return E_POINTER;

	*p = (UINT)m_hMenu;
	return S_OK;
}

ThemeManager::ThemeManager(HWND hwnd, BSTR classlist) : m_theme(NULL), m_partid(0), m_stateid(0)
{
	m_theme = OpenThemeData(hwnd, classlist);

	if (!m_theme) throw pfc::exception_invalid_params();
}

ThemeManager::~ThemeManager()
{
}

void ThemeManager::FinalRelease()
{
	if (m_theme)
	{
		CloseThemeData(m_theme);
		m_theme = NULL;
	}
}

STDMETHODIMP ThemeManager::DrawThemeBackground(IGdiGraphics* gr, int x, int y, int w, int h, int clip_x, int clip_y, int clip_w, int clip_h)
{
	if (!m_theme) return E_POINTER;

	Gdiplus::Graphics* graphics = NULL;
	gr->get__ptr((void**)&graphics);

	RECT rc = { x, y, x + w, y + h };
	RECT clip_rc = { clip_x, clip_y, clip_x + clip_w, clip_y + clip_h };
	LPCRECT pclip_rc = &clip_rc;
	HDC dc = graphics->GetHDC();

	if (clip_x == 0 && clip_y == 0 && clip_w == 0 && clip_h == 0)
	{
		pclip_rc = NULL;
	}

	HRESULT hr = ::DrawThemeBackground(m_theme, dc, m_partid, m_stateid, &rc, pclip_rc);

	graphics->ReleaseHDC(dc);
	return hr;
}

STDMETHODIMP ThemeManager::IsThemePartDefined(int partid, int stateid, VARIANT_BOOL* p)
{
	if (!m_theme || !p) return E_POINTER;

	*p = TO_VARIANT_BOOL(::IsThemePartDefined(m_theme, partid, stateid));
	return S_OK;
}

STDMETHODIMP ThemeManager::SetPartAndStateID(int partid, int stateid)
{
	if (!m_theme) return E_POINTER;

	m_partid = partid;
	m_stateid = stateid;
	return S_OK;
}
