#pragma once

namespace mozjs
{

struct EventStatus
{
    bool isDefaultSuppressed = false;
    bool isHandled = false;
};

} // namespace mozjs
