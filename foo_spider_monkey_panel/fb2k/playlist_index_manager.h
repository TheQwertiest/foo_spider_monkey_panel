#pragma once

namespace smp
{

class PlaylistIndexManager
{
public:
    static PlaylistIndexManager& Get();

    [[nodiscard]] uint64_t GetId( uint32_t index ) const;
    [[nodiscard]] std::optional<uint32_t> GetIndex( uint64_t id ) const;

    void OnPlaylistAdded( uint32_t index );
    void OnPlaylistRemoved();
    void OnPlaylistsReordered( std::span<const size_t> indices );

private:
    PlaylistIndexManager();

    /// @remark Rework if refresh it too slow
    void RefreshMapping();

private:
    mutable std::unordered_map<uint64_t, uint32_t> idToIdx_;
};

} // namespace smp
