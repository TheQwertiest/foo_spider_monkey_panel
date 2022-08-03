#pragma once

namespace smp::config
{

enum class EdgeStyle : uint8_t
{
    NoEdge = 0,
    SunkenEdge,
    GreyEdge,
    Default = NoEdge,
};

struct PanelSettings
{
    qwr::u8string id;
    EdgeStyle edgeStyle = EdgeStyle::Default;
    bool isPseudoTransparent = false;
};

} // namespace smp::config
