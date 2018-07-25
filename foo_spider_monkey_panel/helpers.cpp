#include "stdafx.h"
#include "helpers.h"
#include "script_interface_impl.h"
#include "user_message.h"

#include <MLang.h>

namespace helpers
{
	static void generate_mainmenu_command_path(const pfc::map_t<GUID, mainmenu_group::ptr> &group_guid_map, const service_ptr_t<mainmenu_commands> ptr, pfc::string8_fast &path)
	{
		GUID group_guid = ptr->get_parent();

		while (group_guid_map.have_item(group_guid))
		{
			mainmenu_group::ptr group_ptr = group_guid_map[group_guid];
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
	}
    
	COLORREF convert_argb_to_colorref(DWORD argb)
	{
		return RGB(argb >> RED_SHIFT, argb >> GREEN_SHIFT, argb >> BLUE_SHIFT);
	}

	DWORD convert_colorref_to_argb(DWORD color)
	{
		// COLORREF : 0x00bbggrr
		// ARGB : 0xaarrggbb
        return (GetRValue( color ) << RED_SHIFT)
            | (GetGValue( color ) << GREEN_SHIFT)
            | (GetBValue( color ) << BLUE_SHIFT)
            | 0xff000000;
	}

	HBITMAP create_hbitmap_from_gdiplus_bitmap(Gdiplus::Bitmap* bitmap_ptr)
	{
        Gdiplus::Rect rect;
		rect.X = rect.Y = 0;
		rect.Width = bitmap_ptr->GetWidth();
		rect.Height = bitmap_ptr->GetHeight();

        Gdiplus::BitmapData bmpdata;
		if (bitmap_ptr->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bmpdata) != Gdiplus::Ok)
		{// Error
			return nullptr;
		}

        BITMAP bm;
		bm.bmType = 0;
		bm.bmWidth = bmpdata.Width;
		bm.bmHeight = bmpdata.Height;
		bm.bmWidthBytes = bmpdata.Stride;
		bm.bmPlanes = 1;
		bm.bmBitsPixel = 32;
		bm.bmBits = bmpdata.Scan0;

        HBITMAP hBitmap = CreateBitmapIndirect(&bm);
		bitmap_ptr->UnlockBits(&bmpdata);
		return hBitmap;
	}

