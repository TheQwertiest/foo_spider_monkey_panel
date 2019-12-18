#pragma once

namespace smp::utils
{

class js_process_locations : public process_locations_notify
{
public:
    js_process_locations( int playlist_idx, UINT base, bool to_select );

    void on_completion( metadb_handle_list_cref p_items ) override;
    void on_aborted() override;

private:
    int m_playlist_idx;
    size_t m_base;
    bool m_to_select;
};

} // namespace smp::utils
