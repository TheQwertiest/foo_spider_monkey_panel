#include "stdafx.h"
#include "helpers.h"
#include "script_interface_impl.h"
#include "user_message.h"

#include <MLang.h>

namespace helpers
{
	COLORREF convert_argb_to_colorref(DWORD argb)
	{
		return RGB(argb >> RED_SHIFT, argb >> GREEN_SHIFT, argb >> BLUE_SHIFT);
	}

	DWORD convert_colorref_to_argb(DWORD color)
	{
		// COLORREF : 0x00bbggrr
		// ARGB : 0xaarrggbb
		return (GetRValue(color) << RED_SHIFT) |
			(GetGValue(color) << GREEN_SHIFT) |
			(GetBValue(color) << BLUE_SHIFT) |
			0xff000000;
	}

	IGdiBitmap* query_album_art(album_art_extractor_instance_v2::ptr extractor, GUID& what, VARIANT_BOOL no_load, pfc::string_base* image_path_ptr)
	{
		abort_callback_dummy abort;
		album_art_data_ptr data = extractor->query(what, abort);
		Gdiplus::Bitmap* bitmap = NULL;
		IGdiBitmap* ret = NULL;

		if (!no_load && helpers::read_album_art_into_bitmap(data, &bitmap))
		{
			ret = new com_object_impl_t<GdiBitmap>(bitmap);
		}

		if (image_path_ptr && (no_load || ret))
		{
			album_art_path_list::ptr pathlist = extractor->query_paths(what, abort);

			if (pathlist->get_count() > 0)
			{
				image_path_ptr->set_string(pathlist->get_path(0));
			}
		}

		return ret;
	}

	HBITMAP create_hbitmap_from_gdiplus_bitmap(Gdiplus::Bitmap* bitmap_ptr)
	{
		BITMAP bm;
		Gdiplus::Rect rect;
		Gdiplus::BitmapData bmpdata;
		HBITMAP hBitmap;

		rect.X = rect.Y = 0;
		rect.Width = bitmap_ptr->GetWidth();
		rect.Height = bitmap_ptr->GetHeight();

		if (bitmap_ptr->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bmpdata) != Gdiplus::Ok)
		{
			// Error
			return NULL;
		}

		bm.bmType = 0;
		bm.bmWidth = bmpdata.Width;
		bm.bmHeight = bmpdata.Height;
		bm.bmWidthBytes = bmpdata.Stride;
		bm.bmPlanes = 1;
		bm.bmBitsPixel = 32;
		bm.bmBits = bmpdata.Scan0;

