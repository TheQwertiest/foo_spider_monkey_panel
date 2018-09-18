#pragma once

namespace stats
{

typedef uint32_t stats_t;
struct fields
{
    stats_t playcount = 0;
    stats_t loved = 0;
    pfc::string8 first_played;
    pfc::string8 last_played;
    stats_t rating = 0;
};


bool hashHandle( metadb_handle_ptr const& pMetadb, metadb_index_hash& hash );
fields get( metadb_index_hash hash );
void set( metadb_index_hash hash, fields f );
void refresh( const pfc::list_base_const_t<metadb_index_hash>& hashes );
void refresh( const metadb_index_hash& hash );

}
