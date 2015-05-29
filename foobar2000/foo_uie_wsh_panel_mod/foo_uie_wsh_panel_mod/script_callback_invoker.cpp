#include "stdafx.h"
#include "script_callback_invoker.h"


struct IDToNameEntry 
{
	int id;
	const wchar_t *name;
};

#define _STRINGIFY(x)       #x
#define _TOSTRING(x)        _STRINGIFY(x)
#define DEFINE_ID_NAME_MAP_ENTRY(x)    { CallbackIds::##x, PFC_WIDESTRING(_STRINGIFY(x)) }

static const IDToNameEntry g_idToNames[] = 
{
	DEFINE_ID_NAME_MAP_ENTRY(on_drag_enter),
	DEFINE_ID_NAME_MAP_ENTRY(on_drag_over),
	DEFINE_ID_NAME_MAP_ENTRY(on_drag_leave),
	DEFINE_ID_NAME_MAP_ENTRY(on_drag_drop),
	DEFINE_ID_NAME_MAP_ENTRY(on_key_down),
	DEFINE_ID_NAME_MAP_ENTRY(on_key_up),
	DEFINE_ID_NAME_MAP_ENTRY(on_char),
	DEFINE_ID_NAME_MAP_ENTRY(on_focus),
	DEFINE_ID_NAME_MAP_ENTRY(on_size),
	DEFINE_ID_NAME_MAP_ENTRY(on_paint),
	DEFINE_ID_NAME_MAP_ENTRY(on_timer),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_wheel),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_leave),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_move),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_lbtn_dblclk),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_mbtn_dblclk),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_rbtn_dblclk),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_lbtn_up),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_mbtn_up),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_rbtn_up),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_lbtn_down),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_mbtn_down),
	DEFINE_ID_NAME_MAP_ENTRY(on_mouse_rbtn_down),
	DEFINE_ID_NAME_MAP_ENTRY(on_tooltip_custom_paint),
	DEFINE_ID_NAME_MAP_ENTRY(on_refresh_background_done),
	DEFINE_ID_NAME_MAP_ENTRY(on_item_played),
	DEFINE_ID_NAME_MAP_ENTRY(on_get_album_art_done),
	DEFINE_ID_NAME_MAP_ENTRY(on_load_image_done),
	DEFINE_ID_NAME_MAP_ENTRY(on_playlist_stop_after_current_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_cursor_follow_playback_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_follow_cursor_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_notify_data),
	DEFINE_ID_NAME_MAP_ENTRY(on_font_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_colors_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_starting),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_new_track),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_stop),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_seek),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_pause),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_edited),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_dynamic_info),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_dynamic_info_track),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_time),
	DEFINE_ID_NAME_MAP_ENTRY(on_volume_change),
	DEFINE_ID_NAME_MAP_ENTRY(on_item_focus_change),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_order_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_playlist_switch),
	DEFINE_ID_NAME_MAP_ENTRY(on_playlists_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_playlist_items_added),
	DEFINE_ID_NAME_MAP_ENTRY(on_playlist_items_reordered),
	DEFINE_ID_NAME_MAP_ENTRY(on_playlist_items_removed),
	DEFINE_ID_NAME_MAP_ENTRY(on_playlist_items_selection_change),
	DEFINE_ID_NAME_MAP_ENTRY(on_playlist_item_ensure_visible),
	DEFINE_ID_NAME_MAP_ENTRY(on_metadb_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_selection_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_playback_queue_changed),
	DEFINE_ID_NAME_MAP_ENTRY(on_script_unload),
};

ScriptCallbackInvoker::ScriptCallbackInvoker()
{
}

ScriptCallbackInvoker::~ScriptCallbackInvoker()
{
	reset();
}

void ScriptCallbackInvoker::init(IDispatch * pActiveScriptRoot)
{
	reset();
	if (!pActiveScriptRoot) 
		return;

	m_activeScriptRoot = pActiveScriptRoot;
	int count = _countof(g_idToNames);

	for (int i = 0; i < count; i++)
	{
		int callbackId = g_idToNames[i].id;
		LPOLESTR name = const_cast<LPOLESTR>(g_idToNames[i].name);
		DISPID dispId;
		HRESULT hr = m_activeScriptRoot->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispId);
		if (SUCCEEDED(hr))
		{
			m_callbackInvokerMap[callbackId] = dispId;
		}
	}
}

HRESULT ScriptCallbackInvoker::invoke(int callbackId, VARIANTARG * argv /*= NULL*/, UINT argc /*= 0*/, VARIANT * ret /*= NULL*/)
{
	if (!m_activeScriptRoot) return E_POINTER;
	DISPPARAMS param = { argv, NULL, argc, 0 };
	int dispId;
	if (!m_callbackInvokerMap.query(callbackId, dispId)) return DISP_E_MEMBERNOTFOUND;
	if (dispId == DISPID_UNKNOWN) return DISP_E_MEMBERNOTFOUND;
	return m_activeScriptRoot->Invoke(dispId, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &param, ret, NULL, NULL);
}