	bool execute_context_command_by_name(const char* p_name, metadb_handle_list_cref p_handles, unsigned flags)
	{
		contextmenu_manager::ptr cm;
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

        contextmenu_node* node = nullptr;
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

    // TODO: remove code copy-paste

	bool execute_mainmenu_command_by_name(const char* p_name)
	{
		// First generate a map of all mainmenu_group
		pfc::map_t<GUID, mainmenu_group::ptr> group_guid_text_map;
		build_mainmenu_group_map(group_guid_text_map);

		// Second, generate a list of all mainmenu commands
		service_enum_t<mainmenu_commands> e;
		mainmenu_commands::ptr ptr;
		t_size name_len = strlen(p_name);

		while (e.next(ptr))
		{
			for (t_uint32 idx = 0; idx < ptr->get_command_count(); ++idx)
			{
				pfc::string8_fast path;
				generate_mainmenu_command_path(group_guid_text_map, ptr, path);

				// for new fb2k1.0 commands
				mainmenu_commands_v2::ptr v2_ptr;
				if (ptr->service_query_t(v2_ptr) && v2_ptr->is_command_dynamic( idx ) )
				{
                    mainmenu_node::ptr node = v2_ptr->dynamic_instantiate( idx );
                    mainmenu_node::ptr node_out;

                    if ( get_mainmenu_command_node_recur_v2( node, path, p_name, name_len, node_out ) )
                    {
                        node_out->execute( nullptr );
                        return true;
                    }

                    continue;
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

	bool get_mainmenu_command_status_by_name(const char* p_name, t_uint32 &status)
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
				pfc::string8_fast path;
				generate_mainmenu_command_path(group_guid_text_map, ptr, path);

				// for new fb2k1.0 commands
				mainmenu_commands_v2::ptr v2_ptr;
                if ( ptr->service_query_t( v2_ptr ) && v2_ptr->is_command_dynamic( idx ) )
                {
                    mainmenu_node::ptr node = v2_ptr->dynamic_instantiate( idx );
                    mainmenu_node::ptr retNode;

                    if ( get_mainmenu_command_node_recur_v2( node, path, p_name, name_len, retNode ) )
                    {
                        pfc::string8_fast tmp;
                        retNode->get_display( tmp, status );
                        return true;
                    }

                    continue;
                }

				// old commands
				pfc::string8_fast command;
				ptr->get_name(idx, command);
				path.add_string(command);

				if (match_menu_command(path, p_name, name_len))
				{
					pfc::string8_fast tmp;
					ptr->get_display(idx, tmp, status);
					return true;
				}
			}
		}

		return false;
	}

	static bool get_mainmenu_command_node_recur_v2(mainmenu_node::ptr node, pfc::string8_fast path, const char* p_name, t_size p_name_len, mainmenu_node::ptr &node_out)
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

        switch ( type )
        {
        case mainmenu_node::type_command:
        {
            if ( match_menu_command( path, p_name, p_name_len ) )
            {
                node_out = node;
                return true;
            }
            break;
        }


        case mainmenu_node::type_group:
        {
            if ( !text.is_empty() )
                path.add_char( '/' );

            for ( t_size i = 0; i < node->get_children_count(); ++i )
            {
                mainmenu_node::ptr child = node->get_child( i );

                if ( get_mainmenu_command_node_recur_v2( child, path, p_name, p_name_len, node_out ) )
                    return true;
            }
            break;
        }
		}

		return false;
	}

	bool find_context_command_recur(const char* p_command, pfc::string_base& p_path, contextmenu_node* p_parent, contextmenu_node*& p_out)
	{
		if (p_parent && p_parent->get_type() == contextmenu_item_node::TYPE_POPUP)
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
                    {
                        path += "/";

                        if ( find_context_command_recur( p_command, path, child, p_out ) )
                            return true;

                        break;
                    }
					case contextmenu_item_node::TYPE_COMMAND:
                    {
                        if ( match_menu_command( path, p_command ) )
                        {
                            p_out = child;
                            return true;
                        }
                        break;
                    }
					}
				}
			}
		}

		return false;
	}

	bool is14()
	{
		return core_version_info_v2::get()->test_version(1, 4, 0, 0);
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

	bool read_file(const char* path, pfc::string_base& content)
	{
		HANDLE hFile = uCreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = uCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (!hFileMapping)
		{
			CloseHandle(hFile);
			return false;
		}

		DWORD dwFileSize = GetFileSize(hFile, NULL);
		LPCBYTE pAddr = (LPCBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
		if (!pAddr)
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

			t_size tmp = detect_charset(path);
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
		if (!hFileMapping)
		{
			CloseHandle(hFile);
			return false;
		}

		DWORD dwFileSize = GetFileSize(hFile, NULL);
		LPCBYTE pAddr = (LPCBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
		if (!pAddr)
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

			t_size tmp = detect_charset(pfc::stringcvt::string_utf8_from_wide(path));
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

	bool write_file(const char* path, const pfc::string_base& content, bool write_bom)
	{
		int offset = write_bom ? 3 : 0;
		HANDLE hFile = uCreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = uCreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, content.get_length() + offset, NULL);
		if (!hFileMapping)
		{
			CloseHandle(hFile);
			return false;
		}

		PBYTE pAddr = (PBYTE)MapViewOfFile(hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
		if (!pAddr)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		if (write_bom)
		{
			const BYTE utf8_bom[] = { 0xef, 0xbb, 0xbf };
			memcpy(pAddr, utf8_bom, 3);
		}
		memcpy(pAddr + offset, content.get_ptr(), content.get_length());

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return true;
	}

	int get_encoder_clsid(const WCHAR* format, CLSID* pClsid)
	{
        UINT num = 0;
        UINT size = 0;
        Gdiplus::Status status = Gdiplus::GetImageEncodersSize(&num, &size);
        if ( status != Gdiplus::Ok || !size )
        {
            return -1;
        }

        std::vector<uint8_t> imageCodeInfoBuf( size );
        Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)imageCodeInfoBuf.data();

        status = Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
        if ( status != Gdiplus::Ok )
        {
            return -1;
        }

		for (UINT i = 0; i < num; ++i)
		{
			if (!wcscmp(pImageCodecInfo[i].MimeType, format))
			{
				*pClsid = pImageCodecInfo[i].Clsid;
                return i;
			}
		}

		return -1;
	}

	int get_text_height(HDC hdc, const wchar_t* text, int len)
	{
		SIZE size;
        // TODO: add error checks
		GetTextExtentPoint32(hdc, text, len, &size);
		return size.cy;
	}

	int get_text_width(HDC hdc, LPCTSTR text, int len)
	{
		SIZE size;
        // TODO: add error checks
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
		path.add_char('\\');

		return path;
	}

	pfc::string8_fast get_profile_path()
	{
		pfc::string8_fast path;

		path = file_path_display(core_api::get_profile_path());
		path.fix_dir_separator('\\');

		return path;
	}

	t_size detect_charset(const char* fileName)
	{
		pfc::string8_fast text;
		int textSize = 0;

		try
		{
			file_ptr io;
			abort_callback_dummy abort;
			filesystem::g_open_read(io, fileName, abort);
			io->read_string_raw(text, abort);
			textSize = text.get_length();
		}
		catch (...)
		{
			return 0;
		}

        return detect_text_charset( const_cast<char *>(text.get_ptr()), text.get_length() );
	}

    size_t detect_text_charset( const char* text, size_t textSize )
    {
        _COM_SMARTPTR_TYPEDEF( IMultiLanguage2, IID_IMultiLanguage2 );
        IMultiLanguage2Ptr lang;

        HRESULT hr = lang.CreateInstance( CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER );
        if ( FAILED( hr ) )
        {
            return 0;
        }

        const int maxEncodings = 2;
        int encodingCount = maxEncodings;
        DetectEncodingInfo encodings[maxEncodings];
        int iTextSize = textSize;

        hr = lang->DetectInputCodepage( MLDETECTCP_NONE, 0, (char*)text, &iTextSize, encodings, &encodingCount );
        if ( FAILED( hr ) )
        {
            return 0;
        }

        unsigned codepage = 0;
        bool found = false;

        // MLang fine tunes
        if ( encodingCount == 2 && encodings[0].nCodePage == 1252 )
        {
            switch ( encodings[1].nCodePage )
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

                const char pattern[] = "\x92";
                const char* pPos = std::search( text, text + textSize, pattern, pattern + sizeof( pattern ) - 1 );
                if ( pPos != (text + textSize) )
                {
                    const char pattern2[] = "vldmtr ";
                    pPos = std::search( text, text + textSize, pattern, pattern2 + sizeof( pattern2 ) - 1 );
                    if ( pPos != (text + textSize) )
                    {
                        codepage = encodings[0].nCodePage;
                        fallback = false;
                    }
                }

                if ( fallback )
                {
                    codepage = encodings[1].nCodePage;
                }
                found = true;
            }
            break;
            }
        }

        if ( !found )
        {
            codepage = encodings[0].nCodePage;
        }
        // ASCII?
        if ( codepage == 20127 )
        {
            codepage = 0;
        }

        return codepage;
    }

	void build_mainmenu_group_map(pfc::map_t<GUID, mainmenu_group::ptr>& p_group_guid_text_map)
	{
		service_enum_t<mainmenu_group> e;
		mainmenu_group::ptr ptr;

		while (e.next(ptr))
		{
			GUID guid = ptr->get_guid();
			p_group_guid_text_map.find_or_add(guid) = ptr;
		}
	}

	void estimate_line_wrap(HDC hdc, const wchar_t* text, int len, int width, pfc::list_t<wrapped_item>& out)
	{
        while ( true )
        {
            const wchar_t* next = wcschr(text, '\n');
			if (!next)
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
			wrapped_item item =
			{
				SysAllocStringLen(text, len),
				textWidth
			};
			out.add_item(item);
		}
		else
		{
			textLength = (len * width) / textWidth;

			if (get_text_width(hdc, text, textLength) < width)
			{
				while (get_text_width(hdc, text, std::min(len, textLength + 1)) <= width)
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
				int fallbackTextLength = std::max(textLength, 1);

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

	wchar_t* make_sort_string(const char* in)
	{
		wchar_t* out = new wchar_t[pfc::stringcvt::estimate_utf8_to_wide(in) + 1];
		out[0] = ' ';//StrCmpLogicalW bug workaround.
		pfc::stringcvt::convert_utf8_to_wide_unchecked(out + 1, in);
		return out;
	}

    js_process_locations::js_process_locations( int playlist_idx, UINT base, bool to_select )
        : m_playlist_idx( playlist_idx )
        , m_base( base )
        , m_to_select( to_select )
    {

    }

    void js_process_locations::on_completion( metadb_handle_list_cref p_items )
    {
        pfc::bit_array_val selection( m_to_select );
        auto api = playlist_manager::get();
        t_size playlist = m_playlist_idx == -1 ? api->get_active_playlist() : m_playlist_idx;

        if ( playlist < api->get_playlist_count() && !api->playlist_lock_is_present( playlist ) )
        {
            api->playlist_insert_items( playlist, m_base, p_items, selection );
            if ( m_to_select )
            {
                api->set_active_playlist( playlist );
                api->playlist_set_focus_item( playlist, m_base );
            }
        }
    }

    void js_process_locations::on_aborted()
    {

    }

}
