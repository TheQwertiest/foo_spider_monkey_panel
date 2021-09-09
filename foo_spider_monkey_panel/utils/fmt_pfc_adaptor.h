#pragma once

template <>
struct fmt::formatter<pfc::string8_fast> : formatter<string_view>
{
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format( const pfc::string8_fast& str, FormatContext& ctx )
    {
        return formatter<string_view>::format( string_view{ str.c_str(), str.length() }, ctx );
    }
};
