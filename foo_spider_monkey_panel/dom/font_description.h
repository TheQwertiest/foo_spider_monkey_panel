#pragma once

namespace smp::dom
{

enum class FontSizeUnit
{
    // TODO: add support
    // em,
    px,
};

enum class FontStyle
{
    regular,
    // TODO: add support
    // oblique,
    italic,
};

enum class FontWeight : uint32_t
{
    thin = 100,
    extra_light = 200,
    light = 300,
    semi_light = 350,
    regular = 400,
    medium = 500,
    semi_bold = 600,
    bold = 700,
    extra_bold = 800,
    black = 900,
    extra_black = 950,
};

struct FontDescription
{
    // TODO: replace 'Arial' with sans-serif
    std::string cssFont = "10px 'Arial'";

    // TODO: add fallback support
    // TODO: support generic family (use WhatFont or browser sources to determine which font)
    std::string family = "Arial";
    double size = 10;
    FontSizeUnit sizeUnit = FontSizeUnit::px;
    FontStyle style = FontStyle::regular;
    size_t weight = static_cast<uint32_t>( FontWeight::regular );
};

} // namespace smp::dom
