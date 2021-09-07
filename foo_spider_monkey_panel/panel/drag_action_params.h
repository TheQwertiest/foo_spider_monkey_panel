#pragma once

namespace smp::panel
{

struct DragActionParams
{
    int32_t playlistIdx = -1; // -1 means active playlist
    uint32_t base = 0;
    bool toSelect = true;
    uint32_t effect = DROPEFFECT_NONE;
    std::wstring text;
    bool isInternal = false;
};

} // namespace smp::panel
