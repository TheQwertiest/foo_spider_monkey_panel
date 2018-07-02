#pragma once

namespace mozjs::scope
{

template <typename T>
using unique_ptr = std::unique_ptr<T, void(*)(T*)>;

}
