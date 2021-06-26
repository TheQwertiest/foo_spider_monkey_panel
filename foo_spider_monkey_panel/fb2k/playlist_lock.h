#pragma once

#include <unordered_map>

namespace smp
{

class PlaylistLockManager
{
public:
    static [[nodiscard]] PlaylistLockManager& Get();

    /// @throw qwr::QwrException
    void InitializeLocks();

    /// @throw qwr::QwrException
    void InstallLock( size_t playlistIndex, uint32_t flags );

    /// @throw qwr::QwrException
    void RemoveLock( size_t playlistIndex );

    [[nodiscard]] bool HasLock( size_t playlistIndex );

private:
    PlaylistLockManager() = default;

private:
    std::unordered_map<qwr::u8string, playlist_lock::ptr> knownLocks_;
};

} // namespace smp
