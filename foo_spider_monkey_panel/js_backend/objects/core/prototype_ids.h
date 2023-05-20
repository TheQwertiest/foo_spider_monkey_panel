#pragma once

#include <stdint.h>

namespace mozjs
{

enum class JsPrototypeId : uint32_t
{
    Reserved = 0,
    ActiveX,
    ActiveX_Iterator,
    ContextMenuManager,
    DropSourceAction,
    Enumerator,
    Event,
    EventTarget,
    FbFileInfo,
    FbMetadbHandle,
    FbMetadbHandleList,
    FbMetadbHandleList_Iterator,
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
    //////
    New_PlaybackStopEvent,
    New_PaintEvent,
    New_MouseEvent,
    New_WheelEvent,
    New_KeyboardEvent,
    New_Canvas,
    New_WindowCanvas,
    New_CanvasRenderingContext2d,
    New_CanvasGradient,
    ProrototypeCount
};
}
