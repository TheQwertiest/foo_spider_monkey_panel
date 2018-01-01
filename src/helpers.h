#pragma once

#include "thread_pool.h"
#include "script_interface.h"
#include "json.hpp"

using json = nlohmann::json;

namespace helpers
{
	struct wrapped_item
	{
		BSTR text;
		int width;
	};

	COLORREF convert_argb_to_colorref(DWORD argb);
	DWORD convert_colorref_to_argb(DWORD color);
	IGdiBitmap* query_album_art(album_art_extractor_instance_v2::ptr extractor, GUID& what, VARIANT_BOOL no_load = VARIANT_FALSE, pfc::string_base* image_path_ptr = NULL);
	HBITMAP create_hbitmap_from_gdiplus_bitmap(Gdiplus::Bitmap* bitmap_ptr);
	HRESULT get_album_art_embedded(BSTR rawpath, IGdiBitmap** pp, int art_id);
	HRESULT get_album_art_v2(const metadb_handle_ptr& handle, IGdiBitmap** pp, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL no_load = VARIANT_FALSE, pfc::string_base* image_path_ptr = NULL);
	bool execute_context_command_by_name(const char* p_name, metadb_handle_list_cref p_handles, unsigned flags);
	bool execute_mainmenu_command_by_name(const char* p_name);
	bool execute_mainmenu_command_recur_v2(mainmenu_node::ptr node, pfc::string8_fast path, const char* p_name, t_size p_name_len);
	bool find_context_command_recur(const char* p_command, pfc::string_base& p_path, contextmenu_node* p_parent, contextmenu_node*& p_out);
	bool match_menu_command(const pfc::string_base& path, const char* command, t_size command_len = ~0);
	bool read_album_art_into_bitmap(const album_art_data_ptr& data, Gdiplus::Bitmap** bitmap);
	bool read_file(const char* path, pfc::string_base& content);
	bool read_file_wide(unsigned codepage, const wchar_t* path, pfc::array_t<wchar_t>& content);
	bool write_file(const char* path, const pfc::string_base& content);
	const GUID convert_artid_to_guid(int art_id);
	int get_encoder_clsid(const WCHAR* format, CLSID* pClsid);
	int get_text_height(HDC hdc, const wchar_t* text, int len);
	int get_text_width(HDC hdc, LPCTSTR text, int len);
	int is_wrap_char(wchar_t current, wchar_t next);
	pfc::string8 iterator_to_string8(json::iterator j);
	pfc::string8_fast get_fb2k_component_path();
	pfc::string8_fast get_fb2k_path();
	pfc::string8_fast get_profile_path();
	unsigned detect_charset(const char* fileName);
	unsigned get_colour_from_variant(VARIANT v);
	void build_mainmenu_group_map(pfc::map_t<GUID, mainmenu_group::ptr>& p_group_guid_text_map);
	void estimate_line_wrap(HDC hdc, const wchar_t* text, int len, int width, pfc::list_t<wrapped_item>& out);
	void estimate_line_wrap_recur(HDC hdc, const wchar_t* text, int len, int width, pfc::list_t<wrapped_item>& out);

	template <class T> bool ensure_gdiplus_object(T* obj)
	{
		return ((obj) && (obj->GetLastStatus() == Gdiplus::Ok));
	}

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

	class album_art_async : public simple_thread_task
	{
	public:
		struct t_param
		{
			IFbMetadbHandle* handle;
			int art_id;
			IGdiBitmap* bitmap;
			pfc::stringcvt::string_wide_from_utf8 image_path;

			t_param(IFbMetadbHandle* p_handle, int p_art_id, IGdiBitmap* p_bitmap, const char* p_image_path) : handle(p_handle), art_id(p_art_id), bitmap(p_bitmap), image_path(p_image_path)
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

		album_art_async(HWND notify_hwnd, metadb_handle* handle, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL only_embed, VARIANT_BOOL no_load) : m_notify_hwnd(notify_hwnd), m_handle(handle), m_art_id(art_id), m_need_stub(need_stub), m_only_embed(only_embed), m_no_load(no_load)
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

			t_param(int p_cookie, IGdiBitmap* p_bitmap, BSTR p_path) : cookie(p_cookie), bitmap(p_bitmap), path(p_path)
			{
			}

			~t_param()
			{
				if (bitmap)
					bitmap->Release();
			}
		};

		load_image_async(HWND notify_wnd, BSTR path) : m_notify_hwnd(notify_wnd), m_path(path)
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
		js_process_locations(int playlist_idx, bool to_select) : m_playlist_idx(playlist_idx), m_to_select(to_select)
		{
		}

