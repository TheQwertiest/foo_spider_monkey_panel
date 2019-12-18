#pragma once

#include <stdint.h>

namespace mozjs
{

enum class JsPrototypeId : uint32_t
{
    ActiveX,
    ContextMenuManager,
    DropSourceAction,
    Enumerator,
    FbFileInfo,
    FbMetadbHandle,
    FbMetadbHandleList,
    FbPlaybackQueueItem,
    FbPlayingItemLocation,
    FbProfiler,
    FbTitleFormat,
    FbTooltip,
    FbUiSelectionHolder,
    GdiBitmap,
    GdiFont,
    GdiGraphics,
    GdiRawBitmap,
    HtmlWindow,
    MainMenuManager,
    MeasureStringInfo,
    MenuObject,
    ThemeManager,
    ProrototypeCount
};

}
