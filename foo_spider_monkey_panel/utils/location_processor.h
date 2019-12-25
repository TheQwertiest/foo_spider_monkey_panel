#pragma once

namespace smp::utils
{

class OnProcessLocationsNotify_InsertHandles
    : public process_locations_notify
{
public:
    OnProcessLocationsNotify_InsertHandles( int playlistIdx, UINT baseIdx, bool shouldSelect );

    void on_completion( metadb_handle_list_cref items ) override;
    void on_aborted() override;

private:
    int playlistIdx_;
    size_t baseIdx_;
    bool shouldSelect_;
};

} // namespace smp::utils