		hBitmap = CreateBitmapIndirect(&bm);
		bitmap_ptr->UnlockBits(&bmpdata);
		return hBitmap;
	}

	HRESULT get_album_art_embedded(BSTR rawpath, IGdiBitmap** pp, int art_id)
	{
		if (!pp) return E_POINTER;

		service_enum_t<album_art_extractor> e;
		service_ptr_t<album_art_extractor> ptr;
		pfc::stringcvt::string_utf8_from_wide urawpath(rawpath);
		pfc::string_extension ext(urawpath);
		abort_callback_dummy abort;
		IGdiBitmap* ret = NULL;

		while (e.next(ptr))
		{
			if (ptr->is_our_path(urawpath, ext))
			{
				album_art_extractor_instance_ptr aaep;
				GUID what = helpers::convert_artid_to_guid(art_id);

				try
				{
					aaep = ptr->open(NULL, urawpath, abort);

					Gdiplus::Bitmap* bitmap = NULL;
					album_art_data_ptr data = aaep->query(what, abort);

					if (helpers::read_album_art_into_bitmap(data, &bitmap))
					{
						ret = new com_object_impl_t<GdiBitmap>(bitmap);
						break;
					}
				}
				catch (std::exception&)
				{
				}
			}
		}

		*pp = ret;
		return S_OK;
	}

	HRESULT get_album_art_v2(const metadb_handle_ptr& handle, IGdiBitmap** pp, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL no_load, pfc::string_base* image_path_ptr)
	{
		if (handle.is_empty() || !pp) return E_POINTER;

		GUID what = helpers::convert_artid_to_guid(art_id);
		abort_callback_dummy abort;
		static_api_ptr_t<album_art_manager_v2> aamv2;
		album_art_extractor_instance_v2::ptr aaeiv2;
		IGdiBitmap* ret = NULL;

		try
		{
			aaeiv2 = aamv2->open(pfc::list_single_ref_t<metadb_handle_ptr>(handle), pfc::list_single_ref_t<GUID>(what), abort);

			ret = query_album_art(aaeiv2, what, no_load, image_path_ptr);
		}
		catch (std::exception&)
		{
			if (need_stub)
			{
				album_art_extractor_instance_v2::ptr aaeiv2_stub = aamv2->open_stub(abort);

				try
				{
					album_art_data_ptr data = aaeiv2_stub->query(what, abort);

					ret = query_album_art(aaeiv2_stub, what, no_load, image_path_ptr);
				}
				catch (std::exception&)
				{
				}
			}
		}

		*pp = ret;
		return S_OK;
	}

	bool execute_context_command_by_name(const char* p_name, metadb_handle_list_cref p_handles, unsigned flags)
	{
		contextmenu_node* node = NULL;

		service_ptr_t<contextmenu_manager> cm;
		pfc::string8_fast dummy("");
		contextmenu_manager::g_create(cm);

		if (p_handles.get_count() > 0)
		{
			cm->init_context(p_handles, flags);
		}
		else
		{
			cm->init_context_now_playing(flags);
		}

		if (!find_context_command_recur(p_name, dummy, cm->get_root(), node))
		{
			return false;
		}

		if (node)
		{
			node->execute();
			return true;
		}

		return false;
	}

	bool execute_mainmenu_command_by_name(const char* p_name)
	{
		// First generate a map of all mainmenu_group
		pfc::map_t<GUID, mainmenu_group::ptr> group_guid_text_map;
		build_mainmenu_group_map(group_guid_text_map);

		// Second, generate a list of all mainmenu commands
		service_enum_t<mainmenu_commands> e;
		service_ptr_t<mainmenu_commands> ptr;
		t_size name_len = strlen(p_name);

		while (e.next(ptr))
		{
			for (t_uint32 idx = 0; idx < ptr->get_command_count(); ++idx)
			{
				GUID group_guid = ptr->get_parent();
				pfc::string8_fast path;

				while (group_guid_text_map.have_item(group_guid))
				{
					mainmenu_group::ptr group_ptr = group_guid_text_map[group_guid];
					mainmenu_group_popup::ptr group_popup_ptr;

					if (group_ptr->service_query_t(group_popup_ptr))
					{
						pfc::string8_fast temp;
						group_popup_ptr->get_display_string(temp);

						if (!temp.is_empty())
						{
							temp.add_char('/');
							temp.add_string(path);
							path = temp;
						}
					}

					group_guid = group_ptr->get_parent();
				}

				// for new fb2k1.0 commands
				mainmenu_commands_v2::ptr v2_ptr;

				if (ptr->service_query_t(v2_ptr))
				{
					if (v2_ptr->is_command_dynamic(idx))
					{
						mainmenu_node::ptr node = v2_ptr->dynamic_instantiate(idx);

						if (execute_mainmenu_command_recur_v2(node, path, p_name, name_len))
							return true;
						else
							continue;
					}
				}

				// old commands
				pfc::string8_fast command;
				ptr->get_name(idx, command);
				path.add_string(command);

				if (match_menu_command(path, p_name, name_len))
				{
					ptr->execute(idx, NULL);
					return true;
				}
			}
		}

		return false;
	}

	bool execute_mainmenu_command_recur_v2(mainmenu_node::ptr node, pfc::string8_fast path, const char* p_name, t_size p_name_len)
	{
		pfc::string8_fast text;
		t_uint32 flags;
		t_uint32 type = node->get_type();

		if (type != mainmenu_node::type_separator)
		{
			node->get_display(text, flags);
			if (!text.is_empty())
				path.add_string(text);
		}

		switch (type)
		{
		case mainmenu_node::type_command:
		{
			if (match_menu_command(path, p_name, p_name_len))
			{
				node->execute(NULL);
				return true;
			}
		}
		break;

		case mainmenu_node::type_group:
		{
			if (!text.is_empty())
				path.add_char('/');

			for (t_size i = 0; i < node->get_children_count(); ++i)
			{
				mainmenu_node::ptr child = node->get_child(i);

				if (execute_mainmenu_command_recur_v2(child, path, p_name, p_name_len))
					return true;
			}
		}
		break;
		}

		return false;
	}

	bool find_context_command_recur(const char* p_command, pfc::string_base& p_path, contextmenu_node* p_parent, contextmenu_node*& p_out)
	{
		if (p_parent != NULL && p_parent->get_type() == contextmenu_item_node::TYPE_POPUP)
		{
			for (t_size child_id = 0; child_id < p_parent->get_num_children(); ++child_id)
			{
				pfc::string8_fast path;
				contextmenu_node* child = p_parent->get_child(child_id);

				if (child)
				{
					path = p_path;
					path += child->get_name();

					switch (child->get_type())
					{
					case contextmenu_item_node::TYPE_POPUP:
						path += "/";

						if (find_context_command_recur(p_command, path, child, p_out))
							return true;

						break;

					case contextmenu_item_node::TYPE_COMMAND:
						if (match_menu_command(path, p_command))
						{
							p_out = child;
							return true;
						}
						break;
					}
				}
			}
		}

		return false;
	}

	bool match_menu_command(const pfc::string_base& path, const char* command, t_size command_len)
	{
		if (command_len == ~0)
			command_len = strlen(command);

		if (command_len == path.get_length())
		{
			if (_stricmp(command, path) == 0)
				return true;
		}
		else if (command_len < path.get_length())
		{
			if ((path[path.get_length() - command_len - 1] == '/') &&
				(_stricmp(path.get_ptr() + path.get_length() - command_len, command) == 0))
				return true;
		}

		return false;
	}

	bool read_album_art_into_bitmap(const album_art_data_ptr& data, Gdiplus::Bitmap** bitmap)
	{
		*bitmap = NULL;

		if (!data.is_valid())
			return false;

		// Using IStream
		IStreamPtr is;
		Gdiplus::Bitmap* bmp = NULL;
		bool ret = true;
		HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &is);

		if (SUCCEEDED(hr) && bitmap && is)
		{
			ULONG bytes_written = 0;

			hr = is->Write(data->get_ptr(), data->get_size(), &bytes_written);

			if (SUCCEEDED(hr) && bytes_written == data->get_size())
			{
				bmp = new Gdiplus::Bitmap(is, PixelFormat32bppPARGB);

				if (!ensure_gdiplus_object(bmp))
				{
					ret = false;
					if (bmp) delete bmp;
					bmp = NULL;
				}
			}
		}

		*bitmap = bmp;
		return ret;
	}

	bool read_file(const char* path, pfc::string_base& content)
	{
		HANDLE hFile = uCreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = uCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

		if (hFileMapping == NULL)
		{
			CloseHandle(hFile);
			return false;
		}

		DWORD dwFileSize = GetFileSize(hFile, NULL);
		LPCBYTE pAddr = (LPCBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

		if (pAddr == NULL)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		if (dwFileSize == INVALID_FILE_SIZE)
		{
			UnmapViewOfFile(pAddr);
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		bool status = false;

		if (dwFileSize > 3)
		{
			// UTF16 LE?
			if (pAddr[0] == 0xFF && pAddr[1] == 0xFE)
			{
				const wchar_t* pSource = (const wchar_t *)(pAddr + 2);
				t_size len = (dwFileSize >> 1) - 1;

				content = pfc::stringcvt::string_utf8_from_wide(pSource, len);
				status = true;
			}
			// UTF8?
			else if (pAddr[0] == 0xEF && pAddr[1] == 0xBB && pAddr[2] == 0xBF)
			{
				const char* pSource = (const char *)(pAddr + 3);
				t_size len = dwFileSize - 3;

				content.set_string(pSource, len);
				status = true;
			}
		}

		if (!status)
		{
			const char* pSource = (const char *)(pAddr);
			t_size pSourceSize = dwFileSize;

			UINT tmp = detect_charset(path);
			if (tmp == CP_UTF8)
			{
				content.set_string(pSource, pSourceSize);
			}
			else
			{
				content = pfc::stringcvt::string_utf8_from_ansi(pSource, pSourceSize);
			}
			status = true;
		}

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return status;
	}

	bool read_file_wide(unsigned codepage, const wchar_t* path, pfc::array_t<wchar_t>& content)
	{
		HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

		if (hFileMapping == NULL)
		{
			CloseHandle(hFile);
			return false;
		}

		DWORD dwFileSize = GetFileSize(hFile, NULL);
		LPCBYTE pAddr = (LPCBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

		if (pAddr == NULL)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		if (dwFileSize == INVALID_FILE_SIZE)
		{
			UnmapViewOfFile(pAddr);
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		bool status = false;

		if (dwFileSize > 3)
		{
			// UTF16 LE?
			if (pAddr[0] == 0xFF && pAddr[1] == 0xFE)
			{
				const wchar_t* pSource = (const wchar_t *)(pAddr + 2);
				t_size len = (dwFileSize - 2) >> 1;

				content.set_size(len + 1);
				pfc::__unsafe__memcpy_t(content.get_ptr(), pSource, len);
				content[len] = 0;
				status = true;
			}
			// UTF8-BOM?
			else if (pAddr[0] == 0xEF && pAddr[1] == 0xBB && pAddr[2] == 0xBF)
			{
				const char* pSource = (const char *)(pAddr + 3);
				t_size pSourceSize = dwFileSize - 3;

				const t_size size = pfc::stringcvt::estimate_utf8_to_wide_quick(pSource, pSourceSize);
				content.set_size(size);
				pfc::stringcvt::convert_utf8_to_wide(content.get_ptr(), size, pSource, pSourceSize);
				status = true;
			}
		}

		if (!status)
		{
			const char* pSource = (const char *)(pAddr);
			t_size pSourceSize = dwFileSize;

			UINT tmp = detect_charset(pfc::stringcvt::string_utf8_from_wide(path));
			if (tmp == CP_UTF8)
			{
				const t_size size = pfc::stringcvt::estimate_utf8_to_wide_quick(pSource, pSourceSize);
				content.set_size(size);
				pfc::stringcvt::convert_utf8_to_wide(content.get_ptr(), size, pSource, pSourceSize);
			}
			else
			{
				const t_size size = pfc::stringcvt::estimate_codepage_to_wide(codepage, pSource, pSourceSize);
				content.set_size(size);
				pfc::stringcvt::convert_codepage_to_wide(codepage, content.get_ptr(), size, pSource, pSourceSize);
			}
			status = true;
		}

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return status;
	}

	bool write_file(const char* path, const pfc::string_base& content)
	{
		HANDLE hFile = uCreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = uCreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, content.get_length() + 3, NULL);

		if (hFileMapping == NULL)
		{
			CloseHandle(hFile);
			return false;
		}

		PBYTE pAddr = (PBYTE)MapViewOfFile(hFileMapping, FILE_MAP_WRITE, 0, 0, 0);

		if (pAddr == NULL)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		const BYTE utf8_bom[] = { 0xef, 0xbb, 0xbf };
		memcpy(pAddr, utf8_bom, 3);
		memcpy(pAddr + 3, content.get_ptr(), content.get_length());

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return true;
	}

	const GUID convert_artid_to_guid(int art_id)
	{
		const GUID* guids[] = {
			&album_art_ids::cover_front,
			&album_art_ids::cover_back,
			&album_art_ids::disc,
			&album_art_ids::icon,
			&album_art_ids::artist,
		};

		if (0 <= art_id && art_id < _countof(guids))
		{
			return *guids[art_id];
		}
		else
		{
			return *guids[0];
		}
	}

	int get_encoder_clsid(const WCHAR* format, CLSID* pClsid)
	{
		int ret = -1;

		UINT num = 0;
		UINT size = 0;

		Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

		Gdiplus::GetImageEncodersSize(&num, &size);
		if (size == 0) return ret;

		pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc((size_t)size));
		if (pImageCodecInfo == NULL) return ret;

		Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

		for (UINT j = 0; j < num; ++j)
		{
			if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				ret = j;
				break;
			}
		}

		free(pImageCodecInfo);
		return ret;
	}

	int get_text_height(HDC hdc, const wchar_t* text, int len)
	{
		SIZE size;
		GetTextExtentPoint32(hdc, text, len, &size);
		return size.cy;
	}

	int get_text_width(HDC hdc, LPCTSTR text, int len)
	{
		SIZE size;

		GetTextExtentPoint32(hdc, text, len, &size);
		return size.cx;
	}

	int is_wrap_char(wchar_t current, wchar_t next)
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

	pfc::string8 iterator_to_string8(json::iterator j)
	{
		std::string value = j.value().type() == json::value_t::string ? j.value().get<std::string>() : j.value().dump();
		return value.c_str();
	}

	pfc::string8_fast get_fb2k_component_path()
	{
		pfc::string8_fast path;

		uGetModuleFileName(core_api::get_my_instance(), path);
		path = pfc::string_directory(path);
		path.add_char('\\');
		return path;
	}

	pfc::string8_fast get_fb2k_path()
	{
		pfc::string8_fast path;

		uGetModuleFileName(NULL, path);
		path = pfc::string_directory(path);
		path.add_string("\\");

		return path;
	}

	pfc::string8_fast get_profile_path()
	{
		pfc::string8_fast path;

		path = file_path_display(core_api::get_profile_path());
		path.fix_dir_separator('\\');

		return path;
	}

	unsigned detect_charset(const char* fileName)
	{
		_COM_SMARTPTR_TYPEDEF(IMultiLanguage2, IID_IMultiLanguage2);
		IMultiLanguage2Ptr lang;
		HRESULT hr;

		hr = lang.CreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER);
		// mlang is not working...
		if (FAILED(hr)) return 0;

		const int maxEncodings = 2;
		int encodingCount = maxEncodings;
		DetectEncodingInfo encodings[maxEncodings];
		pfc::string8_fast text;
		int textSize = 0;

		try
		{
			file_ptr io;
			abort_callback_dummy dummy;
			filesystem::g_open_read(io, fileName, dummy);
			io->read_string_raw(text, dummy);
			textSize = text.get_length();
		}
		catch (...)
		{
			return 0;
		}

		hr = lang->DetectInputCodepage(MLDETECTCP_NONE, 0, const_cast<char *>(text.get_ptr()), &textSize, encodings, &encodingCount);

		if (FAILED(hr)) return 0;

		unsigned codepage = 0;
		bool found = false;

		// MLang fine tunes
		if (encodingCount == 2 && encodings[0].nCodePage == 1252)
		{
			switch (encodings[1].nCodePage)
			{
			case 850:
			case 65001:
				found = true;
				codepage = 65001;
				break;
				// DBCS
			case 932: // shift-jis
			case 936: // gbk
			case 949: // korean
			case 950: // big5
			{
				// '¡¯', <= special char
				// "ve" "d" "ll" "m" 't' 're'
				bool fallback = true;
				t_size index;
				if (index = text.find_first("\x92") != pfc_infinite)
				{
					if ((index < text.get_length() - 1) &&
						(strchr("vldmtr ", text[index + 1])))
					{
						codepage = encodings[0].nCodePage;
						fallback = false;
					}
				}
				if (fallback)
					codepage = encodings[1].nCodePage;
				found = true;
			}
			break;
			}
		}

		if (!found)
			codepage = encodings[0].nCodePage;
		// ASCII?
		if (codepage == 20127)
			codepage = 0;

		return codepage;
	}

	unsigned get_colour_from_variant(VARIANT v)
	{
		return (v.vt == VT_R8) ? static_cast<unsigned>(v.dblVal) : v.lVal;
	}

	void build_mainmenu_group_map(pfc::map_t<GUID, mainmenu_group::ptr>& p_group_guid_text_map)
	{
		service_enum_t<mainmenu_group> e;
		service_ptr_t<mainmenu_group> ptr;

		while (e.next(ptr))
		{
			GUID guid = ptr->get_guid();
			p_group_guid_text_map.find_or_add(guid) = ptr;
		}
	}

	void estimate_line_wrap(HDC hdc, const wchar_t* text, int len, int width, pfc::list_t<wrapped_item>& out)
	{
		for (;;)
		{
			const wchar_t* next = wcschr(text, '\n');
			if (next == NULL)
			{
				estimate_line_wrap_recur(hdc, text, wcslen(text), width, out);
				break;
			}

			const wchar_t* walk = next;

			while (walk > text && walk[-1] == '\r')
			{
				--walk;
			}

			estimate_line_wrap_recur(hdc, text, walk - text, width, out);
			text = next + 1;
		}
	}

	void estimate_line_wrap_recur(HDC hdc, const wchar_t* text, int len, int width, pfc::list_t<wrapped_item>& out)
	{
		int textLength = len;
		int textWidth = get_text_width(hdc, text, len);

		if (textWidth <= width || len <= 1)
		{
			wrapped_item item = { SysAllocStringLen(text, len), textWidth };
			out.add_item(item);
		}
		else
		{
			textLength = (len * width) / textWidth;

			if (get_text_width(hdc, text, textLength) < width)
			{
				while (get_text_width(hdc, text, min(len, textLength + 1)) <= width)
				{
					++textLength;
				}
			}
			else
			{
				while (get_text_width(hdc, text, textLength) > width && textLength > 1)
				{
					--textLength;
				}
			}

			{
				int fallbackTextLength = max(textLength, 1);

				while (textLength > 0 && !is_wrap_char(text[textLength - 1], text[textLength]))
				{
					--textLength;
				}

				if (textLength == 0)
				{
					textLength = fallbackTextLength;
				}

				wrapped_item item =
				{
					SysAllocStringLen(text, textLength),
					get_text_width(hdc, text, textLength)
				};

				out.add_item(item);
			}

			if (textLength < len)
			{
				estimate_line_wrap_recur(hdc, text + textLength, len - textLength, width, out);
			}
		}
	}

	void album_art_async::run()
	{
		pfc::string8_fast image_path;
		FbMetadbHandle* handle = NULL;
		IGdiBitmap* bitmap = NULL;

		if (m_handle.is_valid())
		{
			if (m_only_embed)
			{
				get_album_art_embedded(m_rawpath, &bitmap, m_art_id);
				if (bitmap)
					image_path = m_handle->get_path();
			}
			else
			{
				get_album_art_v2(m_handle, &bitmap, m_art_id, m_need_stub, m_no_load, &image_path);
			}

			handle = new com_object_impl_t<FbMetadbHandle>(m_handle);
		}

		t_param param(handle, m_art_id, bitmap, file_path_display(image_path));

		SendMessage(m_notify_hwnd, CALLBACK_UWM_GETALBUMARTASYNCDONE, 0, (LPARAM)&param);
	}

	void load_image_async::run()
	{
		IGdiBitmap* bitmap = NULL;
		Gdiplus::Bitmap* img = new Gdiplus::Bitmap(m_path, PixelFormat32bppPARGB);

		if (helpers::ensure_gdiplus_object(img))
		{
			bitmap = new com_object_impl_t<GdiBitmap>(img);
		}
		else
		{
			if (img) delete img;
			img = NULL;
		}

		t_param param(reinterpret_cast<unsigned>(this), bitmap, m_path);

		SendMessage(m_notify_hwnd, CALLBACK_UWM_LOADIMAGEASYNCDONE, 0, (LPARAM)&param);
	}
}