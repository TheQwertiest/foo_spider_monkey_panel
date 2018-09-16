#pragma once

#include "thread_pool.h"
#include "script_interface.h"
#include "json.hpp"

using json = nlohmann::json;

namespace helpers
{
	struct custom_sort_data
	{
        std::wstring text;
        size_t index;
	};

	struct wrapped_item
	{
		_bstr_t text;
		size_t width;
	};

	COLORREF convert_argb_to_colorref(DWORD argb);
	DWORD convert_colorref_to_argb(DWORD color);
	HBITMAP create_hbitmap_from_gdiplus_bitmap(Gdiplus::Bitmap& bitmap_ptr);
	bool execute_context_command_by_name(const char* p_name, metadb_handle_list_cref p_handles, unsigned flags);
	bool execute_mainmenu_command_by_name(const char* p_name);
	bool get_mainmenu_command_node_recur_v2(mainmenu_node::ptr node, pfc::string8_fast path, const char* p_name, t_size p_name_len, mainmenu_node::ptr &node_out);
	bool get_mainmenu_command_status_by_name(const char* p_name, t_uint32 &status);
	bool find_context_command_recur(const char* p_command, pfc::string_base& p_path, contextmenu_node* p_parent, contextmenu_node*& p_out);
	bool match_menu_command(const pfc::string_base& path, const char* command, t_size command_len = ~0);
	bool read_file(const char* path, pfc::string_base& content);
	bool write_file(const char* path, const pfc::string_base& content, bool write_bom = true);
	int get_encoder_clsid(const WCHAR* format, CLSID* pClsid);
    size_t get_text_height(HDC hdc, std::wstring_view text );
	size_t get_text_width(HDC hdc, std::wstring_view text);
	int is_wrap_char(wchar_t current, wchar_t next);
	pfc::string8 iterator_to_string8(json::iterator j);
	pfc::string8_fast get_fb2k_component_path();
	pfc::string8_fast get_fb2k_path();
	pfc::string8_fast get_profile_path();
	t_size detect_charset(const char* fileName);
    size_t detect_text_charset( const char* text, size_t textSize );
	void build_mainmenu_group_map(pfc::map_t<GUID, mainmenu_group::ptr>& p_group_guid_text_map);
	void estimate_line_wrap(HDC hdc, const std::wstring& text, size_t width, std::list<helpers::wrapped_item>& out);
	void estimate_line_wrap_recur(HDC hdc, std::wstring_view text, size_t width, std::list<helpers::wrapped_item>& out);
	std::wstring make_sort_string(const char* in);

	template <class T>
	bool ensure_gdiplus_object(T* obj)
	{
		return ((obj) && (obj->GetLastStatus() == Gdiplus::Ok));
	}

	template<int direction>
	static int custom_sort_compare(const custom_sort_data& elem1, const custom_sort_data& elem2)
	{
		int ret = direction * StrCmpLogicalW(elem1.text.c_str(), elem2.text.c_str() );
        if ( !ret )
        {
            ret = pfc::sgn_t( (t_ssize)elem1.index - (t_ssize)elem2.index );
        }
		return ret;
	}

    __declspec(noinline) static bool execute_context_command_by_name_SEH( const char* p_name, metadb_handle_list_cref p_handles, unsigned flags )
    {
        bool ret = false;

        __try
        {
            ret = execute_context_command_by_name( p_name, p_handles, flags );
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ret = false;
        }

        return ret;
    }

    __declspec(noinline) static bool execute_mainmenu_command_by_name_SEH( const char* p_name )
    {
        bool ret = false;

        __try
        {
            ret = execute_mainmenu_command_by_name( p_name );
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ret = false;
        }

        return ret;
    }

    __declspec(noinline) static bool get_mainmenu_command_status_by_name_SEH( const char* p_name, t_uint32 &status )
    {
        bool ret = false;

        __try
        {
            ret = get_mainmenu_command_status_by_name( p_name, status );
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ret = false;
        }

        return ret;
    }

    class embed_thread : public threaded_process_callback
    {
    public:
        embed_thread( t_size action, album_art_data_ptr data, metadb_handle_list_cref handles, GUID what ) : m_action( action ), m_data( data ), m_handles( handles ), m_what( what ) {}
        void on_init( HWND p_wnd ) {}
        void run( threaded_process_status& p_status, abort_callback& p_abort )
        {
            t_size count = m_handles.get_count();
            for ( t_size i = 0; i < count; ++i )
            {
                pfc::string8_fast path = m_handles.get_item( i )->get_path();
                p_status.set_progress( i, count );
                p_status.set_item_path( path );
                album_art_editor::ptr ptr;
                if ( album_art_editor::g_get_interface( ptr, path ) )
                {
                    album_art_editor_instance_ptr aaep;
                    try
                    {
                        aaep = ptr->open( NULL, path, p_abort );
                        if ( m_action == 0 )
                        {
                            aaep->set( m_what, m_data, p_abort );
                        }
                        else if ( m_action == 1 )
                        {
                            aaep->remove( m_what );
                        }
                        aaep->commit( p_abort );
                    }
                    catch ( ... )
                    {
                    }
                }
            }
        }
    private:
        t_size m_action; // 0 embed, 1 remove
        album_art_data_ptr m_data;
        metadb_handle_list m_handles;
        GUID m_what;
    };
    
    class js_process_locations : public process_locations_notify
    {
    public:
        js_process_locations( int playlist_idx, UINT base, bool to_select );
        void on_completion( metadb_handle_list_cref p_items );
        void on_aborted();

    private:
        bool m_to_select;
        int m_playlist_idx;
        t_size m_base;
    };
}
