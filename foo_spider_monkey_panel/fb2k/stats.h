#pragma once

namespace smp::stats
{

struct Fields
{
    uint32_t playcount = 0;
    uint32_t loved = 0;
    qwr::u8string first_played;
    qwr::u8string last_played;
    uint32_t rating = 0;
};

[[nodiscard]] bool HashHandle( metadb_handle_ptr const& pMetadb, metadb_index_hash& hash );
[[nodiscard]] Fields GetStats( metadb_index_hash hash );
void SetStats( metadb_index_hash hash, const Fields& f );
void RefreshStats( const pfc::list_base_const_t<metadb_index_hash>& hashes );
void RefreshStats( const metadb_index_hash& hash );

} // namespace smp::stats
