#pragma once

namespace helpers
{

struct StrCmpLogicalCmpData
{
    StrCmpLogicalCmpData( const std::wstring& textId, size_t index )
        : textId( textId )
        , index( index )
    {
    }
    std::wstring textId;
    size_t index;
};

struct wrapped_item
{
    _bstr_t text;
    size_t width;
};

COLORREF convert_argb_to_colorref( DWORD argb );
DWORD convert_colorref_to_argb( COLORREF color );
bool read_file( const char* path, pfc::string_base& content );
bool write_file( const char* path, const pfc::string_base& content, bool write_bom = true );
size_t get_text_height( HDC hdc, std::wstring_view text );
size_t get_text_width( HDC hdc, std::wstring_view text );
int is_wrap_char( wchar_t current, wchar_t next );
pfc::string8_fast get_fb2k_component_path();
pfc::string8_fast get_fb2k_path();
pfc::string8_fast get_profile_path();
t_size detect_charset( const char* fileName );
size_t detect_text_charset( const char* text, size_t textSize );
void estimate_line_wrap( HDC hdc, const std::wstring& text, size_t width, std::list<helpers::wrapped_item>& out );
void estimate_line_wrap_recur( HDC hdc, std::wstring_view text, size_t width, std::list<helpers::wrapped_item>& out );
std::wstring make_sort_string( const char* in );

template <int8_t direction = 1>
bool StrCmpLogicalCmp( const StrCmpLogicalCmpData& a, const StrCmpLogicalCmpData& b )
{
    int ret = direction * StrCmpLogicalW( a.textId.c_str(), b.textId.c_str() );
    if ( !ret )
    {
        return ( a.index < b.index );
    }
    else
    {
        return ( ret < 0 );
    }
}

class js_process_locations : public process_locations_notify
{
public:
    js_process_locations( int playlist_idx, UINT base, bool to_select );

    void on_completion( metadb_handle_list_cref p_items ) override;
    void on_aborted() override;

private:
    bool m_to_select;
    int m_playlist_idx;
    t_size m_base;
};

} // namespace helpers
