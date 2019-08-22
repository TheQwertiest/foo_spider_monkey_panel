#include <stdafx.h>
#include "pfc_helpers_stream.h"

namespace smp::pfc_x
{

std::u8string ReadString( stream_reader& stream, abort_callback& abort )
{ // ripped from `stream_reader::read_string`
    t_uint32 length;
    stream.read_lendian_t( length, abort );

    std::u8string value;
    value.resize( length );
    stream.read_object( value.data(), length, abort );
    value.resize( strlen( value.c_str() ) );

    return value;
}

std::u8string ReadRawString( stream_reader& stream, abort_callback& abort )
{ // ripped from `stream_reader::read_string_raw`
    constexpr size_t delta = 256;

    char buffer[delta];
    std::u8string value;

    for ( size_t delta_done = stream.read( buffer, delta, abort );
          delta_done < delta;
          delta_done = stream.read( buffer, delta, abort ) )
    {
        value.append( buffer, delta_done );
    }

    return value;
}

void WriteString( stream_writer& stream, const std::u8string& val, abort_callback& abort )
{
    stream.write_string( val.c_str(), val.length(), abort );
}

} // namespace smp::pfc_x