		void on_completion(const pfc::list_base_const_t<metadb_handle_ptr>& p_items)
		{
			bit_array_true selection_them;
			bit_array_false selection_none;
			bit_array* select_ptr = &selection_none;
			static_api_ptr_t<playlist_manager> api;
			t_size playlist = m_playlist_idx == -1 ? api->get_active_playlist() : m_playlist_idx;

			if (m_to_select)
			{
				select_ptr = &selection_them;
			}

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

	class com_array_reader
	{
	public:
		com_array_reader() : m_psa(NULL)
		{
			reset();
		}

		com_array_reader(VARIANT* pVarSrc) : m_psa(NULL)
		{
			convert(pVarSrc);
		}

		~com_array_reader()
		{
			reset();
		}

		SAFEARRAY* get_ptr()
		{
			return m_psa;
		}

		long get_lbound()
		{
			return m_lbound;
		}

		long get_ubound()
		{
			return m_ubound;
		}

		int get_count()
		{
			return get_ubound() - get_lbound() + 1;
		}

		bool get_item(long idx, VARIANT& dest)
		{
			if (!m_psa || idx < m_lbound || idx > m_ubound) return false;

			return SUCCEEDED(SafeArrayGetElement(m_psa, &idx, &dest));
		}

		VARIANT operator[](long idx)
		{
			_variant_t var;

			if (!get_item(idx, var))
			{
				throw std::out_of_range("Out of range");
			}

			return var;
		}

		bool convert(VARIANT* pVarSrc)
		{
			reset();

			if (!pVarSrc) return false;

			if ((pVarSrc->vt & VT_ARRAY) && pVarSrc->parray)
			{
				return (SUCCEEDED(SafeArrayCopy(pVarSrc->parray, &m_psa)));
			}
			else if ((pVarSrc->vt & VT_TYPEMASK) == VT_DISPATCH)
			{
				IDispatch* pdisp = pVarSrc->pdispVal;

				if (pVarSrc->vt & VT_BYREF)
				{
					pdisp = *(pVarSrc->ppdispVal);
				}

				if (pdisp)
				{
					return convert_jsarray(pdisp);
				}
			}

			return false;
		}

		void reset()
		{
			m_ubound = -1;
			m_lbound = 0;

			if (m_psa)
			{
				SafeArrayDestroy(m_psa);
				m_psa = NULL;
			}
		}

	private:
		bool convert_jsarray(IDispatch* pdisp)
		{
			if (!pdisp) return false;

			DISPPARAMS params = { 0 };
			_variant_t ret;

			DISPID id_length;
			LPOLESTR slength = L"length";

			if (FAILED(pdisp->GetIDsOfNames(IID_NULL, &slength, 1, LOCALE_USER_DEFAULT, &id_length)))
				return false;

			if (FAILED(pdisp->Invoke(id_length, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, &ret, NULL, NULL)))
				return false;

			if (FAILED(VariantChangeType(&ret, &ret, 0, VT_I4)))
				return false;

			m_lbound = 0;
			m_ubound = ret.lVal - 1;

			SAFEARRAY* psa = SafeArrayCreateVector(VT_VARIANT, 0, get_count());

			if (!psa) goto cleanup_and_return;

			for (long i = m_lbound; i <= m_ubound; ++i)
			{
				DISPID dispid = 0;
				DISPPARAMS params = { 0 };
				wchar_t buf[33];
				LPOLESTR name = buf;
				_variant_t element;
				HRESULT hr = S_OK;

				_itow_s(i, buf, 10);

				if (SUCCEEDED(hr)) hr = pdisp->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
				if (SUCCEEDED(hr)) hr = pdisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, &element, NULL, NULL);

				if (FAILED(hr)) goto cleanup_and_return;
				if (FAILED(SafeArrayPutElement(psa, &i, &element))) goto cleanup_and_return;
			}

			m_psa = psa;
			return true;

		cleanup_and_return:
			reset();
			SafeArrayDestroy(psa);
			return false;
		}

		SAFEARRAY* m_psa;
		long m_lbound, m_ubound;
	};

	template <bool managed = false> class com_array_writer
	{
	public:
		com_array_writer() : m_psa(NULL)
		{
			reset();
		}

		~com_array_writer()
		{
			if (managed)
			{
				reset();
			}
		}

		SAFEARRAY* get_ptr()
		{
			return m_psa;
		}

		long get_count()
		{
			return m_count;
		}

		bool create(long count)
		{
			reset();

			m_psa = SafeArrayCreateVector(VT_VARIANT, 0, count);
			m_count = count;
			return (m_psa != NULL);
		}

		HRESULT put(long idx, VARIANT& pVar)
		{
			if (idx >= m_count) return E_INVALIDARG;
			if (!m_psa) return E_POINTER;

			HRESULT hr = SafeArrayPutElement(m_psa, &idx, &pVar);
			return hr;
		}

		void reset()
		{
			m_count = 0;

			if (m_psa)
			{
				SafeArrayDestroy(m_psa);
				m_psa = NULL;
			}
		}

	private:
		SAFEARRAY* m_psa;
		long m_count;
	};

	class com_array_to_bitarray
	{
	public:
		static bool convert(VARIANT items, unsigned bitArrayCount, bit_array_bittable& out, bool& empty)
		{
			helpers::com_array_reader arrayReader;
			empty = false;

			if (!arrayReader.convert(&items)) return false;
			if (arrayReader.get_count() == 0)
			{
				empty = true;
				out.resize(0);
				return true;
			}

			out.resize(bitArrayCount);

			for (int i = arrayReader.get_lbound(); i < arrayReader.get_count(); ++i)
			{
				_variant_t index;
				arrayReader.get_item(i, index);
				if (FAILED(VariantChangeType(&index, &index, 0, VT_I4))) return false;
				out.set(index.lVal, true);
			}

			return true;
		}
	};
}
