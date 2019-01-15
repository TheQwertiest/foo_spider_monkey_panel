#pragma once

namespace helpers
{

COLORREF convert_argb_to_colorref( DWORD argb );
DWORD convert_colorref_to_argb( COLORREF color );

pfc::string8_fast get_fb2k_component_path();
pfc::string8_fast get_fb2k_path();
pfc::string8_fast get_profile_path();

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
