#pragma once

namespace stats
{

class metadb_index_client_impl 
     : public metadb_index_client
{
public:
    metadb_index_hash transform( const file_info& info, const playable_location& location );

private:
    titleformat_object::ptr titleFormat_;
};

extern metadb_index_client_impl* g_client;
metadb_index_manager::ptr theAPI();

typedef uint32_t stats_t;
struct fields
{
    stats_t playcount = 0;
    stats_t loved = 0;
    pfc::string8 first_played;
    pfc::string8 last_played;
    stats_t rating = 0;
};

fields get( metadb_index_hash hash );
void set( metadb_index_hash hash, fields f );

}
