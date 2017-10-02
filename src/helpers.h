#pragma once

#include "thread_pool.h"
#include "script_interface.h"
#include "json.hpp"

#define TO_VARIANT_BOOL(v) ((v) ? (VARIANT_TRUE) : (VARIANT_FALSE))

using json = nlohmann::json;

namespace helpers
{
	HBITMAP create_hbitmap_from_gdiplus_bitmap(Gdiplus::Bitmap* bitmap_ptr);
	int get_encoder_clsid(const WCHAR* format, CLSID* pClsid);
	bool execute_context_command_by_name(const char* p_name, metadb_handle_list_cref p_handles, unsigned flags);
	bool execute_mainmenu_command_by_name(const char* p_name);
	unsigned detect_charset(const char* fileName);

	inline pfc::string8 iterator_to_string8(json::iterator j)
	{
		std::string value = j.value().type() == json::value_t::string ? j.value().get<std::string>() : j.value().dump();
		return value.c_str();
	}

	inline int get_metadb_from_variant(const VARIANT& obj, IDispatch** ppuk)
	{
		if (obj.vt != VT_DISPATCH || !obj.pdispVal)
			return -1;

		IDispatch* temp = NULL;

		if (SUCCEEDED(obj.pdispVal->QueryInterface(__uuidof(IFbMetadbHandle), (void**)&temp)))
		{
			*ppuk = temp;
			return 0;
		}
		else if (SUCCEEDED(obj.pdispVal->QueryInterface(__uuidof(IFbMetadbHandleList), (void**)&temp)))
		{
			*ppuk = temp;
			return 1;
		}

		return -1;
	}

	inline unsigned get_color_from_variant(VARIANT v)
	{
		return (v.vt == VT_R8) ? static_cast<unsigned>(v.dblVal) : v.lVal;
	}

	inline int get_text_width(HDC hdc, LPCTSTR text, int len)
	{
		SIZE size;

		GetTextExtentPoint32(hdc, text, len, &size);
		return size.cx;
	}

	inline int get_text_height(HDC hdc, const wchar_t* text, int len)
	{
		SIZE size;
		GetTextExtentPoint32(hdc, text, len, &size);
		return size.cy;
	}

	inline int is_wrap_char(wchar_t current, wchar_t next)
	{
		if (iswpunct(current))
			return false;

		if (next == '\0')
			return true;

		if (iswspace(current))
			return true;

		int currentAlphaNum = iswalnum(current);

		if (currentAlphaNum)
		{
			if (iswpunct(next))
				return false;
		}

		return currentAlphaNum == 0 || iswalnum(next) == 0;
	}

	struct wrapped_item
	{
		BSTR text;
		int width;
	};

	extern void estimate_line_wrap(HDC hdc, const wchar_t* text, int len, int width, pfc::list_t<wrapped_item>& out);

	__declspec(noinline) static bool execute_context_command_by_name_SEH(const char* p_name, metadb_handle_list_cref p_handles, unsigned flags)
	{
		bool ret = false;

		__try
		{
			ret = execute_context_command_by_name(p_name, p_handles, flags);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			ret = false;
		}

		return ret;
	}

	__declspec(noinline) static bool execute_mainmenu_command_by_name_SEH(const char* p_name)
	{
		bool ret = false;

		__try
		{
			ret = execute_mainmenu_command_by_name(p_name);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			ret = false;
		}

		return ret;
	}

	inline COLORREF convert_argb_to_colorref(DWORD argb)
	{
		return RGB(argb >> RED_SHIFT, argb >> GREEN_SHIFT, argb >> BLUE_SHIFT);
	}

	inline DWORD convert_colorref_to_argb(DWORD color)
	{
		// COLORREF : 0x00bbggrr
		// ARGB : 0xaarrggbb
		return (GetRValue(color) << RED_SHIFT) |
			(GetGValue(color) << GREEN_SHIFT) |
			(GetBValue(color) << BLUE_SHIFT) |
			0xff000000;
	}

	int int_from_hex_digit(int ch);
	int int_from_hex_byte(const char* hex_byte);

	template <class T>
	bool ensure_gdiplus_object(T* obj)
	{
		return ((obj) && (obj->GetLastStatus() == Gdiplus::Ok));
	}

