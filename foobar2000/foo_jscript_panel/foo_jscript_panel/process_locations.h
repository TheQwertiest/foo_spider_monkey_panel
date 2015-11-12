#pragma once

#include "stdafx.h"

class js_process_locations : public process_locations_notify
{
public:
	js_process_locations(int playlist_idx, bool to_select)
		: m_playlist_idx(playlist_idx), m_to_select(to_select) {}

	void on_completion(const pfc::list_base_const_t<metadb_handle_ptr> & p_items)
	{
		bit_array_true selection_them;
		bit_array_false selection_none;
		bit_array * select_ptr = &selection_them;
		static_api_ptr_t<playlist_manager> pm;
		t_size playlist;

		if (m_playlist_idx == -1)
			playlist = pm->get_active_playlist();
		else
			playlist = m_playlist_idx;

		if (!m_to_select)
			select_ptr = &selection_none;

		if (playlist != pfc_infinite && playlist < pm->get_playlist_count())
		{
			pm->playlist_add_items(playlist, p_items, *select_ptr);
		}
	}
	void on_aborted() {}

private:
	bool m_to_select;
	int m_playlist_idx;
};
