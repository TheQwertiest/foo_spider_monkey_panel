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
    const int playlistIdx_;
    const size_t baseIdx_;
    const bool shouldSelect_;
};

} // namespace smp::utils
