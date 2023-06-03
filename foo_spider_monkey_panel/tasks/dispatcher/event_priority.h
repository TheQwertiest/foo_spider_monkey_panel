#pragma once

namespace smp
{

enum class EventPriority
{
    kNormal,
    kInput,
    kRedraw,
    kResize,
    kControl,
};

} // namespace smp