	const GUID convert_artid_to_guid(int art_id);
	// bitmap must be NULL
	bool read_album_art_into_bitmap(const album_art_data_ptr& data, Gdiplus::Bitmap** bitmap);
	HRESULT get_album_art_v2(const metadb_handle_ptr& handle, IGdiBitmap** pp, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL no_load = VARIANT_FALSE, pfc::string_base* image_path_ptr = NULL);
	HRESULT get_album_art_embedded(BSTR rawpath, IGdiBitmap** pp, int art_id);

	static pfc::string8_fast get_fb2k_path()
	{
		pfc::string8_fast path;

		uGetModuleFileName(NULL, path);
		path = pfc::string_directory(path);
		path.add_string("\\");

		return path;
	}

	static pfc::string8_fast get_fb2k_component_path()
	{
		pfc::string8_fast path;

		uGetModuleFileName(core_api::get_my_instance(), path);
		path = pfc::string_directory(path);
		path.add_char('\\');
		return path;
	}

	static pfc::string8_fast get_profile_path()
	{
		pfc::string8_fast path;

		path = file_path_display(core_api::get_profile_path());
		path.fix_dir_separator('\\');

		return path;
	}

	// File r/w
	bool read_file(const char* path, pfc::string_base& content);
	bool read_file_wide(unsigned codepage, const wchar_t* path, pfc::array_t<wchar_t>& content);
	// Always save as UTF8 BOM
	bool write_file(const char* path, const pfc::string_base& content);

	class album_art_async : public simple_thread_task
	{
	public:
		struct t_param
		{
			IFbMetadbHandle* handle;
			int art_id;
			IGdiBitmap* bitmap;
			pfc::stringcvt::string_wide_from_utf8 image_path;

			t_param(IFbMetadbHandle* p_handle, int p_art_id, IGdiBitmap* p_bitmap, const char* p_image_path)
				: handle(p_handle), art_id(p_art_id), bitmap(p_bitmap), image_path(p_image_path)
			{
			}

			~t_param()
			{
				if (handle)
					handle->Release();

				if (bitmap)
					bitmap->Release();
			}
		};

		album_art_async(HWND notify_hwnd, metadb_handle* handle, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL only_embed, VARIANT_BOOL no_load)
			: m_notify_hwnd(notify_hwnd)
			, m_handle(handle)
			, m_art_id(art_id)
			, m_need_stub(need_stub)
			, m_only_embed(only_embed)
			, m_no_load(no_load)
		{
			if (m_handle.is_valid())
				m_rawpath = pfc::stringcvt::string_wide_from_utf8(m_handle->get_path());
		}

	private:
		virtual void run();
		metadb_handle_ptr m_handle;
		_bstr_t m_rawpath;
		int m_art_id;
		VARIANT_BOOL m_need_stub;
		VARIANT_BOOL m_only_embed;
		VARIANT_BOOL m_no_load;
		HWND m_notify_hwnd;
	};

	class load_image_async : public simple_thread_task
	{
	public:
		struct t_param
		{
			unsigned cookie;
			IGdiBitmap* bitmap;
			_bstr_t path;

			t_param(int p_cookie, IGdiBitmap* p_bitmap, BSTR p_path)
				: cookie(p_cookie), bitmap(p_bitmap), path(p_path)
			{
			}

			~t_param()
			{
				if (bitmap)
					bitmap->Release();
			}
		};

		load_image_async(HWND notify_wnd, BSTR path)
			: m_notify_hwnd(notify_wnd), m_path(path)
		{
		}

	private:
		virtual void run();
		HWND m_notify_hwnd;
		_bstr_t m_path;
	};

	class js_process_locations : public process_locations_notify
	{
	public:
		js_process_locations(int playlist_idx, bool to_select)
			: m_playlist_idx(playlist_idx), m_to_select(to_select)
		{
		}

		void on_completion(const pfc::list_base_const_t<metadb_handle_ptr>& p_items)
		{
			bit_array_true selection_them;
			bit_array_false selection_none;
			bit_array* select_ptr = &selection_them;
			static_api_ptr_t<playlist_manager> api;
			t_size playlist;

			if (m_playlist_idx == -1)
				playlist = api->get_active_playlist();
			else
				playlist = m_playlist_idx;

			if (!m_to_select)
				select_ptr = &selection_none;

			if (playlist != pfc_infinite && playlist < api->get_playlist_count())
			{
				api->playlist_add_items(playlist, p_items, *select_ptr);
			}
		}

		void on_aborted()
		{
		}

	private:
		bool m_to_select;
		int m_playlist_idx;
	};
}
