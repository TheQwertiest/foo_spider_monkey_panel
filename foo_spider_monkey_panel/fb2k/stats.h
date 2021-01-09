#pragma once

namespace smp::stats
{

using stats_t = uint32_t;
struct fields
{
    stats_t playcount = 0;
    stats_t loved = 0;
    std::u8string first_played;
    std::u8string last_played;
    stats_t rating = 0;
};

bool hashHandle( metadb_handle_ptr const& pMetadb, metadb_index_hash& hash );
fields get( metadb_index_hash hash );
void set( metadb_index_hash hash, fields f );
void refresh( const pfc::list_base_const_t<metadb_index_hash>& hashes );
void refresh( const metadb_index_hash& hash );

} // namespace smp::stats
